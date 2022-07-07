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

#include <gtest/gtest.h>
#include <libc_mock.hpp>
#include <cstdarg>
#include "tcc_driver_api_mock.hpp"
#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include <errno.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "gtest_helpers.hpp"
#include "tcc_time_fake.hpp"
#include "tcc_region_manager_internal.h"
#include "linux/tcc_buffer.h"

#define PAGE_SIZE 4096

namespace
{

const std::vector<uint8_t> RTCT_TABLE { //Dump of real rtct table
        0x50, 0x54, 0x43, 0x54, 0x64, 0x01, 0x00, 0x00, 0x01, 0xee, 0x49, 0x4e, 0x54, 0x45, 0x4c, 0x20,
        0x45, 0x44, 0x4b, 0x32, 0x20, 0x20, 0x20, 0x20, 0x05, 0x00, 0x00, 0x00, 0x49, 0x4e, 0x54, 0x4c,
        0x0d, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x09, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
        0x18, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x04, 0x00,
        0x06, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
        0x30, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x30, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x8e, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x30, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x72, 0x30, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x14, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x40, 0x00, 0x00, 0x00, 0x00,
        0xa8, 0x33, 0x00, 0x00
    };

using namespace testing;



const size_t FAKE_INDEX = 0;
const size_t FAKE_SIZE = 8182;
const tcc_config_t* WRONG_TCC_CONFIG = reinterpret_cast<const tcc_config_t*>(0xDEADBEAF);
const unsigned int FAKE_REGION_COUNT = 3;
const int FAKE_NODE_ID = 42;
const int FAKE_BUFFER_FD = 37;

#define FAKE_ERRNO_CODE EFBIG

#define STRINGIFY(x) #x

class BaseTest : public Test
{
protected:
    tcc_cpuid_mock m_tcc_cpuid_mock;
};
class BaseFixture : public tcc_time_fake<BaseTest>
{
};

memory_properties_t FAKE_REGION[FAKE_REGION_COUNT];

static_assert(sizeof(FAKE_REGION) / sizeof(memory_properties_t) == FAKE_REGION_COUNT, "Ops!");

class host_tcc_region_manager_driver_test_wrong_value : public BaseFixture
{
protected:
    void SetUp() override
    {
        memset(FAKE_REGION, 0, sizeof(FAKE_REGION));
        errno = 0;
    }
    void TearDown() override
    {
        EXPECT_ERRNO_EQ(EINVAL);
        errno = 0;
    }
};


TEST_F(host_tcc_region_manager_driver_test_wrong_value, region_manager_driver_destroy_negative)
{
    region_manager_driver_destroy(nullptr);
}
TEST_F(host_tcc_region_manager_driver_test_wrong_value, region_manager_driver_count_negative)
{
    EXPECT_EQ(-1, region_manager_driver_count(nullptr));
}

TEST_F(host_tcc_region_manager_driver_test_wrong_value, region_manager_driver_get_negative)
{
    size_t LOCAL_FAKE_INDEX = 1;
    EXPECT_EQ(nullptr, region_manager_driver_get(nullptr, LOCAL_FAKE_INDEX));
}

TEST_F(host_tcc_region_manager_driver_test_wrong_value, region_manager_driver_mmap_region_manager_null_negative)
{
    EXPECT_EQ(nullptr, region_manager_driver_mmap(nullptr, FAKE_INDEX, FAKE_SIZE));
}

TEST_F(host_tcc_region_manager_driver_test_wrong_value, region_manager_driver_mmap_wrong_size_negative)
{
    region_manager_t* FAKE_REGION_MANAGER_PTR = reinterpret_cast<region_manager_t*>(0xDEADBEAF);
    EXPECT_EQ(nullptr, region_manager_driver_mmap(FAKE_REGION_MANAGER_PTR, FAKE_INDEX, 0));
}

class host_tcc_region_manager_driver_create_test : public BaseFixture
{
protected:
    NiceMock<alloc_mock> m_allock_mock;
    NiceMock<libc_mock> m_lib_mock;

    StrictMock<tcc_driver_api_mock> m_driver_api;

    void SetUp() override
    {
        errno = 0;
    }
    void TearDown() override
    {
        errno = 0;
    }
};

class host_tcc_region_manager_driver_create_with_read_rtct_test : public host_tcc_region_manager_driver_create_test
{
protected:
    void SetUp() override
    {
        host_tcc_region_manager_driver_create_test::SetUp();
        EXPECT_CALL(m_driver_api, tcc_driver_read_rtct(NotNull(), NotNull()))
            .WillOnce(Invoke([&](void** rtct, size_t* size) {
                void* ptr = malloc(RTCT_TABLE.size());
                EXPECT_NE(nullptr, ptr);
                memcpy(ptr, RTCT_TABLE.data(), RTCT_TABLE.size());
                *rtct = ptr;
                *size = RTCT_TABLE.size();
                return 0;
            }));
    }
    void TearDown() override
    {
        host_tcc_region_manager_driver_create_test::TearDown();
    }
};

TEST_F(host_tcc_region_manager_driver_create_test, region_manager_driver_create_destroy_negative)
{
    EXPECT_CALL(m_allock_mock, calloc(_, _)).WillOnce(MOCK_RETURN_VAL_SET_ERRNO(nullptr, ENOMEM));
    EXPECT_EQ(nullptr, region_manager_driver_create(nullptr));
    EXPECT_ERRNO_EQ(EINVAL);
}
TEST_F(host_tcc_region_manager_driver_create_test, region_manager_driver_create_get_region_count_failure_negative)
{
    EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
    EXPECT_EQ(nullptr, region_manager_driver_create(nullptr));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE);
}

TEST_F(host_tcc_region_manager_driver_create_with_read_rtct_test,
    region_manager_driver_create_zero_region_counts_positive)
{
    EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(Return(0));
    region_manager_t* manager = region_manager_driver_create(nullptr);
    EXPECT_NE(nullptr, manager);
    region_manager_driver_destroy(manager);
    EXPECT_ERRNO_EQ(0);
}

TEST_F(host_tcc_region_manager_driver_create_test, region_manager_driver_create_second_calloc_failure_negative)
{
    InSequence s;
    EXPECT_CALL(m_allock_mock, calloc(_, _)).WillOnce(DoDefault());
    EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(Return(FAKE_REGION_COUNT));
    EXPECT_CALL(m_allock_mock, calloc(_, _)).WillOnce(MOCK_RETURN_VAL_SET_ERRNO(nullptr, FAKE_ERRNO_CODE));
    EXPECT_EQ(nullptr, region_manager_driver_create(nullptr));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE);
}

TEST_F(host_tcc_region_manager_driver_create_with_read_rtct_test, region_manager_driver_create_positive)
{
    EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(Return(FAKE_REGION_COUNT));
    auto get_region = [&](unsigned int region_id, memory_properties_t* out) {
        EXPECT_LT(region_id, FAKE_REGION_COUNT);
        FAKE_REGION[region_id].id = region_id;
        *out = FAKE_REGION[region_id];
        return 0;
    };
    EXPECT_CALL(m_driver_api, tcc_driver_get_memory_config(_, NotNull()))
        .Times(FAKE_REGION_COUNT)
        .WillRepeatedly(Invoke(get_region));
    region_manager_t* manager = region_manager_driver_create(nullptr);
    EXPECT_NE(nullptr, manager);
    region_manager_driver_destroy(manager);
    EXPECT_ERRNO_EQ(0);
}

TEST_F(host_tcc_region_manager_driver_create_with_read_rtct_test,
    region_manager_driver_create_destroy_config_not_used_positive)
{
    EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(Return(0));
    region_manager_t* manager = region_manager_driver_create(WRONG_TCC_CONFIG);
    EXPECT_NE(nullptr, manager);
    region_manager_driver_destroy(manager);
    EXPECT_ERRNO_EQ(0);
}

class host_tcc_region_manager_driver_zero_region_count_test : public BaseFixture
{
protected:
    NiceMock<alloc_mock> m_allock_mock;
    NiceMock<libc_mock> m_libc_mock;

    StrictMock<tcc_driver_api_mock> m_driver_api;
    region_manager_t* m_region_manager = nullptr;
    void SetUp() override
    {
        errno = 0;
        EXPECT_CALL(m_driver_api, tcc_driver_read_rtct(NotNull(), NotNull()))
            .WillOnce(Invoke([&](void** rtct, size_t* size) {
                void* ptr = malloc(RTCT_TABLE.size());
                EXPECT_NE(nullptr, ptr);
                memcpy(ptr, RTCT_TABLE.data(), RTCT_TABLE.size());
                *rtct = ptr;
                *size = RTCT_TABLE.size();
                return 0;
            }));
        ON_CALL(m_libc_mock, get_nprocs()).WillByDefault(Return(4));
        EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(Return(0));
        m_region_manager = region_manager_driver_create(WRONG_TCC_CONFIG);
        ASSERT_NE(nullptr, m_region_manager);
    }
    void TearDown() override
    {
        region_manager_driver_destroy(m_region_manager);
        errno = 0;
    }
};

TEST_F(host_tcc_region_manager_driver_zero_region_count_test, region_manager_driver_create_second_time_negative)
{
    ASSERT_ERRNO_EQ(0);
    EXPECT_EQ(nullptr, region_manager_driver_create(nullptr));
    EXPECT_EQ(nullptr, region_manager_driver_create(WRONG_TCC_CONFIG));
    EXPECT_ERRNO_EQ(EBUSY);
}

TEST_F(host_tcc_region_manager_driver_zero_region_count_test, region_manager_driver_get_negative)
{
    EXPECT_EQ(nullptr, region_manager_driver_get(m_region_manager, 1));  //Only default DRAM_FAKE_REGION
    EXPECT_ERRNO_EQ(EINVAL);
}

TEST_F(host_tcc_region_manager_driver_zero_region_count_test, region_manager_driver_mmap_negative)
{
    EXPECT_EQ(nullptr, region_manager_driver_mmap(m_region_manager, 1, PAGE_SIZE));
    EXPECT_ERRNO_EQ(EINVAL);
}

TEST_F(host_tcc_region_manager_driver_zero_region_count_test, region_manager_driver_count_positive)
{
    EXPECT_EQ(1, region_manager_driver_count(m_region_manager));  //Only default DRAM_FAKE_REGION
    EXPECT_ERRNO_EQ(0);
}


class host_tcc_region_manager_driver_test : public BaseFixture
{
protected:
    NiceMock<alloc_mock> m_allock_mock;
    StrictMock<tcc_driver_api_mock> m_driver_api;
    StrictMock<libc_mock> m_libc_mock;
    region_manager_t* m_region_manager = nullptr;
    cpu_set_t cpu_set_from_bits(uint8_t mask)
    {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        for (size_t i = 0; i < sizeof(mask) * 8; i++) {
            if (mask & 1)
                CPU_SET(i, &cpu_set);
            mask >>= 1;
        }
        return cpu_set;
    }

    void SetUp() override
    {
        errno = 0;
        EXPECT_CALL(m_driver_api, tcc_driver_read_rtct(NotNull(), NotNull()))
            .WillOnce(Invoke([&](void** rtct, size_t* size) {
                void* ptr = malloc(RTCT_TABLE.size());
                EXPECT_NE(nullptr, ptr);
                memcpy(ptr, RTCT_TABLE.data(), RTCT_TABLE.size());
                *rtct = ptr;
                *size = RTCT_TABLE.size();
                return 0;
            }));
        EXPECT_CALL(m_libc_mock, get_nprocs()).Times(AnyNumber()).WillRepeatedly(Return(4));
        EXPECT_CALL(m_driver_api, tcc_driver_get_region_count()).WillOnce(Return(FAKE_REGION_COUNT));
        auto get_region = [&](unsigned int region_id, memory_properties_t* out) {
            EXPECT_LT(region_id, FAKE_REGION_COUNT);
            FAKE_REGION[region_id].id = region_id;
            FAKE_REGION[region_id].latency = 100 + region_id;
            FAKE_REGION[region_id].mask = cpu_set_from_bits(0xF);
            FAKE_REGION[region_id].type = static_cast<enum tcc_memory_level>(region_id + 1);
            *out = FAKE_REGION[region_id];
            return 0;
        };
        EXPECT_CALL(m_driver_api, tcc_driver_get_memory_config(_, NotNull()))
            .Times(FAKE_REGION_COUNT)
            .WillRepeatedly(Invoke(get_region));
        m_region_manager = region_manager_driver_create(nullptr);
        ASSERT_NE(nullptr, m_region_manager);
        EXPECT_ERRNO_EQ(0);
        ASSERT_TRUE(m_driver_api.verify_and_clear());
        ASSERT_TRUE(m_allock_mock.verify_and_clear());
    }
    void TearDown() override
    {
        region_manager_driver_destroy(m_region_manager);
        errno = 0;
    }
};

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_count_positive)
{
    EXPECT_EQ(FAKE_REGION_COUNT + 1, region_manager_driver_count(m_region_manager));
    EXPECT_ERRNO_EQ(0);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_get_0_positive)
{
    size_t fake_index = 0;
    const memory_properties_t* region_data = region_manager_driver_get(m_region_manager, fake_index);
    ASSERT_NE(region_data, nullptr);
    EXPECT_EQ(region_data->id, static_cast<unsigned int>(-1));
    EXPECT_ERRNO_EQ(0);
}
TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_get_1_positive)
{
    size_t fake_index = 1;
    const memory_properties_t* region_data = region_manager_driver_get(m_region_manager, fake_index);
    ASSERT_NE(region_data, nullptr);
    EXPECT_EQ(region_data->id, 2U);
    EXPECT_ERRNO_EQ(0);
}
TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_get_2_positive)
{
    size_t fake_index = 2;
    const memory_properties_t* region_data = region_manager_driver_get(m_region_manager, fake_index);
    ASSERT_NE(region_data, nullptr);
    EXPECT_EQ(region_data->id, 1U);
    EXPECT_EQ(errno, 0);
}
TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_get_3_positive)
{
    size_t fake_index = 3;
    const memory_properties_t* region_data = region_manager_driver_get(m_region_manager, fake_index);
    ASSERT_NE(region_data, nullptr);
    EXPECT_EQ(region_data->id, 0U);
    EXPECT_EQ(errno, 0);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_get_index_out_of_range_negative)
{
    size_t fake_index = FAKE_REGION_COUNT + 1;
    EXPECT_EQ(region_manager_driver_get(m_region_manager, fake_index), nullptr);
    EXPECT_ERRNO_EQ(EINVAL);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_tcc_driver_mmap_dram_positive)
{
    size_t fake_index = 0;
    EXPECT_CALL(m_libc_mock, memfd_create(_, _)).WillOnce(DoDefault());
    EXPECT_CALL(m_libc_mock, ftruncate(_, FAKE_SIZE)).WillOnce(DoDefault());
    EXPECT_CALL(m_libc_mock, mmap(_, FAKE_SIZE, _, _, _, _)).WillOnce(Invoke(__real_mmap));
    EXPECT_CALL(m_libc_mock, close(_)).WillOnce(Invoke(__real_close));
    void* memory = region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE);
    EXPECT_NE(nullptr, memory);
    EXPECT_EQ(0, __real_munmap(memory, FAKE_SIZE));
    EXPECT_ERRNO_EQ(0);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_tcc_driver_mmap_dram_memfd_create_failure_negative)
{
    size_t fake_index = 0;
    EXPECT_CALL(m_libc_mock, memfd_create(_, _)).WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
    EXPECT_EQ(nullptr, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_tcc_driver_req_buffer_failure_negative)
{
    size_t fake_index = 1;
    unsigned int expected_tcc_driver_index = 2;
    EXPECT_CALL(m_driver_api, tcc_driver_req_buffer(expected_tcc_driver_index, FAKE_SIZE))
        .WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
    EXPECT_EQ(nullptr, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_open_failure_negative)
{
    size_t fake_index = 1;
    unsigned int expected_tcc_driver_index = 2;
    std::stringstream expected_buffer_file_name;
    expected_buffer_file_name << TCC_BUFFER_NAME << FAKE_NODE_ID;
    EXPECT_CALL(m_driver_api, tcc_driver_req_buffer(expected_tcc_driver_index, FAKE_SIZE))
        .WillOnce(Return(FAKE_NODE_ID));
    //---------------------------------------------------------------------------------------
    EXPECT_CALL(m_libc_mock, open(StrEq(expected_buffer_file_name.str().c_str()), O_RDWR, _))
        .WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
    EXPECT_CALL(m_libc_mock, unlink(StrEq(expected_buffer_file_name.str().c_str()))).WillOnce(Return(0));
    EXPECT_EQ(nullptr, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_open_failure_unlink_failure_negative)
{
    size_t fake_index = 1;
    unsigned int expected_tcc_driver_index = 2;
    std::stringstream expected_buffer_file_name;
    expected_buffer_file_name << TCC_BUFFER_NAME << FAKE_NODE_ID;
    EXPECT_CALL(m_driver_api, tcc_driver_req_buffer(expected_tcc_driver_index, FAKE_SIZE))
        .WillOnce(Return(FAKE_NODE_ID));
    //---------------------------------------------------------------------------------------
    EXPECT_CALL(m_libc_mock, open(StrEq(expected_buffer_file_name.str().c_str()), O_RDWR, _))
        .WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
    EXPECT_CALL(m_libc_mock, unlink(StrEq(expected_buffer_file_name.str().c_str())))
        .WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE+1));
    EXPECT_EQ(nullptr, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE+1);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_unlink_failure_mmap_success_negative)
{
    void* FAKE_TCC_BUFFER = reinterpret_cast<void*>(0xDEADBEAF);
    size_t fake_index = 1;
    unsigned int expected_tcc_driver_index = 2;
    std::stringstream expected_buffer_file_name;
    expected_buffer_file_name << TCC_BUFFER_NAME << FAKE_NODE_ID;
    EXPECT_CALL(m_driver_api, tcc_driver_req_buffer(expected_tcc_driver_index, FAKE_SIZE))
        .WillOnce(Return(FAKE_NODE_ID));
    EXPECT_CALL(m_libc_mock, open(StrEq(expected_buffer_file_name.str().c_str()), O_RDWR, _))
        .WillOnce(Return(FAKE_BUFFER_FD));
    EXPECT_CALL(m_libc_mock, close(FAKE_BUFFER_FD)).WillOnce(Return(0));
    //---------------------------------------------------------------------------------------
    EXPECT_CALL(m_libc_mock, mmap(nullptr, FAKE_SIZE, PROT_READ | PROT_WRITE, _, FAKE_BUFFER_FD, 0))
        .WillOnce(Return(FAKE_TCC_BUFFER));
    EXPECT_EQ(FAKE_TCC_BUFFER, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(0);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_positive)
{
    void* FAKE_TCC_BUFFER = reinterpret_cast<void*>(0xDEADBEAF);
    size_t fake_index = 1;
    unsigned int expected_tcc_driver_index = 2;
    std::stringstream expected_buffer_file_name;
    expected_buffer_file_name << TCC_BUFFER_NAME << FAKE_NODE_ID;
    EXPECT_CALL(m_driver_api, tcc_driver_req_buffer(expected_tcc_driver_index, FAKE_SIZE))
        .WillOnce(Return(FAKE_NODE_ID));
    EXPECT_CALL(m_libc_mock, open(StrEq(expected_buffer_file_name.str().c_str()), O_RDWR, _))
        .WillOnce(Return(FAKE_BUFFER_FD));
    EXPECT_CALL(m_libc_mock, close(FAKE_BUFFER_FD)).WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, mmap(nullptr, FAKE_SIZE, PROT_READ | PROT_WRITE, _, FAKE_BUFFER_FD, 0))
        .WillOnce(Return(FAKE_TCC_BUFFER));
    EXPECT_EQ(FAKE_TCC_BUFFER, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(0);
}

TEST_F(host_tcc_region_manager_driver_test, region_manager_driver_mmap_mmap_failure_negative)
{
    size_t fake_index = 1;
    unsigned int expected_tcc_driver_index = 2;
    std::stringstream expected_buffer_file_name;
    expected_buffer_file_name << TCC_BUFFER_NAME << FAKE_NODE_ID;
    EXPECT_CALL(m_driver_api, tcc_driver_req_buffer(expected_tcc_driver_index, FAKE_SIZE))
        .WillOnce(Return(FAKE_NODE_ID));
    EXPECT_CALL(m_libc_mock, open(StrEq(expected_buffer_file_name.str().c_str()), O_RDWR, _))
        .WillOnce(Return(FAKE_BUFFER_FD));
    EXPECT_CALL(m_libc_mock, close(FAKE_BUFFER_FD)).WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, mmap(nullptr, FAKE_SIZE, PROT_READ | PROT_WRITE, _, FAKE_BUFFER_FD, 0))
        .WillOnce(MOCK_RETURN_VAL_SET_ERRNO(MAP_FAILED, FAKE_ERRNO_CODE));
    EXPECT_EQ(nullptr, region_manager_driver_mmap(m_region_manager, fake_index, FAKE_SIZE));
    EXPECT_ERRNO_EQ(FAKE_ERRNO_CODE);
}
//*/
}  // namespace
