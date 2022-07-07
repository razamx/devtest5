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

extern "C" {
#include "params.h"
}
namespace
{

using namespace ::testing;

class test_cache_lock_benchmark_params_host: public Test
{
};

TEST_F(test_cache_lock_benchmark_params_host, default_positive)
{
    char* params[] = {const_cast<char*>("program"), nullptr};
    EXPECT_NE(-1, parse_params(1, params));
}

}  // namespace
