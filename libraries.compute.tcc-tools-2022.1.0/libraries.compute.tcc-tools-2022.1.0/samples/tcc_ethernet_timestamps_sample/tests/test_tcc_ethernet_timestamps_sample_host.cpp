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
#include "libc_mock.hpp"
#include "tgpio_mock.hpp"
#include "time_convert.h"
#include "args_parse.h"

#define PERIOD 1000000000

typedef struct
{
	char* device;
	unsigned int pin;
} device_settings;

struct settings_t
{
	device_settings tgpio;
	device_settings pps;
	uint64_t period;
};

extern int running;
extern "C" {
	int main(int argc, char** argv);
	int run(settings_t* settings);
	void signal_handler(int dummy);
	uint64_t get_start_sys(uint64_t start_sec);
	void usage(void);
}

///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

//hack to compensate fprintf() calls in implementation
#define EXPECT_STDERR()EXPECT_CALL(m_libc_mock, fprintf(stderr,_,_)).Times(AnyNumber())\

////////////////////////////////////////////////////////////////////////////////
// TEST FIXTURES
////////////////////////////////////////////////////////////////////////////////

namespace
{
using namespace testing;

class test_tcc_ethernet_timestamps_sample_argparse : public Test
{
protected:
	settings_t sample_settings;
	void SetUp() override
	{
		optind = 0;
		sample_settings = {
			{nullptr, 0},
			{nullptr, 0},
			PERIOD
		};
	}
};

class test_tcc_ethernet_timestamps_sample : public Test
{
protected:
	StrictMock<libc_mock> m_libc_mock;
	StrictMock<tgpio_mock> m_tgpio_mock;
	settings_t sample_settings;
	void SetUp() override
	{
		EXPECT_STDERR();
		sample_settings = {
			{(char*)"/dev/ptp0", 1},
			{(char*)"/dev/ptp2", 0},
			PERIOD
		};
	}

	void expect_deinit()
	{
		EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(_, _, 0, 0, 0))
			.WillOnce(Return(-TCC_E_ERROR));
		EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(_, _, PTP_FUNC_NONE, _))
			.WillOnce(Return(-TCC_E_ERROR));
		EXPECT_CALL(m_libc_mock, close(_))
			.Times(2)
			.WillRepeatedly(Return(-1));
	}
};

////////////////////////////////////////////////////////////////////////////////
// COMMAND LINE ARGUMENTS PARSING TESTS
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_help_positive)
{
	::Args args = {"--help"};
	EXPECT_EQ(TCC_PRINT_HELP, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_help_short_positive)
{
	::Args args = {"-h"};
	EXPECT_EQ(TCC_PRINT_HELP, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_invalid_flag_negative)
{
	::Args args = {"?"};
	EXPECT_EQ(TCC_PARSE_ERROR, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_generic_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0",
				   "--period", "1000000000"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ("/dev/ptp0", sample_settings.tgpio.device);
	EXPECT_EQ(1, sample_settings.tgpio.pin);
	EXPECT_EQ("/dev/ptp2", sample_settings.pps.device);
	EXPECT_EQ(0, sample_settings.pps.pin);
	EXPECT_EQ(PERIOD, sample_settings.period);
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_verbose_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0",
				   "--period", "1000000000",
				   "--verbose"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_verbose_short_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0",
				   "--period", "1000000000",
				   "-v"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_tgpio_short_positive)
{
	::Args args = {"-t", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0",
				   "--period", "1000000000"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ("/dev/ptp0", sample_settings.tgpio.device);
	EXPECT_EQ(1, sample_settings.tgpio.pin);
	EXPECT_EQ("/dev/ptp2", sample_settings.pps.device);
	EXPECT_EQ(0, sample_settings.pps.pin);
	EXPECT_EQ(PERIOD, sample_settings.period);
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_pps_short_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "-p", "/dev/ptp2", "0",
				   "--period", "1000000000"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ("/dev/ptp0", sample_settings.tgpio.device);
	EXPECT_EQ(1, sample_settings.tgpio.pin);
	EXPECT_EQ("/dev/ptp2", sample_settings.pps.device);
	EXPECT_EQ(0, sample_settings.pps.pin);
	EXPECT_EQ(PERIOD, sample_settings.period);
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_period_short_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0",
				   "-T", "1000000000"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ("/dev/ptp0", sample_settings.tgpio.device);
	EXPECT_EQ(1, sample_settings.tgpio.pin);
	EXPECT_EQ("/dev/ptp2", sample_settings.pps.device);
	EXPECT_EQ(0, sample_settings.pps.pin);
	EXPECT_EQ(PERIOD, sample_settings.period);
}

// When pins missing they will have default values (current values from structure)
TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_pin_default_values_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0",
				   "--pps", "/dev/ptp2",
				   "--period", "1000000000"};
	sample_settings.tgpio.pin = 1;
	sample_settings.pps.pin = 0;
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ("/dev/ptp0", sample_settings.tgpio.device);
	EXPECT_EQ(1, sample_settings.tgpio.pin);
	EXPECT_EQ("/dev/ptp2", sample_settings.pps.device);
	EXPECT_EQ(0, sample_settings.pps.pin);
	EXPECT_EQ(PERIOD, sample_settings.period);
}

// One of the devices has no pin option, default is used
TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_pin_missing_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0",
				   "--pps", "/dev/ptp2", "0",
				   "--period", "1000000000"};
	sample_settings.tgpio.pin = 1;
	sample_settings.pps.pin = 0;
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ("/dev/ptp0", sample_settings.tgpio.device);
	EXPECT_EQ(1, sample_settings.tgpio.pin);
	EXPECT_EQ("/dev/ptp2", sample_settings.pps.device);
	EXPECT_EQ(0, sample_settings.pps.pin);
	EXPECT_EQ(PERIOD, sample_settings.period);
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_pps_device_missing_negative)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--period", "1000000000"};
	EXPECT_EQ(TCC_PARSE_ERROR, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_tgpio_device_missing_negative)
{
	::Args args = {"--pps", "/dev/ptp2", "0",
				   "--period", "1000000000"};
	EXPECT_EQ(TCC_PARSE_ERROR, parse_args(args.count(), args, &sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample_argparse, parse_period_missing_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0"};
	EXPECT_EQ(TCC_PARSE_SUCCESS, parse_args(args.count(), args, &sample_settings));
	EXPECT_EQ(PERIOD, sample_settings.period);
}

////////////////////////////////////////////////////////////////////////////////
// SERVICE FUNCTIONS TESTING
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_ethernet_timestamps_sample, main_func_help_argv_positive)
{
	::Args args = {"--tgpio", "/dev/ptp0", "1",
				   "--pps", "/dev/ptp2", "0",
				   "--period", "1000000000",
				   "-h"};
	EXPECT_EQ(TCC_E_SUCCESS, fake_main(args.count(), args));
}

TEST_F(test_tcc_ethernet_timestamps_sample, get_start_sys_positive)
{
	EXPECT_EQ(3000000000, get_start_sys(1));
}

TEST_F(test_tcc_ethernet_timestamps_sample, get_start_art_positive)
{
	struct ptp_sys_offset_precise offset_prev = {0};
	offset_prev.device.sec = 111;
	offset_prev.device.nsec = 222;
	offset_prev.sys_realtime.sec = 333;
	offset_prev.sys_realtime.nsec = 444;
    
	struct ptp_sys_offset_precise offset_cur = {0};
	offset_cur.device.sec = 111;
	offset_cur.device.nsec = 222;
	offset_cur.sys_realtime.sec = 333;
	offset_cur.sys_realtime.nsec = 444;

	uint64_t start_sys = get_start_sys(1);
	
	EXPECT_EQ(9223372036854775808, convert_sys_to_art(&offset_prev, &offset_cur, start_sys));
}

////////////////////////////////////////////////////////////////////////////////
// CORE FUNCTIONALITY TESTING
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_ethernet_timestamps_sample, dev1_open_negative)
{
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(-1));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, dev2_open_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(-1));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, dev1_pin_setfunc_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(-TCC_E_ERROR));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, crosstimestamp_stops_after_failed_first_timestamp_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(-TCC_E_ERROR));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, crosstimestamp_stops_after_failed_pps_perout_request_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, 0))
		.WillOnce(Return(-TCC_E_ERROR));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, crosstimestamp_stops_after_failed_second_timestamp_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(-TCC_E_ERROR));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, crosstimestamp_stops_after_failed_third_timestamp_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_libc_mock, usleep(_))
        .WillOnce(Return(0));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, crosstimestamp_stops_after_failed_tgpio_perout_request_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.Times(2)
		.WillRepeatedly(Return(0));
	EXPECT_CALL(m_libc_mock, usleep(_))
        .WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, PTP_PEROUT_ONE_SHOT))
		.WillOnce(Return(-TCC_E_ERROR));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, complete_loop_iteration_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_libc_mock, usleep(_))
        .WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, PTP_PEROUT_ONE_SHOT))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
		.WillOnce(Return(0));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, stopped_negative)
{
	int fake_fd = 123;
	running = 0;
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.pps.device), _, _))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, _, PTP_FUNC_OUT, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, _, PERIOD, _, 0))
		.WillOnce(Return(0));
	expect_deinit();

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
	running = 1;
}

TEST_F(test_tcc_ethernet_timestamps_sample, deinit_perout_request_negative)
{
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(-1));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(_, _, 0, 0, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(_, _, PTP_FUNC_NONE, _))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_libc_mock, close(_))
		.Times(2)
		.WillRepeatedly(Return(-1));

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, deinit_setfunc_negative)
{
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(-1));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(_, _, 0, 0, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(_, _, PTP_FUNC_NONE, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, close(_))
		.Times(2)
		.WillRepeatedly(Return(-1));

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, deinit_close_1_negative)
{
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(-1));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(_, _, 0, 0, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(_, _, PTP_FUNC_NONE, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, close(_))
		.WillOnce(Return(0))
		.WillOnce(Return(-TCC_E_ERROR));

	EXPECT_EQ(-TCC_E_ERROR, run(&sample_settings));
}

TEST_F(test_tcc_ethernet_timestamps_sample, deinit_positive)
{
	EXPECT_CALL(m_libc_mock, open(StrEq(sample_settings.tgpio.device), _, _))
		.WillOnce(Return(-1));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(_, _, 0, 0, 0))
		.WillOnce(Return(0));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(_, _, PTP_FUNC_NONE, _))
		.WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, close(_))
		.Times(2)
		.WillRepeatedly(Return(0));

	EXPECT_EQ(TCC_E_SUCCESS, run(&sample_settings));
}

}
