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
#include <iostream>
#include <cstdarg>
#include <string>
#include <linux/ptp_clock.h>
#include <vector>

#include "gtest_helpers.hpp"
#include "time_helpers.hpp"
#include "time_convert.h"

///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;


namespace
{
using namespace testing;

class test_samples_common_host : public Test
{

protected:
	void SetUp() override
	{
	}
};


////////////////////////////////////////////////////////////////////////////////
//                       art <-> sys conversion tests                         //
////////////////////////////////////////////////////////////////////////////////

void prepare_test_data(ptp_clock_time dt1_sys, // duration between 1st crosst-timestamp and 2nd
					       // cross-timestamp relative to the system clock
		       ptp_clock_time dt2_sys, // duration between 1st cross-timestamp and event
					       // relative to the system clock
		       ptp_sys_offset_precise& offset_prev, ptp_sys_offset_precise& offset_cur,
		       uint64_t& event_art_ns, uint64_t& event_sys_ns)
{
	double art_sys_ratio = 2.0;
	// 1st cross-timestamp
	offset_prev = {
		{10, 100}, //art
		{20, 200}  // sys
	};
	// 2nd cross-timestamp
	offset_cur = {
		offset_prev.device + dt1_sys * art_sys_ratio, //art
		offset_prev.sys_realtime + dt1_sys            //sys
	};
	// expected event timestamp relative to the system time
	ptp_clock_time event_sys = offset_prev.sys_realtime + dt2_sys;
	// event timestamp relative to the art
	ptp_clock_time event_art = offset_prev.device + dt2_sys * art_sys_ratio;

	event_art_ns = ts2ns(event_art);
	event_sys_ns = ts2ns(event_sys);
}

TEST_F(test_samples_common_host, convert_art_sys_crossts_after_measure_positive)
{
	ptp_sys_offset_precise offset_prev, offset_cur;
	uint64_t event_art, event_sys;
	prepare_test_data({2, 200}, {1, 100}, offset_prev, offset_cur, event_art, event_sys);

	EXPECT_EQ(event_sys, convert_art_to_sys(&offset_prev, &offset_cur, event_art));
}

TEST_F(test_samples_common_host, convert_art_sys_crossts_before_measure_positive)
{
	ptp_sys_offset_precise offset_prev, offset_cur;
	uint64_t event_art, event_sys;
	prepare_test_data({1, 100}, {2, 200}, offset_prev, offset_cur, event_art, event_sys);

	EXPECT_EQ(event_sys, convert_art_to_sys(&offset_prev, &offset_cur, event_art));
}

TEST_F(test_samples_common_host, convert_sys_art_crossts_after_measure_positive)
{
	ptp_sys_offset_precise offset_prev, offset_cur;
	uint64_t event_art, event_sys;
	prepare_test_data({2, 200}, {1, 100}, offset_prev, offset_cur, event_art, event_sys);

	EXPECT_EQ(event_art, convert_sys_to_art(&offset_prev, &offset_cur, event_sys));
}

TEST_F(test_samples_common_host, convert_sys_art_crossts_before_measure_positive)
{
	ptp_sys_offset_precise offset_prev, offset_cur;
	uint64_t event_art, event_sys;
	prepare_test_data({1, 100}, {2, 200}, offset_prev, offset_cur, event_art, event_sys);

	EXPECT_EQ(event_art, convert_sys_to_art(&offset_prev, &offset_cur, event_sys));
}

}
