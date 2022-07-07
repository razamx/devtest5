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

#include "libc_pthread_mock.hpp"
#include "pdatasource_mock.hpp"

extern "C" {
#include "talker.c"

typedef struct _PrivTimerItem
{
    int affinity;
    uint32_t period;
    int32_t startTime;
    ptimer_handler pTimerHandler;
    pfini_handler pFiniHandler;
    bool* pStopVariable;
} PrivTimerItem;
}

#include <sys/param.h>

namespace {

using namespace testing;

#define TEST_THREAD_ID 1234
#define TEST_START_TIME 120000
const int gTestNoIterations = 0;
const int gTestSmallNumberOfIterations = 1000;
const int gTestMaxNumberOfIterations = 100000;
const int gTestLargeNumberOfIterations = 100010;

const uint64_t gTestPeriod = UINT64_C(500000);
const int64_t gTestBaseTime = INT64_C(1000000000000);
const uint32_t gTestInitialSequenceNumber = UINT32_C(1);

/**
 * This class aggregates UTs for check TCC TSN E2E sample, talker application
 */
class test_tcc_rt_communication_demo_talker_config : public Test
{
protected:
    StrictMock<pthread_mock> m_pthread_mock;
    StrictMock<pdatasource_mock> m_pdatasource_mock;

    // Sets up the test fixture.
    void SetUp() override {
        // Define the emulated timestamp
        m_time_now = gTestBaseTime + TEST_START_TIME;
        // Reset the number of collected samples in the TSN Talker to zero
        TheTimestampsCount = 0;
        m_config.verbose = false;
        m_config.startTime = TEST_START_TIME;
        m_config.period = gTestPeriod;
        m_config.iterations = gTestSmallNumberOfIterations;
        m_config.sequenceNumber = gTestInitialSequenceNumber;
        m_config.pStopVariable = &m_stop_req;
        m_stop_req = false;

        gConfig = &m_config;
    }

    // Tears down the test fixture.
    void TearDown() override {
    }

    void nextTimestamp() {
        m_time_now += m_config.period;
    }

protected:
    bool m_stop_req;
    PConfig m_config;
    int64_t m_time_now;
};

template<int SamplesCount>
class test_tcc_rt_communication_demo_fixed_samples_talker_config :
      public test_tcc_rt_communication_demo_talker_config
{
protected:
    // Sets up the test fixture.
    void SetUp() override {
        if (SamplesCount) {
            EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
                .Times(MIN(SamplesCount, gTestMaxNumberOfIterations))
                .WillRepeatedly(Return());
        }

        // Setup the base class
        test_tcc_rt_communication_demo_talker_config::SetUp();
        m_config.iterations = SamplesCount;

        // Emulate the requested number of iterations
        for (int i = 0; i < SamplesCount; ++i) {
            timer_handler(m_time_now);
            nextTimestamp();
        }
    }

    // Tears down the test fixture.
    void TearDown() override {
        // Tear down the base class
        test_tcc_rt_communication_demo_talker_config::TearDown();
    }
};

typedef test_tcc_rt_communication_demo_fixed_samples_talker_config<gTestNoIterations>
        test_tcc_rt_communication_demo_no_samples_talker_config;
typedef test_tcc_rt_communication_demo_fixed_samples_talker_config<gTestSmallNumberOfIterations>
        test_tcc_rt_communication_demo_small_samples_talker_config;
typedef test_tcc_rt_communication_demo_fixed_samples_talker_config<gTestLargeNumberOfIterations>
        test_tcc_rt_communication_demo_large_samples_talker_config;

TEST_F(test_tcc_rt_communication_demo_talker_config, tsn_talker_ptimer_handler_small_timer_events_positive)
{
    // Test case configuration constants
    const int TEST_ITERATIONS_COUNT = gTestSmallNumberOfIterations;

    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(TEST_ITERATIONS_COUNT)
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    // First, test the initial invokation of the 'timer_handler' in the TSN Talker application
    timer_handler(m_time_now);

    // The initial invokation of the 'timer_hanlder' should not stop the ptimer
    EXPECT_FALSE(m_stop_req);
    // Check that the 'timer_handler' records the test event
    EXPECT_EQ(TheTimestampsCount, 1);
    EXPECT_EQ(TheTimestamps[0], static_cast<uint64_t>(m_time_now));
    // Check the configuration state update in the initial call to 'timer_handler'
    EXPECT_EQ(m_config.iterations, static_cast<unsigned int>(TEST_ITERATIONS_COUNT - 1));
    EXPECT_EQ(m_config.sequenceNumber, gTestInitialSequenceNumber + 1);
    // Check the published values in the datasource
    EXPECT_NE(storedArgument, (const void *)NULL);
    const GenericPayload *const pPayload = static_cast<const GenericPayload *>(storedArgument);
    EXPECT_EQ(pPayload->type, TALKER);
    EXPECT_EQ(pPayload->sequence_num, static_cast<int>(gTestInitialSequenceNumber));
    EXPECT_EQ(pPayload->load.talker.sec, m_time_now / UNIT2NANO_FACTOR);
    EXPECT_EQ(pPayload->load.talker.nsec, m_time_now % UNIT2NANO_FACTOR);
    // Now, test all the other invokations of the 'timer_handler' in the TSN Talker application
    // to complete the remaining 'TEST_ITERATIONS_COUNT' number of iterations.
    // We already checked the initial invokation, we still have 'TEST_ITERATIONS_COUNT - 1'
    // iterations to complete.
    for (int i = 1; i < TEST_ITERATIONS_COUNT; ++i) {
        // Update the timestmp to refer to the next timer event
        nextTimestamp();
        timer_handler(m_time_now);
        // Check if the new timer event is recorded in the TSN Talker statistics buffer
        EXPECT_EQ(TheTimestampsCount, i + 1) << "Iteration: " << i;
        EXPECT_EQ(TheTimestamps[i],  static_cast<uint64_t>(m_time_now)) << "Iteration: " << i;
        // Check that the ptimer run-request is not
        // reset while the number of iterations is positive
        EXPECT_FALSE(m_stop_req) << "Iteration:" << i;
        // Check the configuration state update in the current call to 'timer_handler'
        EXPECT_EQ(m_config.iterations, static_cast<uint32_t>(TEST_ITERATIONS_COUNT - i - 1)) << "Iteration: " << i;
        EXPECT_EQ(m_config.sequenceNumber, uint32_t(i + 2)) << "Iteration:" << i;
        // Check the published values in the datasource
        EXPECT_EQ(pPayload->type, TALKER) << "Iteration: " << i;
        EXPECT_EQ(pPayload->sequence_num, i + 1) << "Iteration: " << i;
        EXPECT_EQ(pPayload->load.talker.sec, m_time_now / UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pPayload->load.talker.nsec, m_time_now % UNIT2NANO_FACTOR) << "Iteration: " << i;
    }
    // Check the configuration/state after the expected number of ptimer events
    EXPECT_EQ(m_config.iterations, 0u);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(TheTimestampsCount, TEST_ITERATIONS_COUNT);

    // Now, test the 'timer_handler' for the ptimer event beyong the expected number of iterations
    // Update the timestmp to refer to the next timer event
    nextTimestamp();
    timer_handler(m_time_now);
    // The number of collected samples should not be updated now
    EXPECT_EQ(TheTimestampsCount, TEST_ITERATIONS_COUNT);
    // The ptimer run-request shoudl be reset now
    EXPECT_TRUE(m_stop_req);
    // Check that the SHM write block still stores the previous values,
    // it is not updated by the last call to 'timer_handler'
    EXPECT_EQ(pPayload->type, TALKER);
    EXPECT_EQ(pPayload->sequence_num, static_cast<int>(TEST_ITERATIONS_COUNT));
    EXPECT_EQ(pPayload->load.talker.sec, static_cast<int>((m_time_now - gTestPeriod) / UNIT2NANO_FACTOR));
    EXPECT_EQ(pPayload->load.talker.nsec, static_cast<int>((m_time_now - gTestPeriod) % UNIT2NANO_FACTOR));
}

TEST_F(test_tcc_rt_communication_demo_talker_config, talker_stop_on_too_many_samples_negative)
{
    TheTimestampsCount = TIMESTAMPS_COUNT;

    // First, test the initial invokation of the 'timer_handler' in the TSN Talker application
    timer_handler(m_time_now);

    EXPECT_TRUE(m_stop_req);
}

TEST_F(test_tcc_rt_communication_demo_talker_config, talker_stop_on_no_more_iterations_negative)
{
    m_config.iterations = 0;

    // First, test the initial invokation of the 'timer_handler' in the TSN Talker application
    timer_handler(m_time_now);

    EXPECT_TRUE(m_stop_req);
}

TEST_F(test_tcc_rt_communication_demo_talker_config,
       tsn_talker_ptimer_handler_many_timer_events_negative)
{
    // Test case setup
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(gTestMaxNumberOfIterations)
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));
    m_config.iterations = gTestLargeNumberOfIterations;

    // Check publishing too many samples (more than statistics buffer in tsn-talker)
    for (int i = 0; i < gTestLargeNumberOfIterations; ++i) {
        timer_handler(m_time_now);
        nextTimestamp();
    }
    // The number of collected samples should not be updated now
    EXPECT_EQ(TheTimestampsCount, gTestMaxNumberOfIterations);
    // The ptimer run-request should be reset now
    EXPECT_TRUE(m_stop_req);
    // Check that the SHM write block still stores the previous values,
    // it is not updated by the last call to 'timer_handler'
    const GenericPayload *const pPayload = static_cast<const GenericPayload *>(storedArgument);
    EXPECT_EQ(pPayload->type, TALKER);
    EXPECT_EQ(pPayload->sequence_num, gTestMaxNumberOfIterations);
    const int64_t last_expected_time = m_time_now -
        (gTestLargeNumberOfIterations - gTestMaxNumberOfIterations + 1)*gTestPeriod;
    EXPECT_EQ(pPayload->load.talker.sec, last_expected_time / UNIT2NANO_FACTOR);
    EXPECT_EQ(pPayload->load.talker.nsec, last_expected_time % UNIT2NANO_FACTOR);
}

TEST_F(test_tcc_rt_communication_demo_talker_config, talker_create_producer_positive)
{
    void *value = NULL;
    pthread_t thread_id = TEST_THREAD_ID;
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), SaveArg<3>(&value), Return(0)));

    EXPECT_CALL(m_pthread_mock, pthread_detach(TEST_THREAD_ID))
        .WillOnce(Return(0));

    create_producer(&m_config);

    ASSERT_EQ(&m_config, gConfig);

    PrivTimerItem *const pItem = reinterpret_cast<PrivTimerItem *>(value);
    ASSERT_EQ(pItem->period, m_config.period);
    ASSERT_NE(pItem->pStopVariable, (const void *)NULL);
    ASSERT_NE(pItem->pStopVariable, &m_stop_req);
    ASSERT_EQ(pItem->pTimerHandler, timer_handler);
    ASSERT_EQ(pItem->pFiniHandler, show_stats);
    ASSERT_EQ(pItem->startTime, m_config.startTime);

    ASSERT_NE(value, (const void *)NULL);
    free(value);
}

TEST_F(test_tcc_rt_communication_demo_no_samples_talker_config,
       tsn_talker_no_ptimer_events_show_stats_non_verbose_negative)
{
    // Check 'show_stats' with disabled verbose mode
    m_config.verbose = false;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_no_samples_talker_config,
       tsn_talker_no_ptimer_events_show_stats_verbose_negative)
{
    // Check 'show_stats' with enabled verbose mode
    m_config.verbose = true;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "No samples collected\n");
}

TEST_F(test_tcc_rt_communication_demo_small_samples_talker_config,
       tsn_talker_small_number_of_ptimer_events_show_stats_non_verbose_negative)
{
    // Check 'show_stats' with disabled verbose mode
    m_config.verbose = false;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_small_samples_talker_config,
       tsn_talker_small_number_of_ptimer_events_show_stats_verbose_negative)
{
    // Check 'show_stats' with enabled verbose mode
    m_config.verbose = true;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    const char *const expected_sting = "Phase info: Average: " stringify(TEST_START_TIME)
        ", min: " stringify(TEST_START_TIME) ", max: " stringify(TEST_START_TIME) "\n";
    EXPECT_EQ(stdout_string, expected_sting);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_large_samples_talker_config,
       tsn_talker_large_number_of_ptimer_events_show_stats_non_verbose_negative)
{
    // Check 'show_stats' with disabled verbose mode
    m_config.verbose = false;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_large_samples_talker_config,
       tsn_talker_large_number_of_ptimer_events_show_stats_verbose_negative)
{
    // Check 'show_stats' with enabled verbose mode
    m_config.verbose = true;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    const char *const expected_output = "Phase info: Average: " stringify(TEST_START_TIME)
        ", min: " stringify(TEST_START_TIME) ", max: " stringify(TEST_START_TIME) "\n";
    EXPECT_EQ(stdout_string, expected_output);
    EXPECT_EQ(stderr_string, "");
}

} // namespace {
