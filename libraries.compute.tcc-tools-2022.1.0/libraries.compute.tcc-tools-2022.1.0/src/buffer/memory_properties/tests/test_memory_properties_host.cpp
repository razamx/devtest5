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
#include <gmock/gmock.h>
#include "memory_properties.h"
#include "memory_properties_test_utils.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

namespace
{
using namespace testing;
using ::testing::Mock;

class test_memory_properties : public Test
{
};

TEST_F(test_memory_properties, param_a_null_negative)
{
	auto b = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(0, memory_properties_compare(nullptr, &b));
}

TEST_F(test_memory_properties, param_b_null_negative)
{
	auto a = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(0, memory_properties_compare(&a, nullptr));
}

TEST_F(test_memory_properties, bigger_latency_smaller_mask_gets_minus_1_positive)
{
	auto a = prop(.latency=100, .mask=0b10, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(-1, memory_properties_compare(&a, &b));
}

TEST_F(test_memory_properties, bigger_latency_bigger_mask_gets_minus_1_positive)
{
	auto a = prop(.latency=100, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=10, .mask=0b10, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(-1, memory_properties_compare(&a, &b));
}

TEST_F(test_memory_properties, smaller_latency_smaller_mask_gets_1_positive)
{
	auto a = prop(.latency=10, .mask=0b10, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=100, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(1, memory_properties_compare(&a, &b));
}

TEST_F(test_memory_properties, smaller_latency_bigger_mask_gets_minus_minus_1_positive)
{
	auto a = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=100, .mask=0b10, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(1, memory_properties_compare(&a, &b));
}

TEST_F(test_memory_properties, equal_latency_smaller_mask_gets_minus_1_positive)
{
	auto a = prop(.latency=10, .mask=0b10, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(-1, memory_properties_compare(&a, &b));
}

TEST_F(test_memory_properties, equal_latency_bigger_mask_gets_1_positive)
{
	auto a = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=10, .mask=0b10, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(1, memory_properties_compare(&a, &b));
}

TEST_F(test_memory_properties, equal_latency_equal_mask_gets_0_positive)
{
	auto a = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	auto b = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	EXPECT_EQ(0, memory_properties_compare(&a, &b));
}

}
