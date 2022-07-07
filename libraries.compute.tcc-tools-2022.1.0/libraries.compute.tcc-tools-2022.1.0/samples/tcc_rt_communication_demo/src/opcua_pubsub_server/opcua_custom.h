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
#ifndef _TCC_OPCUA_CUSTOM_H_
#define _TCC_OPCUA_CUSTOM_H_

/**
 * @file opcua_custom.h
 * @brief This file defines the API for custom threads management in the OPC UA* Server for
 *        the OPC UA* Subscribers' and OPC UA* Publishers' periodic tasks.
 *
 * This file defines the API for implementing custom periodic OPC UA* Publisher/Subscriber tasks
 * in the OPC UA* Server. The API includes data types and functions for adding, modifying, and
 * removing periodic tasks for OPC UA* Publishers and Subscribers.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "opcua_utils.h"

/**
 * The structure to pass information to the OPC UA* Server Publisher/Subscriber threads
 */
typedef struct threadParams
{
    /** The pointer to the main OPC UA* Server object */
    UA_Server* server;
    /** The pointer to OPC UA* Write/Reader group information */
    void* data;
    /** The pointer to the actual OPC UA* Publisher/Subscriber task */
    UA_ServerCallback callback;
    /** The interval for the periodic publisher/subscriber task */
    UA_Duration interval_ms;
    /** The callback identifier */
    UA_UInt64* callbackId;
    /** The index for the Publisher/Subscriber data for the current thread */
    UA_UInt32 data_index;
    /** The thread ID for the current periodic task */
    pthread_t id;
} threadParams;

/**
 * The publisher thread function
 * @param[in] arg The pointer to the publisher thread information in a previously allocated
 *                structure \c threadParams. The structure will be freed when the thread finishes
 * @return The thread function return value, not used, we just return \c NULL
 */
void* pub_thread(void* arg);
/**
 * The subscriber thread function
 * @param[in] arg The pointer to the subscriber thread information in a previously allocated
 *                structure \c threadParams. The structure will be freed when the thread finishes
 * @return The thread function return value, not used, we just return \c NULL
 */
void* sub_thread(void* arg);

/**
 * Implements the request from the OPC UA* Server to create a custom task (thread) that periodically
 * invokes the passed callback with requested period.
 * @param[in] server The pointer to the main OPC UA* Server object
 * @param[in] callback The callback to be called periodically
 * @param[in] data The pointer to the \c UA_WriterGroup object associated with the current
 *                 OPC UA* Publisher/Subscriber
 * @param[in] interval_ms The requested period for the periodic Publisher/Subscriber tasks in milliseconds
 * @param[out] callbackId The pointer to the callback ID for the just added repeated task
 * @return The success status of the operation
 */
UA_StatusCode UA_PubSubManager_addRepeatedCallback(UA_Server* server,
    UA_ServerCallback callback,
    void* data,
    UA_Double interval_ms,
    UA_UInt64* callbackId);

/**
 * The handler for the OPC UA* Server request to change the interval
 * for the periodic publish/subscribe tasks
 * @param[in] server The pointer to the main OPC UA* Server object
 * @param[in] callbackId
 * @param[in] interval_ms The new interval value in milliseconds
 *                        for the periodic publish/subscribe tasks
 * @return The success status code of the operation
 * @note Currently, this handler is neither used nor implemented
 */
UA_StatusCode UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server* server,
    UA_UInt64 callbackId,
    UA_Double interval_ms);

/**
 * The handler for the OPC UA* Server request to remove the previously added periodic callback
 * @param[in] server The pointer to the main OPC UA* Server object
 * @param[in] callbackId The callback ID for the periodic task to be removed
 * @note Currently, this handler is neither used nor implemented
 */
void UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server* server, UA_UInt64 callbackId);

#endif  // _TCC_OPCUA_CUSTOM_H_
