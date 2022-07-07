/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_OPCUA_COMMON_H_
#define _TCC_OPCUA_COMMON_H_

/**
 * @file opcua_common.h
 * @brief This file defines a common set of helper functions of OPC UA* Servers.
 *
 * This file defines the helper API for other pub/sub logic of OPC UA* Servers.
 */

#include "opcua_shared.h"
#include "pdatasource.h"

#include <open62541/plugin/pubsub.h>
#include <open62541/plugin/pubsub_ethernet_etf.h>
#include <open62541/plugin/pubsub_ethernet_xdp.h>
#include <open62541/server_pubsub.h>
#include <stddef.h>

#define PAYLOAD_SIZE (sizeof(struct GenericPayload))

/**
 * Callback prototype for OPC UA* Read function.
 *
 * @param[in] server The pointer to the OPC UA* Server instance.
 * @param[in] sessionId The pointer to the OPC UA* Node ID object representing the Session ID.
 * @param[in] sessionContext The pointer to the OPC UA* session context.
 * @param[in] nodeId The Node ID object representing the node for which read operation is requested.
 * @param[in] nodeContext The pointer to the OPC UA* node context.
 * @param[in] sourceTimeStamp The flag defining whether to append the timestamp to \c value.
 * @param[in] range The pointer on storage of UA_NumericRange data. If not NULL,
 *            the data source returns only a selection of the nonscalar data. Set
 *            UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not apply.
 * @param[in,out] value The value object to hold the read value and, possibly, its timestamp.
 * @return The OPC UA* status of the read operation, \c UA_STATUSCODE_GOOD on success,
 *       \c any of UA_STATUSCODE_BAD* values on failure.
 * @note The OPC UA* Server uses this callback to read the current value for the corresponding
 *       publisher before this value is sent to the OPC UA* subscribers.
 */
typedef UA_StatusCode(DSCallbackRead)(UA_Server* server,
    const UA_NodeId* sessionId,
    void* sessionContext,
    const UA_NodeId* nodeId,
    void* nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange* range,
    UA_DataValue* value);

/**
 * Callback prototype for OPC UA* Write function.
 *
 * @param[in] server The pointer to the OPC UA* Server instance.
 * @param[in] sessionId The pointer to the OPC UA* Node ID object representing the Session ID.
 * @param[in] sessionContext The pointer to the OPC UA* session context.
 * @param[in] nodeId The Node ID object representing the node for which write operation is requested.
 * @param[in] nodeContext The pointer to the OPC UA* node context.
 * @param[in] range The pointer on storage of UA_NumericRange data. If not NULL,
 *            then the data source shall return only a selection of the nonscalar data. Set
 *            UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not apply.
 * @param[in] value The value object to hold the received value to be
 *            stored/handled in OPC UA* subscribers.
 * @return The OPC UA* status of the write operation, \c UA_STATUSCODE_GOOD on success,
 *            \c any of UA_STATUSCODE_BAD* values on failure.
 * @note The OPC UA* Server uses this callback to write/pass the received value in the OPC UA* subscriber
         to be stored/handled in the current client.
 */
typedef UA_StatusCode(DSCallbackWrite)(UA_Server* server,
    const UA_NodeId* sessionId,
    void* sessionContext,
    const UA_NodeId* nodeId,
    void* nodeContext,
    const UA_NumericRange* range,
    const UA_DataValue* value);


/**
 * @brief Configuration data used for creating an OPC UA* publisher
 */
struct PublisherData
{
    /** The configuration socket priority to be used with this publishing communication */
    int socketPriority;
    /** The configuration time offset to call the publisher callbacks */
    int32_t earlyOffsetNs;
    /** The configuration time offset for the publishing events */
    int32_t publishOffsetNs;
    /** The allocated string with the publisher URL */
    char* url;
    /** The numeric ID representing this publisher */
    int id;
    /** The sequence number sent in the previous publishing event */
    int32_t prev_sequence_num;
    /** The pointer to the OPC UA* read function for the publisher */
    DSCallbackRead* readFunc;
    /** The pointer to the OPC UA* write function for the publisher */
    DSCallbackWrite* writeFunc;
    int xdpQueue;
    int32_t dataSetWriterId;
    int32_t writerGroupId;
    size_t cpuAffinity;
    UA_UadpWriterGroupMessageDataType* writerGroupMessage;
};

/**
 * @brief Configuration data used for creating an OPC UA* subscriber
 */
struct SubscriberData
{
    /** The configuration time offset to call the subscriber callbacks */
    int32_t offsetNs;
    /** The allocated string with the subscriber URL */
    char* url;
    /** The numeric ID representing this subscriber */
    int id;
    /** The pointer to the OPC UA* read function for the subscriber */
    DSCallbackRead* readFunc;
    /** The pointer to the OPC UA* write function for the subscriber */
    DSCallbackWrite* writeFunc;
    /** The pointer to the payload data for the subscriber */
    struct GenericPayload* data;
    /** The XDP Queue ID for the subscriber, or -1 if the XDP Queue is not used */
    int xdpQueue;
    /** The subscribed publisher ID */
    int32_t subscribedPubId;
    /** The subscribed datasource writer ID */
    int32_t subscribedDSWriterId;
    /** The subscribed writer group ID */
    int32_t subscribedWGId;
    /** The requested CPU affinity for the subscriber */
    size_t cpuAffinity;
    /** The pointer to the fields array metadata, to be released when no longer required */
    void* pMetaData_fields;
    /** The pointer to the target data type, to be released when no longer required */
    void* targetVars;
};

/**
 * @brief The configuration data for creating and managing the OPC UA* Server
 */
struct ServerData
{
    /** The allocated publisher interface name */
    char* pubInterface;
    /** The allocated subscriber interface name */
    char* subInterface;
    /** The base publishing/subscribing interval in nanoseconds */
    int64_t cycleTimeNs;
    /** The polling interval in nanoseconds to be used in the OPC UA* reader groups */
    int32_t pollingDurationNs;
    /** The CPU index to be used to run the OPC UA* Server application */
    int cpu;

    /** The number of OPC UA* publishers in the current configuration */
    int pubCount;
    /** The number of OPC UA* subscribers in the current configuration */
    int subCount;
    /** The allocated array of the OPC UA* publisher configuration objects */
    struct PublisherData* pubData;
    /** The allocated array of the OPC UA* subscriber configuration objects */
    struct SubscriberData* subData;

    /** The OPC UA* connection URI to be used in OPC UA* publishers and subscribers */
    UA_String transportProfile;
    /* The CPU affinity for the TCC Workload */
    int tcc_affinity;
    /* The TCC latency */
    uint64_t tcc_latency;
    bool useXDP;
    UA_UInt64 packetCount;
    UA_PubSubTransportLayer* PubSubTransportLayers;
    UA_Server* server;
    uint64_t baseTime;
};


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * Parse the command-line interface (CLI) arguments provided by the user and scripts to the OPC UA* server
 *
 * @param[in] argc The counter of passed arguments
 * @param[in] argv The pointer to arrays of chars containing CLI arguments
 * @return The pointer to allocated struct ServerData with values from parsed CLI arguments
 * @note The content of the returned object should be freed with \c cleanServerData after use and
 *       the \c ServerData object should be freed with \c free system call when no longer needed.
 */
struct ServerData* parseArgs(int argc, char* argv[]);

/**
 * Release the memory allocated for the OPC UA* server's ServerData and submember structures
 *
 * @param[in] sdata The pointer to memory storage of ServerData struct to be freed
 */
void cleanServerData(struct ServerData* sdata);

/**
 * Convert time in \c timespec structure to nanoseconds
 * @param[in] ts The time in \c timespec structure to be converted
 * @return The input time converted to nanoseconds
 */
UA_UInt64 as_nanoseconds(const struct timespec* ts);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _TCC_OPCUA_COMMON_H_

/* vim: set ts=4 sw=4 et: */
