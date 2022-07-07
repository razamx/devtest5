/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_OPCUA_SUBSCRIBE_H_
#define _TCC_OPCUA_SUBSCRIBE_H_

#include <open62541/server.h>

/**
 * @file opcua_subscribe.h
 * @brief This file defines the OPC UA* Server subscribers API
 *
 * This file defines the convenience API to work with OPC UA* Server subscribers.
 */


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

struct UA_Server;
typedef struct UA_Server UA_Server;
struct ServerData;
struct SubscriberData;

/**
 * Create a new OPC UA* subscriber and set it up to work in the OPC UA* server
 *
 * @param[in,out] server  The pointer to the OPC UA* server object to which the new subscriber is added
 * @param[in] sdata       The OPC UA* Server data
 * @param[in] sub         The parameters defining the OPC UA* subscriber object
 * @param[in] connId      Connection configuration node
 * @return The newly allocated structure with data related to the newly created OPC UA* subscriber
 */
int createSubscriber(UA_Server* server, struct ServerData* sdata, struct SubscriberData* sub, UA_NodeId* connId);

/**
 * Add a connection between server and subscriber according to the data specified
 *
 * @param[in,out] server  The pointer to the OPC UA* server object to which the new subscriber is added
 * @param[in] connId      Connection configuration node
 * @param[in] sdata       The OPC UA* server data
 * @param[in] sub         Subscriber connected to server
 * @return The success status, \c 0 on success, \c -1 on failure
 */
int addSubConnection(UA_Server* server, UA_NodeId* connId, struct ServerData* sdata, struct SubscriberData* sub);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _TCC_OPCUA_SUBSCRIBE_H_
