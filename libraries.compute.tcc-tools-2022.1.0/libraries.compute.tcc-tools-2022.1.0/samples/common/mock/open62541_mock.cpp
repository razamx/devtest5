/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "open62541_mock.hpp"

#include <open62541/server_config_default.h>
#include <open62541/plugin/pubsub_ethernet_etf.h>
#include <open62541/plugin/pubsub_ethernet_xdp.h>

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnNull;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(open62541_mock)

void open62541_mock::init() {
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = {
        UA_STATUSCODE_BADUNEXPECTEDERROR,
        0,
        NULL,
        {
            0,
            0
        }
    };
    UA_DataSetFieldResult retval_data_set_field_result = {
        UA_STATUSCODE_BADUNEXPECTEDERROR,
        {
            0,
            0
        }
    };

    ON_CALL(*this, UA_Server_addPubSubConnection(_, _, _))
        .WillByDefault(Return(UA_STATUSCODE_BADUNEXPECTEDERROR));
    ON_CALL(*this, UA_Server_addPublishedDataSet(_, _, _))
        .WillByDefault(Return(retval_add_published_data_set_result));
    ON_CALL(*this, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillByDefault(Return(UA_STATUSCODE_BADUNEXPECTEDERROR));
    ON_CALL(*this, UA_Server_addDataSetField(_, _, _, _))
        .WillByDefault(Return(retval_data_set_field_result));
    ON_CALL(*this, UA_Server_addWriterGroup(_, _, _, _))
        .WillByDefault(Return(UA_STATUSCODE_BADUNEXPECTEDERROR));
    ON_CALL(*this, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillByDefault(Return(UA_STATUSCODE_BADUNEXPECTEDERROR));

    ON_CALL(*this, UA_clear(_, _))
        .WillByDefault(Return());
    ON_CALL(*this, UA_Array_delete(_, _, _))
        .WillByDefault(Return());
    ON_CALL(*this, UA_Server_new())
        .WillByDefault(ReturnNull());
    ON_CALL(*this, UA_Server_delete(_))
        .WillByDefault(Return());
    ON_CALL(*this, UA_copy(_, _, _))
        .WillByDefault(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    ON_CALL(*this, UA_Variant_setScalar(_, _, _))
        .WillByDefault(Return());
    ON_CALL(*this, UA_Array_new(_, _))
        .WillByDefault(ReturnNull());
    ON_CALL(*this, UA_UInt32_random())
        .WillByDefault(Return(0));
    ON_CALL(*this, UA_Log_Stdout_log(_, _, _, _, _))
        .WillByDefault(Return());
    ON_CALL(*this, UA_QualifiedName_equal(_, _))
        .WillByDefault(Return(UA_FALSE));
    ON_CALL(*this, UA_NodeId_order(_, _))
        .WillByDefault(Return(UA_ORDER_EQ));
    ON_CALL(*this, UA_String_equal(_, _))
        .WillByDefault(Return(UA_FALSE));
    ON_CALL(*this, UA_new(_))
        .WillByDefault(ReturnNull());
    ON_CALL(*this, UA_delete(_, _))
        .WillByDefault(Return());
    ON_CALL(*this, UA_Server_setWriterGroupOperational(_, _))
        .WillByDefault(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    ON_CALL(*this, UA_Server_freezeWriterGroupConfiguration(_, _))
        .WillByDefault(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    ON_CALL(*this, UA_Variant_setArray(_, _, _, _))
        .WillByDefault(Return());
    ON_CALL(*this, UA_DateTime_now())
        .WillByDefault(Return(UA_DateTime {}));
    ON_CALL(*this, UA_PubSubTransportLayerEthernetXDP())
        .WillByDefault(Return(UA_PubSubTransportLayer {}));
    ON_CALL(*this, UA_PubSubTransportLayerEthernetETF())
        .WillByDefault(Return(UA_PubSubTransportLayer {}));
    ON_CALL(*this, UA_Server_run(_, _))
        .WillByDefault(Return(UA_STATUSCODE_BADUNEXPECTEDERROR));
    ON_CALL(*this, UA_Server_getConfig(_))
        .WillByDefault(Return(nullptr));
    ON_CALL(*this, UA_ServerConfig_setMinimalCustomBuffer(_, _, _, _, _))
        .WillByDefault(Return(UA_STATUSCODE_BADUNEXPECTEDERROR));
}

UA_StatusCode UA_Server_addPubSubConnection(UA_Server *server,
        const UA_PubSubConnectionConfig *connectionConfig, UA_NodeId *connectionIdentifier) {

    return open62541_mock::get().UA_Server_addPubSubConnection(server, connectionConfig, connectionIdentifier);
}

UA_AddPublishedDataSetResult UA_Server_addPublishedDataSet(UA_Server *server,
        const UA_PublishedDataSetConfig *publishedDataSetConfig, UA_NodeId *pdsIdentifier) {

    return open62541_mock::get().UA_Server_addPublishedDataSet(server, publishedDataSetConfig, pdsIdentifier);
}

UA_StatusCode UA_Server_addDataSourceVariableNode(UA_Server *server,
        const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
        const UA_QualifiedName browseName, const UA_NodeId typeDefinition, const UA_VariableAttributes attr,
        const UA_DataSource dataSource, void *nodeContext, UA_NodeId *outNewNodeId) {

    return open62541_mock::get().UA_Server_addDataSourceVariableNode(
        server, requestedNewNodeId, parentNodeId, referenceTypeId, browseName, typeDefinition,
        attr, dataSource, nodeContext, outNewNodeId
    );
}

UA_DataSetFieldResult UA_Server_addDataSetField(UA_Server *server, const UA_NodeId publishedDataSet,
        const UA_DataSetFieldConfig *fieldConfig, UA_NodeId *fieldIdentifier) {

    return open62541_mock::get().UA_Server_addDataSetField(
        server, publishedDataSet, fieldConfig, fieldIdentifier
    );
}

UA_StatusCode UA_Server_addReaderGroup(UA_Server *server, UA_NodeId connectionIdentifier,
        const UA_ReaderGroupConfig *readerGroupConfig, UA_NodeId *readerGroupIdentifier) {

    return open62541_mock::get().UA_Server_addReaderGroup(server, connectionIdentifier, readerGroupConfig, readerGroupIdentifier);
}

UA_StatusCode UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
        const UA_WriterGroupConfig *writerGroupConfig, UA_NodeId *writerGroupIdentifier) {

    return open62541_mock::get().UA_Server_addWriterGroup(server, connection, writerGroupConfig, writerGroupIdentifier);
}

UA_StatusCode UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupIdentifier,
        const UA_DataSetReaderConfig *dataSetReaderConfig, UA_NodeId *readerIdentifier) {

    return open62541_mock::get().UA_Server_addDataSetReader(
        server, readerGroupIdentifier, dataSetReaderConfig, readerIdentifier
    );
}

UA_StatusCode UA_Server_addDataSetWriter(UA_Server *server, const UA_NodeId writerGroup,
        const UA_NodeId dataSet, const UA_DataSetWriterConfig *dataSetWriterConfig, UA_NodeId *writerIdentifier) {

    return open62541_mock::get().UA_Server_addDataSetWriter(
        server, writerGroup, dataSet, dataSetWriterConfig, writerIdentifier
    );
}

void UA_clear(void *p, const UA_DataType *type) {

    open62541_mock::get().UA_clear(p, type);
}

void UA_Array_delete(void *p, size_t size, const UA_DataType *type) {

    open62541_mock::get().UA_Array_delete(p, size, type);
}

UA_StatusCode UA_Server_DataSetReader_createTargetVariables(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
                                                            UA_TargetVariablesDataType* targetVariables)
{
    return open62541_mock::get().
        UA_Server_DataSetReader_createTargetVariables(server,dataSetReaderIdentifier, targetVariables);
}

UA_Server *UA_Server_new(void)
{
    return open62541_mock::get().UA_Server_new();
}

void UA_Server_delete(UA_Server *server)
{
    open62541_mock::get().UA_Server_delete(server);
}

UA_StatusCode UA_copy(const void *src, void *dst, const UA_DataType *type)
{
    return open62541_mock::get().UA_copy(src, dst, type);
}

void UA_Variant_setScalar(UA_Variant *v, void *p, const UA_DataType *type)
{
    open62541_mock::get().UA_Variant_setScalar(v, p, type);
}

void *UA_Array_new(size_t size, const UA_DataType *type)
{
    return open62541_mock::get().UA_Array_new(size, type);
}

UA_UInt32 UA_UInt32_random(void)
{
    return open62541_mock::get().UA_UInt32_random();
}

void UA_Log_Stdout_log(void *arg, UA_LogLevel level, UA_LogCategory category,
        const char *msg, va_list args)
{
    return open62541_mock::get().UA_Log_Stdout_log(arg, level, category, msg, args);
}

UA_Boolean UA_QualifiedName_equal(const UA_QualifiedName *qn1, const UA_QualifiedName *qn2)
{
    return open62541_mock::get().UA_QualifiedName_equal(qn1, qn2);
}

UA_Order UA_NodeId_order(const UA_NodeId *n1, const UA_NodeId *n2)
{
    return open62541_mock::get().UA_NodeId_order(n1, n2);
}

UA_Boolean UA_String_equal(const UA_String *s1, const UA_String *s2)
{
    return open62541_mock::get().UA_String_equal(s1, s2);
}

void *UA_new(const UA_DataType *type)
{
    return open62541_mock::get().UA_new(type);
}

void UA_delete(void *p, const UA_DataType *type)
{
    open62541_mock::get().UA_delete(p, type);
}

UA_StatusCode UA_Server_setWriterGroupOperational(UA_Server *server, const UA_NodeId writerGroup)
{
    return  open62541_mock::get().UA_Server_setWriterGroupOperational(server, writerGroup);
}

UA_StatusCode UA_Server_freezeWriterGroupConfiguration(UA_Server *server, const UA_NodeId writerGroup)
{
    return  open62541_mock::get().UA_Server_freezeWriterGroupConfiguration(server, writerGroup);
}

void UA_Variant_setArray(UA_Variant *v, void *array, size_t arraySize, const UA_DataType *type)
{
    open62541_mock::get().UA_Variant_setArray(v, array, arraySize, type);
}

UA_DateTime UA_DateTime_now(void)
{
    return open62541_mock::get().UA_DateTime_now();
}

UA_PubSubTransportLayer UA_PubSubTransportLayerEthernetXDP(void)
{
    return open62541_mock::get().UA_PubSubTransportLayerEthernetXDP();
}

UA_PubSubTransportLayer UA_PubSubTransportLayerEthernetETF(void)
{
    return open62541_mock::get().UA_PubSubTransportLayerEthernetETF();
}

UA_StatusCode UA_Server_run(UA_Server *server, const volatile UA_Boolean *running)
{
    return open62541_mock::get().UA_Server_run(server, running);
}

UA_ServerConfig *UA_Server_getConfig(UA_Server *server)
{
    return open62541_mock::get().UA_Server_getConfig(server);
}

UA_StatusCode UA_ServerConfig_setMinimalCustomBuffer(UA_ServerConfig *config,
                                       UA_UInt16 portNumber,
                                       const UA_ByteString *certificate,
                                       UA_UInt32 sendBufferSize,
                                       UA_UInt32 recvBufferSize)
{
    return open62541_mock::get().UA_ServerConfig_setMinimalCustomBuffer(config,
        portNumber, certificate, sendBufferSize, recvBufferSize);
}

// Operators overridden
bool operator==(const UA_NodeId &a, const UA_NodeId &b) {
    return UA_NodeId_equal(&a, &b);
}

bool operator==(const UA_QualifiedName &a, const UA_QualifiedName &b) {
    return UA_QualifiedName_equal(&a, &b);
}

__attribute__((pure)) bool operator==(const UA_DataSource &a, const UA_DataSource &b) {
    return (a.read == b.read) && (a.write == b.write);
}

bool operator==(const UA_PubSubConnectionConfig &a, const UA_PubSubConnectionConfig &b) {
    return UA_String_equal(&a.name, &b.name) && UA_String_equal(&a.transportProfileUri, &b.transportProfileUri) &&
        (a.enabled == b.enabled);
}

bool operator==(const UA_PublishedDataSetConfig &a, const UA_PublishedDataSetConfig &b) {
    return UA_String_equal(&a.name, &b.name) && (a.publishedDataSetType == b.publishedDataSetType);
}

bool operator==(const UA_VariableAttributes &a, const UA_VariableAttributes &b) {
    return UA_String_equal(&a.displayName.locale, &b.displayName.locale) &&
           UA_String_equal(&a.displayName.text, &b.displayName.text) &&
           (a.accessLevel == b.accessLevel);
}

__attribute__((pure)) bool operator==(const UA_DataSetFieldConfig &a, const UA_DataSetFieldConfig &b) {
    return (a.dataSetFieldType == b.dataSetFieldType);
}

bool operator==(const UA_ReaderGroupConfig &a, const UA_ReaderGroupConfig &b) {
    return UA_String_equal(&a.name, &b.name);
}

bool operator==(const UA_WriterGroupConfig &a, const UA_WriterGroupConfig &b) {
    return UA_String_equal(&a.name, &b.name) && (a.enabled == b.enabled) &&
        (a.writerGroupId == b.writerGroupId) && (a.encodingMimeType == b.encodingMimeType);
}

bool operator==(const UA_DataSetReaderConfig &a, const UA_DataSetReaderConfig &b) {
    return UA_String_equal(&a.name, &b.name) && (a.dataSetWriterId == b.dataSetWriterId);
}

bool operator==(const UA_DataSetWriterConfig &a, const UA_DataSetWriterConfig &b) {
    return UA_String_equal(&a.name, &b.name) && (a.dataSetWriterId == b.dataSetWriterId) &&
        (a.keyFrameCount == b.keyFrameCount);
}
