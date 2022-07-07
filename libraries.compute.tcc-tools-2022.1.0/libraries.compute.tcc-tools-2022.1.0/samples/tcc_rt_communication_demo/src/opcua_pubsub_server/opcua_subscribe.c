/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

/**
 * IMPORTANT ANNOUNCEMENT
 * The PubSub Subscriber API is currently not finished. This example can be used
 * to receive and display values that are published by the tutorial_pubsub_publish
 * example in the TargetVariables of Subscriber Information Model.
 */


#include "opcua_subscribe.h"

#include "opcua_common.h"
#include "opcua_utils.h"

#include <linux/if_link.h>
#include <linux/if_xdp.h>
#include <time.h>

/* Subscriber incoming data shared with data manager */
struct GenericPayload subscriber_incoming_data;

#define SUB_MODE_ZERO_COPY

/* Add new connection to the server */
int addSubConnection(UA_Server* server, UA_NodeId* connId, struct ServerData* sdata, struct SubscriberData* sub)
{
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    UA_StatusCode ret;

    check(server, "Wrong 'server' argument");
    check(connId, "Wrong 'connId' argument");
    check(sdata, "Wrong 'sdata' argument");
    check(sub, "Wrong 'sub' argument");

    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = sdata->transportProfile;
    connectionConfig.enabled = UA_TRUE;

    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(sdata->subInterface), UA_STRING(sub->url)};

    if (sdata->useXDP) {
#ifdef SUB_MODE_ZERO_COPY
        connectionConfig.xdp_queue = sub->xdpQueue;
        connectionConfig.xdp_flags |= XDP_FLAGS_DRV_MODE;
        connectionConfig.xdp_bind_flags |= XDP_ZEROCOPY;
#else
        connectionConfig.xdp_queue = sub->xdpQueue;
        connectionConfig.xdp_flags |= XDP_FLAGS_SKB_MODE;
        connectionConfig.xdp_bind_flags |= XDP_COPY;
#endif
    }

    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    if (sub->id == 0) {
        connectionConfig.publisherId.numeric = UA_UInt32_random();
    } else {
        connectionConfig.publisherId.numeric = sub->id;
    }
    ret = UA_Server_addPubSubConnection(server, &connectionConfig, connId);
    check(ret == UA_STATUSCODE_GOOD, "Cannot add new PubSub connection");

    return 0;
error:
    return -1;
}

/* Add ReaderGroup to the created connection */
static int addReaderGroup(UA_Server* server, UA_NodeId* connId, UA_NodeId* readerGroupId)
{
    UA_ReaderGroupConfig readerGroupConfig;
    UA_StatusCode ret;

    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");

    ret = UA_Server_addReaderGroup(server, *connId, &readerGroupConfig, readerGroupId);
    if (ret != UA_STATUSCODE_GOOD) {
        return -1;
    }
    return 0;
}

/* Add DataSetReader to the ReaderGroup */
static int addDataSetReader(UA_Server* server,
    UA_NodeId* readerGroupIdentifier,
    UA_NodeId* datasetReaderIdentifier,
    struct SubscriberData* sub)
{
    UA_DataSetReaderConfig readerConfig;
    UA_StatusCode ret;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    readerConfig.dataSetWriterId = (UA_UInt16)sub->subscribedDSWriterId;
    UA_UInt16 publisherIdentifier = (UA_UInt16)sub->subscribedPubId;
    readerConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data = &publisherIdentifier;
    readerConfig.writerGroupId = (UA_UInt16)sub->subscribedWGId;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType* pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init(pMetaData);

    /* Static definition of number of fields size = 1 */
    pMetaData->name = UA_STRING("DataSet Test");
    pMetaData->fieldsSize = 1;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new(pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    check(pMetaData->fields, "Cannot allocate memory for field meta data");
    sub->pMetaData_fields = (char*)pMetaData->fields;

    /* Sequence and timestamp is UINT64 DataType */
    UA_FieldMetaData_init(&pMetaData->fields[0]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId, &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_TYPES_UINT64;
    pMetaData->fields[0].name = UA_STRING("Sequence timestamp");
    pMetaData->fields[0].valueRank = -3; /* scalar or 1-D array */

    ret = UA_Server_addDataSetReader(server, *readerGroupIdentifier, &readerConfig, datasetReaderIdentifier);
    check(ret == UA_STATUSCODE_GOOD, "Cannot add data set reader");
    return 0;

error:
    return -1;
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader
 */
static int addSubscribedVariables(UA_Server* server, struct SubscriberData* sub, UA_NodeId* datasetReaderIdentifier)
{
    /* Add another variable, as a datasource node. */
    /* 1) Set the variable attributes. */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_StatusCode ret;

    attr.displayName = UA_LOCALIZEDTEXT("en_US", "Time stamp datasource");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* 2) Define where the variable will be added with its corresponding browse name */
    UA_NodeId datasource_node_id = UA_NODEID_STRING(1, "time-stamp-datasource");
    UA_NodeId parent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parent_ref_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variable_type = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_QualifiedName browse_name = UA_QUALIFIEDNAME(1, "Time stamp datasource");

    UA_DataSource data_source;

    data_source.read = sub->readFunc;
    data_source.write = sub->writeFunc;

    /* 3) Add the variable with data source. */
    ret = UA_Server_addDataSourceVariableNode(server,
        datasource_node_id,
        parent_node_id,
        parent_ref_node_id,
        browse_name,
        variable_type,
        attr,
        data_source,
        sub,
        NULL);
    check(ret == UA_STATUSCODE_GOOD, "Cannot add data source variable");

    UA_TargetVariablesDataType targetVars;

    targetVars.targetVariablesSize = 1;
    targetVars.targetVariables =
        (UA_FieldTargetDataType*)UA_calloc(targetVars.targetVariablesSize, sizeof(UA_FieldTargetDataType));
    check(targetVars.targetVariables, "Cannot allocate memory for target variables");
    sub->targetVars = targetVars.targetVariables;

    UA_FieldTargetDataType_init(&targetVars.targetVariables[0]);
    targetVars.targetVariables[0].attributeId = UA_ATTRIBUTEID_VALUE;
    targetVars.targetVariables[0].targetNodeId = datasource_node_id;
    ret = UA_Server_DataSetReader_createTargetVariables(server, *datasetReaderIdentifier, &targetVars);
    if (ret != UA_STATUSCODE_GOOD) {
        UA_free(targetVars.targetVariables);
        sub->targetVars = NULL;
    }
    check(ret == UA_STATUSCODE_GOOD, "Cannot create target variables");

    return 0;
error:

    return -1;
}

int createSubscriber(UA_Server* server,
    __attribute__((unused)) struct ServerData* sdata,
    struct SubscriberData* sub,
    UA_NodeId* connId)
{
    UA_NodeId readerGroupIdentifier;
    UA_NodeId dataSetReaderIdentifier;
    int iret = -1;

    check(server, "Wrong 'server' argument");
    check(sdata, "Wrong 'sdata' argument");
    check(sub, "Wrong 'sub' argument");
    check(connId, "Wrong 'connId' argument");

    iret = addReaderGroup(server, connId, &readerGroupIdentifier);
    check(iret != -1, "Cannot add reader group");
    iret = addDataSetReader(server, &readerGroupIdentifier, &dataSetReaderIdentifier, sub);
    check(iret != -1, "Cannot add data set reader");
    iret = addSubscribedVariables(server, sub, &dataSetReaderIdentifier);
    check(iret != -1, "Cannot add subscribed variables");
    return 0;

error:
    return -1;
}
