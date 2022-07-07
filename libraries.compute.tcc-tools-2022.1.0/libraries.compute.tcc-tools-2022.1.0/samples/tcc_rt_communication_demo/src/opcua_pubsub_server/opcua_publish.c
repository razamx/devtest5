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
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tutorial:
 *
 * Working with Publish/Subscribe
 * ------------------------------
 *
 * Work in progress: This Tutorial will be continuously extended during the next
 * PubSub batches. More details about the PubSub extension and corresponding
 * open62541 API are located here: :ref:`pubsub`.
 *
 * Publishing Fields
 * ^^^^^^^^^^^^^^^^^
 * The PubSub publish example demonstrate the simplest way to publish
 * informations from the information model over UDP multicast using the UADP
 * encoding.
 *
 * **Connection handling**
 *
 * PubSubConnections can be created and deleted on runtime. More details about
 * the system preconfiguration and connection can be found in
 * ``tutorial_pubsub_connection.c``.
 */

#include "opcua_publish.h"

#include "opcua_common.h"
#include "opcua_utils.h"

#include <linux/if_link.h>
#include <linux/if_xdp.h>
#include <time.h>


#define PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS  // TODO define this elsewhere
#define KEYFRAME_COUNT 10
#define PUB_MODE_ZERO_COPY

extern UA_NodeId g_writerGroupIdent;

int addPubSubConnection(UA_Server* serv, UA_NodeId* connId, struct ServerData* sdata, struct PublisherData* pdata)
{
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;

    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    connectionConfig.transportProfileUri = sdata->transportProfile;
    connectionConfig.enabled = UA_TRUE;

    int socketPrio = pdata->socketPriority;
    connectionConfig.etfConfiguration.socketPriority = socketPrio > 0 ? socketPrio : -1;
    UA_StatusCode ret;
    LOG_DBG("Setting socket prio to %d", connectionConfig.etfConfiguration.socketPriority);
    connectionConfig.etfConfiguration.sotxtimeEnabled = UA_TRUE;

    if (sdata->useXDP) {
#ifdef PUB_MODE_ZERO_COPY
        connectionConfig.xdp_queue = pdata->xdpQueue;
        connectionConfig.xdp_flags |= XDP_FLAGS_DRV_MODE;
        connectionConfig.xdp_bind_flags |= XDP_ZEROCOPY;
#else
        connectionConfig.xdp_queue = pdata->xdpQueue;
        connectionConfig.xdp_flags |= XDP_FLAGS_SKB_MODE;
        connectionConfig.xdp_bind_flags |= XDP_COPY;
#endif
    }

    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(sdata->pubInterface), UA_STRING(pdata->url)};

    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    connectionConfig.publisherId.numeric = (UA_UInt16)pdata->id;
    ret = UA_Server_addPubSubConnection(serv, &connectionConfig, connId);
    check(ret == UA_STATUSCODE_GOOD, "Cannot add new PubSub connection");

    return 0;

error:
    return -1;
}

/**
 * **PublishedDataSet handling**
 *
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection. */
static int addPublishedDataSet(UA_Server* server, UA_NodeId* publishedDataSetIdent)
{
    /* The PublishedDataSetConfig contains all necessary public
     * informations for the creation of a new PublishedDataSet
     */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    UA_AddPublishedDataSetResult addRet;

    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    addRet = UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, publishedDataSetIdent);
    if (addRet.addResult != UA_STATUSCODE_GOOD) {
        return -1;
    }
    return 0;
}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one published
 * field. */
static int addDataSetField(UA_Server* server,
    UA_NodeId* publishedDataSetIdent,
    __attribute__((unused)) struct ServerData* sdata,
    struct PublisherData* pdata)
{
    /* Add a variable datasource node. */
    /* Set the variable attributes. */
    UA_StatusCode statusCode;
    UA_DataSetFieldResult dataSetFieldResult;
    UA_VariableAttributes attr = UA_VariableAttributes_default;

    attr.displayName = UA_LOCALIZEDTEXT("en_US", "Payload datasource");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Define where the variable shall be added with which browse name */
    UA_NodeId new_node_id = UA_NODEID_STRING(1, "payload-datasource");
    UA_NodeId parent_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parent_ref_node_id = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variable_type = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);
    UA_QualifiedName browse_name = UA_QUALIFIEDNAME(1, "Shared mem DS");

    UA_DataSource data_source;

    data_source.read = pdata->readFunc;
    data_source.write = pdata->writeFunc;

    statusCode = UA_Server_addDataSourceVariableNode(server,
        new_node_id,
        parent_node_id,
        parent_ref_node_id,
        browse_name,
        variable_type,
        attr,
        data_source,
        pdata,
        NULL);

    if (statusCode != UA_STATUSCODE_GOOD) {
        LOG_ERR("Failed 'UA_Server_addDataSourceVariableNode' with %d", statusCode);
        return -1;
    }

    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("payload");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_STRING(1, "payload-datasource");
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    dataSetFieldResult =
        UA_Server_addDataSetField(server, *publishedDataSetIdent, &dataSetFieldConfig, &dataSetFieldIdent);

    if (dataSetFieldResult.result != UA_STATUSCODE_GOOD) {
        LOG_ERR("Failed 'UA_Server_addDataSetField' with %d", dataSetFieldResult.result);
        return -1;
    }

    return 0;
}

/*
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static int addWriterGroup(UA_Server* server,
    UA_NodeId* connId,
    UA_NodeId* writerGroupIdent,
    struct ServerData* sdata,
    struct PublisherData* pdata)
{
    double intervalMs = (double)sdata->cycleTimeNs / 1e6;
    UA_StatusCode ret;
    LOG_DBG("setting interval to %lf", intervalMs);

    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    // This is in miliseconds, not nanoseconds
    writerGroupConfig.publishingInterval = intervalMs;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = pdata->writerGroupId;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
#if defined PUBSUB_CONFIG_FASTPATH_FIXED_OFFSETS
// TODO: enable rtlevel to increase performance?
// writerGroupConfig.rtLevel            = UA_PUBSUB_RT_FIXED_SIZE;
#endif
    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];

    /* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType
     */
    UA_UadpWriterGroupMessageDataType* writerGroupMessage = UA_UadpWriterGroupMessageDataType_new();
    check(writerGroupMessage != NULL, "Cannot allocate memory for writer group message");
    pdata->writerGroupMessage = writerGroupMessage;
    /* Change message settings of writerGroup to send PublisherId,
     * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
     * of NetworkMessage
     */
    writerGroupMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(
        UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;

    ret = UA_Server_addWriterGroup(server, *connId, &writerGroupConfig, writerGroupIdent);
    check(ret == UA_STATUSCODE_GOOD, "Cannot add new writer group");
    ret = UA_Server_setWriterGroupOperational(server, *writerGroupIdent);
    check(ret == UA_STATUSCODE_GOOD, "Cannot set new writer group operational");
    UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);

    return 0;
error:
    return -1;
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static int addDataSetWriter(UA_Server* server,
    UA_NodeId* writerGroupIdent,
    UA_NodeId* publishedDataSetIdent,
    struct PublisherData* pdata)
{
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_StatusCode statusCode;
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = (UA_UInt16)pdata->dataSetWriterId;
    dataSetWriterConfig.keyFrameCount = KEYFRAME_COUNT;

    statusCode = UA_Server_addDataSetWriter(
        server, *writerGroupIdent, *publishedDataSetIdent, &dataSetWriterConfig, &dataSetWriterIdent);

    if (statusCode != UA_STATUSCODE_GOOD) {
        LOG_ERR("Failed 'UA_Server_addDataSetWriter' with %d", statusCode);
        return -1;
    }

    return 0;
}

int createPublisher(UA_Server* serv, struct ServerData* sdata, struct PublisherData* pdata, UA_NodeId* connId)
{
    int retCode = 0;
    UA_StatusCode retStatus;
    UA_NodeId publishedDataSetIdent;

    if (serv == NULL || sdata == NULL || pdata == NULL || connId == NULL) {
        retCode = -1;
        if (serv == NULL) {
            LOG_ERR("UA_Server pointer cannot be NULL");
        } else if (sdata == NULL) {
            LOG_ERR("ServerData pointer cannot be NULL");
        } else if (pdata == NULL) {
            LOG_ERR("PublisherData pointer cannot be NULL");
        } else {
            LOG_ERR("NodeId pointer cannot be NULL");
        }
        return retCode;
    }

    retCode = addPublishedDataSet(serv, &publishedDataSetIdent);
    if (retCode != 0) {
        LOG_ERR("Unable to add a published data set");
        return retCode;
    }

    retCode = addDataSetField(serv, &publishedDataSetIdent, sdata, pdata);
    if (retCode != 0) {
        LOG_ERR("Unable to add a data set field");
        return retCode;
    }

    retCode = addWriterGroup(serv, connId, &g_writerGroupIdent, sdata, pdata);
    if (retCode != 0) {
        LOG_ERR("Unable to add new writer group");
        return retCode;
    }

    retCode = addDataSetWriter(serv, &g_writerGroupIdent, &publishedDataSetIdent, pdata);
    if (retCode != 0) {
        LOG_ERR("Unable to add a data set writer");
        return retCode;
    }

    retStatus = UA_Server_freezeWriterGroupConfiguration(serv, g_writerGroupIdent);
    if (retStatus != UA_STATUSCODE_GOOD) {
        LOG_ERR("Unable to freeze writer group configuration");
        return -1;
    }

    return 0;
}

// vim: et:ts=4:sw=4
