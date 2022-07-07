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

#include "opcua_publish.h"
#include "opcua_common.h"

#include <gtest/gtest.h>
#include <open62541/server_config_default.h>

using ::testing::_;
using ::testing::Return;
using ::testing::Pointee;
using ::testing::AnyNumber;
using ::testing::ReturnNull;
using ::testing::MatchesRegex;

/* Mocking
 * Note:
 *     Not doing this you have to build 'opcua_common.c'
 *     that has a bulk of dependencies
*/
#include "opcua_utils.h"
log_level_t log_level = log_error;

struct threadParams* g_thread;
UA_UInt16 g_threadRun;
UA_Boolean g_running = true;
UA_NodeId g_writerGroupIdent;
struct ServerData* g_sData;
const UA_Logger *UA_Log_Stdout = NULL;
const UA_DataType UA_TYPES[UA_TYPES_COUNT] = {};
const UA_VariableAttributes UA_VariableAttributes_default = {};

namespace {

/**
 * This class aggregates UTs
 * which validate the OPC UA* Server part of the TCC TSN E2E sample
 */
class test_opcua_publish_c : public testing::Test
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
                .readFunc = NULL,
                .writeFunc = NULL,
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
            this->server_data_->server = this->server_;

            this->publisher_data_ = &pub_data;
            this->server_data_->useXDP = false;
            this->publisher_data_->socketPriority = 0;
        }

        void TearDown() override {
            EXPECT_CALL(open62541_mock_, UA_Server_delete(_))
                .WillOnce(Return());

            UA_Server_delete(this->server_);
        }

        UA_Server *server_;
        struct ServerData *server_data_;
        struct PublisherData *publisher_data_;

        testing::StrictMock<open62541_mock> open62541_mock_;
};


/*
 * API from 'opcua_publish.h'
 */
TEST_F(test_opcua_publish_c, createPublisher_wrong_serv_argument_negative)
{
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(NULL, server_data_, publisher_data_, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("UA_Server pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_serv_argument_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(NULL, server_data_, publisher_data_, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("UA_Server pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_serv_argument_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(NULL, server_data_, publisher_data_, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("UA_Server pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_sdata_argument_negative)
{
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(server_, NULL, publisher_data_, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("ServerData pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_sdata_argument_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(server_, NULL, publisher_data_, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("ServerData pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_sdata_argument_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(server_, NULL, publisher_data_, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("ServerData pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_pdata_argument_negative)
{
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(server_, server_data_, NULL, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("PublisherData pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_pdata_argument_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(server_, server_data_, NULL, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("PublisherData pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_pdata_argument_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    UA_NodeId connId;
    const int res = createPublisher(server_, server_data_, NULL, &connId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("PublisherData pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_conn_id_argument_negative)
{
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("NodeId pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_conn_id_argument_with_errno_negative)
{
    errno = -EINVAL;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("NodeId pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_wrong_conn_id_argument_low_log_level_negative)
{
    log_level = log_undef;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, NULL);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("NodeId pointer cannot be NULL"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addPublishedDataSet_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a published data set"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addPublishedDataSet_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a published data set"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addPublishedDataSet_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to add a published data set"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSourceVariableNode_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a data set field"), std::string::npos);
    EXPECT_NE(captured.find("Failed 'UA_Server_addDataSourceVariableNode' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSourceVariableNode_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a data set field"), std::string::npos);
    EXPECT_NE(captured.find("Failed 'UA_Server_addDataSourceVariableNode' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSourceVariableNode_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to add a data set field"), std::string::npos);
    EXPECT_EQ(captured.find("Failed 'UA_Server_addDataSourceVariableNode' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSetField_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_BADINVALIDARGUMENT, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a data set field"), std::string::npos);
    EXPECT_NE(captured.find("Failed 'UA_Server_addDataSetField' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSetField_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_BADINVALIDARGUMENT, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a data set field"), std::string::npos);
    EXPECT_NE(captured.find("Failed 'UA_Server_addDataSetField' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSetField_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_BADINVALIDARGUMENT, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to add a data set field"), std::string::npos);
    EXPECT_EQ(captured.find("Failed 'UA_Server_addDataSetField' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_allocation_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(ReturnNull());

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add new writer group"), std::string::npos);
    EXPECT_NE(captured.find("Cannot allocate memory for writer group message"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_allocation_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(ReturnNull());

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add new writer group"), std::string::npos);
    EXPECT_NE(captured.find("Cannot allocate memory for writer group message"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_allocation_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(ReturnNull());

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to add new writer group"), std::string::npos);
    EXPECT_EQ(captured.find("Cannot allocate memory for writer group message"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addWriterGroup_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Cannot add new writer group"), std::string::npos);
    EXPECT_NE(captured.find("Unable to add new writer group"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addWriterGroup_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Cannot add new writer group"), std::string::npos);
    EXPECT_NE(captured.find("Unable to add new writer group"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addWriterGroup_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Cannot add new writer group"), std::string::npos);
    EXPECT_EQ(captured.find("Unable to add new writer group"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_setWriterGroupOperational_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add new writer group"), std::string::npos);
    EXPECT_NE(captured.find("Cannot set new writer group operational"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_setWriterGroupOperational_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add new writer group"), std::string::npos);
    EXPECT_NE(captured.find("Cannot set new writer group operational"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_setWriterGroupOperational_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to add new writer group"), std::string::npos);
    EXPECT_EQ(captured.find("Cannot set new writer group operational"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSetWriter_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a data set writer"), std::string::npos);
    EXPECT_NE(captured.find("Failed 'UA_Server_addDataSetWriter' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSetWriter_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to add a data set writer"), std::string::npos);
    EXPECT_NE(captured.find("Failed 'UA_Server_addDataSetWriter' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_addDataSetWriter_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to add a data set writer"), std::string::npos);
    EXPECT_EQ(captured.find("Failed 'UA_Server_addDataSetWriter' with -2136276992"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_freezeWriterGroupConfiguration_fail_negative)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'UA_Server_freezeWriterGroupConfiguration' call in 'createPublisher'
    EXPECT_CALL(open62541_mock_, UA_Server_freezeWriterGroupConfiguration(_, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to freeze writer group configuration"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_freezeWriterGroupConfiguration_fail_with_errno_negative)
{
    errno = -EINVAL;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'UA_Server_freezeWriterGroupConfiguration' call in 'createPublisher'
    EXPECT_CALL(open62541_mock_, UA_Server_freezeWriterGroupConfiguration(_, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Unable to freeze writer group configuration"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_freezeWriterGroupConfiguration_fail_low_log_level_negative)
{
    log_level = log_undef;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'UA_Server_freezeWriterGroupConfiguration' call in 'createPublisher'
    EXPECT_CALL(open62541_mock_, UA_Server_freezeWriterGroupConfiguration(_, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Unable to freeze writer group configuration"), std::string::npos);
}

TEST_F(test_opcua_publish_c, createPublisher_positive)
{
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'UA_Server_freezeWriterGroupConfiguration' call in 'createPublisher'
    EXPECT_CALL(open62541_mock_, UA_Server_freezeWriterGroupConfiguration(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Checking...
    UA_NodeId connectionId;
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_publish_c, createPublisher_with_logs_positive)
{
    log_level = log_debug;
    // Mocking 'addPublishedDataSet'
    UA_AddPublishedDataSetResult retval_add_published_data_set_result = { UA_STATUSCODE_GOOD, 0, NULL, { 0, 0}};
    EXPECT_CALL(open62541_mock_, UA_Server_addPublishedDataSet(_, _, _))
        .WillOnce(Return(retval_add_published_data_set_result));

    // Mocking 'addDataSetField'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSourceVariableNode(_, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    UA_DataSetFieldResult retval_data_set_field_result = { UA_STATUSCODE_GOOD, { 0, 0 } };
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetField(_, _, _, _))
        .WillOnce(Return(retval_data_set_field_result));

    // Mocking 'addWriterGroup'
    UA_UadpWriterGroupMessageDataType writerGroupMessageData = {};
    EXPECT_CALL(open62541_mock_, UA_new(&UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return(&writerGroupMessageData));
    EXPECT_CALL(open62541_mock_, UA_Server_addWriterGroup(_, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_Server_setWriterGroupOperational(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));
    EXPECT_CALL(open62541_mock_, UA_delete(_, &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        .WillOnce(Return());

    // Mocking 'addDataSetWriter'
    EXPECT_CALL(open62541_mock_, UA_Server_addDataSetWriter(_, _, _, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Mocking 'UA_Server_freezeWriterGroupConfiguration' call in 'createPublisher'
    EXPECT_CALL(open62541_mock_, UA_Server_freezeWriterGroupConfiguration(_, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    // Checking...
    UA_NodeId connectionId;
    testing::internal::CaptureStdout();
    const int res = createPublisher(server_, server_data_, publisher_data_, &connectionId);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, 0);
    EXPECT_NE(captured.find("setting interval to 0.500000"), std::string::npos);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_addPubSubConnection_fail_negative)
{
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Cannot add new PubSub connection"), std::string::npos);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_addPubSubConnection_fail_with_errno_negative)
{
    errno = -EINVAL;
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_NE(captured.find("Cannot add new PubSub connection"), std::string::npos);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_addPubSubConnection_fail_low_log_level_negative)
{
    log_level = log_undef;
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_BADINVALIDARGUMENT));

    UA_NodeId connectionId;
    testing::internal::CaptureStderr();
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -1);
    EXPECT_EQ(captured.find("Cannot add new PubSub connection"), std::string::npos);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_positive)
{
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    UA_NodeId connectionId;
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_xdp_positive)
{
    server_data_->useXDP = true;
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    UA_NodeId connectionId;
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_with_socket_priority_positive)
{
    publisher_data_->socketPriority = 10;
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    UA_NodeId connectionId;
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);

    EXPECT_EQ(res, 0);
}

TEST_F(test_opcua_publish_c, addPubSubConnection_with_log_positive)
{
    log_level = log_debug;
    EXPECT_CALL(open62541_mock_, UA_Variant_setScalar(_, _, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]))
        .WillOnce(Return());
    EXPECT_CALL(open62541_mock_, UA_Server_addPubSubConnection(server_, _, _))
        .WillOnce(Return(UA_STATUSCODE_GOOD));

    UA_NodeId connectionId;
    testing::internal::CaptureStdout();
    const int res = addPubSubConnection(server_, &connectionId, server_data_, publisher_data_);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, 0);
    EXPECT_NE(captured.find("Setting socket prio to -1"), std::string::npos);
}

} // namespace {
