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

#include "pconfig.h"
#include "opcua_shared.h"
#include "pdatasource_mock.hpp"
#include "libc_pthread_mock.hpp"

#include <tuple>
#include <sys/param.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "compute.c"
} // extern "C" {

#define TEST_THREAD_ID 1234
#define TEST_START_TIME 250000
#define TEST_PACKET_TYPE TALKER
#define TEST_PROP_LATENCY_US 1200
#define TEST_PROP_LATENCY (TEST_PROP_LATENCY_US*USEC2NANOSEC_FACTOR)

#define TEST_NO_NUMBER_OF_ITERATIONS 0
#define TEST_SMALL_NUMBER_OF_ITERATIONS 2000
#define TEST_LARGE_NUMBER_OF_ITERATIONS 100000
#define TEST_NUMBER_OF_IDLE_ITERATIONS 1000
#define TEST_DEFAULT_NUMBER_OF_ITERATIONS TEST_SMALL_NUMBER_OF_ITERATIONS
#define TEST_MAX_NUMBER_OF_ITERATIONS TEST_LARGE_NUMBER_OF_ITERATIONS
#define TEST_HUGE_NUMBER_OF_ITERATIONS (TEST_LARGE_NUMBER_OF_ITERATIONS + 10)

using namespace testing;

namespace {

typedef struct _PrivTimerItem
{
    int affinity;
    uint32_t period;
    int32_t startTime;
    ptimer_handler pTimerHandler;
    pfini_handler pFiniHandler;
    bool* pStopVariable;
} PrivTimerItem;

const uint64_t gTestPeriod = UINT64_C(500000);
const uint32_t gTestSeqNumberForIdleState = 1234;
const int64_t gTestBaseTime = INT64_C(1000000000000);
const int64_t gTestIdleStateTime = INT64_C(700000000000);
const uint32_t gTestInitialSequenceNumber = UINT32_C(1);

class test_tcc_rt_communication_demo_with_shm_setup : public Test {
protected:
    void SetUp() override {
        // Initialize the local state
        m_time_now = gTestBaseTime + TEST_START_TIME;
        sequence_number = gTestInitialSequenceNumber;
        memset(&genericPayload1, 0, sizeof (genericPayload1));

        // Setup the default configuration
        m_config.iterations = TEST_DEFAULT_NUMBER_OF_ITERATIONS;
        m_config.packetType = TALKER;
        m_config.period = gTestPeriod;
        m_config.startTime = TEST_START_TIME;

        // Init TSN Compute private state
        gPrevSeq = -1;
        gStarted = false;
        gInitSeqLatched = false;
        TheTimestampsCount = 0;
        TheLatestSequenceNumber = 0;

        m_config.incomingDataSource = &genericPayload1;
        gConfig = &m_config;
        m_config.pStopVariable = &m_stop_req;
        m_stop_req = false;
    }
    void TearDown() override {
    }
    void NextTimestamp() {
        m_time_now += gTestPeriod;
    }

protected:
    bool m_stop_req;
    PConfig m_config;
    int64_t m_time_now;
    int32_t sequence_number;
    StrictMock<pthread_mock> m_pthread_mock;
    StrictMock<pdatasource_mock> m_pdatasource_mock;
    // Emulated read shared memory block
    GenericPayload genericPayload1;
};

class test_tcc_rt_communication_demo_stats_config :
    public test_tcc_rt_communication_demo_with_shm_setup,
    public WithParamInterface<std::tuple<bool, int> > {
protected:
    void SetUp() override {
        // First, let the base class initialize its state
        test_tcc_rt_communication_demo_with_shm_setup::SetUp();

        const int itemsCount = std::get<1>(GetParam());
        m_config.verbose = std::get<0>(GetParam());
        m_config.iterations = itemsCount;

        PopulateStatisticsTables(itemsCount);
    }
    void TearDown() override {

        // Finally, let the base class deinitialize its state
        test_tcc_rt_communication_demo_with_shm_setup::TearDown();
    }
    const char *GetExpectedStderr() {
        const int itemsCount = std::get<1>(GetParam());
        if (!itemsCount) {
            return "No samples collected\n";
        } else {
            return "";
        }
    }
    const char *GetExpectedStdout() {
        const bool verbose = std::get<0>(GetParam());
        const int itemsCount = std::get<1>(GetParam());
        if (TEST_NO_NUMBER_OF_ITERATIONS == itemsCount) {
            // Empty string is expected for the case when we have no recorded statistics items
            return "";
        }

        if (verbose) {
            if (TEST_SMALL_NUMBER_OF_ITERATIONS == itemsCount) {
                return
                    "Number of samples: 2000, latest sequence number: 2000\n"
                    "Phase info: average: 250000, min: 250000, max: 250000\n"
                    "Delay average: 1200000, min: 1200000, max: 1200000\n";
            } else {
                // Case for TEST_LARGE_NUMBER_OF_ITERATIONS
                return
                    "Number of samples: 100000, latest sequence number: 100000\n"
                    "Phase info: average: 250000, min: 250000, max: 250000\n"
                    "Delay average: 1200000, min: 1200000, max: 1200000\n";
            }
        } else {
            // 'TEST_SMALL_NUMBER_OF_ITERATIONS' or 'TEST_LARGE_NUMBER_OF_ITERATIONS'
            return "Delay average: 1200000, min: 1200000, max: 1200000\n";
        }
    }

private:
    void PopulateStatisticsTables(int itemsCount) {
        if (itemsCount) {
            EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
                .Times(MIN(itemsCount, TEST_LARGE_NUMBER_OF_ITERATIONS))
                .WillRepeatedly(Return());
        }
        int i;
        // First, emulate idle stage, when we have initial sequence number
        // Update the input (read) shared memory block
        genericPayload1.sequence_num = gTestSeqNumberForIdleState;
        genericPayload1.type = TALKER;
        genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
        genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
        for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
            timer_handler(m_time_now);
            NextTimestamp();
        }
        // No update in collected data is expected, assert it
        ASSERT_EQ(TheTimestampsCount, 0u);

        // Now, run the active stage with updating instances
        sequence_number = gTestInitialSequenceNumber;
        for (i = 0; i < itemsCount; ++i) {
            genericPayload1.sequence_num = sequence_number++;
            genericPayload1.type = TALKER;
            genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
            genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;

            // Invoke the timer handler
            timer_handler(m_time_now);
            NextTimestamp();
        }
    }
};

INSTANTIATE_TEST_CASE_P(
    tcc_rt_communication_demo_stats_config,
    test_tcc_rt_communication_demo_stats_config,
    ::testing::Values(
        std::make_tuple(false, TEST_NO_NUMBER_OF_ITERATIONS),
        std::make_tuple(false, TEST_SMALL_NUMBER_OF_ITERATIONS),
        std::make_tuple(false, TEST_LARGE_NUMBER_OF_ITERATIONS),
        std::make_tuple(true, TEST_NO_NUMBER_OF_ITERATIONS),
        std::make_tuple(true, TEST_SMALL_NUMBER_OF_ITERATIONS),
        std::make_tuple(true, TEST_LARGE_NUMBER_OF_ITERATIONS)
    )
);

TEST_F(test_tcc_rt_communication_demo_with_shm_setup, tsn_compute_collect_in_idle_state_positive)
{
    // Setup the input SHM buffer
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;

    // Now, run the idle iterations
    for (int i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }

    // Timer still should be active
    EXPECT_FALSE(m_stop_req);
    // Check: no update in collected data is expected
    EXPECT_EQ(TheTimestampsCount, 0u);
    // The number of iterations should not be updated in the idle stage
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS));
}

TEST_F(test_tcc_rt_communication_demo_with_shm_setup,
       tsn_compute_collect_first_sample_after_idle_state_positive)
{
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(1)
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    // Setup the input SHM buffer for idle stage
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
    // Now, run the idle iterations
    for (int i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }
    // Prepare the first non-idle sample
    genericPayload1.sequence_num = sequence_number;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;

    // The test step
    timer_handler(m_time_now);

    EXPECT_NE(storedArgument, (const void *)NULL);
    const GenericPayload *const pStoredDatasourcePayload =
        static_cast<const GenericPayload *>(storedArgument);
    // Timer still should be active
    EXPECT_FALSE(m_stop_req);
    // Check: no update in collected data is expected, assert it
    EXPECT_EQ(TheTimestampsCount, 1u);
    // The output buffer should not be updated
    EXPECT_EQ(pStoredDatasourcePayload->sequence_num, sequence_number);
    EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
              (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
              (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec, m_time_now / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec, m_time_now % UNIT2NANO_FACTOR);
    // The number of iterations should be updated by one (we test first iteration)
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS - 1));
}

TEST_F(test_tcc_rt_communication_demo_with_shm_setup, tsn_compute_collect_samples_positive)
{
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(TEST_DEFAULT_NUMBER_OF_ITERATIONS)
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    // Setup the input SHM buffer for idle stage
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
    // Now, run the idle iterations
    for (int i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }

    // The iterative test steps
    for (int i = 0; i < TEST_DEFAULT_NUMBER_OF_ITERATIONS; ++i, NextTimestamp(), ++sequence_number) {
        // Update the input data for the test step, and invoke the timer handler
        genericPayload1.sequence_num = sequence_number;
        genericPayload1.type = TALKER;
        genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
        genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
        timer_handler(m_time_now);

        EXPECT_NE(storedArgument, (const void *)NULL);
        const GenericPayload *const pStoredDatasourcePayload =
            static_cast<const GenericPayload *>(storedArgument);

        // Check the number of collected statistics items
        EXPECT_EQ(TheTimestampsCount, static_cast<unsigned int>(i + 1)) << "Iteration: " << i;
        // Check the output SHM buffer
        EXPECT_EQ(pStoredDatasourcePayload->sequence_num, sequence_number) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
                  (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
                  (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec,
                  m_time_now / UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec,
                  m_time_now % UNIT2NANO_FACTOR) << "Iteration: " << i;
        // The number of iterations should be updated by one (we test first iteration)
        EXPECT_EQ(m_config.iterations,
                  static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS - 1 - i))
                      << "Iteration: " << i;
        // The timer should be active for all iterations except for the last one
        if (i != TEST_DEFAULT_NUMBER_OF_ITERATIONS - 1) {
            EXPECT_FALSE(m_stop_req) << "Iteration: " << i;
        } else {
            EXPECT_TRUE(m_stop_req) << "Iteration: " << i;
        }
    }
}

TEST_F(test_tcc_rt_communication_demo_with_shm_setup, tsn_compute_collect_huge_number_of_samples_positive)
{
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(MIN(TEST_HUGE_NUMBER_OF_ITERATIONS, TEST_LARGE_NUMBER_OF_ITERATIONS))
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    m_config.iterations = TEST_HUGE_NUMBER_OF_ITERATIONS;
    // Setup the input SHM buffer for idle stage
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
    // Now, run the idle iterations
    for (int i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }

    // The iterative test steps
    for (int i = 0; i < TEST_HUGE_NUMBER_OF_ITERATIONS && !m_stop_req;
         ++i, NextTimestamp(), ++sequence_number) {
        // Update the input data for the test step, and invoke the timer handler
        genericPayload1.sequence_num = sequence_number;
        genericPayload1.type = TALKER;
        genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
        genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
        timer_handler(m_time_now);

        EXPECT_NE(storedArgument, (const void *)NULL);
        const GenericPayload *const pStoredDatasourcePayload =
            static_cast<const GenericPayload *>(storedArgument);

        // Check the number of collected statistics items
        EXPECT_EQ(TheTimestampsCount, static_cast<unsigned int>(i + 1)) << "Iteration: " << i;
        // Check the output SHM buffer
        EXPECT_EQ(pStoredDatasourcePayload->sequence_num, sequence_number) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
                  (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
                  (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec,
                  m_time_now / UNIT2NANO_FACTOR) << "Iteration: " << i;
        EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec,
                  m_time_now % UNIT2NANO_FACTOR) << "Iteration: " << i;
        // The number of iterations should be updated by one (we test first iteration)
        EXPECT_EQ(m_config.iterations,
                  static_cast<unsigned int>(TEST_HUGE_NUMBER_OF_ITERATIONS - 1 - i))
                      << "Iteration: " << i;
        // The timer should be active for all iterations except for the last one
        if (i != TEST_MAX_NUMBER_OF_ITERATIONS - 1) {
            EXPECT_FALSE(m_stop_req) << "Iteration: " << i;
        } else {
            EXPECT_TRUE(m_stop_req) << "Iteration: " << i;
        }
    }
}

TEST_F(test_tcc_rt_communication_demo_with_shm_setup,
       tsn_compute_no_crash_after_huge_number_of_samples_negative)
{
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(MIN(TEST_HUGE_NUMBER_OF_ITERATIONS, TEST_LARGE_NUMBER_OF_ITERATIONS))
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    m_config.iterations = TEST_HUGE_NUMBER_OF_ITERATIONS;
    // Setup the input SHM buffer for idle stage
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
    // Now, run the idle iterations
    int i;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }

    // The iterative test steps, we should run out of statistics item,
    // and should stop iterations on stop timer request
    for (i = 0; i < TEST_HUGE_NUMBER_OF_ITERATIONS && !m_stop_req;
         ++i, NextTimestamp(), ++sequence_number) {
        // Update the input data for the test step, and invoke the timer handler
        genericPayload1.sequence_num = sequence_number;
        genericPayload1.type = TALKER;
        genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
        genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
        timer_handler(m_time_now);
    }
    ASSERT_TRUE(m_stop_req);
    ASSERT_EQ(i, TEST_MAX_NUMBER_OF_ITERATIONS);
    const int64_t last_time = m_time_now - gTestPeriod;

    // Now execute a few more timer handlers
    for (; i < TEST_HUGE_NUMBER_OF_ITERATIONS; ++i, NextTimestamp(), ++sequence_number) {
        // Update the input data for the test step, and invoke the timer handler
        genericPayload1.sequence_num = sequence_number;
        genericPayload1.type = TALKER;
        genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
        genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
        testing::internal::CaptureStdout();
        testing::internal::CaptureStderr();
        timer_handler(m_time_now);
        const std::string &stdout_string = testing::internal::GetCapturedStdout();
        const std::string &stderr_string = testing::internal::GetCapturedStderr();
        EXPECT_EQ(stdout_string, "");
        EXPECT_EQ(stderr_string, "No more statistics items\n");
    }

    // Check the results
    EXPECT_NE(storedArgument, (const void *)NULL);
        const GenericPayload *const pStoredDatasourcePayload =
            static_cast<const GenericPayload *>(storedArgument);
    // Check the number of collected statistics items
    EXPECT_EQ(TheTimestampsCount, static_cast<unsigned int>(TEST_MAX_NUMBER_OF_ITERATIONS));
    // Check the output SHM buffer
    EXPECT_EQ(pStoredDatasourcePayload->sequence_num, TEST_MAX_NUMBER_OF_ITERATIONS);
    EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
              (last_time - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
              (last_time - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec, last_time / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec, last_time % UNIT2NANO_FACTOR);
    EXPECT_EQ(m_config.iterations, 0u);
}

/*
 * This test case checks handling duplicate TALKER packets in the TSN Compute.
 * After packets for idle state, we send:
 * 1) The initial packet with a new sequence number;
 * 2) The second packet that is identical to the initial packet;
 * - This packet should not be appended to the statistics data, but the number of iterations
 *   should be decreased, as we handle a new timer event;
 * 3) The third packet with updated sequence number;
 * - This packet should be added to the statistics data;
 */
TEST_F(test_tcc_rt_communication_demo_with_shm_setup,
       tsn_compute_ptimer_handler_repeated_timer_events_positive)
{
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(2)
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    // Setup the input SHM buffer for idle stage
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
    // Now, run the idle iterations
    for (int i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }
    // Prepare and execute the first non-idle sample
    genericPayload1.sequence_num = sequence_number;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Now, prepare and execute the 2nd non-idle sample, identical to the 1st sample
    NextTimestamp();
    timer_handler(m_time_now);

    // Check the results of handling of the 2nd sample, identical to the 1st one
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(TheTimestampsCount, 1u);
    // The output buffer should contain the response to the first sample
    EXPECT_NE(storedArgument, (const void *)NULL);
    const GenericPayload *pStoredDatasourcePayload =
        static_cast<const GenericPayload *>(storedArgument);
    EXPECT_EQ(pStoredDatasourcePayload->sequence_num, sequence_number);
    EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY - gTestPeriod) / UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY - gTestPeriod) % UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec,
              (m_time_now - gTestPeriod) / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec,
              (m_time_now - gTestPeriod) % UNIT2NANO_FACTOR);
    // The number of iterations should be updated by two (handled 2 identical samples)
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS - 2));

    // Prepare the 3rd sample with update incoming data
    NextTimestamp();
    ++sequence_number;
    ++sequence_number;
    genericPayload1.sequence_num = sequence_number;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);

    // Check the results of handling of the 3rd sample
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(TheTimestampsCount, 2u);
    EXPECT_NE(storedArgument, (const void *)NULL);
    pStoredDatasourcePayload = static_cast<const GenericPayload *>(storedArgument);
    // The output buffer should contain the response to the first sample
    EXPECT_EQ(pStoredDatasourcePayload->sequence_num, sequence_number);
    EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec, m_time_now / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec, m_time_now % UNIT2NANO_FACTOR);
    // The number of iterations should be updated by two (handled 2 identical samples)
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS - 3));
}

/*
 * This test case checks handling COMPUTE packets in
 * the TSN Compute configured to listen to TALKER packets.
 * After packets for idle state, we send:
 * 1) The initial TALKER packet with a new sequence number;
 * 2) The COMPUTE packet;
 * - This packet should not be appended to the statistics data, but the number of iterations
 *   should be decreased, as we handled a new timer event;
 * 3) The third TALKER packet with updated sequence number;
 * - This packet should be added to the statistics data;
 */
TEST_F(test_tcc_rt_communication_demo_with_shm_setup,
       tsn_compute_ptimer_handler_handle_alien_compute_packet_negative)
{
    void *storedArgument = NULL;
    EXPECT_CALL(m_pdatasource_mock, put_value_in_datasource(_))
        .Times(2)
        .WillRepeatedly(DoAll(SaveArg<0>(&storedArgument), Return()));

    // Setup the input SHM buffer for idle stage
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = gTestIdleStateTime / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = gTestIdleStateTime % UNIT2NANO_FACTOR;
    // Now, run the idle iterations
    for (int i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        NextTimestamp();
    }
    // Prepare and execute the first non-idle sample
    genericPayload1.sequence_num = sequence_number;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);

    // Now, prepare and execute the 2nd non-idle sample, with incorrect packet type (COMPUTE)
    ++sequence_number;
    genericPayload1.sequence_num = sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    NextTimestamp();

    // Execute the test step, check the console output
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    timer_handler(m_time_now);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "Incoming data wrong type: 0xeeff0011\n");

    // Check the results of handling of the 2nd incorrect sample (COMPUTE)
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(TheTimestampsCount, 1u);
    // The output buffer should contain the response to the first sample
    EXPECT_NE(storedArgument, (const void *)NULL);
    const GenericPayload *pStoredDatasourcePayload =
        static_cast<const GenericPayload *>(storedArgument);
    EXPECT_EQ(pStoredDatasourcePayload->sequence_num, 1);
    EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY - gTestPeriod) / UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY - gTestPeriod) % UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec,
              (m_time_now - gTestPeriod) / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec,
              (m_time_now - gTestPeriod) % UNIT2NANO_FACTOR);
    // The number of iterations should be updated by two (handled 2 identical samples)
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS - 2));

    // Prepare the 3rd sample with update correct incoming data
    NextTimestamp();
    ++sequence_number;
    genericPayload1.sequence_num = sequence_number;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);

    // Check the results of handling of the 3rd sample
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(TheTimestampsCount, 2u);
    // The output buffer should contain the response to the first sample
    EXPECT_NE(storedArgument, (const void *)NULL);
    pStoredDatasourcePayload = static_cast<const GenericPayload *>(storedArgument);
    EXPECT_EQ(pStoredDatasourcePayload->sequence_num, sequence_number);
    EXPECT_EQ(pStoredDatasourcePayload->type, COMPUTE);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.lost_packets_num, 0);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_sec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.prev_nsec,
              static_cast<int>((m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR));
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_sec, m_time_now / UNIT2NANO_FACTOR);
    EXPECT_EQ(pStoredDatasourcePayload->load.compute.out_nsec, m_time_now % UNIT2NANO_FACTOR);
    // The number of iterations should be updated by two (handled 2 identical samples)
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_DEFAULT_NUMBER_OF_ITERATIONS - 3));
}

TEST_F(test_tcc_rt_communication_demo_with_shm_setup, talker_create_compute_positive)
{
    void *value = NULL;
    pthread_t thread_id = TEST_THREAD_ID;
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), SaveArg<3>(&value), Return(0)));

    EXPECT_CALL(m_pthread_mock, pthread_detach(TEST_THREAD_ID))
        .WillOnce(Return(0));

    create_compute(&m_config);

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

TEST_P(test_tcc_rt_communication_demo_stats_config, tsn_compute_show_stats_positive)
{
    // Check 'show_stats' with enabled verbose mode
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();

    EXPECT_EQ(stdout_string, GetExpectedStdout());
    EXPECT_EQ(stderr_string, GetExpectedStderr());
}

} // namespace {
