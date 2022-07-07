/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/******************************************************************************
 *
 * Copyright (c) 2020-2021, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet_etf.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <pthread.h>
#include <sched.h>
#include <time.h>

#include "ptimer.h"
#include "opcua_common.h"
#include "opcua_custom.h"

/* INTERNAL structs START ***************************************************
 * The following internal structures from src/pubsub/ua_pubsub.h are required
 * for checking the callback type. If the lib changes, then these structs
 * should be updated as well.
 * TODO: find out if there are better ways to check the Repeatedcallback type
 */

#define TAILQ_ENTRY(type)                                              \
    struct                                                             \
    {                                                                  \
        struct type* tqe_next;  /* next element */                     \
        struct type** tqe_prev; /* address of previous next element */ \
    }

#define LIST_ENTRY(type)                                              \
    struct                                                            \
    {                                                                 \
        struct type* le_next;  /* next element */                     \
        struct type** le_prev; /* address of previous next element */ \
    }

#define LIST_HEAD(name, type)                      \
    struct name                                    \
    {                                              \
        struct type* lh_first; /* first element */ \
    }

typedef struct UA_ReaderGroup
{
    UA_ReaderGroupConfig config;
    UA_NodeId identifier;
    UA_NodeId linkedConnection;
    LIST_ENTRY(UA_ReaderGroup) listEntry;
    LIST_HEAD(UA_ListOfPubSubDataSetReader, UA_DataSetReader) readers;
    /* for simplified information access */
    UA_UInt32 readersCount;
    UA_UInt64 subscribeCallbackId;
    UA_Boolean subscribeCallbackIsRegistered;
} UA_ReaderGroup;

/* Offsets for buffered messages in the PubSub fast path. */
typedef enum
{
    UA_PUBSUB_OFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER,
    UA_PUBSUB_OFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER,
    UA_PUBSUB_OFFSETTYPE_TIMESTAMP_PICOSECONDS,
    UA_PUBSUB_OFFSETTYPE_TIMESTAMP,     /* source pointer */
    UA_PUBSUB_OFFSETTYPE_TIMESTAMP_NOW, /* no source */
    UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE,
    UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT,
    UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW
    /* Add more offset types as needed */
} UA_NetworkMessageOffsetType;

typedef struct
{
    UA_NetworkMessageOffsetType contentType;
    union
    {
        union
        {
            UA_DataValue* value;
            size_t valueBinarySize;
        } value;
        UA_DateTime* timestamp;
    } offsetData;
    size_t offset;
} UA_NetworkMessageOffset;

typedef struct UA_PubSubConnection
{
    UA_PubSubConnectionConfig* config;
    /* internal fields */
    UA_PubSubChannel* channel;
    UA_NodeId identifier;
    LIST_HEAD(UA_ListOfWriterGroup, UA_WriterGroup) writerGroups;
    LIST_HEAD(UA_ListOfPubSubReaderGroup, UA_ReaderGroup) readerGroups;
    size_t readerGroupsSize;
    TAILQ_ENTRY(UA_PubSubConnection) listEntry;
    UA_UInt16 configurationFreezeCounter;
} UA_PubSubConnection;

typedef struct
{
    UA_ByteString buffer;             /* The precomputed message buffer */
    UA_NetworkMessageOffset* offsets; /* Offsets for changes in the message buffer */
    size_t offsetsSize;
} UA_NetworkMessageOffsetBuffer;


typedef struct UA_WriterGroup
{
    UA_WriterGroupConfig config;
    /* internal fields */
    LIST_ENTRY(UA_WriterGroup) listEntry;
    UA_NodeId identifier;
    UA_PubSubConnection* linkedConnection;
    LIST_HEAD(UA_ListOfDataSetWriter, UA_DataSetWriter) writers;
    UA_UInt32 writersCount;
    UA_UInt64 publishCallbackId;
    UA_Boolean publishCallbackIsRegistered;
    UA_PubSubState state;
    UA_NetworkMessageOffsetBuffer bufferedMessage;
    UA_UInt16 sequenceNumber; /* Increased after every succressfully sent message */
} UA_WriterGroup;

/* INTERNAL structs END *******************************************************/

#define CLOCKID CLOCK_TAI
#define ONESEC_IN_NSEC (1000 * 1000 * 1000)
#define PUB_THREAD_PRIORITY 90
#define SUB_THREAD_PRIORITY 90

extern struct threadParams* g_thread;
extern UA_UInt16 g_threadRun;
extern UA_Boolean g_running;
extern UA_NodeId g_writerGroupIdent;
extern struct ServerData* g_sData;
UA_UInt16 g_indexPub;
UA_UInt16 g_indexSub;

static void normalize(struct timespec* timeSpecValue)
{
    /* If nsec is bigger than sec, increment the sec
     * and realign the nsec value
     */
    while (timeSpecValue->tv_nsec > (ONESEC_IN_NSEC - 1)) {
        timeSpecValue->tv_sec += 1;
        timeSpecValue->tv_nsec -= ONESEC_IN_NSEC;
    }

    /* If nsec is a negative value, go back to the
     * previous second and realign the nanosec
     */
    while (timeSpecValue->tv_nsec < 0) {
        timeSpecValue->tv_sec -= 1;
        timeSpecValue->tv_nsec += ONESEC_IN_NSEC;
    }
}

static pthread_t threadCreation(UA_Int16 threadPriority,
    size_t coreAffinity,
    void* (*thread)(void*),
    char* applicationName,
    void* serverConfig)
{
    struct sched_param schedParam;
    pthread_t threadID;
    cpu_set_t cpuset;
    UA_Int32 ret = 0;

    /* TODO: check if this should be run earlier. Also check setRTpriority() */
    ret = pthread_create(&threadID, NULL, thread, serverConfig);
    if (ret != 0) {
        UA_free(serverConfig);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, ":%s Cannot create thread\n", applicationName);
        exit(1);
    }
    schedParam.sched_priority = threadPriority;

    ret = pthread_setschedparam(threadID, SCHED_RR, &schedParam);
    if (ret != 0) {
        UA_free(serverConfig);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "pthread_setschedparam: failed\n");
        exit(1);
    }
    UA_LOG_INFO(UA_Log_Stdout,
        UA_LOGCATEGORY_USERLAND,
        "\npthread_setschedparam:%s Thread priority is %d \n",
        applicationName,
        schedParam.sched_priority);

    /* Set thread CPU affinity */
    CPU_ZERO(&cpuset);
    CPU_SET(coreAffinity, &cpuset);

    ret = pthread_setaffinity_np(threadID, sizeof(cpu_set_t), &cpuset);
    if (ret) {
        UA_free(serverConfig);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "pthread_setaffinity_np ret: %s\n", strerror(ret));
        exit(1);
    }

    if (CPU_ISSET(coreAffinity, &cpuset)) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s CPU CORE: %ld\n", applicationName, coreAffinity);
    }

    return threadID;
}

/* Custom publisher thread */
void* pub_thread(void* arg)
{
    struct timespec delay30ns;
    struct timespec nextnanosleeptime;
    struct timespec temp_t;
    UA_UInt64 tx_timestamp;
    UA_ServerCallback pubCallback;
    UA_Server* server;
    UA_WriterGroup* currentWriterGroup;
    UA_Int32 earlyOffsetNs = 0;
    UA_Int32 publishOffsetNs = 0;
    UA_UInt32 ind = 0;
    UA_UInt64 cycleTimeNs = 0;
    struct PublisherData* pData;

    delay30ns.tv_sec = 0;
    delay30ns.tv_nsec = 30;
    /* Initialize value for nextnanosleeptime timespec */
    tx_timestamp = 0;
    nextnanosleeptime.tv_nsec = 0;
    nextnanosleeptime.tv_sec = 0;
    threadParams* threadArgumentsPublisher = (threadParams*)arg;
    server = threadArgumentsPublisher->server;
    pubCallback = threadArgumentsPublisher->callback;
    currentWriterGroup = (UA_WriterGroup*)threadArgumentsPublisher->data;
    pData = g_sData->pubData;
    ind = threadArgumentsPublisher->data_index;
    earlyOffsetNs = pData[ind].earlyOffsetNs;
    publishOffsetNs = pData[ind].publishOffsetNs;
    cycleTimeNs = g_sData->cycleTimeNs;

    /* Define Ethernet ETF transport settings */
    UA_EthernetETFWriterGroupTransportDataType ethernetETFtransportSettings;
    memset(&ethernetETFtransportSettings, 0, sizeof(UA_EthernetETFWriterGroupTransportDataType));

    ethernetETFtransportSettings.txtime_enabled = UA_TRUE;
    ethernetETFtransportSettings.transmission_time = 0;

    /* Encapsulate ETF config in transportSettings */
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));

    /* TODO: transportSettings encoding and type to be defined */
    transportSettings.content.decoded.data = &ethernetETFtransportSettings;
    currentWriterGroup->config.transportSettings = transportSettings;

    /* Get current time and compute the next nanosleeptime to the nearest 5th second */
    clock_gettime(CLOCKID, &temp_t);
    tx_timestamp = ptimer_base_time(g_sData->baseTime, temp_t.tv_sec);
    tx_timestamp *= ONESEC_IN_NSEC;
    /* Add publish offset to tx time*/
    tx_timestamp += publishOffsetNs;

    /* First packet tx_timestamp */
    ethernetETFtransportSettings.transmission_time = tx_timestamp;

    while (g_running) {
        /* Calculate publisher wake-up time using earlyOffsetNs
         * Publisher wakes up earlier to be able to catch
         * the ETF transmission time
         */
        nextnanosleeptime.tv_nsec = (tx_timestamp - earlyOffsetNs) % ONESEC_IN_NSEC;
        nextnanosleeptime.tv_sec = (tx_timestamp - earlyOffsetNs) / ONESEC_IN_NSEC;
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptime, NULL);
        if (!g_running) {
            /* If the 'g_running' flag resets during sleep, finish processing */
            break;
        }

        pubCallback(server, currentWriterGroup);
        tx_timestamp += cycleTimeNs;
        ethernetETFtransportSettings.transmission_time = tx_timestamp;

        /* The following lines is required for the XDP/ZC communication in 5.10 Kernel,
           as without this line, the packets are sent in pairs,
           making poor communication statistics */
        clock_nanosleep(CLOCKID, 0, &delay30ns, NULL);
    }

    UA_free(threadArgumentsPublisher);
    return (void*)NULL;
}

/* Custom subscriber thread */
void* sub_thread(void* arg)
{
    UA_Server* server;
    UA_ReaderGroup* currentReaderGroup;
    UA_ServerCallback subCallback;
    struct timespec nextnanosleeptimeSub;
    UA_UInt64 cycleTimeNs = 0;
    UA_UInt32 offsetNs = 0;
    UA_UInt32 ind = 0;
    struct SubscriberData* sData;


    threadParams* threadArgumentsSubscriber = (threadParams*)arg;
    server = threadArgumentsSubscriber->server;
    subCallback = threadArgumentsSubscriber->callback;
    currentReaderGroup = (UA_ReaderGroup*)threadArgumentsSubscriber->data;
    cycleTimeNs = g_sData->cycleTimeNs;
    sData = g_sData->subData;
    ind = threadArgumentsSubscriber->data_index;
    offsetNs = sData[ind].offsetNs;


    /* Get current time and compute the next nanosleeptime to the nearest 5th second */
    clock_gettime(CLOCKID, &nextnanosleeptimeSub);
    nextnanosleeptimeSub.tv_sec = ptimer_base_time(g_sData->baseTime, nextnanosleeptimeSub.tv_sec);
    nextnanosleeptimeSub.tv_nsec = offsetNs;
    normalize(&nextnanosleeptimeSub);

    while (g_running) {
        clock_nanosleep(CLOCKID, TIMER_ABSTIME, &nextnanosleeptimeSub, NULL);
        if (!g_running) {
            /* If the 'g_running' flag resets during sleep, finish processing */
            break;
        }

        subCallback(server, currentReaderGroup);
        nextnanosleeptimeSub.tv_nsec += cycleTimeNs;
        normalize(&nextnanosleeptimeSub);
    }

    UA_free(threadArgumentsSubscriber);
    return (void*)NULL;
}

/* The following 3 functions are originally/normally declared in ua_pubsub.h
 * But we want a customized cyclic interrupt as well as our own threads
 * so we use custom threads to do it. The library will call these functions
 * when it needs to register a callback, where our threads will be used instead
 * of ua_timer's
 */

UA_StatusCode UA_PubSubManager_addRepeatedCallback(UA_Server* server,
    UA_ServerCallback callback,
    void* data,
    UA_Double interval_ms,
    UA_UInt64* callbackId)
{
    /* Initialize arguments required for the thread to run */
    threadParams* params = (threadParams*)UA_malloc(sizeof(threadParams));
    if (!params) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Pass the value required for the threads */
    params->server = server;
    params->data = data;
    params->callback = callback;
    params->interval_ms = interval_ms;
    params->callbackId = callbackId;

    /* Check the writer group identifier and create the thread accordingly */
    UA_WriterGroup* tmpWriter = (UA_WriterGroup*)data;

    if (UA_NodeId_equal(&tmpWriter->identifier, &g_writerGroupIdent)) {
        char threadNamePub[10] = "Publisher";
        params->data_index = g_indexPub;
        g_thread[g_threadRun].id = threadCreation(
            PUB_THREAD_PRIORITY, g_sData->pubData[g_indexPub].cpuAffinity, pub_thread, threadNamePub, params);
        g_indexPub++;
    } else {
        char threadNameSub[11] = "Subscriber";
        params->data_index = g_indexSub;
        g_thread[g_threadRun].id = threadCreation(
            SUB_THREAD_PRIORITY, g_sData->subData[g_indexSub].cpuAffinity, sub_thread, threadNameSub, params);
        g_indexSub++;
    }

    g_threadRun++;

    return UA_STATUSCODE_GOOD;
}

__attribute__((const)) UA_StatusCode UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server* server,
    UA_UInt64 callbackId,
    UA_Double interval_ms)
{
    /* Callback interval need not be modified as it is thread based.
     * The thread uses nanosleep to calculate cycle time. A modification to the
     * nanosleep value changes cycle time.
     */
    (void)server;
    (void)callbackId;
    (void)interval_ms;

    return UA_STATUSCODE_GOOD;
}

void UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server* server, UA_UInt64 callbackId)
{
    (void)server;
    (void)callbackId;
    /* TODO move pthread_join here? */
}
