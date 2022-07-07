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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "listener.c"
} // extern "C" {

#include "libc_pthread_mock.hpp"

#define TEST_THREAD_ID 1234
#define TEST_START_TIME 250000
#define TEST_PACKET_TYPE TALKER
#define TEST_PROP_LATENCY_US 1200
#define TEST_PROP_LATENCY (TEST_PROP_LATENCY_US*USEC2NANOSEC_FACTOR)

#define TEST_NO_NUMBER_OF_ITERATIONS 0
#define TEST_SMALL_NUMBER_OF_ITERATIONS 100
#define TEST_LARGE_NUMBER_OF_ITERATIONS 2000
#define TEST_NUMBER_OF_IDLE_ITERATIONS 1000

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
const uint32_t gTestInitialSequenceNumber = UINT32_C(1);

const char *const gExpectedOutputNoSamples =
    "\n*** Statistics for packets transmission ****************\n"
    "    No samples collected\n"
    "********************************************************\n\n";
const char *const gExpectedOutputSmallNumberOfSamples =
    "\n*** Statistics for packets transmission ****************\n"
    "    Received " stringify(TEST_SMALL_NUMBER_OF_ITERATIONS) " packets, "
    "the last sequence number: " stringify(TEST_SMALL_NUMBER_OF_ITERATIONS) "\n"
    "    Minimum packet latency: " stringify(TEST_PROP_LATENCY_US) " us\n"
    "    Maximum packet latency: " stringify(TEST_PROP_LATENCY_US) " us\n"
    "    Average packet latency: " stringify(TEST_PROP_LATENCY_US) " us\n"
    "********************************************************\n\n";
const char *const gExpectedOutputLargeNumberOfSamples =
    "\n*** Statistics for packets transmission ****************\n"
    "    Received " stringify(TEST_LARGE_NUMBER_OF_ITERATIONS) " packets, "
    "the last sequence number: " stringify(TEST_LARGE_NUMBER_OF_ITERATIONS) "\n"
    "    Minimum packet latency: " stringify(TEST_PROP_LATENCY_US) " us\n"
    "    Maximum packet latency: " stringify(TEST_PROP_LATENCY_US) " us\n"
    "    Average packet latency: " stringify(TEST_PROP_LATENCY_US) " us\n"
    "********************************************************\n\n";

/**
 * This class aggregates UTs for check TCC TSN E2E sample, listener application
 */
class test_tcc_rt_communication_demo_listener_config : public testing::Test
{
protected:
    // Sets up the test fixture.
    void SetUp() override {
        // Define the emulated timestamp
        m_time_now = gTestBaseTime + TEST_START_TIME;
        // Reset the number of collected samples in the TSN Listener to zero
        TheStatsCount = 0;
        TheLastSequence = 0;
        // Use verbose output on default
        m_config.verbose = true;
        // Emulate listening to TALKER packets
        m_config.packetType = TEST_PACKET_TYPE;
        // Define the usual test period for the test cases
        m_config.period = gTestPeriod;
        // Update to use test start time (within the test period time frame)
        m_config.startTime = TEST_START_TIME;
        // Update the initial sequence number
        m_config.sequenceNumber = gTestInitialSequenceNumber;
        // Do not use the log file on default
        m_config.pLogFile = NULL;

        // Reset the private state in tsn-listener
        gPrevSeq = -1;
        gStarted = false;
        gInitSeqLatched = false;

        m_config.incomingDataSource = &genericPayload1;
        gConfig = &m_config;
        m_config.pStopVariable = &m_stop_req;
        m_stop_req = false;
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
    GenericPayload genericPayload1;
    StrictMock<pthread_mock> m_pthread_mock;
};

template<int SamplesCount>
class test_tcc_rt_communication_demo_fixed_samples_listener_config :
      public test_tcc_rt_communication_demo_listener_config
{
protected:
    // Sets up the test fixture.
    void SetUp() override {
        // Setup the base class
        test_tcc_rt_communication_demo_listener_config::SetUp();
        m_config.iterations = SamplesCount;

        // Emulate idle state first
        int i;
        memset(&genericPayload1, 0, sizeof (genericPayload1));
        genericPayload1.type = TEST_PACKET_TYPE;
        genericPayload1.sequence_num = gTestSeqNumberForIdleState;
        for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
            timer_handler(m_time_now);
            nextTimestamp();
            // Keep the Read SHM state in the idle state
        }
        ASSERT_EQ(TheStatsCount, 0u);
        ASSERT_EQ(genericPayload1.type, static_cast<unsigned int>(TEST_PACKET_TYPE));
        ASSERT_EQ(genericPayload1.sequence_num, static_cast<int>(gTestSeqNumberForIdleState));

        // Active iterations go now
        uint32_t test_sequence_number = m_config.sequenceNumber;
        for (i = 0; i < SamplesCount && !m_stop_req; ++i, ++test_sequence_number, nextTimestamp()) {
            // Prepare the new sample data in the mocked shared memory
            genericPayload1.sequence_num = test_sequence_number;
            genericPayload1.type = TEST_PACKET_TYPE;
            genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
            genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;

            timer_handler(m_time_now);
        }
        ASSERT_EQ(TheStatsCount, static_cast<unsigned int>(SamplesCount));
    }

    // Tears down the test fixture.
    void TearDown() override {
        // Tear down the base class
        test_tcc_rt_communication_demo_listener_config::TearDown();
    }
};

typedef test_tcc_rt_communication_demo_fixed_samples_listener_config<TEST_NO_NUMBER_OF_ITERATIONS>
        test_tcc_rt_communication_demo_no_samples_listener_config;
typedef test_tcc_rt_communication_demo_fixed_samples_listener_config<TEST_SMALL_NUMBER_OF_ITERATIONS>
        test_tcc_rt_communication_demo_small_samples_listener_config;
typedef test_tcc_rt_communication_demo_fixed_samples_listener_config<TEST_LARGE_NUMBER_OF_ITERATIONS>
        test_tcc_rt_communication_demo_large_samples_listener_config;

TEST_F(test_tcc_rt_communication_demo_listener_config,
       tsn_listener_ptimer_handler_small_timer_talker_packets_positive)
{
    // Setup the test case
    m_config.iterations = TEST_LARGE_NUMBER_OF_ITERATIONS;

    // Emulate idle state first
    int i;
    memset(&genericPayload1, 0, sizeof (genericPayload1));
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);

        nextTimestamp();
        // Keep the Read SHM state in the idle state
    }
    // Check the TSN Listener, it should not collect any samples in idle state
    EXPECT_EQ(TheStatsCount, 0u);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS));
    EXPECT_EQ(genericPayload1.type, static_cast<unsigned int>(TEST_PACKET_TYPE));
    EXPECT_EQ(genericPayload1.sequence_num, static_cast<int>(gTestSeqNumberForIdleState));

    // Active iterations go now:
    // First, test the initial active (with an updated sequence number) invokation of
    // the 'timer_handler' in the TSN Listener application
    uint32_t test_sequence_number = gTestInitialSequenceNumber;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Check the state update after the first active iteration, one item should be recorded
    EXPECT_EQ(TheStatsCount, 1u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[0], static_cast<uint64_t>(m_time_now));
    EXPECT_EQ(TheStatsOrig[0], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 1));
    // Prepare the state for the next iteration
    nextTimestamp();
    ++test_sequence_number;

    // Now, test the other active (with updated sequence numbers)  invokation of
    // the 'timer_handler' in the TSN Listener application
    for (i = 1; i < TEST_LARGE_NUMBER_OF_ITERATIONS; ++i) {
        // Prepare the new sample data in the mocked shared memory
        genericPayload1.sequence_num = test_sequence_number;
        genericPayload1.type = TEST_PACKET_TYPE;
        genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
        genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;

        // Now, let the handler handle the timer event
        timer_handler(m_time_now);

        // Check the results (updates in the state of TSN Listener) of the handler
        EXPECT_EQ(TheStatsCount, static_cast<uint64_t>(i + 1)) << "Iteration: " << i;
        // Check the value of the first recorded statistics item
        EXPECT_EQ(TheStatsNow[i], static_cast<uint64_t>(m_time_now)) << "Iteration: " << i;
        EXPECT_EQ(TheStatsOrig[i],
                  static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY)) << "Iteration: " << i;
        EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber + i) << "Iteration: " << i;
        if (i != TEST_LARGE_NUMBER_OF_ITERATIONS - 1) {
            // This is not the last iteration
            EXPECT_FALSE(m_stop_req) << "Iteration: " << i;
        } else {
            // This is the last iteration
            EXPECT_TRUE(m_stop_req) << "Iteration: " << i;
        }

        // Prepare the state for the next iteration
        nextTimestamp();
        ++test_sequence_number;
    }
    // Check the state after all the iterations
    EXPECT_EQ(m_config.iterations, 0u);
    EXPECT_EQ(TheStatsCount, static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS));
    EXPECT_EQ(TheLastSequence, static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS));
}

/*
 * This test case checks handling duplicate TALKER packets in the TSN Listener.
 * After packets for idle state, we send:
 * 1) The initial packet with a new sequence number;
 * 2) The second packet that is identical to the initial packet;
 * - This packet should not be appended to the statistics data, but the number of iterations
 *   should be decreased, as we handle a new timer event;
 * 3) The third packet with updated sequence number;
 * - This packet should be added to the statistics data;
 */
TEST_F(test_tcc_rt_communication_demo_listener_config,
       tsn_listener_ptimer_handler_repeated_timer_events_positive)
{
    // Setup the test case
    m_config.iterations = TEST_LARGE_NUMBER_OF_ITERATIONS;

    // Emulate idle state first
    int i;
    memset(&genericPayload1, 0, sizeof (genericPayload1));
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        nextTimestamp();
        // Keep the Read SHM state in the idle state
    }
    // Check the TSN Listener, it should not collect any samples in idle state

    // Active iterations go now:
    // First, test the initial active (with an updated sequence number) invokation of
    // the 'timer_handler' in the TSN Listener application
    uint32_t test_sequence_number = gTestInitialSequenceNumber;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    ASSERT_EQ(TheStatsCount, 1u);
    ASSERT_EQ(TheStatsNow[0], static_cast<uint64_t>(m_time_now));
    ASSERT_EQ(TheStatsOrig[0], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    ASSERT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    ASSERT_FALSE(m_stop_req);
    ASSERT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 1));
    // Now, send the same package to the TSN Listener timer handler, with only timestamp updated
    nextTimestamp();
    timer_handler(m_time_now);
    // Check the state update after the repeated active iteration, no updated to the state
    // is expected, except for the number of iterations that should be decreased
    EXPECT_EQ(TheStatsCount, 1u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[0], m_time_now - gTestPeriod);
    EXPECT_EQ(TheStatsOrig[0], m_time_now - TEST_PROP_LATENCY - gTestPeriod);
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 2));

    // Prepare the state for the next iteration, update the sequence number now
    nextTimestamp();
    ++test_sequence_number;
    // Send the updated packet to the TSN Listener, check it is reflected in the statistics
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Check the state update now
    EXPECT_EQ(TheStatsCount, 2u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[1], static_cast<uint64_t>(m_time_now));
    EXPECT_EQ(TheStatsOrig[1], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber + 1);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 3));
}

/*
 * This test case checks handling COMPUTE packets in
 * the TSN Listener configured to listen to TALKER packets.
 * After packets for idle state, we send:
 * 1) The initial TALKER packet with a new sequence number;
 * 2) The COMPUTE packet;
 * - This packet should not be appended to the statistics data, but the number of iterations
 *   should be decreased, as we handled a new timer event;
 * 3) The third TALKER packet with updated sequence number;
 * - This packet should be added to the statistics data;
 */
TEST_F(test_tcc_rt_communication_demo_listener_config,
       tsn_listener_ptimer_handler_handle_alien_compute_packet_negative)
{
    // Setup the test case
    m_config.iterations = TEST_LARGE_NUMBER_OF_ITERATIONS;

    // Emulate idle state first
    int i;
    memset(&genericPayload1, 0, sizeof (genericPayload1));
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        nextTimestamp();
        // Keep the Read SHM state in the idle state
    }
    // Check the TSN Listener, it should not collect any samples in idle state

    // Active iterations go now:
    // First, test the initial active (with an updated sequence number) invokation of
    // the 'timer_handler' in the TSN Listener application
    uint32_t test_sequence_number = gTestInitialSequenceNumber;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    ASSERT_EQ(TheStatsCount, 1u);
    ASSERT_EQ(TheStatsNow[0], static_cast<uint64_t>(m_time_now));
    ASSERT_EQ(TheStatsOrig[0], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    ASSERT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    ASSERT_FALSE(m_stop_req);
    ASSERT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 1));
    // Now, send the same package to the TSN Listener timer handler, with only timestamp updated
    nextTimestamp();
    ++test_sequence_number;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.compute.prev_sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.compute.prev_nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    timer_handler(m_time_now);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "Wrong packet type: 0xeeff0011\n");
    // Check the state update after the repeated active iteration, no updated to the state
    // is expected, except for the number of iterations that should be decreased
    EXPECT_EQ(TheStatsCount, 1u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[0], m_time_now - gTestPeriod);
    EXPECT_EQ(TheStatsOrig[0], m_time_now - TEST_PROP_LATENCY - gTestPeriod);
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    ASSERT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 2));

    // Prepare the state for the next iteration, update the sequence number now
    nextTimestamp();
    ++test_sequence_number;
    // Send the updated packet to the TSN Listener, check it is reflected in the statistics
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = TEST_PACKET_TYPE;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Check the state update now
    EXPECT_EQ(TheStatsCount, 2u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[1], static_cast<uint64_t>(m_time_now));
    EXPECT_EQ(TheStatsOrig[1], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber + 2);
    ASSERT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 3));
}

TEST_F(test_tcc_rt_communication_demo_listener_config,
       tsn_listener_ptimer_handler_small_timer_compute_packets_positive)
{
    // Setup the test case
    m_config.packetType = COMPUTE;
    m_config.iterations = TEST_LARGE_NUMBER_OF_ITERATIONS;

    // Emulate idle state first
    int i;
    memset(&genericPayload1, 0, sizeof (genericPayload1));
    genericPayload1.type = COMPUTE;
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        nextTimestamp();
        // Keep the Read SHM state in the idle state
    }
    // Check the TSN Listener, it should not collect any samples in idle state
    EXPECT_EQ(TheStatsCount, 0u);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS));
    EXPECT_EQ(genericPayload1.type, static_cast<unsigned int>(COMPUTE));
    EXPECT_EQ(genericPayload1.sequence_num, static_cast<int>(gTestSeqNumberForIdleState));

    // Active iterations go now:
    // First, test the initial active (with an updated sequence number) invokation of
    // the 'timer_handler' in the TSN Listener application
    uint32_t test_sequence_number = gTestInitialSequenceNumber;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.compute.prev_sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.compute.prev_nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Check the state update after the first active iteration, one item should be recorded
    EXPECT_EQ(TheStatsCount, 1u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[0], static_cast<uint64_t>(m_time_now));
    EXPECT_EQ(TheStatsOrig[0], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 1));
    // Prepare the state for the next iteration
    nextTimestamp();
    ++test_sequence_number;

    // Now, test the other active (with updated sequence numbers)  invokation of
    // the 'timer_handler' in the TSN Listener application
    for (i = 1; i < TEST_LARGE_NUMBER_OF_ITERATIONS; ++i) {
        // Prepare the new sample data in the mocked shared memory
        genericPayload1.sequence_num = test_sequence_number;
        genericPayload1.type = COMPUTE;
        genericPayload1.load.compute.prev_sec =
            (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
        genericPayload1.load.compute.prev_nsec =
            (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;

        // Now, let the handler handle the timer event
        timer_handler(m_time_now);

        // Check the results (updates in the state of TSN Listener) of the handler
        EXPECT_EQ(TheStatsCount, static_cast<uint64_t>(i + 1)) << "Iteration: " << i;
        // Check the value of the first recorded statistics item
        EXPECT_EQ(TheStatsNow[i], static_cast<uint64_t>(m_time_now)) << "Iteration: " << i;
        EXPECT_EQ(TheStatsOrig[i],
                  static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY)) << "Iteration: " << i;
        EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber + i) << "Iteration: " << i;
        if (i != TEST_LARGE_NUMBER_OF_ITERATIONS - 1) {
            // This is not the last iteration
            EXPECT_FALSE(m_stop_req) << "Iteration: " << i;
        } else {
            // This is the last iteration
            EXPECT_TRUE(m_stop_req) << "Iteration: " << i;
        }

        // Prepare the state for the next iteration
        nextTimestamp();
        ++test_sequence_number;
    }
    // Check the state after all the iterations
    EXPECT_EQ(m_config.iterations, 0u);
    EXPECT_EQ(TheStatsCount, static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS));
    EXPECT_EQ(TheLastSequence, static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS));
}

/*
 * This test case checks handling duplicate TALKER packets in the TSN Listener.
 * After packets for idle state, we send:
 * 1) The initial packet with a new sequence number;
 * 2) The second packet that is identical to the initial packet;
 * - This packet should not be appended to the statistics data, but the number of iterations
 *   should be decreased, as we handle a new timer event;
 * 3) The third packet with updated sequence number;
 * - This packet should be added to the statistics data;
 */
TEST_F(test_tcc_rt_communication_demo_listener_config,
       tsn_listener_ptimer_handler_repeated_compute_packets_positive)
{
    // Update the expected packet type
    m_config.packetType = COMPUTE;
    // Setup the test case
    m_config.iterations = TEST_LARGE_NUMBER_OF_ITERATIONS;

    // Emulate idle state first
    int i;
    memset(&genericPayload1, 0, sizeof (genericPayload1));
    genericPayload1.type = COMPUTE;
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        nextTimestamp();
        // Keep the Read SHM state in the idle state
    }
    // Check the TSN Listener, it should not collect any samples in idle state

    // Active iterations go now:
    // First, test the initial active (with an updated sequence number) invokation of
    // the 'timer_handler' in the TSN Listener application
    uint32_t test_sequence_number = gTestInitialSequenceNumber;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.compute.prev_sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.compute.prev_nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    ASSERT_EQ(TheStatsCount, 1u);
    ASSERT_EQ(TheStatsNow[0], static_cast<uint64_t>(m_time_now));
    ASSERT_EQ(TheStatsOrig[0], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    ASSERT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    EXPECT_FALSE(m_stop_req);
    ASSERT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 1));
    // Now, send the same package to the TSN Listener timer handler, with only timestamp updated
    nextTimestamp();
    timer_handler(m_time_now);
    // Check the state update after the repeated active iteration, no updated to the state
    // is expected, except for the number of iterations that should be decreased
    EXPECT_EQ(TheStatsCount, 1u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[0], m_time_now - gTestPeriod);
    EXPECT_EQ(TheStatsOrig[0], m_time_now - TEST_PROP_LATENCY - gTestPeriod);
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 2));

    // Prepare the state for the next iteration, update the sequence number now
    nextTimestamp();
    ++test_sequence_number;
    // Send the updated packet to the TSN Listener, check it is reflected in the statistics
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.compute.prev_sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.compute.prev_nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Check the state update now
    EXPECT_EQ(TheStatsCount, 2u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[1], static_cast<uint64_t>(m_time_now));
    EXPECT_EQ(TheStatsOrig[1], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber + 1);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 3));
}

/*
 * This test case checks handling COMPUTE packets in
 * the TSN Listener configured to listen to TALKER packets.
 * After packets for idle state, we send:
 * 1) The initial TALKER packet with a new sequence number;
 * 2) The COMPUTE packet;
 * - This packet should not be appended to the statistics data, but the number of iterations
 *   should be decreased, as we handled a new timer event;
 * 3) The third TALKER packet with updated sequence number;
 * - This packet should be added to the statistics data;
 */
TEST_F(test_tcc_rt_communication_demo_listener_config,
       tsn_listener_ptimer_handler_handle_alien_talker_packet_negative)
{
    // Setup the test case
    m_config.iterations = TEST_LARGE_NUMBER_OF_ITERATIONS;
    // Update the expected packet type
    m_config.packetType = COMPUTE;

    // Emulate idle state first
    int i;
    memset(&genericPayload1, 0, sizeof (genericPayload1));
    genericPayload1.type = COMPUTE;
    genericPayload1.sequence_num = gTestSeqNumberForIdleState;
    for (i = 0; i < TEST_NUMBER_OF_IDLE_ITERATIONS; ++i) {
        timer_handler(m_time_now);
        nextTimestamp();
        // Keep the Read SHM state in the idle state
    }
    // Check the TSN Listener, it should not collect any samples in idle state

    // Active iterations go now:
    // First, test the initial active (with an updated sequence number) invokation of
    // the 'timer_handler' in the TSN Listener application
    uint32_t test_sequence_number = gTestInitialSequenceNumber;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.compute.prev_sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.compute.prev_nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    ASSERT_EQ(TheStatsCount, 1u);
    ASSERT_EQ(TheStatsNow[0], static_cast<uint64_t>(m_time_now));
    ASSERT_EQ(TheStatsOrig[0], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    ASSERT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    ASSERT_FALSE(m_stop_req);
    ASSERT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 1));
    // Now, send the same package to the TSN Listener timer handler, with only timestamp updated
    nextTimestamp();
    ++test_sequence_number;
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = TALKER;
    genericPayload1.load.talker.sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.talker.nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    timer_handler(m_time_now);
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, "");
    EXPECT_EQ(stderr_string, "Wrong packet type: 0xaabbccdd\n");
    // Check the state update after the repeated active iteration, no updated to the state
    // is expected, except for the number of iterations that should be decreased
    EXPECT_EQ(TheStatsCount, 1u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[0], m_time_now - gTestPeriod);
    EXPECT_EQ(TheStatsOrig[0], m_time_now - TEST_PROP_LATENCY - gTestPeriod);
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 2));

    // Prepare the state for the next iteration, update the sequence number now
    nextTimestamp();
    ++test_sequence_number;
    // Send the updated packet to the TSN Listener, check it is reflected in the statistics
    genericPayload1.sequence_num = test_sequence_number;
    genericPayload1.type = COMPUTE;
    genericPayload1.load.compute.prev_sec = (m_time_now - TEST_PROP_LATENCY) / UNIT2NANO_FACTOR;
    genericPayload1.load.compute.prev_nsec = (m_time_now - TEST_PROP_LATENCY) % UNIT2NANO_FACTOR;
    timer_handler(m_time_now);
    // Check the state update now
    EXPECT_EQ(TheStatsCount, 2u);
    // Check the value of the first recorded statistics item
    EXPECT_EQ(TheStatsNow[1], static_cast<uint64_t>(m_time_now));
    EXPECT_EQ(TheStatsOrig[1], static_cast<uint64_t>(m_time_now - TEST_PROP_LATENCY));
    EXPECT_EQ(TheLastSequence, gTestInitialSequenceNumber + 2);
    EXPECT_FALSE(m_stop_req);
    EXPECT_EQ(m_config.iterations,
              static_cast<unsigned int>(TEST_LARGE_NUMBER_OF_ITERATIONS - 3));
}

TEST_F(test_tcc_rt_communication_demo_listener_config, listener_create_consumer_positive)
{
    void *value = NULL;
    pthread_t thread_id = TEST_THREAD_ID;
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(
            DoAll(SetArrayArgument<0>(&thread_id, &thread_id + 1), SaveArg<3>(&value), Return(0)));

    EXPECT_CALL(m_pthread_mock, pthread_detach(TEST_THREAD_ID))
        .WillOnce(Return(0));

    create_consumer(&m_config);

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

TEST_F(test_tcc_rt_communication_demo_no_samples_listener_config,
       tsn_listener_show_stats_non_verbose_stats_no_samples_positive)
{
    // Check 'show_stats' with disabled verbose mode
    m_config.verbose = false;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, gExpectedOutputNoSamples);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_no_samples_listener_config,
       tsn_listener_no_ptimer_events_show_stats_verbose_negative)
{
    // Check 'show_stats' with enabled verbose mode
    m_config.verbose = true;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, gExpectedOutputNoSamples);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_small_samples_listener_config,
       tsn_listener_small_number_of_ptimer_events_show_stats_non_verbose_negative)
{
    // Check 'show_stats' with disabled verbose mode
    m_config.verbose = false;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, gExpectedOutputSmallNumberOfSamples);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_small_samples_listener_config,
       tsn_listener_small_number_of_ptimer_events_show_stats_verbose_negative)
{
    // Check 'show_stats' with enabled verbose mode
    m_config.verbose = true;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, gExpectedOutputSmallNumberOfSamples);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_large_samples_listener_config,
       tsn_listener_large_number_of_ptimer_events_show_stats_non_verbose_negative)
{
    // Check 'show_stats' with disabled verbose mode
    m_config.verbose = false;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, gExpectedOutputLargeNumberOfSamples);
    EXPECT_EQ(stderr_string, "");
}

TEST_F(test_tcc_rt_communication_demo_large_samples_listener_config,
       tsn_listener_large_number_of_ptimer_events_show_stats_verbose_negative)
{
    // Check 'show_stats' with enabled verbose mode
    m_config.verbose = true;
    testing::internal::CaptureStdout();
    testing::internal::CaptureStderr();
    show_stats();
    const std::string &stdout_string = testing::internal::GetCapturedStdout();
    const std::string &stderr_string = testing::internal::GetCapturedStderr();
    EXPECT_EQ(stdout_string, gExpectedOutputLargeNumberOfSamples);
    EXPECT_EQ(stderr_string, "");
}

} // namespace {
