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

#include "alloc_mock.hpp"
#include "libc_pthread_mock.hpp"

extern "C" {
#define kill kill_fake

void ptimer_atexit(void);
int kill_fake(pid_t pid, int signal);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include "ptimer.c"
#pragma GCC diagnostic pop

#undef kill
}  // extern "C"

using ::testing::Mock;
using ::testing::AllOf;
using namespace ::testing;
using ::testing::HasSubstr;

namespace {

#define TEST_THREAD_ID 1234
#define TEST_EVENTS_MAX_COUNT 1000
#define TEST_TIMER_CORRECT_PERIOD 500000u
#define TEST_TIMER_CORRECT_START_TIME 250000

/**
 * This class is used for basic UTs for TCC TSN E2E sample timer module defined in \ref ptimer.h
 */
class test_tcc_rt_communication_demo_timer_basic : public testing::Test
{
protected:
     void SetUp() override {
        gTimerCount = 0;
    }
    void TearDown() override {
    }
};


class test_tcc_rt_communication_demo_timer_mock : public testing::Test
{
protected:
     void SetUp() override {
        atexit(ptimer_atexit);
        m_pthread_mock = new StrictMock<pthread_mock>();
        m_test_count = 1;
        gTimerCount = 0;
        m_stop_request = false;
        m_timer_finish_count = 0;
        m_timer_handler_count = 0;

        // Config initialization
        m_stop_request = false;
        m_config.verbose = false;
        m_config.packetType = 0;
        m_config.startTime = TEST_TIMER_CORRECT_START_TIME;
        m_config.period = TEST_TIMER_CORRECT_PERIOD;
        m_config.iterations = 0;
        m_config.sequenceNumber = 0;
        m_config.pLogFile = const_cast<char *>("test-log-file.log");
        m_config.incomingDataSource = NULL;
        m_config.pStopVariable = &m_stop_request;

        // Initialize the timer item object
        m_timer_item.period = TEST_TIMER_CORRECT_PERIOD;
        m_timer_item.startTime = TEST_TIMER_CORRECT_START_TIME;
        m_timer_item.pTimerHandler = &TestTimerHandler;
        m_timer_item.pFiniHandler = &TestTimerFinishHandler;
        m_timer_item.pStopVariable = &m_stop_request;
    }
    void TearDown() override {
        delete m_alloc_mock;
        m_alloc_mock = NULL;
        delete m_pthread_mock;
        m_pthread_mock = NULL;
    }

protected:
    static void TestTimerHandler(uint64_t timestamp) {
        (void)timestamp;
        ++m_timer_handler_count;
    }
    static void TestTimerHandlerOneIteration(uint64_t timestamp) {
        (void)timestamp;
        ++m_timer_handler_count;
        m_stop_request = true;
    }
    static void TestTimerHandlerManyIteration(uint64_t timestamp) {
        (void)timestamp;
        ++m_timer_handler_count;
        --m_test_count;
        if (!m_test_count) {
            m_stop_request = true;
        }
    }
    static void TestTimerFinishHandler() {
        ++m_timer_finish_count;
    }

protected:
    PConfig m_config;
    TimerItem m_timer_item;
    static int m_test_count;
    static bool m_stop_request;
    static int m_timer_handler_count;
    static int m_timer_finish_count;

public:
    static NiceMock<alloc_mock> *m_alloc_mock;
    static StrictMock<pthread_mock> *m_pthread_mock;
};
int test_tcc_rt_communication_demo_timer_mock::m_test_count = 0;
bool test_tcc_rt_communication_demo_timer_mock::m_stop_request = false;
int test_tcc_rt_communication_demo_timer_mock::m_timer_finish_count = 0;
int test_tcc_rt_communication_demo_timer_mock::m_timer_handler_count = 0;
NiceMock<alloc_mock> *test_tcc_rt_communication_demo_timer_mock::m_alloc_mock = NULL;
StrictMock<pthread_mock> *test_tcc_rt_communication_demo_timer_mock::m_pthread_mock = NULL;

/**
 * This class is used for UTs with config support for TCC TSN E2E sample timer module
 * defined in \ref ptimer.h
 */
class test_tcc_rt_communication_demo_timer_config : public testing::Test
{
protected:
     void SetUp() override {
        gTimerCount = 0;
        m_timer_handler_count = 0;
        m_timer_finish_count = 0;

        // Config initialization
        m_stop_request = false;
        m_config.verbose = false;
        m_config.packetType = 0;
        m_config.startTime = TEST_TIMER_CORRECT_START_TIME;
        m_config.period = TEST_TIMER_CORRECT_PERIOD;
        m_config.iterations = 0;
        m_config.sequenceNumber = 0;
        m_config.pLogFile = const_cast<char *>("test-log-file.log");
        m_config.incomingDataSource = NULL;
        m_config.pStopVariable = &m_stop_request;

        // Initialize the timer item object
        m_timer_item.period = TEST_TIMER_CORRECT_PERIOD;
        m_timer_item.startTime = TEST_TIMER_CORRECT_START_TIME;
        m_timer_item.pTimerHandler = &TestTimerHandler;
        m_timer_item.pFiniHandler = &TestTimerFinishHandler;
        m_timer_item.pStopVariable = &m_stop_request;
    }
    void TearDown() override {
    }

protected:
    static void TestTimerHandler(uint64_t timestamp) {
        (void)timestamp;
        ++m_timer_handler_count;
    }
    static void TestTimerFinishHandler() {
        ++m_timer_finish_count;
    }

protected:
    PConfig m_config;
    bool m_stop_request;
    TimerItem m_timer_item;
    static int m_timer_handler_count;
    static int m_timer_finish_count;
};
int test_tcc_rt_communication_demo_timer_config::m_timer_finish_count = 0;
int test_tcc_rt_communication_demo_timer_config::m_timer_handler_count = 0;

// Test the Timers count management
TEST_F(test_tcc_rt_communication_demo_timer_basic, ptimer_get_timers_count_positive)
{
    uint32_t initial_timer_count = get_timers_count();
    EXPECT_EQ(initial_timer_count, 0);
    gTimerCount = 42;
    uint32_t timer_count = get_timers_count();
    EXPECT_EQ(timer_count, 42);
}

TEST_F(test_tcc_rt_communication_demo_timer_basic, ptimer_inc_timers_count_positive)
{
    uint32_t timer_count = get_timers_count();
    ASSERT_EQ(timer_count, 0);

    inc_timers_count();
    timer_count = get_timers_count();
    EXPECT_EQ(timer_count, 1);

    // Emulate adding 20 more timers
    for (int i = 0; i < 20; ++i) inc_timers_count();
    timer_count = get_timers_count();
    EXPECT_EQ(timer_count, 21);
}

TEST_F(test_tcc_rt_communication_demo_timer_basic, ptimer_dec_timers_count_positive)
{
    // Initialize the initial timers count
    ASSERT_EQ(get_timers_count(), 0);
    for (int i = 0; i < 42; ++i) inc_timers_count();
    ASSERT_EQ(get_timers_count(), 42);

    // Test single invokation first
    dec_timers_count();
    EXPECT_EQ(get_timers_count(), 41);

    // Test invoking 10 more increments
    for (int i = 0; i < 10; ++i) dec_timers_count();
    EXPECT_EQ(get_timers_count(), 31);

    // Test approaching zero in the timers count
    for (int i = 0; i < 31; ++i) dec_timers_count();
    EXPECT_EQ(get_timers_count(), 0);

    // Test thurther decreasing the timers count
    dec_timers_count();
    EXPECT_EQ(get_timers_count(), UINT32_C(-1));
}

TEST_F(test_tcc_rt_communication_demo_timer_config, ptimer_stop_positive)
{
    ASSERT_FALSE(m_stop_request);

    ptimer_stop(&m_config);

    EXPECT_TRUE(m_stop_request);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_start_positive)
{
    pthread_t thread_id = TEST_THREAD_ID;
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), Return(0)));
    EXPECT_CALL(*m_pthread_mock, pthread_detach(TEST_THREAD_ID))
        .WillOnce(Return(0));

    ptimer_start(&m_timer_item);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_create_positive)
{
    void *value = NULL;
    pthread_t thread_id = TEST_THREAD_ID;
    EXPECT_CALL(*m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), SaveArg<3>(&value), Return(0)));

    EXPECT_CALL(*m_pthread_mock, pthread_detach(TEST_THREAD_ID))
        .WillOnce(Return(0));

    ptimer_create(&m_config, TestTimerHandler, TestTimerFinishHandler);

    ASSERT_NE(value, (const void *)NULL);
    free(value);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_setup_thread_positive)
{
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));

    ptimer_setup_thread(0);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_basic_positive)
{
    m_stop_request = true;
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));

    TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
    item->period = TEST_TIMER_CORRECT_PERIOD;
    item->pTimerHandler = TestTimerHandler;
    item->pFiniHandler = TestTimerFinishHandler;
    item->startTime = TEST_TIMER_CORRECT_START_TIME;
    item->pStopVariable = &m_stop_request;
    *item->pStopVariable = true;

    testing::internal::CaptureStdout();
    void *const res = ptimer_thread(item);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, (const void *)NULL);
    EXPECT_EQ(m_timer_handler_count, 0);
    EXPECT_EQ(m_timer_finish_count, 1);
    EXPECT_THAT(captured, AllOf(HasSubstr("[INFO] Selected base time: "), HasSubstr(", time now: ")));
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_with_base_time_positive)
{
    m_stop_request = true;
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));

    timespec test_time_now = {};
    clock_gettime(CLOCK_ID, &test_time_now);
    TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
    item->period = TEST_TIMER_CORRECT_PERIOD;
    item->pTimerHandler = TestTimerHandler;
    item->pFiniHandler = TestTimerFinishHandler;
    item->startTime = TEST_TIMER_CORRECT_START_TIME;
    item->pStopVariable = &m_stop_request;
    *item->pStopVariable = true;
    item->baseTime = test_time_now.tv_sec + 3;

    testing::internal::CaptureStdout();
    void *const res = ptimer_thread(item);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, (const void *)NULL);
    EXPECT_EQ(m_timer_handler_count, 0);
    EXPECT_EQ(m_timer_finish_count, 1);
    EXPECT_THAT(captured, AllOf(HasSubstr("[INFO] Selected base time: "), HasSubstr(", time now: ")));
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_with_base_time_negative)
{
    auto test = [this] {
        m_stop_request = true;
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
            .WillOnce(Return(0));

        timespec test_time_now = {};
        clock_gettime(CLOCK_ID, &test_time_now);
        TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
        item->period = TEST_TIMER_CORRECT_PERIOD;
        item->pTimerHandler = TestTimerHandler;
        item->pFiniHandler = TestTimerFinishHandler;
        item->startTime = TEST_TIMER_CORRECT_START_TIME;
        item->pStopVariable = &m_stop_request;
        *item->pStopVariable = true;
        item->baseTime = test_time_now.tv_sec + 500;

        ptimer_thread(item);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "Provided wrong base time");
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_with_base_time_in_past_negative)
{
    auto test = [this] {
        m_stop_request = true;
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
            .WillOnce(Return(0));

        timespec test_time_now = {};
        clock_gettime(CLOCK_ID, &test_time_now);
        TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
        item->period = TEST_TIMER_CORRECT_PERIOD;
        item->pTimerHandler = TestTimerHandler;
        item->pFiniHandler = TestTimerFinishHandler;
        item->startTime = TEST_TIMER_CORRECT_START_TIME;
        item->pStopVariable = &m_stop_request;
        *item->pStopVariable = true;
        item->baseTime = test_time_now.tv_sec - 300;

        ptimer_thread(item);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), "Provided wrong base time");
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_null_fini_handler_positive)
{
    m_stop_request = true;
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));

    TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
    item->period = TEST_TIMER_CORRECT_PERIOD;
    item->pTimerHandler = TestTimerHandler;
    item->pFiniHandler = NULL;
    item->startTime = TEST_TIMER_CORRECT_START_TIME;
    item->pStopVariable = &m_stop_request;
    *item->pStopVariable = true;

    testing::internal::CaptureStdout();
    void *const res = ptimer_thread(item);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, (const void *)NULL);
    EXPECT_EQ(m_timer_handler_count, 0);
    EXPECT_EQ(m_timer_finish_count, 0);
    EXPECT_THAT(captured, AllOf(HasSubstr("[INFO] Selected base time: "), HasSubstr(", time now: ")));
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_one_iteration_positive)
{
    m_stop_request = true;
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));

    TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
    item->period = TEST_TIMER_CORRECT_PERIOD;
    item->pTimerHandler = TestTimerHandlerManyIteration;
    item->pFiniHandler = TestTimerFinishHandler;
    item->startTime = TEST_TIMER_CORRECT_START_TIME;
    item->pStopVariable = &m_stop_request;
    *item->pStopVariable = false;

    testing::internal::CaptureStdout();
    void *const res = ptimer_thread(item);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, (const void *)NULL);
    EXPECT_EQ(m_timer_handler_count, 1);
    EXPECT_EQ(m_timer_finish_count, 1);
    EXPECT_TRUE(m_stop_request);
    EXPECT_THAT(captured, AllOf(HasSubstr("[INFO] Selected base time: "), HasSubstr(", time now: ")));
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_many_iterations_positive)
{
    m_stop_request = true;
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));

    TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
    item->period = TEST_TIMER_CORRECT_PERIOD;
    item->pTimerHandler = TestTimerHandlerManyIteration;
    item->pFiniHandler = TestTimerFinishHandler;
    item->startTime = TEST_TIMER_CORRECT_START_TIME;
    item->pStopVariable = &m_stop_request;
    *item->pStopVariable = false;
    m_test_count = TEST_EVENTS_MAX_COUNT;

    testing::internal::CaptureStdout();
    void *const res = ptimer_thread(item);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, (const void *)NULL);
    EXPECT_EQ(m_timer_handler_count, TEST_EVENTS_MAX_COUNT);
    EXPECT_EQ(m_timer_finish_count, 1);
    EXPECT_TRUE(m_stop_request);
    EXPECT_THAT(captured, AllOf(HasSubstr("[INFO] Selected base time: "), HasSubstr(", time now: ")));
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_exit_request_positive)
{
    m_stop_request = true;
    EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_pthread_mock, kill_fake(_, SIGTERM))
        .WillOnce(Return(0));

    TimerItem *const item = static_cast<TimerItem *>(calloc(1, sizeof (TimerItem)));
    item->period = TEST_TIMER_CORRECT_PERIOD;
    item->pTimerHandler = TestTimerHandler;
    item->pFiniHandler = TestTimerFinishHandler;
    item->startTime = TEST_TIMER_CORRECT_START_TIME;
    item->pStopVariable = &m_stop_request;
    *item->pStopVariable = true;
    gTimerCount = 1;

    testing::internal::CaptureStdout();
    void *const res = ptimer_thread(item);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, (const void *)NULL);
    EXPECT_EQ(m_timer_handler_count, 0);
    EXPECT_EQ(m_timer_finish_count, 1);
    EXPECT_THAT(captured, AllOf(HasSubstr("[INFO] Selected base time: "), HasSubstr(", time now: ")));
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_thread_null_argument_negative)
{
    auto test = [this] {
        ptimer_thread(NULL);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void\\* ptimer_thread\\(void\\*\\): "
        "Assertion 'argument' failed: Bad incoming 'argument'";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_start_create_thread_failed_negative)
{
    auto test = [this] {
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, _, _, _))
            .WillOnce(Return(-1));

        ptimer_start(&m_timer_item);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_start\\(TimerItem\\*\\): "
        "Assertion '!res' failed: Failed creating PTimer thread";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_start_wrong_thread_id_negative)
{
    auto test = [this] {
        pthread_t thread_id = 0;
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, _, _, _))
            .WillOnce(DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), Return(0)));

        ptimer_start(&m_timer_item);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_start\\(TimerItem\\*\\): "
        "Assertion 'ThePTimerThreadId' failed: No thread ID for the created timer thread";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_start_pthread_detach_failed_negative)
{
    auto test = [this] {
        pthread_t thread_id = TEST_THREAD_ID;
        EXPECT_CALL(*m_pthread_mock, pthread_create(_, _, _, _))
            .WillOnce(DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), Return(0)));
        EXPECT_CALL(*m_pthread_mock, pthread_detach(TEST_THREAD_ID))
            .WillOnce(Return(-1));

        ptimer_start(&m_timer_item);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_start\\(TimerItem\\*\\): "
        "Assertion '!res' failed: Failed to detach PTimer thread";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_setup_thread_setaffinity_failed_negative)
{
    auto test = [this] {
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
            .WillOnce(Return(-1));

        ptimer_setup_thread(0);
    };

    const char *const expected_message =
        ".*Assertion '!res' failed: Failed setting timer thread affinity";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_setup_thread_setschedparam_failed_negative)
{
    auto test = [this] {
        EXPECT_CALL(*m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
            .WillOnce(Return(0));
        EXPECT_CALL(*m_pthread_mock, pthread_setschedparam(_, SCHED_RR, _))
            .WillOnce(Return(-1));

        ptimer_setup_thread(0);
    };

    const char *const expected_message =
        ".*Assertion '!res' failed: Failed setting timer thread priority";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_create_null_config_negative)
{
    auto test = [this] {
        ptimer_create(NULL, TestTimerHandler, TestTimerFinishHandler);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_create\\(PConfig\\*, ptimer_handler, pfini_handler): "
        "Assertion 'config' failed: Wrong 'config' argument";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_create_null_handler_negative)
{
    auto test = [this] {
        ptimer_create(&m_config, NULL, TestTimerFinishHandler);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_create\\(PConfig\\*, ptimer_handler, pfini_handler): "
        "Assertion 'pTimerHandler' failed: Wrong 'pTimerHandler' argument";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_create_wrong_period_negative)
{
    auto test = [this] {
        m_config.period = 0;

        ptimer_create(&m_config, TestTimerHandler, NULL);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_create\\(PConfig\\*, ptimer_handler, pfini_handler\\): "
        "Assertion 'config->period > 0' failed: Wrong 'period' argument: 0";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_create_no_stop_variable_negative)
{
    auto test = [this] {
        m_config.pStopVariable = NULL;

        ptimer_create(&m_config, TestTimerHandler, NULL);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_create\\(PConfig\\*, ptimer_handler, pfini_handler\\): "
        "Assertion 'config->pStopVariable' failed: Wrong 'pStopVariable' argument";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

TEST_F(test_tcc_rt_communication_demo_timer_mock, ptimer_create_alloc_failed_negative)
{
    auto test = [this] {
        m_alloc_mock = new NiceMock<alloc_mock>();
        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(TimerItem)))
            .WillOnce(ReturnNull());

        ptimer_create(&m_config, TestTimerHandler, NULL);
    };

    const char *const expected_message =
        "\\*\\*\\*\\*\\* ERROR: .*: void ptimer_create\\(PConfig\\*, ptimer_handler, pfini_handler\\): "
        "Assertion 'TimerConfig != NULL' failed: Cannot allocate memory for timer configuration";
    EXPECT_EXIT(test(), ::testing::ExitedWithCode(1), expected_message);
}

} // namespace {

void ptimer_atexit(void)
{
    bool success = true;
    if (test_tcc_rt_communication_demo_timer_mock::m_alloc_mock)
    {
        success = success && test_tcc_rt_communication_demo_timer_mock::m_alloc_mock->verify_and_clear();
        delete test_tcc_rt_communication_demo_timer_mock::m_alloc_mock;
        test_tcc_rt_communication_demo_timer_mock::m_alloc_mock = NULL;
    }
    if (test_tcc_rt_communication_demo_timer_mock::m_pthread_mock)
    {
        success = success && test_tcc_rt_communication_demo_timer_mock::m_pthread_mock->verify_and_clear();
        delete test_tcc_rt_communication_demo_timer_mock::m_pthread_mock;
        test_tcc_rt_communication_demo_timer_mock::m_pthread_mock = NULL;
    }

    if (!success)
    {
        // Exit with different code, if verification fails
        _exit(2);
    }
}
