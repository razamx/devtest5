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

#include "tcc_config_mock.hpp"
#include "tcc_region_manager_internal.h"
#include "linux/tcc_buffer.h"
#include <gtest/gtest.h>
#include <libc_mock.hpp>
#include <cstdarg>
#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include <errno.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <algorithm>
#include "mock_common.hpp"
#include "gtest_helpers.hpp"
#include "tcc_time_fake.hpp"

#define PAGE_SIZE 4096

#define EXPECT_EQ_MEM_PROP(expected, actual)                                                                       \
    ({                                                                                                             \
        memory_properties_t expected_val = (expected);                                                             \
        memory_properties_t actual_val = (actual);                                                                 \
        EXPECT_EQ(expected_val.id, actual_val.id) << "'" << #expected << "' vs '" << #actual << "'";               \
        EXPECT_EQ(expected_val.latency, actual_val.latency) << "'" << #expected << "' vs '" << #actual << "'";     \
        EXPECT_EQ(expected_val.type, actual_val.type) << "'" << #expected << "' vs '" << #actual << "'";           \
        EXPECT_EQ(expected_val.size, actual_val.size) << "'" << #expected << "' vs '" << #actual << "'";           \
        EXPECT_EQ(CPU_COUNT(&(expected_val.mask)), CPU_COUNT(&(actual_val.mask)))                                  \
            << "'" << #expected << "' vs '" << #actual << "'";                                                     \
        for (int cpu = 0; cpu < std::min(CPU_COUNT(&(expected_val.mask)), CPU_COUNT(&(actual_val.mask))); cpu++) { \
            EXPECT_TRUE(CPU_ISSET(cpu, &(expected_val.mask)) == CPU_ISSET(cpu, &(actual_val.mask)))                \
                << "Different masks for CPU " << cpu << " '" << #expected << "' vs '" << #actual << "'";           \
        }                                                                                                          \
    })

namespace
{
using namespace testing;

#define FAKE_ERRNO_CODE EFBIG

class tcc_cpuid_mock_impl : public Test
{
protected:
    StrictMock<tcc_cpuid_mock> m_tcc_cpuid_mock;
};
class host_tcc_region_manager_default_base : public tcc_time_fake<tcc_cpuid_mock_impl>
{
};

class host_tcc_region_manager_default_test_wrong_value : public host_tcc_region_manager_default_base
{
protected:
    const size_t FAKE_SIZE = 8182;
    const size_t FAKE_INDEX = 1;
    void SetUp() override
    {
        errno = 0;
    }
    void TearDown() override
    {
        EXPECT_ERRNO_EQ(EINVAL);
        errno = 0;
    }
};


TEST_F(host_tcc_region_manager_default_test_wrong_value, region_manager_default_destroy_negative)
{
    region_manager_default_destroy(nullptr);
}
TEST_F(host_tcc_region_manager_default_test_wrong_value, region_manager_default_count_negative)
{
    EXPECT_EQ(-1, region_manager_default_count(nullptr));
}

TEST_F(host_tcc_region_manager_default_test_wrong_value, region_manager_default_get_negative)
{
    EXPECT_EQ(nullptr, region_manager_default_get(nullptr, FAKE_INDEX));
}

TEST_F(host_tcc_region_manager_default_test_wrong_value, region_manager_default_mmap_region_manager_null_negative)
{
    EXPECT_EQ(nullptr, region_manager_default_mmap(nullptr, FAKE_INDEX, FAKE_SIZE));
}

TEST_F(host_tcc_region_manager_default_test_wrong_value, region_manager_default_mmap_wrong_size_negative)
{
    region_manager_t* FAKE_REGION_MANAGER_PTR = reinterpret_cast<region_manager_t*>(0xDEADBEAF);
    EXPECT_EQ(nullptr, region_manager_default_mmap(FAKE_REGION_MANAGER_PTR, FAKE_INDEX, 0));
}

class host_tcc_region_manager_default_create_test : public host_tcc_region_manager_default_base
{
protected:
    NiceMock<alloc_mock> m_allock_mock;
    StrictMock<libc_mock> m_libc_mock;
    region_manager_t* m_manager = nullptr;
    const int FAKE_CPU_COUNT = 16;
    void SetUp() override
    {
        EXPECT_CALL(m_libc_mock, get_nprocs()).WillRepeatedly(Return(FAKE_CPU_COUNT));
        errno = 0;
    }
    void TearDown() override
    {
        region_manager_default_destroy(m_manager);
        errno = 0;
    }
    memory_properties_t get_prop_for(unsigned int latency, tcc_memory_level type)
    {
        memory_properties_t prop;
        memset(&prop, 0, sizeof(prop));
        prop.id = static_cast<unsigned int>(-1);
        prop.size = static_cast<size_t>(-1);
        prop.latency = latency;
        prop.type = type;
        fill_cpu_mask(&(prop.mask));
        return prop;
    }
};

TEST_F(host_tcc_region_manager_default_create_test, _create_destroy_nullptr_positive)
{
    m_manager = region_manager_default_create(nullptr);
    ASSERT_NE(nullptr, m_manager);
    EXPECT_ERRNO_EQ(0);
}
TEST_F(host_tcc_region_manager_default_create_test, double_create_call_negative)
{
    m_manager = region_manager_default_create(nullptr);
    ASSERT_NE(nullptr, m_manager);
    EXPECT_EQ(nullptr, region_manager_default_create(nullptr));
    EXPECT_ERRNO_EQ(EBUSY);
}

class host_tcc_region_manager_default_test : public host_tcc_region_manager_default_create_test
{
protected:
    void SetUp() override
    {
        host_tcc_region_manager_default_create_test::SetUp();
        m_manager = region_manager_default_create(nullptr);
        ASSERT_NE(nullptr, m_manager);
    }
    const size_t TEST_MMAP_SIZE = 4096;
    int FAKE_FD = 42;
	void* FAKE_PTR = reinterpret_cast<void*>(static_cast<size_t>(0xEDAEDAEDAEDA));
};

TEST_F(host_tcc_region_manager_default_test, count_positive)
{
    EXPECT_EQ(1, region_manager_default_count(m_manager));
}
TEST_F(host_tcc_region_manager_default_test, get_positive)
{
    const memory_properties_t* prop = region_manager_default_get(m_manager, 0);
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ_MEM_PROP(*prop, get_prop_for(0, TCC_MEM_DRAM));
}
TEST_F(host_tcc_region_manager_default_test, get_out_of_range_negative)
{
    EXPECT_EQ(nullptr, region_manager_default_get(m_manager, 1));
}
TEST_F(host_tcc_region_manager_default_test, get_mmap_out_of_range_negative)
{
    EXPECT_EQ(nullptr, region_manager_default_mmap(m_manager, 1, TEST_MMAP_SIZE));
}

TEST_F(host_tcc_region_manager_default_test, get_mmap_mmap_size_zero_negative)
{
    EXPECT_EQ(nullptr, region_manager_default_mmap(m_manager, 0, 0));
}

TEST_F(host_tcc_region_manager_default_test, get_mmap_memfd_create_failure_negative)
{
    EXPECT_CALL(m_libc_mock, memfd_create(NotNull(), 0)).WillOnce(Return(-1));
    EXPECT_EQ(nullptr, region_manager_default_mmap(m_manager, 0, TEST_MMAP_SIZE));
}
TEST_F(host_tcc_region_manager_default_test, get_mmap_ftruncate_failure_negative)
{
    EXPECT_CALL(m_libc_mock, memfd_create(NotNull(), 0)).WillOnce(Return(FAKE_FD));
    EXPECT_CALL(m_libc_mock, ftruncate(FAKE_FD, TEST_MMAP_SIZE)).WillOnce(Return(-1));
    EXPECT_CALL(m_libc_mock, close(FAKE_FD)).WillOnce(Return(0));
    EXPECT_EQ(nullptr, region_manager_default_mmap(m_manager, 0, TEST_MMAP_SIZE));
}
TEST_F(host_tcc_region_manager_default_test, get_mmap_mmap_failure_negative)
{
    EXPECT_CALL(m_libc_mock, memfd_create(NotNull(), 0)).WillOnce(Return(FAKE_FD));
    EXPECT_CALL(m_libc_mock, ftruncate(FAKE_FD, TEST_MMAP_SIZE)).WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, mmap(NULL, TEST_MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, FAKE_FD, 0))
        .WillOnce(Return(MAP_FAILED));
    EXPECT_CALL(m_libc_mock, close(FAKE_FD)).WillOnce(Return(0));
    EXPECT_EQ(nullptr, region_manager_default_mmap(m_manager, 0, TEST_MMAP_SIZE));
}
TEST_F(host_tcc_region_manager_default_test, get_mmap_positive)
{
    EXPECT_CALL(m_libc_mock, memfd_create(NotNull(), 0)).WillOnce(Return(FAKE_FD));
    EXPECT_CALL(m_libc_mock, ftruncate(FAKE_FD, TEST_MMAP_SIZE)).WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, mmap(NULL, TEST_MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, FAKE_FD, 0))
        .WillOnce(Return(FAKE_PTR));
    EXPECT_CALL(m_libc_mock, close(FAKE_FD)).WillOnce(Return(0));

	EXPECT_EQ(FAKE_PTR, region_manager_default_mmap(m_manager, 0, TEST_MMAP_SIZE));
}

class host_tcc_region_manager_default_config_test : public host_tcc_region_manager_default_create_test
{
protected:
	const tcc_config_t* FAKE_CONFIG_PTR=reinterpret_cast<const tcc_config_t*>(0xBEDABEDA);
	StrictMock<tcc_config_mock> m_config_mock;
	void init_manager()
	{
		m_manager = region_manager_default_create(FAKE_CONFIG_PTR);
		ASSERT_NE(nullptr, m_manager);
	}
	const size_t FAKE_L2_LATERNCY  = 5;
	const size_t FAKE_L3_LATERNCY  = 50;
	const size_t FAKE_DRAM_LATERNCY  = 500;
	const tcc_config_item_t m_config_item_l2 = {TCC_MEM_L2, 4096*10};
	const tcc_config_item_t m_config_item_l3 = {TCC_MEM_L3, 4096*100};
	const tcc_config_item_t m_config_item_dram = {TCC_MEM_DRAM, 4096*1000};
};

TEST_F(host_tcc_region_manager_default_config_test, empty_config)
{
	init_manager();
	const memory_properties_t* prop =  region_manager_default_get(m_manager, 0);
	ASSERT_NE(prop, nullptr);
	EXPECT_EQ_MEM_PROP(*prop, get_prop_for(0, TCC_MEM_DRAM));
}

//*/
}  // namespace
