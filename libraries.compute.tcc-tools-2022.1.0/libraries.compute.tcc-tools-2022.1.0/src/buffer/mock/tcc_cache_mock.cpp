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

#include "tcc_cache_mock.hpp"

#include "tcc/cache.h"

using namespace ::testing;

COMMON_MOCK_INTERFACE_IMPLEMENTATION(tcc_cache_mock)

void tcc_cache_mock::init()
{
    ON_CALL(*this, tcc_cache_init(_)).WillByDefault(Return(TCC_E_ERROR));
    ON_CALL(*this, tcc_cache_finish()).WillByDefault(Return(TCC_E_ERROR));
    ON_CALL(*this, tcc_cache_free(_)).WillByDefault(::testing::Invoke(free));
    ON_CALL(*this, tcc_cache_malloc(_, _)).WillByDefault(ReturnNull());
}

int tcc_cache_init(int8_t cpu_id)
{
    return tcc_cache_mock::get().tcc_cache_init(cpu_id);
}

int tcc_cache_finish(void)
{
    return tcc_cache_mock::get().tcc_cache_finish();
}

void tcc_cache_free(void* ptr)
{
    return tcc_cache_mock::get().tcc_cache_free(ptr);
}

void* tcc_cache_malloc(size_t size, uint64_t latency)
{
    return tcc_cache_mock::get().tcc_cache_malloc(size, latency);
}
