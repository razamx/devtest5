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

#ifndef OPEN62541_MOCK_H
#define OPEN62541_MOCK_H

#include "mock_common.hpp"
#include "gmock/gmock.h"

#include <open62541/server_pubsub.h>
#include <open62541/plugin/pubsub.h>
#include <open62541/plugin/log_stdout.h>

struct open62541_mock {
    MOCK_METHOD3(UA_Server_addPubSubConnection, UA_StatusCode(UA_Server *server,
        const UA_PubSubConnectionConfig *connectionConfig, UA_NodeId *connectionIdentifier));

    MOCK_METHOD3(UA_Server_addPublishedDataSet, UA_AddPublishedDataSetResult(UA_Server *server,
        const UA_PublishedDataSetConfig *publishedDataSetConfig, UA_NodeId *pdsIdentifier));

    MOCK_METHOD10(UA_Server_addDataSourceVariableNode, UA_StatusCode(UA_Server *server,
        const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
        const UA_QualifiedName browseName, const UA_NodeId typeDefinition, const UA_VariableAttributes attr,
        const UA_DataSource dataSource, void *nodeContext, UA_NodeId *outNewNodeId));

    MOCK_METHOD4(UA_Server_addDataSetField, UA_DataSetFieldResult(UA_Server *server,
        const UA_NodeId publishedDataSet, const UA_DataSetFieldConfig *fieldConfig, UA_NodeId *fieldIdentifier));

    MOCK_METHOD4(UA_Server_addReaderGroup, UA_StatusCode(UA_Server *server, UA_NodeId connectionIdentifier,
        const UA_ReaderGroupConfig *readerGroupConfig, UA_NodeId *readerGroupIdentifier));
    MOCK_METHOD4(UA_Server_addWriterGroup, UA_StatusCode(UA_Server *server, const UA_NodeId connection,
        const UA_WriterGroupConfig *writerGroupConfig, UA_NodeId *writerGroupIdentifier));

    MOCK_METHOD4(UA_Server_addDataSetReader, UA_StatusCode(UA_Server *server, UA_NodeId readerGroupIdentifier,
        const UA_DataSetReaderConfig *dataSetReaderConfig, UA_NodeId *readerIdentifier));
    MOCK_METHOD5(UA_Server_addDataSetWriter, UA_StatusCode(UA_Server *server, const UA_NodeId writerGroup,
        const UA_NodeId dataSet, const UA_DataSetWriterConfig *dataSetWriterConfig, UA_NodeId *writerIdentifier));

    MOCK_METHOD2(UA_clear, void(void *p, const UA_DataType *type));
    MOCK_METHOD3(UA_Array_delete, void(void *p, size_t size, const UA_DataType *type));

    MOCK_METHOD3(UA_Server_DataSetReader_createTargetVariables,
        UA_StatusCode(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
                      UA_TargetVariablesDataType* targetVariables));
    MOCK_METHOD0(UA_Server_new, UA_Server *());
    MOCK_METHOD1(UA_Server_delete, void (UA_Server *server));
    MOCK_METHOD3(UA_copy, UA_StatusCode (const void *src, void *dst, const UA_DataType *type));
    MOCK_METHOD3(UA_Variant_setScalar, void (UA_Variant *v, void *p, const UA_DataType *type));
    MOCK_METHOD2(UA_Array_new, void *(size_t size, const UA_DataType *type));
    MOCK_METHOD0(UA_UInt32_random, UA_UInt32 ());
    MOCK_METHOD5(UA_Log_Stdout_log, void (void *arg, UA_LogLevel level, UA_LogCategory category,
        const char *msg, va_list args));
    MOCK_METHOD2(UA_QualifiedName_equal, UA_Boolean (const UA_QualifiedName *qn1, const UA_QualifiedName *qn2));
    MOCK_METHOD2(UA_NodeId_order, UA_Order (const UA_NodeId *n1, const UA_NodeId *n2));
    MOCK_METHOD2(UA_String_equal, UA_Boolean (const UA_String *s1, const UA_String *s2));
    MOCK_METHOD1(UA_new, void *(const UA_DataType *type));
    MOCK_METHOD2(UA_delete, void (void *p, const UA_DataType *type));
    MOCK_METHOD2(UA_Server_setWriterGroupOperational, UA_StatusCode (UA_Server *server, const UA_NodeId writerGroup));
    MOCK_METHOD2(UA_Server_freezeWriterGroupConfiguration, UA_StatusCode (UA_Server *server, const UA_NodeId writerGroup));
    MOCK_METHOD4(UA_Variant_setArray, void (UA_Variant *v, void *array, size_t arraySize, const UA_DataType *type));
    MOCK_METHOD0(UA_DateTime_now, UA_DateTime (void));
    MOCK_METHOD0(UA_PubSubTransportLayerEthernetXDP, UA_PubSubTransportLayer (void));
    MOCK_METHOD0(UA_PubSubTransportLayerEthernetETF, UA_PubSubTransportLayer (void));
    MOCK_METHOD2(UA_Server_run, UA_StatusCode (UA_Server *server, const volatile UA_Boolean *running));
    MOCK_METHOD1(UA_Server_getConfig, UA_ServerConfig *(UA_Server *server));
    MOCK_METHOD5(UA_ServerConfig_setMinimalCustomBuffer, UA_StatusCode (UA_ServerConfig *config,
                                       UA_UInt16 portNumber,
                                       const UA_ByteString *certificate,
                                       UA_UInt32 sendBufferSize,
                                       UA_UInt32 recvBufferSize));

    COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(open62541_mock);
};

// Operators overridden
bool operator==(const UA_NodeId &a, const UA_NodeId &b);
bool operator==(const UA_QualifiedName &a, const UA_QualifiedName &b);
bool operator==(const UA_DataSource &a, const UA_DataSource &b);
bool operator==(const UA_PubSubConnectionConfig &a, const UA_PubSubConnectionConfig &b);
bool operator==(const UA_PublishedDataSetConfig &a, const UA_PublishedDataSetConfig &b);
bool operator==(const UA_VariableAttributes &a, const UA_VariableAttributes &b);
bool operator==(const UA_DataSetFieldConfig &a, const UA_DataSetFieldConfig &b);
bool operator==(const UA_ReaderGroupConfig &a, const UA_ReaderGroupConfig &b);
bool operator==(const UA_WriterGroupConfig &a, const UA_WriterGroupConfig &b);
bool operator==(const UA_DataSetReaderConfig &a, const UA_DataSetReaderConfig &b);
bool operator==(const UA_DataSetWriterConfig &a, const UA_DataSetWriterConfig &b);

#endif // OPEN62541_MOCK_H
