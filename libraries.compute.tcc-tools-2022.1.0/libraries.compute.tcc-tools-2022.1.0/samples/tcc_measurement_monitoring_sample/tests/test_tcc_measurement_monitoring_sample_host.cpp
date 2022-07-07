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
#include "args_helper.hpp"
#include "tcc_measurement_mock.hpp"
#include "time_convert.h"
#include "args_parse.h"


struct settings_t
{
	int num;
};

extern int running;
extern "C" {
	int main(int argc, char** argv);
	void print_usagesage(void);
}

///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

////////////////////////////////////////////////////////////////////////////////
// TEST FIXTURES
////////////////////////////////////////////////////////////////////////////////

namespace
{
using namespace testing;

class test_tcc_measurement_monitoring_sample_argparse : public Test
{
protected:
	settings_t sample_settings;
	void SetUp() override
	{
		optind = 0;
		sample_settings = { 0 };
	}
};

class test_tcc_measurement_monitoring_sample : public Test
{
protected:
	StrictMock<measurement_mock> m_measurement_mock;
	settings_t sample_settings;
	void SetUp() override
	{
		sample_settings = {0};
	}
};

////////////////////////////////////////////////////////////////////////////////
// COMMAND LINE ARGUMENTS PARSING TESTS
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_measurement_monitoring_sample_argparse, parse_help_positive)
{
	::Args args = {"--help"};
	EXPECT_EQ(TCC_PRINT_HELP, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_measurement_monitoring_sample_argparse, parse_help_short_positive)
{
	::Args args = {"-h"};
	EXPECT_EQ(TCC_PRINT_HELP, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_measurement_monitoring_sample_argparse, parse_invalid_flag_negative)
{
	::Args args = {"?"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
}

////////////////////////////////////////////////////////////////////////////////
// CORE FUNCTIONALITY TESTING
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_measurement_monitoring_sample, get_buffer_size_from_env_negative)
{
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_buffer_size_from_env(_))
	 	.WillOnce(Return(0));
	::Args args = {""};
	EXPECT_EQ(-TCC_E_BAD_PARAM, main(args.count(), args));
}
TEST_F(test_tcc_measurement_monitoring_sample, measurement_buffer_check_negative)
{
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_buffer_size_from_env(_))
	 	.WillOnce(Return(10000));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_time_unit_from_env())
	 	.WillOnce(Return(TCC_TU_CLK));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_deadline_from_env(_))
	 	.WillOnce(Return(1000));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_convert_time_units_to_clock( _, _))
	 	.WillOnce(Return(1000));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_buffer_init( _, _, _))
	 	.WillOnce(Return(-1));
	
	::Args args = {""};
	EXPECT_EQ(-TCC_E_ERROR, main(args.count(), args));
}

}
