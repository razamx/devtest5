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

#include "pointer_chasing.h"

static void pointer_chase_run_read_workload_hook();
#define pointer_chase_run_read_workload(a1, a2) \
    pointer_chase_run_read_workload(a1, a2); \
    pointer_chase_run_read_workload_hook();

extern "C" {
#include "pointer_chasing_workload.c"
}

#include "tcc_cache_mock.hpp"
#include "libc_pthread_mock.hpp"

#define RTC_TEST_AFFINITY (2)
#define RTC_TEST_LATENCY (200)

static int gIterationsLeft = 0;
namespace {

using namespace testing;

/**
 * This class aggregates simple UTs for check the Pointer Chasing workload in RTC Sample
 */
class test_tcc_rtc_pointer_chasing_simple : public Test
{
protected:
    // Sets up the test fixture.
    void SetUp() override {
        gWorkloadBuffer = NULL;
        gBackgroundStressReq = false;
    }

    // Tears down the test fixture.
    void TearDown() override {
    }

protected:
    StrictMock<pthread_mock> m_pthread_mock;
    StrictMock<tcc_cache_mock> m_tcc_cache_mock;
    cache_line_node_t m_test_nodes[WORKLOAD_BUFFER_SIZE / sizeof (cache_line_node_t)];
};

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_init_already_init_negative)
{
    gWorkloadBuffer = m_test_nodes;

    internal::CaptureStderr();
    const int res = pc_init(RTC_TEST_LATENCY, RTC_TEST_AFFINITY);
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -EBUSY);
    EXPECT_NE(capsured.find("Error: the Poitner Chasing support already initialized"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_init_tcc_init_failed_negative)
{
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(-TCC_E_NOT_AVAILABLE));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish());

    internal::CaptureStdout();
    const int res = pc_init(RTC_TEST_LATENCY, RTC_TEST_AFFINITY);
    const std::string &capsured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(res, -TCC_E_NOT_AVAILABLE);
    EXPECT_NE(capsured.find("Resource is not available or insufficient"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_init_allocation_failed_negative)
{
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(WORKLOAD_BUFFER_SIZE, RTC_TEST_LATENCY))
        .WillOnce(Return(nullptr));

    internal::CaptureStderr();
    const int res = pc_init(RTC_TEST_LATENCY, RTC_TEST_AFFINITY);
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -ENOMEM);
    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_NE(capsured.find("Error: cannot allocate buffer for the pointer chasing workload"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_init_pthread_create_failed_negative)
{
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(WORKLOAD_BUFFER_SIZE, RTC_TEST_LATENCY))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(&gStressThread, NULL, background_stress_thread, NULL))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));

    internal::CaptureStderr();
    const int res = pc_init(RTC_TEST_LATENCY, RTC_TEST_AFFINITY);
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, -EINVAL);
    EXPECT_FALSE(gBackgroundStressReq);
    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_NE(capsured.find("Error: cannot create the background stress thread"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_init_positive)
{
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(WORKLOAD_BUFFER_SIZE, RTC_TEST_LATENCY))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(&gStressThread, NULL, background_stress_thread, NULL))
        .WillOnce(Return(0));

    const int res = pc_init(RTC_TEST_LATENCY, RTC_TEST_AFFINITY);

    EXPECT_EQ(res, 0);
    EXPECT_TRUE(gBackgroundStressReq);
    EXPECT_EQ(gWorkloadBuffer, m_test_nodes);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_fini_pthread_join_failed_negative)
{
    gBackgroundStressReq = true;
    gWorkloadBuffer = m_test_nodes;
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(gStressThread, NULL))
        .WillOnce(Return(-EINVAL));

    internal::CaptureStderr();
    pc_fini();
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_FALSE(gBackgroundStressReq);
    EXPECT_NE(capsured.find("Warning: cannot join the stress thread: res:"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_fini_no_thread_no_pointer_positive)
{
    gBackgroundStressReq = false;
    gWorkloadBuffer = NULL;

    pc_fini();

    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_FALSE(gBackgroundStressReq);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_fini_no_thread_positive)
{
    gBackgroundStressReq = false;
    gWorkloadBuffer = m_test_nodes;
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));

    pc_fini();

    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_FALSE(gBackgroundStressReq);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_fini_no_pointer_positive)
{
    gBackgroundStressReq = true;
    gWorkloadBuffer = NULL;
    EXPECT_CALL(m_pthread_mock, pthread_join(gStressThread, NULL))
        .WillOnce(Return(0));

    pc_fini();

    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_FALSE(gBackgroundStressReq);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_fini_positive)
{
    gBackgroundStressReq = true;
    gWorkloadBuffer = m_test_nodes;
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(gStressThread, NULL))
        .WillOnce(Return(0));

    pc_fini();

    EXPECT_EQ(gWorkloadBuffer, nullptr);
    EXPECT_FALSE(gBackgroundStressReq);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_get_workload_buffer_non_null_with_size_positive)
{
    size_t size;
    cache_line_node_t *nodes;
    gWorkloadBuffer = m_test_nodes;

    nodes = pc_get_workload_buffer(&size);

    EXPECT_EQ(nodes, m_test_nodes);
    EXPECT_EQ(size, WORKLOAD_BUFFER_SIZE);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_get_workload_buffer_non_null_no_size_positive)
{
    cache_line_node_t *nodes;
    gWorkloadBuffer = m_test_nodes;

    nodes = pc_get_workload_buffer(NULL);

    EXPECT_EQ(nodes, m_test_nodes);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_get_workload_buffer_null_with_size_positive)
{
    size_t size;
    cache_line_node_t *nodes;

    nodes = pc_get_workload_buffer(&size);

    EXPECT_EQ(nodes, nullptr);
    EXPECT_EQ(size, 0);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, pc_get_workload_buffer_null_no_size_positive)
{
    cache_line_node_t *nodes;

    nodes = pc_get_workload_buffer(NULL);

    EXPECT_EQ(nodes, nullptr);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, background_stress_affinity_failed_positive)
{
    void *ret;
    gIterationsLeft = 10;
    gBackgroundStressReq = true;
    EXPECT_CALL(m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(m_pthread_mock, pthread_getschedparam(_, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_setschedparam(_, SCHED_OTHER, _))
        .WillOnce(Return(0));

    internal::CaptureStderr();
    ret = background_stress_thread(NULL);
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_NE(ret, nullptr);
    EXPECT_NE(capsured.find("Warning: stress thread: cannot set CPU affinity: res:"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, background_stress_getschedparam_failed_positive)
{
    void *ret;
    gIterationsLeft = 10;
    gBackgroundStressReq = true;
    EXPECT_CALL(m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_getschedparam(_, _, _))
        .WillOnce(Return(-EINVAL));

    internal::CaptureStderr();
    ret = background_stress_thread(NULL);
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_NE(ret, nullptr);
    EXPECT_NE(capsured.find("Warning: stress thread: cannot get thread params: res:"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, background_stress_setschedparam_failed_positive)
{
    void *ret;
    gIterationsLeft = 10;
    gBackgroundStressReq = true;
    EXPECT_CALL(m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_getschedparam(_, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_setschedparam(_, SCHED_OTHER, _))
        .WillOnce(Return(-EINVAL));

    internal::CaptureStderr();
    ret = background_stress_thread(NULL);
    const std::string &capsured = testing::internal::GetCapturedStderr();

    EXPECT_NE(ret, nullptr);
    EXPECT_NE(capsured.find("Warning: stress thread: cannot set thread params: res:"), std::string::npos);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, background_stress_thread_no_iterations_positive)
{
    void *ret;
    EXPECT_CALL(m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_getschedparam(_, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_setschedparam(_, SCHED_OTHER, _))
        .WillOnce(Return(0));

    ret = background_stress_thread(NULL);

    EXPECT_EQ(ret, nullptr);
}

TEST_F(test_tcc_rtc_pointer_chasing_simple, background_stress_thread_ten_terations_positive)
{
    void *ret;
    gIterationsLeft = 10;
    gBackgroundStressReq = true;
    EXPECT_CALL(m_pthread_mock, pthread_setaffinity_np(_, sizeof (cpu_set_t), _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_getschedparam(_, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_pthread_mock, pthread_setschedparam(_, SCHED_OTHER, _))
        .WillOnce(Return(0));

    ret = background_stress_thread(NULL);

    EXPECT_NE(ret, nullptr);
}

#if 0
static void *background_stress_thread(void *args)
{
    int res;
    int sched_policy;
    cpu_set_t cpu_set;
    struct sched_param sp = {0};

    /* Set the CPU affinity for the stress thread */
    CPU_ZERO(&cpu_set);
    CPU_SET(INTERNAL_STRESS_AFFINITY, &cpu_set);
    res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (res) {
        pr_err("Warning: stress thread: cannot set CPU affinity: res: %d, errno: %d\n", res, errno);
    }

    /* Update the stress thread scheduling policy */
    res = pthread_getschedparam(pthread_self(), &sched_policy, &sp);
    if (!res) {
        sp.sched_priority = INTERNAL_STRESS_PRIO;
        res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
        if (res) {
            pr_err("Warning: stress thread: cannot set thread params: res: %d, errno: %d\n", res, errno);
        }
    } else {
        pr_err("Warning: stress thread: cannot get thread params: res: %d, errno: %d\n", res, errno);
    }

    pointer_chase_create_linear(gStressBuffer, sizeof (gStressBuffer));
    while (gBackgroundStressReq) {
        /* Additionally store the returned pointer in 'args',
           this is done just to make it more difficult for the compiler to compile
           this block of code out, it is not used in other resprcts */
        args = pointer_chase_run_read_workload(gStressBuffer, INTERNAL_STRESS_NODES);
    }

    return args;
}
#endif

} // namespace {

void pointer_chase_run_read_workload_hook()
{
    if (!--gIterationsLeft) {
        gBackgroundStressReq = false;
    }
}
