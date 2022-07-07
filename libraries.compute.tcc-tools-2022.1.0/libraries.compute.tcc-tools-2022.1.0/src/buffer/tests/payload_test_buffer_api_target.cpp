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

/**
 * @file new_api_sample.cpp
 * @brief Intel(R) TCC Tools API test
 */


#include "tcc/cache.h"
#include "tcc/err_code.h"
#include "gtest/gtest.h"
#include <inttypes.h>
#include <exception>
#include <string>
#include "gtest_helpers.hpp"
#include "SubprocessTest.hpp"
#include "tcc_log.h"


namespace {
using namespace testing;

class TccNewApiTest : public SubprocessTest<Test> {
public:
	int8_t cpuid = 3;
	uint64_t latency_L2;
	uint64_t latency_DRAM;

	uint64_t latency_from_env(const std::string& env)
	{
		char* value = getenv(env.c_str());
		if (value == nullptr) {
			throw std::invalid_argument("Environment variable " + env + " doesn't set");
		}
		return strtoul(value, NULL, 10);
	}

	void SetUp() override
	{
		latency_L2 = latency_from_env("TEST_L2_LATENCY");
		latency_DRAM = latency_from_env("TEST_DRAM_LATENCY");
	}
	void TearDown() override
	{
		ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
	}

};

#define BUFFER_SIZE             100


SUBPROCESS_TEST_F(TccNewApiTest, full_flow_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(this->cpuid));
	void *mem = tcc_cache_malloc(BUFFER_SIZE, this->latency_L2);
	ASSERT_NE(nullptr, mem);
	tcc_cache_free(mem);
}

SUBPROCESS_TEST_F(TccNewApiTest, full_flow_with_two_tcc_buffer_malloc_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(this->cpuid));
	void *mem = tcc_cache_malloc(BUFFER_SIZE, this->latency_L2);
	ASSERT_NE(nullptr, mem);
	void *mem2 = tcc_cache_malloc(BUFFER_SIZE, this->latency_L2);
	ASSERT_NE(nullptr, mem2);
	tcc_cache_free(mem);
	tcc_cache_free(mem2);
}

SUBPROCESS_TEST_F(TccNewApiTest, full_flow_with_two_diff_latency_tcc_buffer_malloc_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(this->cpuid));
	void *mem = tcc_cache_malloc(BUFFER_SIZE, this->latency_L2);
	ASSERT_NE(nullptr, mem);
	void *mem2 = tcc_cache_malloc(BUFFER_SIZE, this->latency_DRAM);
	ASSERT_NE(nullptr, mem2);
	tcc_cache_free(mem);
	tcc_cache_free(mem2);
}

SUBPROCESS_TEST_F(TccNewApiTest, tcc_buffer_init_with_minus_one_cpuid_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(-1));
}

SUBPROCESS_TEST_F(TccNewApiTest, tcc_buffer_init_with_invalid_cpuid_negative)
{
	ASSERT_EQ(-TCC_E_BAD_PARAM, tcc_cache_init(-10));
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
	ASSERT_EQ(-TCC_E_BAD_PARAM, tcc_cache_init(127));
}

SUBPROCESS_TEST_F(TccNewApiTest, tcc_buffer_init_double_call_negative)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(-1));
	ASSERT_EQ(-TCC_E_NOT_AVAILABLE, tcc_cache_init(-1));
}

SUBPROCESS_TEST_F(TccNewApiTest, tcc_buffer_finish_without_init_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_finish());
}


SUBPROCESS_TEST_F(TccNewApiTest, full_flow_with_tcc_buffer_malloc_default_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(this->cpuid));
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_set_default_latency(this->latency_L2));
	void *mem = tcc_cache_malloc_default(BUFFER_SIZE);
	ASSERT_NE(nullptr, mem);
	tcc_cache_free(mem);
}

SUBPROCESS_TEST_F(TccNewApiTest, full_flow_with_tcc_buffer_realloc_positive)
{
	ASSERT_EQ(TCC_E_SUCCESS, tcc_cache_init(this->cpuid));
	void *mem = tcc_cache_malloc(BUFFER_SIZE, this->latency_L2);
	ASSERT_NE(nullptr, mem);
	mem = tcc_cache_realloc(mem, 2 * BUFFER_SIZE);
	ASSERT_NE(nullptr, mem);
	tcc_cache_free(mem);
}

}
