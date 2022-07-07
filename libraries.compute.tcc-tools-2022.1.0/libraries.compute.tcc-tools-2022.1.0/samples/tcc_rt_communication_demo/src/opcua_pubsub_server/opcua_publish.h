/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_OPCUA_PUBLISH_H_
#define _TCC_OPCUA_PUBLISH_H_

#include <open62541/server.h>

/**
 * @file opcua_publish.h
 * @brief This file defines the OPC UA* Server publishers API
 *
 * This file defines the convenience API to work with OPC UA* Server publishers.
 */


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

struct UA_Server;
typedef struct UA_Server UA_Server;
struct ServerData;
struct PublisherData;

/**
 * Create a new publisher and set it up to work with the OPC UA* server
 *
 * @param[in,out] serv         The pointer to the OPC UA* server object to which the new publisher will be added
 * @param[in] sdata            The structure with OPC UA* server data
 * @param[in] pdata            The structure with OPC UA* publisher data
 * @param[in] connId           Connection configuration node
 * @return The success status, \c 0 on success, \c -1 on failure
 */
int createPublisher(UA_Server* serv, struct ServerData* sdata, struct PublisherData* pdata, UA_NodeId* connId);

/**
 * Add a connection between server and publisher according to the data specified.
 *
 * @param[in,out] serv           The pointer to the OPC UA* server object
 * @param[in] connId             Connection configuration node
 * @param[in] sdata              The structure with OPC UA* server data
 * @param[in] pdata              The structure with OPC UA* publisher data
 * @return The success status, \c 0 on success, \c 1 on failure
 */
int addPubSubConnection(UA_Server* serv, UA_NodeId* connId, struct ServerData* sdata, struct PublisherData* pdata);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _TCC_OPCUA_PUBLISH_H_

// vim: et:ts=4:sw=4
