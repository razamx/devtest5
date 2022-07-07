/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */

#include "opcua_common.h"

#include "pconfig.h"

#include "json_helper.h"
#include "opcua_utils.h"

#include <getopt.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <open62541/server_config.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/pubsub_ethernet.h>

extern struct GenericPayload subscriber_incoming_data;

static const char cli_opts[] = "c:l:b:h";
static const struct option long_cli_opts[] = {{"config-file", required_argument, NULL, 'c'},
    {"log-level", required_argument, NULL, 'l'},
    {"base-time", required_argument, NULL, 'b'},
    {"help", no_argument, NULL, 'h'},
    {NULL, no_argument, NULL, 0}};
struct flavor_to_packet_map
{
    const char* flavor;
    uint32_t packet_type;
};
struct log_map
{
    const char* log_name;
    log_level_t log_level;
};
struct log_map name_to_level[] = {{"debug", log_debug}, {"info", log_info}, {"error", log_error}, {NULL, log_undef}};
log_level_t log_level = log_error;

static PConfig talker_config = {}, listener_config = {}, compute_config = {};
static struct GenericPayload local_payload;

__attribute__((pure)) static log_level_t get_log_level_by_name(const char* log_name)
{
    log_level_t level = log_undef;
    struct log_map* iter = name_to_level;

    while (iter->log_name != NULL) {
        if (strcmp(log_name, iter->log_name) == 0) {
            level = iter->log_level;
            break;
        }
        iter++;
    }

    return level;
}

static struct ServerData* parseJson(struct json_object* json, int64_t baseTime)
{
    int i, j;
    int subCount;
    int pubCount;
    int tcc_latency;
    struct json_object* pubs;
    struct json_object* subs;
    struct ServerData* sdata;
    struct PublisherData* pubData;
    struct SubscriberData* subData;
    struct json_object* tcc_settings;
    struct json_object *talker = NULL, *listener = NULL, *compute = NULL;

    check(json, "Wrong argument");

    sdata = (struct ServerData*)calloc(1, sizeof(struct ServerData));
    check(sdata != NULL, "Out of memory");

    LOG_INFO("Parsing OPC UA* server config");
    sdata->pubInterface = getString(json, "publisher_interface");
    sdata->subInterface = getString(json, "subscriber_interface");
    sdata->useXDP = getBool(json, "use_xdp");
    sdata->cpu = getOptionalInt(json, "cpu", get_nprocs() - 1);
    sdata->pollingDurationNs = getInt(json, "polling_duration_ns");
    sdata->packetCount = getInt64(json, "packet_count");
    sdata->cycleTimeNs = getInt64(json, "cycle_time_ns");
    LOG_INFO("Found: pub if %s, sub if %s, cycleTimeNs %ld pollNs %d, packet_count %lu",
        sdata->pubInterface,
        sdata->subInterface,
        sdata->cycleTimeNs,
        sdata->pollingDurationNs,
        sdata->packetCount);

    i = 0;
    pubs = getValue(json, "publishers");
    pubCount = countChildrenEntries(pubs);
    pubData = NULL;
    if (pubCount > 0) {
        pubData = (struct PublisherData*)calloc(pubCount, sizeof(struct PublisherData));
        check(pubData != NULL, "Out of memory");

        json_object_object_foreach(pubs, key, pubJson)
        {
            (void)key;
            struct PublisherData* pd = &pubData[i];
            pd->earlyOffsetNs = getInt(pubJson, "early_offset_ns");
            pd->publishOffsetNs = getInt(pubJson, "publish_offset_ns");
            pd->socketPriority = getInt(pubJson, "socket_prio");
            pd->xdpQueue = getInt(pubJson, "xdp_queue");
            pd->url = getString(pubJson, "url");
            pd->id = getInt(pubJson, "pub_id");
            pd->dataSetWriterId = getInt(pubJson, "dataset_writer_id");
            pd->writerGroupId = getInt(pubJson, "writer_group_id");
            pd->cpuAffinity = getInt(pubJson, "cpu_affinity");

            LOG_INFO("Found publisher with url %s offset(ns) %d publishOffset(ns) %d prio %d cpu %lu",
                pd->url,
                pd->earlyOffsetNs,
                pd->publishOffsetNs,
                pd->socketPriority,
                pd->cpuAffinity);
            i++;
        }
    }

    j = 0;
    subs = getValue(json, "subscribers");
    subCount = countChildrenEntries(subs);
    subData = NULL;
    if (subCount > 0) {
        subData = (struct SubscriberData*)calloc(subCount, sizeof(struct SubscriberData));
        check(subData != NULL, "Out of memory");

        json_object_object_foreach(subs, key, subJson)
        {
            (void)key;
            struct SubscriberData* sd = &subData[j];
            sd->offsetNs = getInt64(subJson, "offset_ns");
            sd->url = getString(subJson, "url");
            sd->xdpQueue = getInt(subJson, "xdp_queue");
            sd->cpuAffinity = getInt(subJson, "cpu_affinity");
            sd->data = &subscriber_incoming_data;
            sd->id = getInt(subJson, "sub_id");
            sd->subscribedPubId = getInt(subJson, "subscribed_pub_id");
            sd->subscribedDSWriterId = getInt(subJson, "subscribed_dataset_writer_id");
            sd->subscribedWGId = getInt(subJson, "subscribed_writer_group_id");

            LOG_INFO("Found subscriber with url %s offset(ns) %d cpu %lu", sd->url, sd->offsetNs, sd->cpuAffinity);
            j++;
        }
    }

    sdata->pubCount = pubCount;
    sdata->subCount = subCount;
    sdata->pubData = pubData;
    sdata->subData = subData;
    LOG_INFO("Found %d publishers and %d subscribers", sdata->pubCount, sdata->subCount);

    talker = getOptionalValue(json, "talker");
    listener = getOptionalValue(json, "listener");
    compute = getOptionalValue(json, "compute");
    if (!talker && !listener && !compute) {
        LOG_ERR("Necessary roles of data manager are missing")
        goto error;
    }
    if ((talker && compute) || (listener && compute)) {
        LOG_ERR("Mutual exclusive roles on one side");
        goto error;
    }
    if (talker) {
        talker_config.sequenceNumber = 1;
        talker_config.affinity = getInt(talker, "affinity");
        talker_config.iterations = getInt(talker, "iterations");
        talker_config.startTime = getInt(talker, "start-offset");
        talker_config.period = getInt(talker, "period");
        talker_config.baseTime = baseTime;
        create_producer(&talker_config);
        local_payload.type = TALKER;
    }
    if (listener) {
        char* flavor_str = NULL;
        struct flavor_to_packet_map avail_flavors[] = {{"talker", TALKER}, {"compute", COMPUTE}, {NULL, 0}};
        bool known_flavor = false;
        uint32_t packet_type = 0;
        size_t iteration = 0;
        listener_config.affinity = getInt(listener, "affinity");
        listener_config.startTime = getInt(listener, "start-offset");
        listener_config.iterations = getInt(listener, "iterations");
        listener_config.pLogFile = getString(listener, "output-file");
        flavor_str = getString(listener, "flavor");
        while (avail_flavors[iteration].flavor) {
            if (!strcmp(flavor_str, avail_flavors[iteration].flavor)) {
                known_flavor = true;
                packet_type = avail_flavors[iteration].packet_type;
                break;
            }
            iteration++;
        }
        free(flavor_str);
        check(known_flavor == true, "Listener's flavor was not recognized\n");
        listener_config.packetType = packet_type;
        listener_config.period = getInt(listener, "period");
        listener_config.incomingDataSource = &subscriber_incoming_data;
        listener_config.baseTime = baseTime;
        create_consumer(&listener_config);
    }
    if (compute) {
        compute_config.affinity = getInt(compute, "affinity");
        compute_config.pLogFile = getString(compute, "output-file");
        compute_config.iterations = getInt(compute, "iterations");
        compute_config.startTime = getInt(compute, "start-offset");
        compute_config.period = getInt(compute, "period");
        compute_config.incomingDataSource = &subscriber_incoming_data;
        compute_config.packetType = TALKER;
        compute_config.baseTime = baseTime;
        create_compute(&compute_config);
        local_payload.type = COMPUTE;
    }

    /* Parse the JSON configuration relevant to the TCC Workload */
    tcc_settings = getValue(json, "tcc_settings");
    tcc_latency = getInt(tcc_settings, "latency");
    sdata->tcc_affinity = getInt(tcc_settings, "affinity");
    if (tcc_latency < 0) {
        LOG_INFO("Detected invalid workload latency, using default latency of 25ns");
        tcc_latency = 25;
    }
    sdata->tcc_latency = tcc_latency;

    return sdata;

error:
    LOG_ERR("Failed to parse command line arguments");
    exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void usage(const char* caller)
{
    fprintf(stderr,
        "\nUsage: %s [OPTIONS]\n\n"
        "\t-c, --config-file=<configuration file>\n"
        "\t\tMandatory option.\n"
        "\t\tDefine name of file in JSON format with configuration for %s.\n\n"
        "\t-b, --base-time=<requested-UTC-base-time>\n"
        "\t\tOptional argument to define the UTC start time for network communications\n\n"
        "\t-l, --log-level=<log level>\n"
        "\t\tDefine log level which will be used for output from %s.\n"
        "\t\tPossible log levels are: debug, info, error.\n"
        "\t\tLog level error is set as per default.\n\n"
        "\t-h, --help\n"
        "\t\tShow this help message.\n\n"
        "Example: %s --config-file=opcua_server_config.json"
        " --log-level=info\n\n",
        caller,
        caller,
        caller,
        caller);
    exit(EXIT_FAILURE);
}

struct ServerData* parseArgs(int argc, char* argv[])
{
    char* jsonPath = NULL;
    int opt = -1;
    int long_idx = -1;
    int64_t baseTime = 0;
    struct json_object* js = NULL;
    struct ServerData* sdata;
    struct json_object* run_apps;
    struct json_object* opcuaJson;
    log_level_t parsed_level = log_undef;

    if (argc < 2) {
        usage(argv[0]);
    }

    while (-1 != (opt = getopt_long(argc, argv, cli_opts, long_cli_opts, &long_idx))) {
        switch (opt) {
        case 'c':
            jsonPath = optarg;
            break;
        case 'l':
            parsed_level = get_log_level_by_name(optarg);
            if (parsed_level == log_undef) {
                LOG_ERR("Provided unknown log level name '%s'. "
                        "Type [-h] to see usage information",
                    optarg);
                exit(EXIT_FAILURE);
            }
            log_level = parsed_level;
            break;
        case 'b':
            baseTime = atoll(optarg);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            break;
        }
    }

    check(jsonPath != NULL, "Path to json not specified");

    if (access(jsonPath, R_OK) == -1) {
        LOG_ERR("Cannot access '%s' file", jsonPath);
        exit(EXIT_FAILURE);
    }

    LOG_DBG("Reading JSON config from %s", jsonPath);
    js = json_object_from_file(jsonPath);

    run_apps = getValue(js, "run-apps");
    opcuaJson = getValue(run_apps, "opcua_server");
    sdata = parseJson(opcuaJson, baseTime);
    check(sdata != NULL, "Failed to parse JSON");
    sdata->baseTime = baseTime;

    // Free json since we shouldn't be using it outside of parsing
    json_object_put(js);

    return sdata;

error:
    LOG_ERR("Failed to parse arguments");
    exit(EXIT_FAILURE);
}

static void cleanPubData(struct PublisherData* sdata)
{
    if (sdata == NULL) {
        return;
    }
    free(sdata->url);
    // UA_UadpWriterGroupMessageDataType_delete( sdata->writerGroupMessage );
}

static void cleanSubData(struct SubscriberData* sdata)
{
    if (sdata == NULL) {
        return;
    }
    free(sdata->url);
    free(sdata->pMetaData_fields);
    free(sdata->targetVars);
}

void cleanDataMgrData(void)
{
    if (listener_config.pLogFile) {
        free(listener_config.pLogFile);
        listener_config.pLogFile = NULL;
    }
    if (compute_config.pLogFile) {
        free(compute_config.pLogFile);
        compute_config.pLogFile = NULL;
    }
}

void cleanServerData(struct ServerData* sdata)
{
    if (sdata == NULL) {
        return;
    }

    free(sdata->pubInterface);
    free(sdata->subInterface);
    for (int iter = 0; iter < sdata->pubCount; iter++) {
        // Destory container members (like url...)
        cleanPubData(&(sdata->pubData[iter]));
    }
    // Destroy containers
    free(sdata->pubData);
    for (int iter = 0; iter < sdata->subCount; iter++) {
        // Destory container members (like url...)
        cleanSubData(&(sdata->subData[iter]));
    }
    // Destroy containers
    free(sdata->subData);

    // Clean talker/listener/compute related data
    cleanDataMgrData();
}

__attribute__((pure)) UA_UInt64 as_nanoseconds(const struct timespec* ts)
{
    return (uint64_t)(ts->tv_sec) * UINT64_C(1000000000) + (uint64_t)(ts->tv_nsec);
}

// vim: et:ts=4:sw=4
