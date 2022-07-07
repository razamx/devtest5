/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/**
 * @file multicallback_server.c
 * @brief Implements \c opcua_server application
 *
 * The \c opcua_server application implements OPC UA* publish/subscribe communication between
 * publishers and subscribers defined in the JSON configuration file.
 * For publishers, the server reads the sample values in the Shared Memory and publishes them to
 * the network using open62541 library.
 * For subscribers, the server receives the network packets from remote publishers and stores
 * the received packets in the Shared Memory.
 */

#include "opcua_common.h"
#include "opcua_custom.h"
#include "opcua_datasource.h"
#include "opcua_publish.h"
#include "opcua_subscribe.h"
#include "opcua_utils.h"
#include "pdatasource.h"

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <tcc/err_code.h>

/* NW State polling period: 100ms */
#define NIC_STATE_POLLING_TIME_US (100000)
/* NW State maximum number of tries: 100 -> 10 seconds */
#define NIC_STATE_POLLING_TRIES_LIMIT (100)
/* The filename template for the file which exposes the NIC state */
#define CARRIER_FILENAME_PATTERN "/sys/class/net/%s/carrier"

/* Globals */
struct threadParams* g_thread;
UA_UInt16 g_threadRun;
UA_Boolean g_running = true;
UA_NodeId g_writerGroupIdent;
struct ServerData* g_sData;

static bool is_nw_interface_up(const char* interfaceName);

static void stopHandler(__attribute__((unused)) int sign)
{
    g_running = false;
}

static void configureServer(struct ServerData* sdata)
{
    sdata->transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");

    for (int i = 0; i < sdata->pubCount; i++) {
        struct PublisherData* pub = &sdata->pubData[i];
        pub->readFunc = &readValueFromTalkerDS;
        pub->writeFunc = &dummyDSWrite;
    }

    for (int i = 0; i < sdata->subCount; i++) {
        struct SubscriberData* sub = &sdata->subData[i];
        sub->readFunc = &dummyDSRead;
        sub->writeFunc = &writeValueToListenerDS;
    }
}

static UA_Server* setupOpcuaServer(struct ServerData* sdata)
{
    int ret;
    UA_StatusCode sret;
    UA_ServerConfig* config;
    struct PublisherData* pub = NULL;
    UA_NodeId connId = UA_NODEID_NULL;
    struct SubscriberData* sub = NULL;
    UA_Server* server = UA_Server_new();

    check(server, "Cannot create new server");
    sdata->server = server;
    config = UA_Server_getConfig(server);
    check(config, "Cannot get server config");

    sret = UA_ServerConfig_setMinimal(config, getpid(), NULL);
    check(sret == UA_STATUSCODE_GOOD, "Cannot set minimal config for server");

    /* Set up transport layers through UDPMP and Ethernet */
    config->pubsubTransportLayers = (UA_PubSubTransportLayer*)UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    check(config->pubsubTransportLayers != NULL, "Out of memory");
    sdata->PubSubTransportLayers = config->pubsubTransportLayers;
    config->pubsubTransportLayersSize = 0;

    if (sdata->useXDP) {
        config->pubsubTransportLayers[0] = UA_PubSubTransportLayerEthernetXDP();
    } else {
        config->pubsubTransportLayers[0] = UA_PubSubTransportLayerEthernetETF();
    }

    config->pubsubTransportLayersSize++;

    for (int i = 0; i < sdata->pubCount; i++) {
        for (int count = 0; !is_nw_interface_up(sdata->pubInterface);) {
            if (++count >= NIC_STATE_POLLING_TRIES_LIMIT) {
                LOG_ERR("The publisher network interface is down");
                exit(EXIT_FAILURE);
            }
            usleep(NIC_STATE_POLLING_TIME_US);
        }
        pub = &sdata->pubData[i];
        ret = addPubSubConnection(server, &connId, sdata, pub);
        check(ret != -1, "Failed to create new connection for publisher");
        ret = createPublisher(server, sdata, pub, &connId);
        check(ret != -1, "Failed to create publisher");
    }

    for (int i = 0; i < sdata->subCount; i++) {
        for (int count = 0; !is_nw_interface_up(sdata->subInterface);) {
            if (++count >= NIC_STATE_POLLING_TRIES_LIMIT) {
                LOG_ERR("The subscriber network interface is down");
                exit(EXIT_FAILURE);
            }
            usleep(NIC_STATE_POLLING_TIME_US);
        }

        sub = &sdata->subData[i];

        /* Skip adding a new socket (PubSubConnection)
         * using XDP while already having a publisher on the same interface's queue.
         * This is because XDP sockets are bi-directional, so both
         * TX/RX queue is bound together to a single socket.
         * Uni direction sockets are available in 5.5+ kernels only and
         * requires a different bpf program.
         */
        if (sdata->pubCount && sdata->useXDP && pub && sub->xdpQueue == pub->xdpQueue &&
            strcmp(sdata->pubInterface, sdata->subInterface) == 0) {
            LOG_INFO("Round trip AF_XDP same queue, same interface, sharing layer");
        } else if (sdata->pubCount && sdata->useXDP) {
            LOG_INFO("Round trip AF_XDP different queue, use new layer");
            config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernetXDP();
            config->pubsubTransportLayersSize++;
            ret = addSubConnection(server, &connId, sdata, sub);
            check(ret != -1, "Failed to create new connection for subscriber");
        } else if (sdata->pubCount) {
            LOG_INFO("Round trip AF_PACKET use new layer");
            config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernetETF();
            config->pubsubTransportLayersSize++;
            ret = addSubConnection(server, &connId, sdata, sub);
            check(ret != -1, "Failed to create new connection for subscriber");
        } else {
            LOG_INFO("Single trip AF_XDP or AF_PACKET addSubConnection only");
            ret = addSubConnection(server, &connId, sdata, sub);
            check(ret != -1, "Failed to create new connection for subscriber");
        }

        ret = createSubscriber(server, sdata, sub, &connId);
        check(ret != -1, "Failed to create subscriber");
    }

    return server;

error:
    if (server) {
        UA_Server_delete(server);
    }
    return NULL;
}

static void check_clock_tai_presence(void)
{
    struct timespec dummy;
    int iret = -1;

    iret = clock_gettime(CLOCK_TAI, &dummy);
    if (iret == -1) {
        // EINVAL related to unsupported clk_id
        check(errno != EINVAL,
            "CLOCK_TAI clock identifier is not supported by "
            "the system. This type of clock required for sample execution.");
    }
    return;
error:
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    int i, ret = 0;
    UA_Server* server = NULL;
    UA_UInt16 threadCount = 0;
    struct ServerData* sdata = NULL;
    UA_StatusCode servrun = UA_STATUSCODE_BADINTERNALERROR;

    // Check for support of CLOCK_TAI on the target system
    check_clock_tai_presence();

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    sdata = parseArgs(argc, argv);
    check(sdata != NULL, "Failed to process input parameters");
    configureServer(sdata);

    if (sdata->pubCount > 0) {
        // Initialize datasource resources if any publisher was found in config
        ret = prepare_datasource(sdata->tcc_latency, sdata->tcc_affinity);
        check(TCC_E_SUCCESS == ret, "Failed to prepare the datasource");
    }

    threadCount = sdata->pubCount + sdata->subCount;
    g_threadRun = 0;
    g_thread = (struct threadParams*)calloc(threadCount, sizeof(struct threadParams));
    g_sData = sdata;

    /* Pub/sub threads will be started by the WG/RG addRepeatedCallback() */
    server = setupOpcuaServer(sdata);
    check(server != NULL, "Failed to create OPC UA* server");

    servrun = UA_Server_run(server, &g_running);

    /* Pub/sub threads are started by the server. Wait until they finish. */
    for (i = 0; i < g_threadRun; i++) {
        ret = pthread_join(g_thread[i].id, NULL);
        if (ret != 0) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "\nPthread Join Failed:%ld\n", g_thread[i].id);
        }
    }

    if (sdata->pubCount > 0) {
        // If any publisher was found in config,
        // print statistics for the collected measurement data
        print_datasource_statistic();
        destroy_datasource();
    }

    UA_Server_delete(server);
    free(g_thread);
    cleanServerData(sdata);
    free(sdata);

    return servrun == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;

error:
    if ((sdata != NULL) && (sdata->pubCount > 0)) {
        // Free TCC resources if any publisher was found in config
        destroy_datasource();
    }
    cleanServerData(sdata);
    free(g_thread);
    free(sdata);
    exit(EXIT_FAILURE);
}

bool is_nw_interface_up(const char* interfaceName)
{
    bool up;
    FILE* stream;
    char buffer[PATH_MAX];

    if (!interfaceName || snprintf(NULL, 0, CARRIER_FILENAME_PATTERN, interfaceName) >= (int)(sizeof(buffer))) {
        /* Wrong argument */
        return false;
    }

    snprintf(buffer, sizeof(buffer), CARRIER_FILENAME_PATTERN, interfaceName);

    stream = fopen(buffer, "r");
    up = stream && fgetc(stream) == '1' && fgetc(stream) == '\n';
    if (stream) {
        fclose(stream);
    }

    return up;
}

/* vim: set ts=4 sw=4 et: */
