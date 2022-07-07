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
#include "clk2timespec_mock.hpp"
#include <vector>
#include <map>
#include "time_helpers.hpp"
#include <tcc/measurement.h>
#include <measurement.h>
#include <iostream>

std::ostream& operator<< (std::ostream& stream, const std::pair<uint64_t, timespec>& params)
{
        stream << "{\"clk\":"<< params.first << ",\"timespec\":" << params.second << "}";
        return stream;
}

namespace
{

    using namespace ::testing;

    class test_measurement_clk_and_timespec_host: public Test
    {
    protected:
        StrictMock<::clk2timespec_mock> m_clk2timespec_mocks;
        std::vector<std::pair<uint64_t, struct timespec>> m_data {
            {1234567890123456789UL, {123, 456}},
            {9876543210987654321UL, {654, 321}}
        };
    };

    TEST_F(test_measurement_clk_and_timespec_host, tcc_measurement_convert_clock_to_timespec_positive)
    {
        InSequence s;
        for(auto& val: m_data) {
            uint64_t& clock = val.first;
            struct timespec& ts = val.second;
            EXPECT_CALL(m_clk2timespec_mocks, clk2timespec(_)).WillOnce(Return(ts));
            EXPECT_EQ(tcc_measurement_convert_clock_to_timespec(clock), ts) << val;
        }
    }

    TEST_F(test_measurement_clk_and_timespec_host, tcc_measurement_convert_timespec_to_clock_positive)
    {
        InSequence s;
        for (auto& val: m_data) {
            uint64_t& clock = val.first;
            struct timespec& ts = val.second;
            EXPECT_CALL(m_clk2timespec_mocks, timespec2clk(_)).WillOnce(Return(clock));
            EXPECT_EQ(tcc_measurement_convert_timespec_to_clock(ts), clock) << val;
        }
    }
    //*/
} // namespace