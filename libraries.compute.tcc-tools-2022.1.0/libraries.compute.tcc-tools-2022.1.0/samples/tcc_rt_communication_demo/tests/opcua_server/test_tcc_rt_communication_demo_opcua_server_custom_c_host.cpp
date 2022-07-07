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
#include "opcua_custom.c"
} // extern "C"

#include "opcua_utils.h"

#include "alloc_mock.hpp"
#include "open62541_mock.hpp"
#include "libc_pthread_mock.hpp"

#include <gtest/gtest.h>

using namespace ::testing;

struct threadParams *g_thread = NULL;
UA_UInt16 g_threadRun;
UA_Boolean g_running = true;
UA_NodeId g_writerGroupIdent;
struct ServerData* g_sData = NULL;
const UA_Logger *UA_Log_Stdout = NULL;
log_level_t log_level = log_error;

static void test_opcua_custom_c_exit(void);


namespace {

void *void_thread(__attribute__((unused)) void *arg)
{
    return NULL;
}

UA_Logger gUaTestLogger = {
    .log = NULL,
    .context = NULL,
    .clear = NULL,
};

const pthread_t gTestThreadId = 1234;
const time_t gTestSeconds = 100;
const long gTestNanoSeconds0G5 = 500000000;
const long gTestNanoSeconds1G5 = 1500000000;
const long gTestNanoSeconds2G5 = 2500000000;
const long gTestNanoSeconds20G5 = 20500000000;
const long gTestNanoSecondsMinus20G5 = -20500000000;

class test_opcua_custom_c : public testing::Test
{
protected:

    void SetUp() override {
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
            .url = const_cast<char *>("opc.eth://20-20-20-20-20-20"),
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
            .server = NULL,
            .baseTime = 0
        };

        gUaTestLogger.log = &test_logger_log;
        UA_Log_Stdout = &gUaTestLogger;

        g_sData = NULL;
        m_test_count = 0;
        g_indexPub = 0;
        g_indexSub = 0;
        log_level = log_error;
        server_ = reinterpret_cast<UA_Server *>(0x80001000);
        server_data_ = &server_data;
        publisher_data_ = &pub_data;
        subscriber_data_ = &sub_data;
        publisher_data_->cpuAffinity = 0;
        server_data_->baseTime = 0;

        // Allocate mocks
        m_alloc_mock = new NiceMock<alloc_mock>();
        m_pthread_mock = new StrictMock<pthread_mock>();
        m_open62541_mock = new StrictMock<open62541_mock>();

        static threadParams threads[10] = {};
        g_thread = threads;
    }

    void TearDown() override {
        delete m_alloc_mock;
        m_alloc_mock = NULL;
        delete m_pthread_mock;
        m_pthread_mock = NULL;
        delete m_open62541_mock;
        m_open62541_mock = NULL;
    }
protected:
    static void test_server_callback(__attribute__((unused)) UA_Server *server,
        __attribute__((unused)) void *data) {
        if (!m_test_count) {
            g_running = false;
        } else {
            --m_test_count;
        }
    }
    static void test_logger_log(__attribute__((unused)) void *ctx,
        __attribute__((unused)) UA_LogLevel lvl,
        __attribute__((unused)) UA_LogCategory cat,
        __attribute__((unused)) const char *msg,
        __attribute__((unused)) va_list args) {
    }

public:
    static int m_test_count;
    static NiceMock<alloc_mock> *m_alloc_mock;
    static StrictMock<pthread_mock> *m_pthread_mock;
    static StrictMock<open62541_mock> *m_open62541_mock;

protected:
    UA_Server *server_;
    struct ServerData *server_data_;
    struct PublisherData *publisher_data_;
    struct SubscriberData *subscriber_data_;
};
int test_opcua_custom_c::m_test_count = 0;
NiceMock<alloc_mock> *test_opcua_custom_c::m_alloc_mock = NULL;
StrictMock<pthread_mock> *test_opcua_custom_c::m_pthread_mock = NULL;
StrictMock<open62541_mock> *test_opcua_custom_c::m_open62541_mock = NULL;

TEST_F(test_opcua_custom_c, normalize_500M_positive)
{
    timespec timespecValue;
    timespecValue.tv_sec = gTestSeconds;
    timespecValue.tv_nsec = gTestNanoSeconds0G5;

    normalize(&timespecValue);

    EXPECT_EQ(timespecValue.tv_sec, gTestSeconds);
    EXPECT_EQ(timespecValue.tv_nsec, gTestNanoSeconds0G5);
}

TEST_F(test_opcua_custom_c, normalize_1G5_positive)
{
    timespec timespecValue;
    timespecValue.tv_sec = gTestSeconds;
    timespecValue.tv_nsec = gTestNanoSeconds1G5;

    normalize(&timespecValue);

    EXPECT_EQ(timespecValue.tv_sec, gTestSeconds + 1);
    EXPECT_EQ(timespecValue.tv_nsec, gTestNanoSeconds0G5);
}

TEST_F(test_opcua_custom_c, normalize_2G5_positive)
{
    timespec timespecValue;
    timespecValue.tv_sec = gTestSeconds;
    timespecValue.tv_nsec = gTestNanoSeconds2G5;

    normalize(&timespecValue);

    EXPECT_EQ(timespecValue.tv_sec, gTestSeconds + 2);
    EXPECT_EQ(timespecValue.tv_nsec, gTestNanoSeconds0G5);
}

TEST_F(test_opcua_custom_c, normalize_20G5_positive)
{
    timespec timespecValue;
    timespecValue.tv_sec = gTestSeconds;
    timespecValue.tv_nsec = gTestNanoSeconds20G5;

    normalize(&timespecValue);

    EXPECT_EQ(timespecValue.tv_sec, gTestSeconds + 20);
    EXPECT_EQ(timespecValue.tv_nsec, gTestNanoSeconds0G5);
}

TEST_F(test_opcua_custom_c, normalize_minus_20G5_positive)
{
    timespec timespecValue;
    timespecValue.tv_sec = gTestSeconds;
    timespecValue.tv_nsec = gTestNanoSecondsMinus20G5;

    normalize(&timespecValue);

    EXPECT_EQ(timespecValue.tv_sec, gTestSeconds - 21);
    EXPECT_EQ(timespecValue.tv_nsec, gTestNanoSeconds0G5);
}

TEST_F(test_opcua_custom_c, UA_PubSubManager_changeRepeatedCallbackInterval_positive)
{
    const UA_StatusCode res = UA_PubSubManager_changeRepeatedCallbackInterval(NULL, 0, 0.0);

    EXPECT_EQ(res, UA_STATUSCODE_GOOD);
}

TEST_F(test_opcua_custom_c, UA_PubSubManager_removeRepeatedPubSubCallback_positive)
{
    UA_PubSubManager_removeRepeatedPubSubCallback(NULL, 0);
}

TEST_F(test_opcua_custom_c, threadCreation_positive)
{
    char appName[] = "app-name";
    threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
        .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));

    const pthread_t id = threadCreation(0, 0, void_thread, appName, params);

    EXPECT_EQ(id, gTestThreadId);
    UA_free(params);
}

TEST_F(test_opcua_custom_c, threadCreation_large_affinity_positive)
{
    char appName[] = "app-name";
    threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
        .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));

    const pthread_t id = threadCreation(0, 1024*1024*1024, void_thread, appName, params);

    EXPECT_EQ(id, gTestThreadId);
    UA_free(params);
}

TEST_F(test_opcua_custom_c, threadCreation_no_log_positive)
{
    UA_Log_Stdout = NULL;
    char appName[] = "app-name";
    threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
        .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));

    const pthread_t id = threadCreation(0, 0, void_thread, appName, params);

    EXPECT_EQ(id, gTestThreadId);
    UA_free(params);
}

TEST_F(test_opcua_custom_c, threadCreation_no_log_handler_positive)
{
    gUaTestLogger.log = NULL;
    char appName[] = "app-name";
    threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
        .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));

    const pthread_t id = threadCreation(0, 0, void_thread, appName, params);

    EXPECT_EQ(id, gTestThreadId);
    UA_free(params);
}

TEST_F(test_opcua_custom_c, threadCreation_setaffinity_fail_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        char appName[] = "app-name";
        threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
            .WillOnce(Return(-1));

        threadCreation(0, 0, void_thread, appName, params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, threadCreation_setaffinity_fail_no_log_negative)
{
    UA_Log_Stdout = NULL;
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        char appName[] = "app-name";
        threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
            .WillOnce(Return(-1));

        threadCreation(0, 0, void_thread, appName, params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, threadCreation_setschedparam_fail_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        char appName[] = "app-name";
        threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
            .WillOnce(Return(-1));

        threadCreation(0, 0, void_thread, appName, params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, threadCreation_setschedparam_fail_no_log_negative)
{
    UA_Log_Stdout = NULL;
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        char appName[] = "app-name";
        threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
            .WillOnce(Return(-1));

        threadCreation(0, 0, void_thread, appName, params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, threadCreation_pthread_create_fail_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        char appName[] = "app-name";
        threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(-1)));

        threadCreation(0, 0, void_thread, appName, params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, threadCreation_pthread_create_fail_no_log_negative)
{
    UA_Log_Stdout = NULL;
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        char appName[] = "app-name";
        threadParams *const params = static_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, void_thread, params))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(-1)));

        threadCreation(0, 0, void_thread, appName, params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, pub_thread_positive)
{
    g_running = true;
    m_test_count = 10;
    g_sData = server_data_;
    UA_WriterGroup writerGroup = {};
    UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

    threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    params->server = server;
    params->data_index = 0;
    params->callback = &test_server_callback;
    params->data = &writerGroup;

    void *const res = pub_thread(params);

    EXPECT_EQ(res, nullptr);
}

TEST_F(test_opcua_custom_c, pub_thread_with_base_time_positive)
{
    g_running = true;
    m_test_count = 10;
    g_sData = server_data_;
    UA_WriterGroup writerGroup = {};
    UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

    struct timespec test_time_now;
    clock_gettime(CLOCKID, &test_time_now);
    threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    server_data_->baseTime = test_time_now.tv_sec + 3;
    params->server = server;
    params->data_index = 0;
    params->callback = &test_server_callback;
    params->data = &writerGroup;

    void *const res = pub_thread(params);

    EXPECT_EQ(res, nullptr);
}

TEST_F(test_opcua_custom_c, pub_thread_with_base_time_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        g_running = true;
        m_test_count = 10;
        g_sData = server_data_;
        UA_WriterGroup writerGroup = {};
        UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

        struct timespec test_time_now;
        clock_gettime(CLOCKID, &test_time_now);
        threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        server_data_->baseTime = test_time_now.tv_sec + 500;
        params->server = server;
        params->data_index = 0;
        params->callback = &test_server_callback;
        params->data = &writerGroup;

        pub_thread(params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, pub_thread_with_base_time_in_past_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        g_running = true;
        m_test_count = 10;
        g_sData = server_data_;
        UA_WriterGroup writerGroup = {};
        UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

        struct timespec test_time_now;
        clock_gettime(CLOCKID, &test_time_now);
        threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        server_data_->baseTime = test_time_now.tv_sec - 300;
        params->server = server;
        params->data_index = 0;
        params->callback = &test_server_callback;
        params->data = &writerGroup;

        pub_thread(params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, sub_thread_positive)
{
    g_running = true;
    m_test_count = 10;
    g_sData = server_data_;
    UA_WriterGroup writerGroup = {};
    UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

    threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    params->server = server;
    params->data_index = 0;
    params->callback = &test_server_callback;
    params->data = &writerGroup;

    void *const res = sub_thread(params);

    EXPECT_EQ(res, nullptr);
}

TEST_F(test_opcua_custom_c, sub_thread_with_base_time_positive)
{
    g_running = true;
    m_test_count = 10;
    g_sData = server_data_;
    UA_WriterGroup writerGroup = {};
    UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

    struct timespec test_time_now;
    clock_gettime(CLOCKID, &test_time_now);
    threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
    server_data_->baseTime = test_time_now.tv_sec + 3;
    params->server = server;
    params->data_index = 0;
    params->callback = &test_server_callback;
    params->data = &writerGroup;

    void *const res = sub_thread(params);

    EXPECT_EQ(res, nullptr);
}

TEST_F(test_opcua_custom_c, sub_thread_with_base_time_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        g_running = true;
        m_test_count = 10;
        g_sData = server_data_;
        UA_WriterGroup writerGroup = {};
        UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

        struct timespec test_time_now;
        clock_gettime(CLOCKID, &test_time_now);
        threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        server_data_->baseTime = test_time_now.tv_sec + 500;
        params->server = server;
        params->data_index = 0;
        params->callback = &test_server_callback;
        params->data = &writerGroup;

        sub_thread(params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, sub_thread_with_base_time_in_past_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);
        g_running = true;
        m_test_count = 10;
        g_sData = server_data_;
        UA_WriterGroup writerGroup = {};
        UA_Server *const server = reinterpret_cast<UA_Server *>(0x80001200);

        struct timespec test_time_now;
        clock_gettime(CLOCKID, &test_time_now);
        threadParams *const params = reinterpret_cast<threadParams *>(UA_malloc(sizeof (threadParams)));
        server_data_->baseTime = test_time_now.tv_sec - 300;
        params->server = server;
        params->data_index = 0;
        params->callback = &test_server_callback;
        params->data = &writerGroup;

        sub_thread(params);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, addRepeatedCallback_publisher_positive)
{
    void *params = NULL;
    g_sData = server_data_;
    UA_UInt64 callbackId = 0x1234;

    EXPECT_CALL(*m_open62541_mock, UA_NodeId_order(_, _))
        .WillOnce(Return(UA_ORDER_EQ));
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, _, _))
        .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), SaveArg<3>(&params), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));

    UA_StatusCode res = UA_PubSubManager_addRepeatedCallback(server_, NULL, NULL, 500000, &callbackId);

    EXPECT_EQ(res, UA_STATUSCODE_GOOD);
    EXPECT_NE(params, nullptr);
    UA_free(params);
}

TEST_F(test_opcua_custom_c, addRepeatedCallback_publisher_alloc_failed_negative)
{
    UA_UInt64 callbackId = 0x1234;

    EXPECT_CALL(*m_alloc_mock, malloc(sizeof (threadParams)))
        .WillOnce(Return(nullptr));

    UA_StatusCode res = UA_PubSubManager_addRepeatedCallback(server_, NULL, NULL, 500000, &callbackId);

    EXPECT_EQ(res, UA_STATUSCODE_BADOUTOFMEMORY);
}

TEST_F(test_opcua_custom_c, addRepeatedCallback_publisher_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);

        g_sData = server_data_;
        UA_UInt64 callbackId = 0x1234;

        EXPECT_CALL(*m_open62541_mock, UA_NodeId_order(_, _))
            .WillOnce(Return(UA_ORDER_EQ));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, _, _))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
            .WillOnce(Return(-1));

        UA_PubSubManager_addRepeatedCallback(server_, NULL, NULL, 500000, &callbackId);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

TEST_F(test_opcua_custom_c, addRepeatedCallback_subscriber_positive)
{
    void *params = NULL;
    g_sData = server_data_;
    UA_UInt64 callbackId = 0x1234;

    EXPECT_CALL(*m_open62541_mock, UA_NodeId_order(_, _))
        .WillOnce(Return(UA_ORDER_MORE));
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, _, _))
        .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), SaveArg<3>(&params), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));

    UA_StatusCode res = UA_PubSubManager_addRepeatedCallback(server_, NULL, NULL, 500000, &callbackId);

    EXPECT_EQ(res, UA_STATUSCODE_GOOD);
    EXPECT_NE(params, nullptr);
    UA_free(params);
}

TEST_F(test_opcua_custom_c, addRepeatedCallback_subscriber_negative)
{
    auto test = [this] {
        atexit(test_opcua_custom_c_exit);

        g_sData = server_data_;
        UA_UInt64 callbackId = 0x1234;

        EXPECT_CALL(*m_open62541_mock, UA_NodeId_order(_, _))
            .WillOnce(Return(UA_ORDER_MORE));
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, nullptr, _, _))
            .WillOnce(DoAll(SetArgPointee<0>(gTestThreadId), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(gTestThreadId, SCHED_RR, _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(gTestThreadId, sizeof (cpu_set_t), _))
            .WillOnce(Return(-1));

        UA_PubSubManager_addRepeatedCallback(server_, NULL, NULL, 500000, &callbackId);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "");
}

} // namespace {

void test_opcua_custom_c_exit(void)
{
    bool correct = true;
    if (test_opcua_custom_c::m_alloc_mock) {
        correct = correct && test_opcua_custom_c::m_alloc_mock->verify_and_clear();
        delete test_opcua_custom_c::m_alloc_mock;
        test_opcua_custom_c::m_alloc_mock = NULL;
    }
    if (test_opcua_custom_c::m_pthread_mock)
    {
        correct = correct && test_opcua_custom_c::m_pthread_mock->verify_and_clear();
        delete test_opcua_custom_c::m_pthread_mock;
        test_opcua_custom_c::m_pthread_mock = NULL;
    }
    if (test_opcua_custom_c::m_open62541_mock)
    {
        correct = correct && test_opcua_custom_c::m_open62541_mock->verify_and_clear();
        delete test_opcua_custom_c::m_open62541_mock;
        test_opcua_custom_c::m_open62541_mock = NULL;
    }

    if (!correct)
    {
        // If the final verification fails, exit with a different code (2)
        _exit(2);
    }
}
