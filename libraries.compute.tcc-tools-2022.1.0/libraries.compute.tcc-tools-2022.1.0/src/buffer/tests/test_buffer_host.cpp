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
#include "tcc/cache.h"
#include <stdio.h>
#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include "gtest_file_helpers.hpp"
#include "region_manager_mock.hpp"

namespace
{

	using namespace ::testing;

	const unsigned int UNUSED_LATENCY_CLK = 5566655;
	const unsigned int FAKE_LATENCY = 30;
	const unsigned int FAKE_SIZE = 57;
	const unsigned int FAKE_CALLOC_NUMBER = 2;
	const unsigned int FAKE_CPU_NUMBER = 8;
	void *WRONG_PTR = reinterpret_cast<void *>(0xDEADBEAF);

	class tcc_buffer_not_initialyzed : public Test
	{
	};

	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_malloc_not_initialyzed_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_malloc(FAKE_SIZE, FAKE_LATENCY));
	}
	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_malloc_default_not_initialyzed_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_malloc_default(FAKE_SIZE));
	}
	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_calloc_not_initialyzed_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_calloc(FAKE_CALLOC_NUMBER, FAKE_SIZE, FAKE_LATENCY));
	}
	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_calloc_default_not_initialyzed_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_calloc_default(FAKE_CALLOC_NUMBER, FAKE_SIZE));
	}
	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_realloc_not_initialyzed_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_realloc(WRONG_PTR, FAKE_SIZE));
	}
	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_free_not_initialyzed_negative)
	{
		tcc_cache_free(WRONG_PTR);
	}
	TEST_F(tcc_buffer_not_initialyzed, tcc_buffer_finish_not_initialyzed_negative)
	{
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
	}

	class tcc_buffer_cpu_number_negative : public Test
	{
	protected:
		StrictMock<libc_mock> m_libc_mock;
		void SetUp() override
		{
			EXPECT_CALL(m_libc_mock, get_nprocs())
				.Times(AnyNumber())
				.WillRepeatedly(Return(FAKE_CPU_NUMBER));
		}
	};

	TEST_F(tcc_buffer_cpu_number_negative, tcc_buffer_cpu_number_less_than_minus_one_negative)
	{
		int8_t cpuid_less_than_minus_one = -2;
		EXPECT_EQ(-TCC_E_BAD_PARAM, tcc_cache_init(cpuid_less_than_minus_one));
	}
	TEST_F(tcc_buffer_cpu_number_negative, tcc_buffer_cpu_number_to_big_negative)
	{
		EXPECT_EQ(-TCC_E_BAD_PARAM, tcc_cache_init(FAKE_CPU_NUMBER));
	}
	TEST_F(tcc_buffer_cpu_number_negative, tcc_buffer_cpu_number_sched_setaffinity_failure_negative)
	{
		EXPECT_CALL(m_libc_mock, sched_setaffinity(_, sizeof(cpu_set_t), NotNull()))
			.Times(1);
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(FAKE_CPU_NUMBER - 1));
	}

	const std::string TCC_CONFIG_CONTENT = "\
{\
	\"L2\": {\
		\"buffer_size_bytes\": 262144\
	},\
	\"L3\": {\
		\"buffer_size_bytes\": 524288\
	}\
}";

	const uint64_t CONF_MAX_L2_LATENCY = 20;
	const uint64_t CONF_MAX_L3_LATENCY = 40;
	const unsigned PROPERTIES_L2_01_INDEX = 1;
	const unsigned PROPERTIES_L2_23_INDEX = 2;
	const unsigned PROPERTIES_L3_0123_INDEX = 3;
	void *SOME_WRONG_BUFFER_PTR[42];

	cpu_set_t get_cpu_set(std::vector<unsigned> set)
	{
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		for (auto cpu : set)
		{
			CPU_SET(cpu, &cpu_set);
		}
		return cpu_set;
	}
	memory_properties_t init_property(
		unsigned int id,
		cpu_set_t mask,
    	unsigned int latency,
    	enum tcc_memory_level type,
    	size_t size
	){
		memory_properties_t ret;
		memset(&ret, 0, sizeof(ret));
		ret.id=id;
		ret.mask=mask;
		ret.latency=latency;
		ret.type=type;
		ret.size=size;
		return ret;
	}

	std::vector<memory_properties_t> PROPERTIES = {
		//id	mask			latency	type		size
		init_property(345, get_cpu_set({0, 1, 2, 3}), 150, TCC_MEM_L3, 1024 * 1024),
		init_property(123, get_cpu_set({0, 1}), 20, TCC_MEM_L2, 1024 * 64),
		init_property(322, get_cpu_set({2, 3}), 20, TCC_MEM_L2, 1024 * 128)
	};

	class tcc_buffer_init_fixture : public Test
	{
	protected:
		NiceMock<alloc_mock> m_alloc_mock;
		StrictMock<libc_mock> m_libc_mock;
		StrictMock<region_manager_mock> m_region_manager_mock;
		std::map<std::string, std::string> m_env;
		void SetUp() override
		{
			ON_CALL(m_libc_mock, getenv(_)).WillByDefault(Invoke([&](const char *key) -> char * {
				if (m_env.find(key) == m_env.end())
				{
					return nullptr;
				};
				return const_cast<char *>(m_env[key].c_str());
			}));
			EXPECT_CALL(m_libc_mock, getenv(_)).Times(AnyNumber());
			ON_CALL(m_libc_mock, close(_)).WillByDefault(Invoke(__real_close));
			EXPECT_CALL(m_libc_mock, close(_)).Times(AnyNumber());
			EXPECT_CALL(m_region_manager_mock, is_tcc_buffer_exists(NotNull())).Times(AnyNumber());
			ON_CALL(m_region_manager_mock, region_manager_count(NotNull())).WillByDefault(Invoke([&](...) { return PROPERTIES.size(); }));
			ON_CALL(m_region_manager_mock, region_manager_get(NotNull(), _)).WillByDefault(Invoke([&](region_manager_t *, size_t count) {
				TCC_LOG_DEBUG("region_manager_get:count:%zu", count);
				return &(PROPERTIES[count]);
			}));
			EXPECT_CALL(m_region_manager_mock, region_manager_destroy(_))
				.Times(AnyNumber())
				.WillRepeatedly(Invoke(free));
			EXPECT_CALL(m_libc_mock, get_nprocs())
				.Times(AnyNumber())
				.WillRepeatedly(Return(FAKE_CPU_NUMBER));
		}
	};

	TEST_F(tcc_buffer_init_fixture, calloc_failure_on_allocate_memory_to_store_state_negative)
	{
		EXPECT_CALL(m_alloc_mock, calloc(_, _)).WillOnce(ReturnNull());
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(-1));
	}
	TEST_F(tcc_buffer_init_fixture, no_available_config_file_negative)
	{
		EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(-1));
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(-1));
	}
	TEST_F(tcc_buffer_init_fixture, tcc_buffer_config_reader_failure_negative)
	{
		EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(0));
		EXPECT_CALL(m_libc_mock, open(_, _, _)).WillOnce(Return(-1));
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(-1));
	}
	TEST_F(tcc_buffer_init_fixture, tcc_buffer_config_region_manager_create_failure_negative)
	{
		EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(0));
		//----------------------------------------------------------
		EXPECT_CALL(m_libc_mock, open(_, _, _)).WillOnce(Return(fake_file::create_fd(TCC_CONFIG_CONTENT)));
		EXPECT_CALL(m_region_manager_mock, region_manager_create(NotNull())).WillOnce(ReturnNull());
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(-1));
	}

	TEST_F(tcc_buffer_init_fixture, tcc_buffer_config_allocators_manager_create_failure_negative)
	{
		region_manager_t *FAKE_REGION_MANAGER = static_cast<region_manager_t *>(malloc(1));
		EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(0));
		EXPECT_CALL(m_libc_mock, open(_, _, _)).WillOnce(Return(fake_file::create_fd(TCC_CONFIG_CONTENT)));
		//----------------------------------------------------------
		EXPECT_CALL(m_region_manager_mock, region_manager_create(NotNull())).WillOnce(Return(FAKE_REGION_MANAGER));
		EXPECT_CALL(m_region_manager_mock, region_manager_count(NotNull())).WillOnce(Return(-1));
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(-1));
	}
	TEST_F(tcc_buffer_init_fixture, tcc_buffer_init_fixture_default_positive)
	{
		region_manager_t *FAKE_REGION_MANAGER = static_cast<region_manager_t *>(malloc(1));
		EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(0));
		EXPECT_CALL(m_libc_mock, open(_, _, _)).WillOnce(Return(fake_file::create_fd(TCC_CONFIG_CONTENT)));
		EXPECT_CALL(m_region_manager_mock, region_manager_create(NotNull())).WillOnce(Return(FAKE_REGION_MANAGER));
		//----------------------------------------------------------
		EXPECT_CALL(m_region_manager_mock, region_manager_count(NotNull())).Times(AnyNumber());
		EXPECT_CALL(m_region_manager_mock, region_manager_get(NotNull(), _)).Times(AnyNumber());
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_init(-1));
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
	}

	TEST_F(tcc_buffer_init_fixture, tcc_buffer_init_fixture_with_cpuid_positive)
	{
		region_manager_t *FAKE_REGION_MANAGER = static_cast<region_manager_t *>(malloc(1));
		EXPECT_CALL(m_libc_mock, sched_setaffinity(_, sizeof(cpu_set_t), NotNull())).WillOnce(Return(0));
		EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(0));
		EXPECT_CALL(m_libc_mock, open(_, _, _)).WillOnce(Return(fake_file::create_fd(TCC_CONFIG_CONTENT)));
		EXPECT_CALL(m_region_manager_mock, region_manager_create(NotNull())).WillOnce(Return(FAKE_REGION_MANAGER));
		EXPECT_CALL(m_region_manager_mock, region_manager_count(NotNull())).Times(AnyNumber());
		EXPECT_CALL(m_region_manager_mock, region_manager_get(NotNull(), _)).Times(AnyNumber());
		EXPECT_CALL(m_region_manager_mock, is_tcc_buffer_exists(NotNull())).Times(AnyNumber());
		//----------------------------------------------------------

		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_init(FAKE_CPU_NUMBER - 1));
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
	}
	class tcc_buffer_test : public tcc_buffer_init_fixture
	{
	protected:
		std::vector<void *> m_property_buffers{3};
		void SetUp() override
		{
			tcc_buffer_init_fixture::SetUp();
			region_manager_t *FAKE_REGION_MANAGER = static_cast<region_manager_t *>(malloc(1));
			EXPECT_CALL(m_libc_mock, access(_, _)).WillOnce(Return(0));
			EXPECT_CALL(m_libc_mock, open(_, _, _)).WillOnce(Return(fake_file::create_fd(TCC_CONFIG_CONTENT)));
			EXPECT_CALL(m_region_manager_mock, region_manager_create(NotNull())).WillOnce(Return(FAKE_REGION_MANAGER));
			EXPECT_CALL(m_region_manager_mock, region_manager_count(NotNull())).Times(AnyNumber());
			EXPECT_CALL(m_region_manager_mock, region_manager_get(NotNull(), _)).Times(AnyNumber());
			EXPECT_CALL(m_region_manager_mock, is_tcc_buffer_exists(NotNull())).Times(AnyNumber());
			ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(-1));

			EXPECT_CALL(m_region_manager_mock, region_manager_mmap(FAKE_REGION_MANAGER, Lt(PROPERTIES.size()), _))
				.Times(AnyNumber())
				.WillRepeatedly(Invoke([&](region_manager_t *, size_t id, size_t size) {
					EXPECT_EQ(m_property_buffers[id], nullptr);
					m_property_buffers[id] = malloc(size);
					EXPECT_NE(m_property_buffers[id], nullptr);
					TCC_LOG_DEBUG("return pointer:%p with size %zu", m_property_buffers[id], size);
					return m_property_buffers[id];
				}));
		}
		void TearDown() override
		{
			EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
			for (auto buf : m_property_buffers)
			{
				free(buf);
			}
		}
	};
	TEST_F(tcc_buffer_test, tcc_buffer_init_double_init_negative)
	{
		ASSERT_EQ(-TCC_E_NOT_AVAILABLE, tcc_cache_init(-1));
	}

	TEST_F(tcc_buffer_test, tcc_buffer_set_default_latency_negative)
	{
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
		EXPECT_CALL(m_alloc_mock, calloc(_, _)).WillOnce(ReturnNull());
		EXPECT_EQ(-TCC_E_ERROR, tcc_cache_init(-1));
		EXPECT_EQ(-TCC_E_NOT_AVAILABLE, tcc_cache_set_default_latency(CONF_MAX_L2_LATENCY-1));
	}

	TEST_F(tcc_buffer_test, tcc_buffer_malloc_default_latency_negative)
	{
		EXPECT_CALL(m_libc_mock, sched_getaffinity(_, _, _)).WillOnce(Invoke([&](pid_t, size_t, cpu_set_t *set) {
			*set = get_cpu_set({1, 2, 3, 4, 5, 6, 7, 8});
			return 0;
		}));
		EXPECT_EQ(nullptr, tcc_cache_malloc_default(FAKE_SIZE));
	}
	TEST_F(tcc_buffer_test, tcc_buffer_malloc_default_latency_to_small_negative)
	{
		EXPECT_CALL(m_libc_mock, sched_getaffinity(_, _, _)).WillOnce(Invoke([&](pid_t, size_t, cpu_set_t *set) {
			*set = get_cpu_set({1, 2, 3, 4, 5, 6, 7, 8});
			return 0;
		}));
		EXPECT_EQ(nullptr, tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L2_LATENCY - 1));
	}
	TEST_F(tcc_buffer_test, tcc_buffer_malloc_get_cpu_afinity_failure_negative)
	{
		EXPECT_CALL(m_libc_mock, sched_getaffinity(_, _, _)).WillOnce(Return(-1));
		EXPECT_EQ(nullptr, tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L2_LATENCY));
	}
	TEST_F(tcc_buffer_test, tcc_buffer_calloc_get_cpu_afinity_failure_negative)
	{
		EXPECT_CALL(m_libc_mock, sched_getaffinity(_, _, _)).WillOnce(Return(-1));
		EXPECT_EQ(nullptr, tcc_cache_calloc(1, FAKE_SIZE, CONF_MAX_L2_LATENCY));
	}

	TEST_F(tcc_buffer_test, tcc_buffer_realloc_wrong_ptr_realloc_failure_negative)
	{
		EXPECT_CALL(m_alloc_mock, realloc(SOME_WRONG_BUFFER_PTR, FAKE_SIZE)).WillOnce(ReturnNull());
		EXPECT_EQ(nullptr, tcc_cache_realloc(SOME_WRONG_BUFFER_PTR, FAKE_SIZE));
	}
	TEST_F(tcc_buffer_test, tcc_buffer_free_positive)
	{
		void *ptr = malloc(FAKE_SIZE);
		ASSERT_NE(nullptr, ptr);
		tcc_cache_free(ptr);
	}
	class tcc_buffer_01_mask_test : public tcc_buffer_test
	{
	protected:
		void SetUp() override
		{
			tcc_buffer_test::SetUp();
			EXPECT_CALL(m_region_manager_mock, is_tcc_buffer_exists(NotNull())).Times(AnyNumber());
			EXPECT_CALL(m_libc_mock, sched_getaffinity(_, _, _))
				.WillOnce(
					Invoke([&](pid_t, size_t, cpu_set_t *set) {
						*set = get_cpu_set({0, 1});
						return 0;
					}));
		}
	};

	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_l2_latency_positive)
	{
		EXPECT_NE(nullptr, tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L2_LATENCY));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_less_than_l2_latency_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L2_LATENCY - 1));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_no_default_latency_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_malloc_default(FAKE_SIZE));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_default_zero_latency_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_malloc_default(FAKE_SIZE));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_default_l2_latency_positive)
	{
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_set_default_latency(CONF_MAX_L2_LATENCY));
		EXPECT_NE(nullptr, tcc_cache_malloc_default(FAKE_SIZE));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_default_l2_latency_negative)
	{
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_set_default_latency(CONF_MAX_L2_LATENCY-1));
		EXPECT_EQ(nullptr, tcc_cache_malloc_default(FAKE_SIZE));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_l2_latency_positive)
	{
		EXPECT_NE(nullptr, tcc_cache_calloc(1, FAKE_SIZE, CONF_MAX_L2_LATENCY));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_l2_latency_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_calloc(1, FAKE_SIZE, CONF_MAX_L2_LATENCY - 1));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_default_zero_latency_negative)
	{
		EXPECT_EQ(nullptr, tcc_cache_calloc_default(1, FAKE_SIZE));
	}
		TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_default_l2_latency_positive)
	{
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_set_default_latency(CONF_MAX_L2_LATENCY));
		EXPECT_NE(nullptr, tcc_cache_calloc_default(1, FAKE_SIZE));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_default_l2_latency_negative)
	{
		EXPECT_EQ(TCC_E_SUCCESS, tcc_cache_set_default_latency(CONF_MAX_L2_LATENCY-1));
		EXPECT_EQ(nullptr, tcc_cache_calloc_default(1, FAKE_SIZE));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_l3_latency_positive)
	{
		EXPECT_NE(nullptr, tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L3_LATENCY));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_malloc_less_than_l3_latency_positive)
	{
		EXPECT_NE(nullptr, tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L3_LATENCY - 1));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_l3_latency_positive)
	{
		EXPECT_NE(nullptr, tcc_cache_calloc(1, FAKE_SIZE, CONF_MAX_L3_LATENCY));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_calloc_l3_latency_negative)
	{
		EXPECT_NE(nullptr, tcc_cache_calloc(1, FAKE_SIZE, CONF_MAX_L3_LATENCY - 1));
	}
	TEST_F(tcc_buffer_01_mask_test, tcc_buffer_realloc_positive)
	{
		void *ptr = tcc_cache_malloc(FAKE_SIZE, CONF_MAX_L2_LATENCY);
		ASSERT_NE(nullptr, ptr);
		EXPECT_NE(nullptr, tcc_cache_realloc(ptr, FAKE_SIZE + 1));
	}

} // namespace
