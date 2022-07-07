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
#include "alloc_mock.hpp"
#include "libc_mock.hpp"

#include "opcua_subscribe.h"
#include "opcua_common.h"

#include <gtest/gtest.h>
#include <open62541/server_config_default.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::Pointee;
using ::testing::MatchesRegex;
using ::testing::AnyNumber;
using ::testing::SaveArgPointee;
using ::testing::SetErrnoAndReturn;

struct threadParams* g_thread;
UA_UInt16 g_threadRun;
UA_Boolean g_running = true;
UA_NodeId g_writerGroupIdent;
struct ServerData* g_sData;
const UA_Logger *UA_Log_Stdout = NULL;
const UA_DataType UA_TYPES[UA_TYPES_COUNT] = {};
const UA_VariableAttributes UA_VariableAttributes_default = {};

/* Mocking
 * Note:
 *     Not doing this you have to build 'opcua_common.c'
 *     that has a bulk of dependencies
*/
#include "opcua_utils.h"
log_level_t log_level = log_error;



namespace {

#define SUB_CALLBACK_BODY ({ \
        printf("'%s' has been triggered!\n", __func__); \
        return UA_STATUSCODE_GOOD; \
    })

UA_StatusCode sub_read_func(__attribute__((unused)) UA_Server *server,
    __attribute__((unused)) const UA_NodeId *sessionId,
    __attribute__((unused)) void *sessionContext,
    __attribute__((unused)) const UA_NodeId *nodeId,
    __attribute__((unused)) void *nodeContext,
    __attribute__((unused)) UA_Boolean includeSourceTimeStamp,
    __attribute__((unused)) const UA_NumericRange *range,
    __attribute__((unused)) UA_DataValue *value) {

    SUB_CALLBACK_BODY;
}

UA_StatusCode sub_write_func(__attribute__((unused)) UA_Server *server,
    __attribute__((unused)) const UA_NodeId *sessionId,
    __attribute__((unused)) void *sessionContext,
    __attribute__((unused)) const UA_NodeId *nodeId,
    __attribute__((unused)) void *nodeContext,
    __attribute__((unused)) const UA_NumericRange *range,
    __attribute__((unused)) const UA_DataValue *value) {

    SUB_CALLBACK_BODY;
}

/**
 * This class aggregates UTs
 * which validate the OPC UA* Server part of the TCC TSN E2E sample
 */
class test_opcua_subscribe_c : public testing::Test
{
    protected:

        void SetUp() override {
            errno = 0;
            log_level = log_error;

            // Set expectations for the mocked functions
            // 'UA_clear'
            EXPECT_CALL(open62541_mock_, UA_clear(_, _))
                .Times(AnyNumber());
            // 'UA_Array_delete'
            EXPECT_CALL(open62541_mock_, UA_Array_delete(_, _, _))
                .Times(AnyNumber());
            EXPECT_CALL(open62541_mock_, UA_Server_new())
                .WillOnce(Return(reinterpret_cast<UA_Server *>(0x80001000)));

            // Populate this class' fields with data
            static struct PublisherData pub_data = {
                .socketPriority = 1,
                .earlyOffsetNs = -300000,
                .publishOffsetNs = 325000,
                .url = const_cast<char*>("opc.eth://20-20-20-20-20-20"),
                .id = 0,
                .prev_sequence_num = -1,
                .readFunc = NULL,
                .writeFunc = NULL,
                .xdpQueue = 0,
                .dataSetWriterId = 0,
                .writerGroupId = 0,
                .cpuAffinity = 0,
                .writerGroupMessage = NULL
            };

            static struct SubscriberData sub_data = {
                .offsetNs = 50000,
                .url = const_cast<char*>("opc.eth://20-20-20-20-20-20"),
                .id = 0,
                .readFunc = sub_read_func,
                .writeFunc = sub_write_func,
                .data = NULL,
                .xdpQueue = 10,
                .subscribedPubId = 0,
                .subscribedDSWriterId = 0,
                .subscribedWGId = 0,
                .cpuAffinity = 0,
                .pMetaData_fields = NULL,
                .targetVars = NULL
            };

            static struct ServerData server_data = {
                .pubInterface = const_cast<char*>("enp1s0"),
                .subInterface = const_cast<char*>("enp1s0"),
                .cycleTimeNs = 500000,
                .pollingDurationNs = 100000,
                .cpu = 0,
                .pubCount = 1,
                .subCount = 1,
                .pubData = &pub_data,
                .subData = &sub_data,
                .transportProfile = UA_STRING(const_cast<char*>("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp")),
                .tcc_latency = 10U,
                .useXDP = NULL,
                .packetCount = 2000,
                .PubSubTransportLayers = NULL,
                .server = NULL
            };

            this->server_ = UA_Server_new();

            this->server_data_ = &server_data;

            this->subscriber_data_ = &sub_data;

            // Set default behavior for critical system calls
            EXPECT_CALL(libc_mock_, fprintf(_, _, _))
                .WillRepeatedly(Invoke(vfprintf));
            EXPECT_CALL(libc_mock_, close(_))
                .WillRepeatedly(Invoke(__real_close));
            EXPECT_CALL(libc_mock_, fopen(_, _))
                .WillRepeatedly(Invoke(__real_fopen));
            EXPECT_CALL(libc_mock_, fread(_, _, _, _))
                .WillRepeatedly(Invoke(__real_fread));
            EXPECT_CALL(libc_mock_, read(_, _, _))
                .WillRepeatedly(Invoke(__real_read));
            EXPECT_CALL(libc_mock_, write(_, _, _))
                .WillRepeatedly(Invoke(__real_write));
            EXPECT_CALL(libc_mock_, fclose(_))
                .WillRepeatedly(Invoke(__real_fclose));
            EXPECT_CALL(libc_mock_, close(_))
                .WillRepeatedly(Invoke(__real_close));
            EXPECT_CALL(libc_mock_, clock_gettime(_, _))
                .WillRepeatedly(Invoke(__real_clock_gettime));
        }

        void TearDown() override {
            EXPECT_CALL(open62541_mock_, UA_Server_delete(_))
                .WillOnce(Return());
            UA_Server_delete(this->server_);
        }

        UA_NodeId connId_;
        UA_Server *server_;
        struct ServerData *server_data_;
        struct SubscriberData *subscriber_data_;

        testing::StrictMock<open62541_mock> open62541_mock_;
        testing::NiceMock<alloc_mock> alloc_mock_;
        testing::StrictMock<libc_mock> libc_mock_;
};


/*
 * API from 'opcua_subscribe.h'
 */
TEST_F(test_opcua_subscribe_c, createSubscriber_no_server_negative)
{
    testing::internal::CaptureStderr();
    const int res = createSubscriber(NULL, NULL, NULL, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'server' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_server_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(NULL, NULL, NULL, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'server' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_server_low_log_level_negative)
{

    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(NULL, NULL, NULL, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'server' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_sdata_negative)
{
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, NULL, subscriber_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sdata' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_sdata_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, NULL, subscriber_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sdata' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_sdata_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, NULL, subscriber_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'sdata' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_sub_negative)
{
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, server_data_, NULL, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sub' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_sub_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, server_data_, NULL, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sub' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_no_sub_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, server_data_, NULL, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'sub' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_fails_no_connection_id_negative)
{
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, server_data_, subscriber_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'connId' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_fails_no_connection_id_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, server_data_, subscriber_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'connId' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_fails_no_connection_id_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = createSubscriber(server_, server_data_, subscriber_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'connId' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_addReaderGroup_fail_negative)
{
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot add reader group"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_new_array_fail_negative)
{
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(ReturnNull());

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured.find("Cannot allocate memory for field meta data"), std::string::npos);
    EXPECT_NE(captured.find("Cannot add data set reader"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_new_array_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(ReturnNull());

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured.find("Cannot allocate memory for field meta data"), std::string::npos);
    EXPECT_NE(captured.find("Cannot add data set reader"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_new_array_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(ReturnNull());

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_EQ(captured.find("Cannot allocate memory for field meta data"), std::string::npos);
    EXPECT_EQ(captured.find("Cannot add data set reader"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_addDataSetReader_fail_negative)
{
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured.find("Cannot add data set reader"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_addDataSetReader_fail_with_errno_negative)
{
    errno = -EINVAL;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured.find("Cannot add data set reader"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_addDataSetReader_fail_low_log_level_negative)
{
    log_level = log_undef;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_EQ(captured.find("Cannot add data set reader"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_fails_when_UA_Server_addDataSourceVariableNode_fails_negative)
{
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot add data source variable"), std::string::npos);
    EXPECT_NE(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_addDataSourceVariableNode_fail_with_errno_negative)
{
    errno = -EINVAL;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot add data source variable"), std::string::npos);
    EXPECT_NE(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_addDataSourceVariableNode_fail_low_log_level_negative)
{
    log_level = log_undef;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_EQ(captured_text.find("Cannot add data source variable"), std::string::npos);
    EXPECT_EQ(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_createTargetVariables_calloc_fail_negative)
{
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(alloc_mock_, calloc(1, sizeof(UA_FieldTargetDataType)))
        .WillOnce(ReturnNull());

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot allocate memory for target variables"), std::string::npos);
    EXPECT_NE(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_createTargetVariables_calloc_fail_with_errno_negative)
{
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(alloc_mock_, calloc(1, sizeof(UA_FieldTargetDataType)))
        .WillOnce(SetErrnoAndReturn(-EINVAL, nullptr));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot allocate memory for target variables"), std::string::npos);
    EXPECT_NE(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_createTargetVariables_calloc_fail_low_log_level_negative)
{
    log_level = log_undef;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(alloc_mock_, calloc(1, sizeof(UA_FieldTargetDataType)))
        .WillOnce(ReturnNull());

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_EQ(captured_text.find("Cannot allocate memory for target variables"), std::string::npos);
    EXPECT_EQ(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_createTargetVariables_fail_negative)
{
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_DataSetReader_createTargetVariables(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot create target variables"), std::string::npos);
    EXPECT_NE(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_createTargetVariables_fail_with_errno_negative)
{
    errno = -EINVAL;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_DataSetReader_createTargetVariables(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_NE(captured_text.find("Cannot create target variables"), std::string::npos);
    EXPECT_NE(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_createTargetVariables_fail_low_log_level_negative)
{
    log_level = log_undef;
    UA_FieldMetaData array[1];
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(server_, _, _, _, _, _, _, _, _, NULL))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_DataSetReader_createTargetVariables(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    testing::internal::CaptureStderr();
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    const std::string &captured_text = testing::internal::GetCapturedStderr();

    EXPECT_EQ(retval_create_subscriber, -1);
    EXPECT_EQ(captured_text.find("Cannot create target variables"), std::string::npos);
    EXPECT_EQ(captured_text.find("Cannot add subscribed variables"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, createSubscriber_positive)
{
    UA_FieldMetaData array[1];
    UA_TargetVariablesDataType targetVars = {};
    // Mocking 'addReaderGroup'
    EXPECT_CALL(open62541_mock_, UA_Server_addReaderGroup(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addDataSetReader'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetReader(server_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Array_new(1, _))
        .WillOnce(Return(array));
    EXPECT_CALL(open62541_mock_, UA_copy(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'addSubscribedVariables'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_DataSetReader_createTargetVariables(_, _, _))
        .WillOnce(DoAll(SaveArgPointee<2>(&targetVars), Return(UA_STATUSCODE_GOOD)));

    // Checking...
    const int retval_create_subscriber = createSubscriber(server_, server_data_, subscriber_data_, &connId_);
    EXPECT_EQ(retval_create_subscriber, 0);
    EXPECT_TRUE(targetVars.targetVariables != NULL);
    free(targetVars.targetVariables);
}

TEST_F(test_opcua_subscribe_c, add_connection_random_id_positive)
{
    subscriber_data_->id = 0;
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, _))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_UInt32_random())
        .WillOnce(Return(1234));

    const int res = addSubConnection(server_, &connId_, server_data_, subscriber_data_);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_subscribe_c, add_connection_defined_id_positive)
{
    subscriber_data_->id = 1;
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, _))
        .WillOnce(Return());

    const int res = addSubConnection(server_, &connId_, server_data_, subscriber_data_);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_subscribe_c, add_connection_use_xdp_positive)
{
    subscriber_data_->id = 1;
    server_data_->useXDP = true;
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, _))
        .WillOnce(Return());

    const int res = addSubConnection(server_, &connId_, server_data_, subscriber_data_);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_subscribe_c, add_connection_add_connection_fail_negative)
{
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, _))
        .WillOnce(Return());

    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Cannot add new PubSub connection"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_add_connection_fail_errno_negative)
{
    errno = -EINVAL;
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, _))
        .WillOnce(Return());

    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Cannot add new PubSub connection"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_add_connection_fail_low_log_level_negative)
{
    log_level = log_undef;
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, _))
        .WillOnce(Return());

    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Cannot add new PubSub connection"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_sub_argument_negative)
{
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, server_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sub' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_sub_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, server_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sub' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_sub_log_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, server_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'sub' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_sdata_argument_negative)
{
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, NULL, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sdata' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_sdata_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, NULL, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'sdata' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_sdata_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, &connId_, NULL, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'sdata' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_conn_id_argument_negative)
{
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, NULL, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'connId' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_conn_id_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, NULL, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'connId' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_conn_id_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(server_, NULL, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'connId' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_server_argument_negative)
{
    testing::internal::CaptureStderr();
    const int res = addSubConnection(NULL, &connId_, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'server' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_server_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(NULL, &connId_, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Wrong 'server' argument"), std::string::npos);
}

TEST_F(test_opcua_subscribe_c, add_connection_wrong_server_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = addSubConnection(NULL, &connId_, server_data_, subscriber_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Wrong 'server' argument"), std::string::npos);
}

} // namespace {
