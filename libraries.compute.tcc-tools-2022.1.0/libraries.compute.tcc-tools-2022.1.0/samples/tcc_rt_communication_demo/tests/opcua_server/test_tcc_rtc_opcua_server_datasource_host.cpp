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

extern "C" {
#include "opcua_datasource.c"
}

#include "opcua_common.h"
#include "opcua_shared.h"
#include "open62541_mock.hpp"
#include "pdatasource_mock.hpp"

#include <gtest/gtest.h>

using testing::_;

#define TEST_DATASOURCE_SEQUENCE (2000)
#define TEST_DATASOURCE_SEQ_NUMBER (1234)
#define TEST_DATASOURCE_SEQ_NUMBER_2 (1235)
#define TEST_DATASOURCE_TIMESTAMP_SEC (0x12345678l)
#define TEST_DATASOURCE_TIMESTAMP_NSEC (0x76543210l)
#define TEST_DATASOURCE_TIMESTAMP (0x1234567876543210l)
#define TEST_DATASOURCE_TIMESTAMP_SEC_2 (0x1234567cl)
#define TEST_DATASOURCE_TIMESTAMP_NSEC_2 (0x7654321al)
#define TEST_DATASOURCE_TIMESTAMP_2 (0x1234567c7654321al)

log_level_t log_level = log_error;
struct ServerData *g_sData;
UA_Boolean g_running = true;
const UA_Logger *UA_Log_Stdout = NULL;
const UA_DataType UA_TYPES[UA_TYPES_COUNT] = {};
static ServerData TheTestServerData;

extern "C" bool TheDsStarted;
extern "C" bool TheDsFinished;
extern "C" size_t TheTransmitCount;
extern "C" uint32_t TheDsInitialSequence;
extern "C" bool TheDsInitialSequenceLatched;

namespace {

/**
 * This class aggregates UTs for testing the Datasource for the OPC UA* Server
 */
class test_opcua_datasource_basic : public testing::Test
{
protected:
    void SetUp() override
    {
        m_sequence = 0;
        g_running = true;
        log_level = log_error;
        TheDsStarted = false;
        TheDsFinished = false;
        TheTransmitCount = 0;
        TheDsInitialSequence = 0;
        TheDsInitialSequenceLatched = false;

        // Reset the subscriber data structure
        memset(&m_subscriber_data, 0, sizeof (m_subscriber_data));

        // Initialize the output payload buffer
        memset(&m_out_payload, 0, sizeof (m_out_payload));
        m_subscriber_data.data = &m_out_payload;

        // Initialize the Publisher
        memset(&m_publisher_data, 0, sizeof (m_publisher_data));

        // Initialize a scalar value
        m_scalar = 1;
        m_scalar_variant.type = &UA_TYPES[UA_TYPES_UINT32];
        m_scalar_variant.data = &m_scalar;
        m_scalar_variant.arrayLength = 0;
        m_scalar_value.value = m_scalar_variant;

        // Initialize a test incoming payload
        m_in_payload.type = TALKER;
        m_in_payload.sequence_num = TEST_DATASOURCE_SEQ_NUMBER;
        m_in_payload.load.talker.sec = TEST_DATASOURCE_TIMESTAMP_SEC;
        m_in_payload.load.talker.nsec = TEST_DATASOURCE_TIMESTAMP_NSEC;

        // Initialize a test incoming payload
        m_in_payload_2.type = TALKER;
        m_in_payload_2.sequence_num = TEST_DATASOURCE_SEQ_NUMBER_2;
        m_in_payload_2.load.talker.sec = TEST_DATASOURCE_TIMESTAMP_SEC_2;
        m_in_payload_2.load.talker.nsec = TEST_DATASOURCE_TIMESTAMP_NSEC_2;

        // Initialize a bytes array value for incoming payload
        m_variant.type = &UA_TYPES[UA_TYPES_BYTE];
        m_variant.data = &m_in_payload;
        m_variant.arrayLength = sizeof (m_in_payload);
        m_data_value.value = m_variant;

        // Initialize the test server data
        memset(&TheTestServerData, 0, sizeof (TheTestServerData));
        g_sData = &TheTestServerData;
        g_sData->packetCount = 2000;
    }

    void TearDown() override
    {
    }

public:
    void update()
    {
        m_in_payload.sequence_num = ++m_sequence;
    }

    uint32_t m_scalar;
    uint32_t m_sequence;
    UA_Variant m_variant;
    UA_Variant m_scalar_variant;
    UA_DataValue m_data_value;
    UA_DataValue m_scalar_value;
    GenericPayload m_in_payload;
    GenericPayload m_in_payload_2;
    GenericPayload m_out_payload;
    PublisherData m_publisher_data;
    SubscriberData m_subscriber_data;
    testing::StrictMock<open62541_mock> m_open62541_mock;
    testing::StrictMock<pdatasource_mock> m_datasource_mock;
};

TEST_F(test_opcua_datasource_basic, dummy_ds_write_positive)
{
    UA_StatusCode code = dummyDSWrite(NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
}

TEST_F(test_opcua_datasource_basic, dummy_ds_read_positive)
{
    UA_StatusCode code = dummyDSRead(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
}

TEST_F(test_opcua_datasource_basic, write_value_to_listener_ds_no_running_positive)
{
    g_running = false;
    UA_StatusCode code = writeValueToListenerDS(NULL, NULL, NULL, NULL, &m_subscriber_data, NULL, &m_data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
}

TEST_F(test_opcua_datasource_basic, write_value_to_listener_ds_positive)
{
    UA_StatusCode code = writeValueToListenerDS(NULL, NULL, NULL, NULL, &m_subscriber_data, NULL, &m_data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_EQ(m_out_payload.type, TALKER);
    EXPECT_EQ(m_out_payload.sequence_num, TEST_DATASOURCE_SEQ_NUMBER);
    EXPECT_EQ(m_out_payload.load.talker.sec, TEST_DATASOURCE_TIMESTAMP_SEC);
    EXPECT_EQ(m_out_payload.load.talker.nsec, TEST_DATASOURCE_TIMESTAMP_NSEC);
}

TEST_F(test_opcua_datasource_basic, write_value_to_listener_ds_no_logs_negative)
{
    int u = UA_Variant_isScalar(&m_scalar_variant);
    EXPECT_EQ(u, 1);

    testing::internal::CaptureStdout();
    UA_StatusCode code = writeValueToListenerDS(NULL, NULL, NULL, NULL, &m_subscriber_data, NULL, &m_scalar_value);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_EQ(m_out_payload.type, 0);
    EXPECT_EQ(m_out_payload.sequence_num, 0);
    EXPECT_EQ(m_out_payload.load.talker.sec, 0);
    EXPECT_EQ(m_out_payload.load.talker.nsec, 0);
    EXPECT_EQ(stdout_string, "");
}

TEST_F(test_opcua_datasource_basic, write_value_to_listener_ds_with_logs_negative)
{
    log_level = log_debug;
    int u = UA_Variant_isScalar(&m_scalar_variant);
    EXPECT_EQ(u, 1);

    testing::internal::CaptureStdout();
    UA_StatusCode code = writeValueToListenerDS(NULL, NULL, NULL, NULL, &m_subscriber_data, NULL, &m_scalar_value);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_EQ(m_out_payload.type, 0);
    EXPECT_EQ(m_out_payload.sequence_num, 0);
    EXPECT_EQ(m_out_payload.load.talker.sec, 0);
    EXPECT_EQ(m_out_payload.load.talker.nsec, 0);
    EXPECT_NE(stdout_string.find("Malformed incoming packet: incorrect type"), std::string::npos);
}

TEST_F(test_opcua_datasource_basic, write_value_to_listener_ds_wrong_argument_no_logs_negative)
{
    log_level = log_undef;
    int u = UA_Variant_isScalar(&m_scalar_variant);
    EXPECT_EQ(u, 1);

    testing::internal::CaptureStderr();
    UA_StatusCode code = writeValueToListenerDS(NULL, NULL, NULL, NULL, NULL, NULL, &m_scalar_value);
    const std::string &stderr_string = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, UA_STATUSCODE_BADINVALIDARGUMENT);
    EXPECT_EQ(m_out_payload.type, 0);
    EXPECT_EQ(m_out_payload.sequence_num, 0);
    EXPECT_EQ(m_out_payload.load.talker.sec, 0);
    EXPECT_EQ(m_out_payload.load.talker.nsec, 0);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_opcua_datasource_basic, write_value_to_listener_ds_wrong_argument_with_logs_negative)
{
    int u = UA_Variant_isScalar(&m_scalar_variant);
    EXPECT_EQ(u, 1);

    testing::internal::CaptureStderr();
    UA_StatusCode code = writeValueToListenerDS(NULL, NULL, NULL, NULL, NULL, NULL, &m_scalar_value);
    const std::string &stderr_string = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, UA_STATUSCODE_BADINVALIDARGUMENT);
    EXPECT_EQ(m_out_payload.type, 0);
    EXPECT_EQ(m_out_payload.sequence_num, 0);
    EXPECT_EQ(m_out_payload.load.talker.sec, 0);
    EXPECT_EQ(m_out_payload.load.talker.nsec, 0);
    EXPECT_NE(stderr_string.find("The 'nodeContext' argument is missing"), std::string::npos);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_wrong_argument_no_logs_negative)
{
    log_level = log_undef;
    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    testing::internal::CaptureStderr();
    const UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, NULL, UA_FALSE, NULL, &data_value);
    const std::string &stderr_string = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, UA_STATUSCODE_BADINVALIDARGUMENT);
    EXPECT_FALSE(data_value.hasSourceTimestamp);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_wrong_argument_with_logs_negative)
{
    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    testing::internal::CaptureStderr();
    const UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, NULL, UA_FALSE, NULL, &data_value);
    const std::string &stderr_string = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, UA_STATUSCODE_BADINVALIDARGUMENT);
    EXPECT_FALSE(data_value.hasSourceTimestamp);
    EXPECT_NE(stderr_string.find("The 'nodeContext' argument is missing"), std::string::npos);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_no_running_positive)
{
    g_running = false;

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    const UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_FALSE(data_value.hasSourceTimestamp);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_finished_state_positive)
{
    TheDsFinished = true;

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    const UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_FALSE(data_value.hasSourceTimestamp);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_positive)
{
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillOnce(testing::Return(&m_in_payload));
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillOnce(testing::Return());

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    const UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_FALSE(TheDsStarted);
    EXPECT_FALSE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, TEST_DATASOURCE_SEQ_NUMBER);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_start_positive)
{
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillOnce(testing::Return(&m_in_payload))
        .WillOnce(testing::Return(&m_in_payload_2));
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillOnce(testing::Return())
        .WillOnce(testing::Return());

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(TheDsStarted);
    EXPECT_FALSE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, TEST_DATASOURCE_SEQ_NUMBER);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_no_item_in_ds_positive)
{
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillOnce(testing::Return(&m_in_payload))
        .WillOnce(testing::Return(&m_in_payload_2))
        .WillOnce(testing::Return(nullptr));
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillOnce(testing::Return())
        .WillOnce(testing::Return())
        .WillOnce(testing::Return());

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    EXPECT_EQ(code, UA_STATUSCODE_GOOD);

    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    EXPECT_EQ(code, UA_STATUSCODE_GOOD);

    EXPECT_TRUE(TheDsStarted);
    EXPECT_FALSE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, TEST_DATASOURCE_SEQ_NUMBER);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_finish_positive)
{
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillRepeatedly(testing::Return());
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillRepeatedly(testing::DoAll(testing::InvokeWithoutArgs(this, &test_opcua_datasource_basic::update),
                                       testing::Return(&m_in_payload)));

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code;
    for (int i = 0; i < TEST_DATASOURCE_SEQUENCE + 1; ++i) {
        code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
        EXPECT_EQ(code, UA_STATUSCODE_GOOD);
        EXPECT_FALSE(TheDsFinished);
    }
    // The final invocation
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(TheDsStarted);
    EXPECT_TRUE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, 1);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_finish_with_log_positive)
{
    log_level = log_info;
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillRepeatedly(testing::Return());
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillRepeatedly(testing::DoAll(testing::InvokeWithoutArgs(this, &test_opcua_datasource_basic::update),
                                       testing::Return(&m_in_payload)));

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code;
    for (int i = 0; i < TEST_DATASOURCE_SEQUENCE + 1; ++i) {
        code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
        EXPECT_EQ(code, UA_STATUSCODE_GOOD);
        EXPECT_FALSE(TheDsFinished);
    }
    // The final invocation
    testing::internal::CaptureStdout();
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(TheDsStarted);
    EXPECT_TRUE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, 1);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
    EXPECT_NE(stdout_string.find("Reached limit of max transmit counts, stopping publisher"), std::string::npos);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_start_with_timestamp_positive)
{
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillOnce(testing::Return(&m_in_payload))
        .WillOnce(testing::Return(&m_in_payload_2));
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillOnce(testing::Return())
        .WillOnce(testing::Return());
    EXPECT_CALL(m_open62541_mock, UA_DateTime_now())
        .WillOnce(testing::Return(TEST_DATASOURCE_TIMESTAMP))
        .WillOnce(testing::Return(TEST_DATASOURCE_TIMESTAMP_2));

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_TRUE, NULL, &data_value);
    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(data_value.hasSourceTimestamp);
    EXPECT_EQ(data_value.sourceTimestamp, TEST_DATASOURCE_TIMESTAMP);
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_TRUE, NULL, &data_value);
    EXPECT_EQ(data_value.sourceTimestamp, TEST_DATASOURCE_TIMESTAMP_2);

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(TheDsStarted);
    EXPECT_FALSE(TheDsFinished);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
    EXPECT_TRUE(data_value.hasSourceTimestamp);
    EXPECT_EQ(TheDsInitialSequence, TEST_DATASOURCE_SEQ_NUMBER);
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_same_packet_no_log_positive)
{
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillRepeatedly(testing::Return());
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillRepeatedly(testing::DoAll(testing::InvokeWithoutArgs(this, &test_opcua_datasource_basic::update),
                                       testing::Return(&m_in_payload)));

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code;
    for (int i = 0; i < TEST_DATASOURCE_SEQUENCE/2; ++i) {
        code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
        EXPECT_EQ(code, UA_STATUSCODE_GOOD);
        EXPECT_FALSE(TheDsFinished);
    }
    --m_sequence;
    // The final invocation
    testing::internal::CaptureStdout();
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(TheDsStarted);
    EXPECT_FALSE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, 1);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
    EXPECT_EQ(stdout_string, "");
}

TEST_F(test_opcua_datasource_basic, read_value_from_ds_same_packet_with_log_positive)
{
    log_level = log_info;
    EXPECT_CALL(m_open62541_mock, UA_Variant_setArray(_, _, sizeof (GenericPayload), _))
        .WillRepeatedly(testing::Return());
    EXPECT_CALL(m_datasource_mock, get_value_from_datasource())
        .WillRepeatedly(testing::DoAll(testing::InvokeWithoutArgs(this, &test_opcua_datasource_basic::update),
                                       testing::Return(&m_in_payload)));

    UA_DataValue data_value;
    memset(&data_value, 0, sizeof (data_value));

    UA_StatusCode code;
    for (int i = 0; i < TEST_DATASOURCE_SEQUENCE/2; ++i) {
        code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
        EXPECT_EQ(code, UA_STATUSCODE_GOOD);
        EXPECT_FALSE(TheDsFinished);
    }
    --m_sequence;
    // The final invocation
    testing::internal::CaptureStdout();
    code = readValueFromTalkerDS(NULL, NULL, NULL, NULL, &m_publisher_data, UA_FALSE, NULL, &data_value);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();

    EXPECT_EQ(code, UA_STATUSCODE_GOOD);
    EXPECT_TRUE(TheDsStarted);
    EXPECT_FALSE(TheDsFinished);
    EXPECT_EQ(TheDsInitialSequence, 1);
    EXPECT_TRUE(TheDsInitialSequenceLatched);
    EXPECT_EQ(stdout_string, "");
}

} // namespace {
