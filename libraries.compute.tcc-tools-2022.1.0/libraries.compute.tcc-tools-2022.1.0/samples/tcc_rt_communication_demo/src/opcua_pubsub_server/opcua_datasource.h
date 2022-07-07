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
#ifndef _TCC_OPCUA_DATASOURCE_H_
#define _TCC_OPCUA_DATASOURCE_H_

/**
 * @file opcua_datasource.h
 * @brief This file defines the API for the OPC UA* Datasource used for passing values between simulated
 *        sensors/actuators and OPC UA* Subscribers/Publishers (network).
 *
 * This file defines the API for the OPC UA* Datasource used for passing values from simulated
 * sensors to the OPC UA* Publisher (network) and from the OPC UA* Subscriber (network) to
 * the simulated actuator.
 */

#include <inttypes.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <stdio.h>
#include <string.h>

#include "opcua_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * A dummy (no-operation) handler for the OPC UA* Server Write callbacks
 * @param[in] server The pointer to the OPC UA* Server object
 * @param[in] sessionId The session ID
 * @param[in] sessionContext The pointer to the session context
 * @param[in] nodeId The OPC UA* Server node ID
 * @param[in] nodeContext The pointer to the node context
 * @param[in] range The pointer to the range object defining the correct range for the current value
 * @param[in] value The pointer to the value object to be written
 * @return The success code for the operation
 */
UA_StatusCode dummyDSWrite(UA_Server* server,
    const UA_NodeId* sessionId,
    void* sessionContext,
    const UA_NodeId* nodeId,
    void* nodeContext,
    const UA_NumericRange* range,
    const UA_DataValue* value);

/**
 * A dummy (no-operation) handler for the OPC UA* Server Read callbacks
 * @param[in] server The pointer to the OPC UA* Server object
 * @param[in] sessionId The session ID
 * @param[in] sessionContext The pointer to the session context
 * @param[in] nodeId The OPC UA* Server node ID
 * @param[in] nodeContext The pointer to the node context
 * @param[in] sourceTimeStamp Indication if the timestamp needs to be included in the output data
 * @param[in] range The pointer to the range object defining the correct range for the current value
 * @param[in] value The pointer to the value object to be read
 * @return The success code for the operation
 */
UA_StatusCode dummyDSRead(UA_Server* server,
    const UA_NodeId* sessionId,
    void* sessionContext,
    const UA_NodeId* nodeId,
    void* nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange* range,
    UA_DataValue* value);
/**
 * Callback function executes a read operation on the memory region
 *
 * @param server The pointer on storage of UA_Server data with generic OPC UA* server settings
 * @param sessionId The pointer on storage of UA_NodeId data with identifier for a session in OPC UA* server memory
 * @param sessionContext The pointer on storage of user's specified/prepared/preserved data
 * @param nodeId The pointer on storage of UA_NodeId data with identifier for a node in OPC UA* server memory
 * @param nodeContext The pointer on storage of user's specified/prepared/preserved data
 * @param sourceTimeStamp The Boolean value with indication of necessity to pass/check the timestamp
 * @param range The pointer on storage of UA_NumericRange data. If not NULL,
 *        then the data source shall return only a selection of the (nonscalar) data. Set
 *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not apply.
 * @param value The pointer on storage of Ua_dataValue data. The (non-NULL) DataValue that has been written by the
 * client.
 *        The data source contains the written data, the result status, and optionally a sourcetimestamp.
 * @return Status code to be logged by the open62541 library
 * @note Function uses the API provided by the data manager to receive payload data from the data source
 */
UA_StatusCode readValueFromTalkerDS(UA_Server* server,
    const UA_NodeId* sessionId,
    void* sessionContext,
    const UA_NodeId* nodeId,
    void* nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange* range,
    UA_DataValue* value);

/**
 * Callback function executes a write operation on the memory region
 *
 * @param server The pointer on storage of UA_Server data with generic OPC UA* server settings
 * @param sessionId The pointer on storage of UA_NodeId data with identifier for a session in OPC UA* server memory
 * @param sessionContext The pointer on storage of user's specified/prepared/preserved data
 * @param nodeId The pointer on storage of UA_NodeId data with identifier for a node in OPC UA* server memory
 * @param nodeContext The pointer on storage of user's specified/prepared/preserved data
 * @param range The pointer on storage of UA_NumericRange data. If not NULL,
 *        then the data source shall return only a selection of the (nonscalar) data. Set
 *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not apply.
 * @param data The pointer to the received (in the OPC UA* subscriber) data to be stored in the memory
 * @return Status code to be logged by the open62541 library
 */
UA_StatusCode writeValueToListenerDS(UA_Server* server,
    const UA_NodeId* sessionId,
    void* sessionContext,
    const UA_NodeId* nodeId,
    void* nodeContext,
    const UA_NumericRange* range,
    const UA_DataValue* data);

#ifdef __cplusplus
} /* extern "C" { */
#endif /* __cplusplus */

#endif /* _TCC_OPCUA_DATASOURCE_H_ */
