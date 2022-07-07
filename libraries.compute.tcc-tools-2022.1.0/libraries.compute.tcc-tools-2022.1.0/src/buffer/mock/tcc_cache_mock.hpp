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

#ifndef TCC_CACHE_MOCK_H
#define TCC_CACHE_MOCK_H

#include "mock_common.hpp"
#include "gmock/gmock.h"

struct tcc_cache_mock {
    MOCK_METHOD1(tcc_cache_init, int (int8_t));
    MOCK_METHOD0(tcc_cache_finish, int());
    MOCK_METHOD1(tcc_cache_free, void (void*));
    MOCK_METHOD2(tcc_cache_malloc, void* (size_t, uint64_t));

    COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(tcc_cache_mock);
};

#endif // TCC_CACHE_MOCK_H
