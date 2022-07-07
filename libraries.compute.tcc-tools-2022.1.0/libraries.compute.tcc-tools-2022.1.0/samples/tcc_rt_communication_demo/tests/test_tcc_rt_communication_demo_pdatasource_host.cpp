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
#include "pdatasource.c"
}

#include "libc_mock.hpp"
#include "tcc_cache_mock.hpp"
#include "itt_notify_mock.hpp"
#include "libc_pthread_mock.hpp"
#include "tcc_measurement_mock.hpp"

#include <SubprocessTest.hpp>

namespace {

using namespace ::testing;

const int gTestTccMemoryAffinity = 2;
const int gTestTccMemoryLatency = 1000;

/**
 * This class aggregates UTs for check TCC TSN E2E sample, talker appli
 */
class test_tcc_rt_communication_demo_pdatasource_config : public SubprocessTest<Test>
{
protected:
    // Sets up the test fixture.
    void SetUp() override {
        gCacheLines = NULL;
        gTargetNode = NULL;
    }

    // Tears down the test fixture.
    void TearDown() override {
        gCacheLines = NULL;
    }

protected:
    StrictMock<libc_mock> m_libc_mock;
    StrictMock<pthread_mock> m_pthread_mock;
    StrictMock<tcc_cache_mock> m_tcc_cache_mock;
    StrictMock<itt_notify_mock> m_itt_notify_mock;
    cache_line_node_t m_test_nodes[127*1024/sizeof (cache_line_node_t)];
};

class test_tcc_rt_communication_demo_prepared_pdatasource :
    public test_tcc_rt_communication_demo_pdatasource_config
{
protected:
    // Sets up the test fixture.
    void SetUp() override {
        test_tcc_rt_communication_demo_pdatasource_config::SetUp();

        __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
        __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
        char resp1[] = "libtcc_collector.so";
        char resp2[] = "Workload:100000";
        EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
            .WillOnce(Return(test_domain));
        EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
            .WillOnce(Return(test_string_handle));
        EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
            .WillOnce(Return(resp1));
        EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
            .WillOnce(Return(resp2));
        EXPECT_CALL(m_libc_mock, mlockall(_))
            .WillOnce(Return(0));
        // Expectations for the 'pc_init'
        EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
            .WillOnce(Return(0));
        EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
            .WillOnce(Return(m_test_nodes));
        EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
            .WillOnce(Return(0));

        const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);

        EXPECT_EQ(res, TCC_E_SUCCESS);
        EXPECT_TRUE(m_libc_mock.verify_and_clear());
    }

    // Tears down the test fixture.
    void TearDown() override {
        test_tcc_rt_communication_demo_pdatasource_config::TearDown();
    }
};

class test_tcc_rt_communication_demo_ready_pdatasource :
    public test_tcc_rt_communication_demo_prepared_pdatasource
{
protected:
    void SetUp() override {
        test_tcc_rt_communication_demo_prepared_pdatasource::SetUp();
    }
    void TearDown() override {
        EXPECT_TRUE(m_libc_mock.verify_and_clear());

        EXPECT_CALL(m_libc_mock, munlockall()).WillOnce(Return(0));
        EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish()).WillOnce(Return(0));
        EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
        EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));

        destroy_datasource();
        EXPECT_EQ(gCacheLines, nullptr);
        EXPECT_TRUE(m_libc_mock.verify_and_clear());

        test_tcc_rt_communication_demo_prepared_pdatasource::TearDown();
    }
};

class test_tcc_rt_communication_demo_datasource_mocked_measurement :
    public test_tcc_rt_communication_demo_ready_pdatasource
{
protected:
    void SetUp() override {
        test_tcc_rt_communication_demo_ready_pdatasource::SetUp();
    }
    void TearDown() override {
        test_tcc_rt_communication_demo_ready_pdatasource::TearDown();
    }

protected:
    StrictMock<measurement_mock> m_measurement_mock;
};

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp1[] = "libtcc_collector.so";
    char resp2[] = "Workload:100000";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(Return(resp1));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(Return(resp2));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(0));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(Return(0));

    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);

    EXPECT_EQ(res, TCC_E_SUCCESS);
    EXPECT_NE(gCacheLines, nullptr);

    // Deinitialization
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));
    pc_fini();
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_default_collector_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp2[] = "Workload:100000";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(ReturnNull());
    EXPECT_CALL(m_libc_mock, setenv(_, _, 0))
        .WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(Return(resp2));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(0));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(Return(0));

    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);

    EXPECT_EQ(res, TCC_E_SUCCESS);
    EXPECT_NE(gCacheLines, nullptr);

    // Deinitialization
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));
    pc_fini();
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_default_buffer_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp1[] = "libtcc_collector.so";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(Return(resp1));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(ReturnNull());
    EXPECT_CALL(m_libc_mock, setenv(_, _, 0))
        .WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(0));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(Return(0));

    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);

    EXPECT_EQ(res, TCC_E_SUCCESS);
    EXPECT_NE(gCacheLines, nullptr);

    // Deinitialization
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));
    pc_fini();
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_itt_failed_negative)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(ReturnNull());
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish());
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    internal::CaptureStderr();
    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, TCC_E_NOT_AVAILABLE);
    EXPECT_NE(captured.find("Cannot initialize measurement sub-system"), std::string::npos);
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_setenv_collector_failed_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp2[] = "Workload:100000";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(ReturnNull());
    EXPECT_CALL(m_libc_mock, setenv(_, _, 0))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(Return(resp2));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(0));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    internal::CaptureStderr();
    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, TCC_E_SUCCESS);
    EXPECT_NE(gCacheLines, nullptr);
    EXPECT_NE(captured.find("Failed to set environment"), std::string::npos);

    // Deinitialization
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));
    pc_fini();
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_setenv_buffer_failed_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp1[] = "libtcc_collector.so";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(Return(resp1));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(ReturnNull());
    EXPECT_CALL(m_libc_mock, setenv(_, _, 0))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(0));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    internal::CaptureStderr();
    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, TCC_E_SUCCESS);
    EXPECT_NE(gCacheLines, nullptr);
    EXPECT_NE(captured.find("Failed to set environment"), std::string::npos);

    // Deinitialization
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));
    pc_fini();
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_mlockall_fail_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp1[] = "libtcc_collector.so";
    char resp2[] = "Workload:100000";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(Return(resp1));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(Return(resp2));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(-EINVAL));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_malloc(_, _))
        .WillOnce(Return(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_create(_, _, _, _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    internal::CaptureStderr();
    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);
    const std::string &captured = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, TCC_E_SUCCESS);
    EXPECT_NE(gCacheLines, nullptr);
    EXPECT_NE(captured.find("Cannot lock pages in memory"), std::string::npos);

    // Deinitialization
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));
    pc_fini();
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, datasource_prepare_pc_init_fail_positive)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    char resp1[] = "libtcc_collector.so";
    char resp2[] = "Workload:100000";
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_))
        .WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_))
        .WillOnce(Return(test_string_handle));
    EXPECT_CALL(m_libc_mock, getenv("INTEL_LIBITTNOTIFY64"))
        .WillOnce(Return(resp1));
    EXPECT_CALL(m_libc_mock, getenv("TCC_MEASUREMENTS_BUFFERS"))
        .WillOnce(Return(resp2));
    EXPECT_CALL(m_libc_mock, mlockall(_))
        .WillOnce(Return(0));
    // Expectations for the 'pc_init'
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_init(_))
        .WillOnce(Return(-EINVAL));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    internal::CaptureStdout();
    internal::CaptureStderr();
    const int res = prepare_datasource(gTestTccMemoryLatency, gTestTccMemoryAffinity);
    const std::string &capturedOut = testing::internal::GetCapturedStdout();
    const std::string &capturedErr = testing::internal::GetCapturedStderr();

    EXPECT_EQ(res, TCC_E_NOT_AVAILABLE);
    EXPECT_EQ(gCacheLines, nullptr);
    EXPECT_NE(capturedOut.find("Error description"), std::string::npos);
    EXPECT_NE(capturedErr.find("Cannot init pointer chasing workload"), std::string::npos);
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, put_value_without_initialization_negative)
{
    ASSERT_EQ(gCacheLines, nullptr);
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    int test_value = 0;
    internal::CaptureStderr();
    put_value_in_datasource(&test_value);
    const std::string &output = testing::internal::GetCapturedStderr();

    EXPECT_NE(output.find("No datasource target node to put a new value"), std::string::npos);
}

TEST_F(test_tcc_rt_communication_demo_pdatasource_config, get_value_without_initialization_negative)
{
    ASSERT_EQ(gCacheLines, nullptr);
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    internal::CaptureStderr();
    void *value = get_value_from_datasource();
    const std::string &output = testing::internal::GetCapturedStderr();

    ASSERT_EQ(value, nullptr);
    EXPECT_NE(output.find("No workload buffer to get a value"), std::string::npos);
}

TEST_F(test_tcc_rt_communication_demo_prepared_pdatasource, destroy_datasource_tcc_cache_finish_failed_positive)
{
    EXPECT_CALL(m_libc_mock, munlockall()).WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish()).WillOnce(Return(-TCC_E_ACCESS));
    EXPECT_CALL(m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));

    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));

    internal::CaptureStdout();
    destroy_datasource();
    const std::string &output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output, "");
}

TEST_F(test_tcc_rt_communication_demo_prepared_pdatasource, destroy_datasource_munlockall_failed_positive)
{
    EXPECT_CALL(m_libc_mock, munlockall()).WillOnce(Return(-EINVAL));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish()).WillOnce(Return(0));

    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));

    destroy_datasource();
}

TEST_F(test_tcc_rt_communication_demo_prepared_pdatasource, destroy_datasource_positive)
{
    EXPECT_CALL(m_libc_mock, munlockall()).WillOnce(Return(0));
    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_finish()).WillOnce(Return(0));

    EXPECT_CALL(m_tcc_cache_mock, tcc_cache_free(m_test_nodes));
    EXPECT_CALL(m_pthread_mock, pthread_join(_, _)).WillOnce(Return(0));

    destroy_datasource();
}

TEST_F(test_tcc_rt_communication_demo_ready_pdatasource, get_put_values_positive)
{
    EXPECT_CALL(m_itt_notify_mock, __itt_task_begin(_, _, _, _));
    EXPECT_CALL(m_itt_notify_mock, __itt_task_end(_));

    int value;
    void *pReturnedValue;
    put_value_in_datasource(&value);

    pReturnedValue = get_value_from_datasource();
    EXPECT_EQ(pReturnedValue, &value);
}

TEST_F(test_tcc_rt_communication_demo_datasource_mocked_measurement, print_statistic_positive)
{
    const char expected_message[] =
        "\n*** Statistics for TCC workload ************************* \n"
        "    Minimum latency: 0 CPU cycles (0 us)\n"
        "    Maximum latency: 0 CPU cycles (0 us)\n"
        "    Average latency: 0 CPU cycles (0 us)\n"
        "******************************************************** \n\n";

    tcc_measurement measurement = {};
    EXPECT_CALL(m_measurement_mock, tcc_measurement_get(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(&measurement), Return(TCC_E_SUCCESS)));
    EXPECT_CALL(m_libc_mock, fopen(_, _))
        .WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fclose(_))
        .WillRepeatedly(Invoke(__real_fclose));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _))
        .WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, close(_))
        .WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_measurement_mock, tcc_measurement_print_history(_, _, _))
        .WillOnce(Return(TCC_E_SUCCESS));
    EXPECT_CALL(m_measurement_mock, tcc_measurement_convert_clock_to_timespec(_))
        .Times(3)
        .WillRepeatedly(Return((timespec){0, 0}));

    internal::CaptureStdout();
    print_datasource_statistic();
    const std::string &output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, expected_message);
}

TEST_F(test_tcc_rt_communication_demo_datasource_mocked_measurement, print_statistic_get_measurement_fail_negative)
{
    EXPECT_CALL(m_measurement_mock, tcc_measurement_get(_, _, _))
        .WillOnce(Return(TCC_E_NOT_AVAILABLE));
    EXPECT_CALL(m_libc_mock, fopen(_, _))
        .Times(1)
        .WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fclose(_))
        .WillRepeatedly(Invoke(__real_fclose));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _))
        .WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, close(_))
        .WillRepeatedly(Invoke(__real_close));

    internal::CaptureStderr();
    print_datasource_statistic();
    const std::string &output = testing::internal::GetCapturedStderr();

    EXPECT_NE(output.find("tcc_measurement_get(domain, measurement, &tcc_measurement_ptr) failed"), std::string::npos);
}

TEST_F(test_tcc_rt_communication_demo_datasource_mocked_measurement, print_statistic_history_fail_negative)
{
    const char expected_message[] =
        "\n*** Statistics for TCC workload ************************* \n"
        "    Minimum latency: 0 CPU cycles (0 us)\n"
        "    Maximum latency: 0 CPU cycles (0 us)\n"
        "    Average latency: 0 CPU cycles (0 us)\n"
        "******************************************************** \n\n";

    tcc_measurement measurement = {};
    EXPECT_CALL(m_measurement_mock, tcc_measurement_get(_, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(&measurement), Return(TCC_E_SUCCESS)));
    EXPECT_CALL(m_libc_mock, fopen(_, _))
        .WillRepeatedly(Invoke(__real_fopen));
    EXPECT_CALL(m_libc_mock, fclose(_))
        .WillRepeatedly(Invoke(__real_fclose));
    EXPECT_CALL(m_libc_mock, fread(_, _, _, _))
        .WillRepeatedly(Invoke(__real_fread));
    EXPECT_CALL(m_libc_mock, close(_))
        .WillRepeatedly(Invoke(__real_close));
    EXPECT_CALL(m_measurement_mock, tcc_measurement_print_history(_, _, _))
        .WillOnce(Return(TCC_E_ACCESS));
    EXPECT_CALL(m_measurement_mock, tcc_measurement_convert_clock_to_timespec(_))
        .Times(3)
        .WillRepeatedly(Return((timespec){0, 0}));

    internal::CaptureStdout();
    internal::CaptureStderr();
    print_datasource_statistic();
    const std::string &output = testing::internal::GetCapturedStderr();
    const std::string &stdoutput = testing::internal::GetCapturedStdout();

    EXPECT_EQ(stdoutput, expected_message);
    EXPECT_NE(output.find("Unable to dump TCC statistic to file"), std::string::npos);
}

} // namespace {
