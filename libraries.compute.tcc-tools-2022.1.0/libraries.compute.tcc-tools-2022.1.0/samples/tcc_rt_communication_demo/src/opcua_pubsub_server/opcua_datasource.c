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
 * Copyright (c) 2020, Intel Corporation
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
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_ethernet_etf.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <assert.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "json_helper.h"
#include "opcua_common.h"
#include "opcua_utils.h"

static bool TheDsStarted = false;
static bool TheDsFinished = false;
static size_t TheTransmitCount = 0;
static uint32_t TheDsInitialSequence = 0;
static bool TheDsInitialSequenceLatched = false;

extern struct ServerData* g_sData;
extern UA_Boolean g_running;
static struct GenericPayload local_payload;

__attribute__((const)) UA_StatusCode dummyDSRead(__attribute__((unused)) UA_Server* server,
    __attribute__((unused)) const UA_NodeId* sessionId,
    __attribute__((unused)) void* sessionContext,
    __attribute__((unused)) const UA_NodeId* nodeId,
    __attribute__((unused)) void* nodeContext,
    __attribute__((unused)) UA_Boolean sourceTimeStamp,
    __attribute__((unused)) const UA_NumericRange* range,
    __attribute__((unused)) UA_DataValue* value)
{
    return UA_STATUSCODE_GOOD;
}


__attribute__((const)) UA_StatusCode dummyDSWrite(__attribute__((unused)) UA_Server* server,
    __attribute__((unused)) const UA_NodeId* sessionId,
    __attribute__((unused)) void* sessionContext,
    __attribute__((unused)) const UA_NodeId* nodeId,
    __attribute__((unused)) void* nodeContext,
    __attribute__((unused)) const UA_NumericRange* range,
    __attribute__((unused)) const UA_DataValue* value)
{
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode readValueFromTalkerDS(__attribute__((unused)) UA_Server* server,
    __attribute__((unused)) const UA_NodeId* sessionId,
    __attribute__((unused)) void* sessionContext,
    __attribute__((unused)) const UA_NodeId* nodeId,
    void* nodeContext,
    UA_Boolean sourceTimeStamp,
    __attribute__((unused)) const UA_NumericRange* range,
    UA_DataValue* value)
{
    if (!nodeContext) {
        LOG_ERR("The 'nodeContext' argument is missing");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    /* Check if processing has been already finished */
    if (TheDsFinished) {
        return UA_STATUSCODE_GOOD;
    }
    if (!g_running) {
        return UA_STATUSCODE_GOOD;
    }

    struct PublisherData* pdata = (struct PublisherData*)nodeContext;

    // There may be a race condition here since the talker thread may be
    // modifying the memory at the same time we're reading it
    // In theory, timing control should make sure that rt-app finishes writing
    // before we start reading, thus negating the need for accessing control
    // Depending on the scheduler this may or may not be true.
    // We use short timeslice between writer from tsn-apps and this reader code:
    // writer schedules it's write operation on a few microseconds before
    // reader's read operation.
    // This callback is calling at start of application by library engine in order to
    // test data avalability. This means that first execution time of callback is
    // not related to startTime, thus datasource might not be ready at this moment.
    // If datasource is not ready yet we just give library pointer to local
    // static empty payload instead of NULL pointer.
    struct GenericPayload* load = (struct GenericPayload*)get_value_from_datasource();
    if (load == NULL) {
        load = &local_payload;
    }

    // Check for sequence number, one reason of this is to be sure
    // that real payload generator started sharing of data.
    int32_t seq = load->sequence_num;
    if (!TheDsInitialSequenceLatched) {
        TheDsInitialSequence = seq;
        TheDsInitialSequenceLatched = true;
    }
    // Move to "started" state only if we get a valid (non-zero) sequence number
    // different from the initial sequence number (that was read at the application start-up).
    if (!TheDsStarted && (uint32_t)seq != TheDsInitialSequence && seq) {
        TheDsStarted = true;
    }

    // Declare counter responsible for current transmit idx,
    // initialize it with zero despite it is static
    if (TheDsStarted) {
        if (TheTransmitCount == g_sData->packetCount) {
            LOG_INFO("Reached limit of max transmit counts, stopping publisher\n");
            TheDsFinished = true;
        }
        ++TheTransmitCount;
    }

    // We only check if the current and previous sequence numbers are the same
    // and not if the current is one greater than the previous because it's
    // possible for the talker to miss multiple packets in a row.
    // Only when they are equal do we know that the data is stale.
    // Does not warn if seqnum is 0 since this means that the publisher has not
    // started yet
    if (seq == pdata->prev_sequence_num && TheDsStarted) {
        if (seq != 0) {
            // Just tell the user if there's packet loss; still transmit anyways
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Packet miss: seq number %d is same as previous", seq);
        }
    }
    pdata->prev_sequence_num = seq;

    // Using zero-copy reads
    UA_Variant_setArray(&value->value, load, PAYLOAD_SIZE, &UA_TYPES[UA_TYPES_BYTE]);
    value->hasValue = true;
    value->value.storageType = UA_VARIANT_DATA_NODELETE;

    if (sourceTimeStamp) {
        value->hasSourceTimestamp = true;
        value->sourceTimestamp = UA_DateTime_now();
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode writeValueToListenerDS(__attribute__((unused)) UA_Server* server,
    __attribute__((unused)) const UA_NodeId* sessionId,
    __attribute__((unused)) void* sessionContext,
    __attribute__((unused)) const UA_NodeId* nodeId,
    void* nodeContext,
    __attribute__((unused)) const UA_NumericRange* range,
    const UA_DataValue* data)
{
    if (!nodeContext) {
        LOG_ERR("The 'nodeContext' argument is missing");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if (!g_running) {
        return UA_STATUSCODE_GOOD;
    }

    // Validate incoming data is byte array
    UA_Variant v = data->value;
    if (UA_Variant_isScalar(&v) || v.type != &UA_TYPES[UA_TYPES_BYTE]) {
        LOG_DBG("Malformed incoming packet: incorrect type");
        return UA_STATUSCODE_GOOD;
    }

    struct SubscriberData* sub = (struct SubscriberData*)nodeContext;
    struct GenericPayload* incoming_data = sub->data;
    memcpy(incoming_data, v.data, PAYLOAD_SIZE);

    return UA_STATUSCODE_GOOD;
}
