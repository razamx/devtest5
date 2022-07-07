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

#include "libc_mock.hpp"
#include "TccDriverFixture.hpp"
#include "SubprocessTest.hpp"
#include "tcc_buffer_test_helpers.hpp"
#include "tcc_time_fake.hpp"

namespace
{
	using namespace testing;
	using namespace tcc_buffer_test_utils;

	class TestBase: public Test
	{
	protected:
		StrictMock<tcc_cpuid_mock> m_tcc_cpuid_mock;
	};

	class TestTccDriverHelpersBase : public
		TccDriverFixture<
			ProcessorFixture<
				ConfigFixture<
					LibcMockFixture<
							CheckSetUpTearDownSeqFixture<
								tcc_time_fake<
									TestBase
							>>>>>>{};

	class TestTccDriverHelpers : public TestTccDriverHelpersBase
	{

	protected:
		void SetUp()
		{
			TestTccDriverHelpersBase::SetUp();
		}
		void check_memory(void *mem, size_t size)
		{
			// check that no segfault
			memset(mem, 0, size);
		}

		bool check_memory_zero(void *mem, size_t size)
		{
			uint8_t *ptr = (uint8_t *)mem;
			for (size_t i = 0; i < size; i++)
			{
				if (ptr[i] != 0)
					return false;
			}
			return true;
		}

		std::unique_ptr<char[]> generate_data(size_t size)
		{
			auto ptr = std::unique_ptr<char[]>(new char[size]);
			for (size_t i = 0; i < size; i++)
			{
				ptr[i] = i;
			}
			return ptr;
		}
	};
	////////////////////////////////////////////////////////////////////////////////
	/// Tests for initialization & deinitialization
	////////////////////////////////////////////////////////////////////////////////

	class test_tcc_driver_init_deinit : public TestTccDriverHelpers
	{
	protected:
		void TearDown() override
		{
			EXPECT_EQ(tcc_cache_finish(), 0);
			TestTccDriverHelpers::TearDown();
		}
	};

	TEST_F(test_tcc_driver_init_deinit, default_settings_positive)
	{

		// Get the initial affinity
		cpu_set_t affinity_before;
		ASSERT_EQ(0, sched_getaffinity(0, sizeof(cpu_set_t), &affinity_before));

		int ret = tcc_cache_init(-1);
		ASSERT_EQ(TCC_E_SUCCESS, ret);

		// Check that process affinity hasn't changed
		cpu_set_t affinity_after;
		ASSERT_EQ(0, sched_getaffinity(0, sizeof(cpu_set_t), &affinity_after));
		EXPECT_TRUE(CPU_EQUAL(&affinity_before, &affinity_after));
	}

	TEST_F(test_tcc_driver_init_deinit, with_cpuid_is_to_small_negative)
	{

		int ret = tcc_cache_init(-2);
		EXPECT_NE(TCC_E_SUCCESS, ret);
	}
	TEST_F(test_tcc_driver_init_deinit, cpuid_is_zero_positive)
	{

		unsigned cpuid = NUM_PROCS - 1;
		cpu_set_t expect_affinity_after = cpu_set_from_list({cpuid});

		int ret = tcc_cache_init(cpuid);
		EXPECT_EQ(TCC_E_SUCCESS, ret);

		// Check that affinity set to the expected
		cpu_set_t affinity_after;
		ASSERT_EQ(0, sched_getaffinity(0, sizeof(cpu_set_t), &affinity_after));
		EXPECT_TRUE(CPU_EQUAL(&affinity_after, &expect_affinity_after));
	}

	TEST_F(test_tcc_driver_init_deinit, cpuid_is_maximum_positive)
	{

		unsigned cpuid = NUM_PROCS - 1;
		cpu_set_t expect_affinity_after = cpu_set_from_list({cpuid});

		int ret = tcc_cache_init(cpuid);
		EXPECT_EQ(TCC_E_SUCCESS, ret);

		// Check that affinity set to the expected
		cpu_set_t affinity_after;
		ASSERT_EQ(0, sched_getaffinity(0, sizeof(cpu_set_t), &affinity_after));
		EXPECT_TRUE(CPU_EQUAL(&affinity_after, &expect_affinity_after));
	}

	TEST_F(test_tcc_driver_init_deinit, with_cpuid_is_to_big_negative)
	{

		int ret = tcc_cache_init((int8_t)NUM_PROCS);
		EXPECT_NE(TCC_E_SUCCESS, ret);
	}

	TEST_F(test_tcc_driver_init_deinit, with_negative_cpuid_positive)
	{

		int ret = tcc_cache_init(-1);
		EXPECT_EQ(TCC_E_SUCCESS, ret);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Tests for memory functions without default latency
	////////////////////////////////////////////////////////////////////////////////

	class test_tcc_driver_memory : public TestTccDriverHelpers
	{
	protected:
		void SetUp() override
		{

			TestTccDriverHelpers::SetUp();
			int ret = tcc_cache_init((int8_t)DEFAULT_CPUID);
			ASSERT_EQ(0, ret);
			TCC_LOG_INFO("---- TEST BEGIN ----");
		}

		void TearDown() override
		{

			TCC_LOG_INFO("---- TEST END ----");
			TestTccDriverHelpers::TearDown();
			int ret = tcc_cache_finish();
			EXPECT_EQ(TCC_E_SUCCESS, ret);
		}
	};

	TEST_F(test_tcc_driver_memory, malloc_dram_positive)
	{
		void *mem = tcc_cache_malloc(ALLOC_SIZE, LATENCY_DRAM);
		ASSERT_NE(nullptr, mem);
		check_memory(mem, ALLOC_SIZE);
		tcc_cache_free(mem);
	}

	TEST_F(test_tcc_driver_memory, malloc_cache_positive)
	{

		void *mem = tcc_cache_malloc(ALLOC_SIZE, LATENCY_DEFAULT_CACHE);
		ASSERT_NE(nullptr, mem);
		check_memory(mem, ALLOC_SIZE);
		tcc_cache_free(mem);
	}

	TEST_F(test_tcc_driver_memory, malloc_too_small_latency_negative)
	{

		void *mem = tcc_cache_malloc(ALLOC_SIZE, LATENCY_TOO_SMALL);
		ASSERT_EQ(nullptr, mem);
	}

	TEST_F(test_tcc_driver_memory, calloc_single_allocation_dram_positive)
	{

		void *mem = tcc_cache_calloc(ALLOC_SIZE / 4, 4, LATENCY_DRAM);
		ASSERT_NE(nullptr, mem);
		check_memory(mem, ALLOC_SIZE);
		check_memory_zero(mem, ALLOC_SIZE);
		tcc_cache_free(mem);
	}

	TEST_F(test_tcc_driver_memory, calloc_single_allocation_cache_positive)
	{

		void *mem = tcc_cache_calloc(ALLOC_SIZE / 4, 4, LATENCY_DEFAULT_CACHE);
		ASSERT_NE(nullptr, mem);
		check_memory(mem, ALLOC_SIZE);
		check_memory_zero(mem, ALLOC_SIZE);
		tcc_cache_free(mem);
	}

	TEST_F(test_tcc_driver_memory, calloc_too_small_latency_negative)
	{

		void *mem = tcc_cache_calloc(ALLOC_SIZE / 4, 4, LATENCY_TOO_SMALL);
		ASSERT_EQ(nullptr, mem);
	}

	TEST_F(test_tcc_driver_memory, realloc_dram_positive)
	{

		auto data = generate_data(ALLOC_SIZE);

		void *mem1 = tcc_cache_malloc(ALLOC_SIZE, LATENCY_DRAM);
		ASSERT_NE(nullptr, mem1);
		check_memory(mem1, ALLOC_SIZE);
		memcpy(mem1, data.get(), ALLOC_SIZE);

		void *mem2 = tcc_cache_realloc(mem1, ALLOC_SIZE * 2);
		ASSERT_NE(nullptr, mem2);
		ASSERT_EQ(0, memcmp(mem2, data.get(), 16));

		tcc_cache_free(mem2);
	}

	TEST_F(test_tcc_driver_memory, realloc_cache_positive)
	{

		void *mem1 = tcc_cache_malloc(ALLOC_SIZE, LATENCY_DEFAULT_CACHE);
		ASSERT_NE(nullptr, mem1);
		check_memory(mem1, ALLOC_SIZE);

		void *mem2 = tcc_cache_realloc(mem1, ALLOC_SIZE * 2);
		ASSERT_NE(nullptr, mem2);
		check_memory(mem2, ALLOC_SIZE);
		tcc_cache_free(mem2);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Tests for memory functions with default latency
	////////////////////////////////////////////////////////////////////////////////

	class test_tcc_driver_memory_with_default_latency : public TestTccDriverHelpers
	{
	protected:
		void SetUp() override
		{

			TestTccDriverHelpers::SetUp();
			int ret = tcc_cache_init((int8_t)DEFAULT_CPUID);
			ASSERT_EQ(0, ret);
			TCC_LOG_INFO("---- TEST BEGIN ----");
		}

		void TearDown() override
		{

			TCC_LOG_INFO("---- TEST END ----");
			int ret = tcc_cache_finish();
			TestTccDriverHelpers::TearDown();
			EXPECT_EQ(TCC_E_SUCCESS, ret);
		}
	};

	TEST_F(test_tcc_driver_memory_with_default_latency, malloc_positive)
	{

		tcc_cache_set_default_latency(LATENCY_DEFAULT_CACHE);
		void *mem = tcc_cache_malloc_default(ALLOC_SIZE);
		ASSERT_NE(nullptr, mem);
		check_memory(mem, ALLOC_SIZE);
		tcc_cache_free(mem);
	}

	TEST_F(test_tcc_driver_memory_with_default_latency, calloc_positive)
	{
		tcc_cache_set_default_latency(LATENCY_DEFAULT_CACHE);
		void *mem = tcc_cache_calloc_default(ALLOC_SIZE / 4, 4);
		ASSERT_NE(nullptr, mem);
		check_memory(mem, ALLOC_SIZE);
		tcc_cache_free(mem);
	}

} // namespace