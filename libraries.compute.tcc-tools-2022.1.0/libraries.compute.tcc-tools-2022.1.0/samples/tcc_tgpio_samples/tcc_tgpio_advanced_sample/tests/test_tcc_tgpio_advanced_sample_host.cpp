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
#include "alloc_mock.hpp"
#include "tgpio_mock.hpp"
#include "swgpio_mock.hpp"
#include "tcc/err_code.h"
#include <iostream>
#include <cstdarg>
#include <string>
#include <linux/ptp_clock.h>
#include <vector>


#include "tgpio_sample.h"
#include "swgpio.h"
#include "tgpio.h"

#include "gtest_helpers.hpp"
#include "time_helpers.hpp"
#include "args_helper.hpp"

#define TIMESHIFT 2

extern int running;
extern "C" {
	int parse_args(int argc, char** argv, settings_t* sample_setting);
	int parse_periods_config(const char* file, period_pin_config** config, size_t* pins_count, Mode mode);
	int set_rt();
	int tgpio_out(period_pin_config* config, size_t pins_count);
	int swgpio_out(period_pin_config* config, size_t pins_count);
	int swgpio_in(settings_t* sample_settingss);
	int tgpio_in(settings_t* sample_settingss);
};


///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

////////////////////////////////////////////////////////////////////////////////
// HELPERS
///////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// GTEST EXPECT/MATCH/ETC HELPERS
//------------------------------------------------------------------------------

#define TIMESPEC_COMP_EPS 0.000000001

//hack to compensate fprintf() calls in implementation
#define EXPECT_STDERR()EXPECT_CALL(m_libc_mock, fprintf(stderr,_,_)).Times(AnyNumber())

// Checks output to the log file
#define fprintfLogEq(expect_timestamp, expect_pin) Invoke([expect_timestamp, expect_pin](FILE*, const char* format, va_list args)\
{ \
	char buffer[1024]; \
	vsnprintf(buffer, 1024, format, args); \
	char expect_buffer[1024]; \
	sprintf(expect_buffer, "%.9lf, %d\n", expect_timestamp, expect_pin); \
	EXPECT_STREQ(expect_buffer, buffer); \
	return strlen(buffer); \
})


//------------------------------------------------------------------------------
// ART - TSC CONVERSION HELPERS
//------------------------------------------------------------------------------

const double ART_TO_TSC_RATIO = 2;  // art clock faster then tsc clock
const int ART_TO_TSC_OFFSET = 10;   // offset between art clock and tsc clock
const size_t WAIT_DELAY = BILLION;  // delay between two time measurements

// calculates timestamps for PTP_SYS_OFFSET_PRECISE2 ioctls
void calc_timestamps(uint64_t tsc1, uint64_t& tsc2, uint64_t& art1, uint64_t& art2)
{
	tsc2 = tsc1 + WAIT_DELAY;
	art1 = tsc1 - ART_TO_TSC_OFFSET;
	art2 = tsc1 + WAIT_DELAY * ART_TO_TSC_RATIO - ART_TO_TSC_OFFSET;
}

// calculates signal generation start timestamp (art clock)
// Begining of the next+1 second + offset from config
uint64_t calc_start_art(uint64_t tsc1, uint64_t art1, uint64_t config_start)
{
	uint64_t start_tsc = set_ns_to_zero(tsc1) + 3 * BILLION + config_start;
	uint64_t delta = start_tsc - tsc1;
	return art1 + delta * ART_TO_TSC_RATIO;
}

////////////////////////////////////////////////////////////////////////////////
// TEST FIXTURES
////////////////////////////////////////////////////////////////////////////////

namespace
{
using namespace testing;

class test_tcc_gpio_sample_host : public Test
{
protected:
	StrictMock<libc_mock> m_libc_mock;
	StrictMock<tgpio_mock> m_tgpio_mock;
	StrictMock<swgpio_mock> m_swgpio_mock;
	NiceMock<alloc_mock> m_alloc_mock;

	void SetUp() override
	{
		EXPECT_STDERR();
	}
};

class test_tcc_gpio_sample_config_parse : public Test
{
protected:
	StrictMock<libc_mock> m_libc_mock;
	NiceMock<alloc_mock> m_alloc_mock;

	void SetUp() override
	{
		EXPECT_STDERR();
	}
};

class test_tcc_gpio_sample_argparse : public Test
{
protected:
	settings_t sample_setting;

	void SetUp() override
	{
		optind = 0;
		sample_setting = init_settings();
	}

	struct argument {
		char* key;
		char* good_argument;
		char* bad_argument;
	};

	settings_t init_settings()
	{
		return {
			NULL,
			SCENARIO_UNKNOWN,
			MODE_UNKNOWN,
		};
	}

	/* Negative argument parse testing.
	 * Check every argument: [no argument at all, no value, bad value] when
	 * other is correct
	 */
	void negative_arguments_test(const std::vector<argument>& arguments)
	{
		settings_t sample_setting;
		for (size_t testing_i = 0; testing_i < arguments.size(); testing_i++) {
			// No key at all
			::Args args;
			for (size_t i = 0; i < arguments.size(); i++) {
				if (i != testing_i) {
					args.push(arguments[i].key);
					args.push(arguments[i].good_argument);
				}
			}
			sample_setting = init_settings();
			optind = 0;
			EXPECT_EQ(-TCC_E_BAD_PARAM, parse_args(args.count(), args, &sample_setting));

			// No argument
			args.clear();
			for (size_t i = 0; i < arguments.size(); i++) {
				args.push(arguments[i].key);
				if (i != testing_i) {
					args.push(arguments[i].good_argument);
				}
			}
			sample_setting = init_settings();
			optind = 0;
			EXPECT_EQ(-TCC_E_BAD_PARAM, parse_args(args.count(), args, &sample_setting));

			// Bad argument
			args.clear();
			for(size_t i = 0; i < arguments.size(); i++) {
				args.push(arguments[i].key);
				if(i != testing_i) {
					args.push(arguments[i].good_argument);
				} else {
					args.push(arguments[i].bad_argument);
				}
			}
			sample_setting = init_settings();
			optind = 0;
			EXPECT_EQ(-TCC_E_BAD_PARAM, parse_args(args.count(), args, &sample_setting));
		}
	}
};

class test_tcc_gpio_sample_set_rt : public Test
{
protected:
	StrictMock<libc_mock> m_libc_mock;

	void SetUp() override
	{
		EXPECT_STDERR();
	}
};

/////////////////////////////////////////////////
//        command line parse tests             //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_argparse, parse_args_help_positive)
{
	::Args args = {"--help"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
}

TEST_F(test_tcc_gpio_sample_argparse, parse_args_help_short_positive)
{
	::Args args = {"-h"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
}

TEST_F(test_tcc_gpio_sample_argparse, parse_args_output_soft_positive)
{
	::Args args = {"--mode", "soft",
				   "--signal_file", "file.txt"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
	EXPECT_EQ(SCENARIO_OUTPUT, sample_setting.scenario);
	EXPECT_EQ(MODE_SW_GPIO, sample_setting.mode);
	EXPECT_EQ("file.txt", sample_setting.signal_file);
}

TEST_F(test_tcc_gpio_sample_argparse, parse_args_output_tgpio_positive)
{
	::Args args = {"--mode", "tgpio",
				   "--signal_file", "file.txt"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
	EXPECT_EQ(SCENARIO_OUTPUT, sample_setting.scenario);
	EXPECT_EQ(MODE_TGPIO, sample_setting.mode);
	EXPECT_EQ("file.txt", sample_setting.signal_file);
}

TEST_F(test_tcc_gpio_sample_argparse, parse_args_scenario_short_positive)
{
	::Args args = {"--mode", "tgpio",
				   "--signal_file", "file.txt"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
	EXPECT_EQ(SCENARIO_OUTPUT, sample_setting.scenario);
	EXPECT_EQ(MODE_TGPIO, sample_setting.mode);
	EXPECT_EQ("file.txt", sample_setting.signal_file);
}

TEST_F(test_tcc_gpio_sample_argparse, parse_args_mode_short_positive)
{
	::Args args = {"-m", "tgpio",
				   "--signal_file", "file.txt"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
	EXPECT_EQ(SCENARIO_OUTPUT, sample_setting.scenario);
	EXPECT_EQ(MODE_TGPIO, sample_setting.mode);
	EXPECT_EQ("file.txt", sample_setting.signal_file);
}

TEST_F(test_tcc_gpio_sample_argparse, parse_args_signal_file_short_positive)
{
	::Args args = {"--mode", "tgpio",
				   "-c", "file.txt"};
	EXPECT_EQ(TCC_E_SUCCESS, parse_args(args.count(), args, &sample_setting));
	EXPECT_EQ(SCENARIO_OUTPUT, sample_setting.scenario);
	EXPECT_EQ(MODE_TGPIO, sample_setting.mode);
	EXPECT_EQ("file.txt", sample_setting.signal_file);
}


/////////////////////////////////////////////////
//        periods config parse tests           //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_host, parse_periods_soft_signal_config_positive)
{
	char fake_config[] = "666 4 10101\n";
	FILE *fake_config_file = fmemopen(fake_config, strlen(fake_config), "r");
	ASSERT_NE(nullptr, fake_config_file);

	EXPECT_CALL(m_libc_mock, fopen(StrEq("/path/config"), StrEq("r"))).WillOnce(Return(fake_config_file));
	EXPECT_CALL(m_libc_mock, fclose(_)).Times(1);

	period_pin_config *config = nullptr;
	size_t pins_count = 0;
	EXPECT_EQ(TCC_E_SUCCESS, parse_periods_config("/path/config", &config, &pins_count, MODE_SW_GPIO));
	ASSERT_NE(nullptr, config);
	ASSERT_EQ(1, pins_count);

	EXPECT_EQ(666, config[0].pin);
	EXPECT_EQ(4, config[0].offset);
	EXPECT_EQ(10101, config[0].period);

	free(config);
}


TEST_F(test_tcc_gpio_sample_host, parse_periods_soft_two_signal_config_positive)
{
	char fake_config[] =
		"666 4 10101\n"	\
		"777 8 12345\n";
	FILE *fake_config_file = fmemopen(fake_config, strlen(fake_config), "r");
	ASSERT_NE(nullptr, fake_config_file);

	EXPECT_CALL(m_libc_mock, fopen(StrEq("/path/config"), StrEq("r"))).WillOnce(Return(fake_config_file));
	EXPECT_CALL(m_libc_mock, fclose(fake_config_file)).Times(1);
	period_pin_config *config = nullptr;
	size_t pins_count = 0;
	EXPECT_EQ(TCC_E_SUCCESS, parse_periods_config("/path/config", &config, &pins_count, MODE_SW_GPIO));
	ASSERT_NE(nullptr, config);
	ASSERT_EQ(2, pins_count);

	EXPECT_EQ(666, config[0].pin);
	EXPECT_EQ(4, config[0].offset);
	EXPECT_EQ(10101, config[0].period);

	EXPECT_EQ(777, config[1].pin);
	EXPECT_EQ(8, config[1].offset);
	EXPECT_EQ(12345, config[1].period);

	free(config);
}


TEST_F(test_tcc_gpio_sample_host, parse_periods_tgpio_signal_config_positive)
{
	char fake_config[] = "666 4 10101 13 /dev/ice\n";
	FILE *fake_config_file = fmemopen(fake_config, strlen(fake_config), "r");
	ASSERT_NE(nullptr, fake_config_file);

	EXPECT_CALL(m_libc_mock, fopen(StrEq("/path/config"), StrEq("r"))).WillOnce(Return(fake_config_file));
	EXPECT_CALL(m_libc_mock, fclose(fake_config_file)).Times(1);

	period_pin_config *config = nullptr;;
	size_t pins_count = 0;
	EXPECT_EQ(TCC_E_SUCCESS, parse_periods_config("/path/config", &config, &pins_count, MODE_TGPIO));
	ASSERT_NE(nullptr, config);
	ASSERT_EQ(1, pins_count);

	EXPECT_EQ(666, config[0].pin);
	EXPECT_EQ(4, config[0].offset);
	EXPECT_EQ(10101, config[0].period);
	EXPECT_EQ(13, config[0].channel);
	EXPECT_EQ(0, strncmp("/dev/ice", config[0].device, 16));

	free(config);
}


TEST_F(test_tcc_gpio_sample_host, parse_periods_tgpio_two_signal_config_positive)
{
	char fake_config[] =
		"666 4 10101 13 /dev/ice\n"	\
		"777 8 12345 42 /dev/fake\n";
	FILE *fake_config_file = fmemopen(fake_config, strlen(fake_config), "r");
	ASSERT_NE(nullptr, fake_config_file);

	EXPECT_CALL(m_libc_mock, fopen(StrEq("/path/config"), StrEq("r"))).WillOnce(Return(fake_config_file));
	EXPECT_CALL(m_libc_mock, fclose(fake_config_file)).Times(1);
	period_pin_config *config = nullptr;;
	size_t pins_count = 0;
	EXPECT_EQ(TCC_E_SUCCESS, parse_periods_config("/path/config", &config, &pins_count, MODE_TGPIO));
	ASSERT_NE(nullptr, config);
	ASSERT_EQ(2, pins_count);

	EXPECT_EQ(666, config[0].pin);
	EXPECT_EQ(4, config[0].offset);
	EXPECT_EQ(10101, config[0].period);
	EXPECT_EQ(13, config[0].channel);
	EXPECT_EQ(0, strncmp("/dev/ice", config[0].device, 16));

	EXPECT_EQ(777, config[1].pin);
	EXPECT_EQ(8, config[1].offset);
	EXPECT_EQ(12345, config[1].period);
	EXPECT_EQ(42, config[1].channel);
	EXPECT_EQ(0, strncmp("/dev/fake", config[1].device, 16));

	free(config);
}


TEST_F(test_tcc_gpio_sample_host, parse_periods_config_null_args_negative)
{
	const char *fake_file = "/path/config";
	period_pin_config *fake_config = nullptr;
	size_t fake_pins_count = 0;

	EXPECT_EQ(-TCC_E_BAD_PARAM, parse_periods_config(nullptr, &fake_config, &fake_pins_count, MODE_TGPIO));
	EXPECT_EQ(-TCC_E_BAD_PARAM, parse_periods_config(fake_file, nullptr, &fake_pins_count, MODE_TGPIO));
	EXPECT_EQ(-TCC_E_BAD_PARAM, parse_periods_config(fake_file, &fake_config, nullptr, MODE_TGPIO));

	free(fake_config);
}


TEST_F(test_tcc_gpio_sample_host, parse_periods_config_invalid_malloc_negative)
{
	char fake_config[] = "666 4 10101\n";
	FILE *fake_config_file = fmemopen(fake_config, strlen(fake_config), "r");
	ASSERT_NE(nullptr, fake_config_file);

	period_pin_config *config = nullptr;;
	size_t fake_pins_count = 0;
	EXPECT_CALL(m_libc_mock, fopen(StrEq("/path/config"), StrEq("r"))).WillOnce(Return(fake_config_file));
	EXPECT_CALL(m_libc_mock, fclose(fake_config_file)).Times(1);
	EXPECT_CALL(m_alloc_mock, malloc(_)).WillOnce(ReturnNull());
	EXPECT_EQ(-TCC_E_MEMORY, parse_periods_config("/path/config", &config, &fake_pins_count, MODE_TGPIO));
}


TEST_F(test_tcc_gpio_sample_host, parse_periods_config_invalid_config_format_negative)
{
	char fake_config[] = "13\n";
	FILE *fake_config_file = fmemopen(fake_config, strlen(fake_config), "r");
	ASSERT_NE(nullptr, fake_config_file);
	EXPECT_CALL(m_libc_mock, fopen(StrEq("/path/config"), StrEq("r"))).WillOnce(Return(fake_config_file));
	EXPECT_CALL(m_libc_mock, fclose(fake_config_file)).Times(1);
	period_pin_config *config = (period_pin_config*)0xDEADBEEF;
	size_t pins_count = 111;
	EXPECT_EQ(-TCC_E_BAD_PARAM, parse_periods_config("/path/config", &config, &pins_count, MODE_SW_GPIO));
	EXPECT_EQ(nullptr, config);
	EXPECT_EQ(0, pins_count);
}

//////////////////////////////////////////////
//              set_rt tests                //
//////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_set_rt, set_rt_negative1)
{
	EXPECT_CALL(m_libc_mock, mlockall(MCL_CURRENT | MCL_FUTURE)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, sched_setaffinity(0, sizeof(cpu_set_t), _)).WillOnce(Return(1));
	EXPECT_EQ(-TCC_E_ERROR, set_rt());
}

TEST_F(test_tcc_gpio_sample_set_rt, set_rt_negative2)
{
	EXPECT_CALL(m_libc_mock, mlockall(MCL_CURRENT | MCL_FUTURE)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, sched_setaffinity(0, sizeof(cpu_set_t), _)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, sched_setscheduler(0, SCHED_FIFO, _)).WillOnce(Return(1));
	EXPECT_EQ(-TCC_E_ERROR, set_rt());
}

TEST_F(test_tcc_gpio_sample_set_rt, set_rt_positive)
{
	EXPECT_CALL(m_libc_mock, mlockall(MCL_CURRENT | MCL_FUTURE)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, sched_setaffinity(0, sizeof(cpu_set_t), _)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, sched_setscheduler(0, SCHED_FIFO, _)).WillOnce(Return(0));
	EXPECT_EQ(TCC_E_SUCCESS, set_rt());
}

/////////////////////////////////////////////////
//              tgpio_out tests                //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_host, tgpio_out_null_config_negative)
{
	EXPECT_EQ(-TCC_E_BAD_PARAM, tgpio_out(nullptr, 5));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_calloc_negative)
{
	std::vector<period_pin_config> config = {period_pin_config{1, 1000, 1000000, 1, "/dev/ptp0"}};

	EXPECT_CALL(m_alloc_mock, calloc(_,_))
		.WillOnce(ReturnNull());

	EXPECT_EQ(-TCC_E_MEMORY, tgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_open_fails_negative)
{
	int err_fd = -1;
	std::vector<period_pin_config> config = {period_pin_config{1, 1000, 1000000, 1, "/dev/ptp0"}};

	// Open PTP device failed
	EXPECT_CALL(m_libc_mock, open(StrEq(config[0].device),_,_))
		.WillOnce(Return(-1));

	// deinit
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(err_fd, config[0].channel, 0, 0, 0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(err_fd, 1, PTP_FUNC_NONE, config[0].channel))
		.WillOnce(Return(-TCC_E_ERROR));

	// Close PTP device
	EXPECT_CALL(m_libc_mock, close(err_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_NOT_AVAILABLE, tgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_cross_timestamp1_fails_negative)
{
	int fake_fd = 123;
	std::vector<period_pin_config> config = {period_pin_config{1, 1000, 1000000, 1, "/dev/ptp0"}};

	// Open PTP device
	EXPECT_CALL(m_libc_mock, open(StrEq(config[0].device),_,_))
		.WillOnce(Return(fake_fd));

	// Get current time calc relation between clocks (failed)
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(-TCC_E_NOT_AVAILABLE));

	// deinit
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, config[0].channel, 0, 0,0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_NONE, config[0].channel))
		.WillOnce(Return(-TCC_E_ERROR));

	// Close PTP device
	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_NOT_AVAILABLE, tgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_cross_timestamp2_fails_negative)
{
	int fake_fd = 123;
	std::vector<period_pin_config> config = {period_pin_config{1, 1000, 1000000, 1, "/dev/ptp0"}};

	// Open PTP device
	EXPECT_CALL(m_libc_mock, open(StrEq(config[0].device),_,_))
		.WillOnce(Return(fake_fd));

	// Get current time calc relation between clocks (failed)
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(TCC_E_SUCCESS))
		.WillOnce(Return(-TCC_E_NOT_AVAILABLE));
	EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
		.WillOnce(Return(0));

	// deinit
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, config[0].channel, 0, 0, 0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_NONE, config[0].channel))
		.WillOnce(Return(-TCC_E_ERROR));

	// Close PTP device
	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_NOT_AVAILABLE, tgpio_out(config.data(), config.size()));
}


TEST_F(test_tcc_gpio_sample_host, tgpio_out_pin_setfunc_fails_negative)
{
	int fake_fd = 123;
	std::vector<period_pin_config> config = {
		{ 1, 1 * BILLION + 123, 2 * BILLION + 456 } // pin, offset, period
	};

	// Open PTP device
	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	// Get current time calc relation between clocks
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(TCC_E_SUCCESS))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
		.WillOnce(Return(0));

	// init
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_OUT, 0))
		.WillOnce(Return(-TCC_E_ERROR));

	// deinit
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, 0, 0, 0, 0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_NONE, 0))
		.WillOnce(Return(-TCC_E_ERROR));

	// Close PTP device
	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, tgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_perout_request_fails_negative)
{
	int fake_fd = 123;
	std::vector<period_pin_config> config = {
		{ 1, 1 * BILLION + 123, 2 * BILLION + 456 } // pin, offset, period
	};

	// Open PTP device
	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	// Get current time calc relation between clocks
	EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
		.WillOnce(Return(TCC_E_SUCCESS))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
		.WillOnce(Return(0));

	// init
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_OUT, 0))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, 0, _, _, 0))
		.WillOnce(Return(-TCC_E_ERROR));

	// deinit
	EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, 0, 0, 0, 0))
		.WillOnce(Return(-TCC_E_ERROR));
	EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_NONE, 0))
		.WillOnce(Return(-TCC_E_ERROR));

	// Close PTP device
	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, tgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_single_pin_positive)
{
	int fake_fd = 123;
	// Config format: pin, offset (ns), period (ns)
	std::vector<period_pin_config> config = {
		period_pin_config{ 1, ts2ns(1, 123), ts2ns(2, 456), 1, "/dev/ptp1" }
	};

	// calculate the tsc and art timestamps which will be get by PTP_SYS_OFFSET_PRECISE2 ioctl
	uint64_t tsc1 = ts2ns(123456, 789);
	uint64_t tsc2, art1, art2;
	calc_timestamps(tsc1, tsc2, art1, art2);

	// base_time = begining of the next+1 second ({ts1.tv_sec + sleep + 2, 0})
	// start =  (base_time + offset from config), converted to the ART clock
	uint64_t start_art = calc_start_art(tsc1, art1, config[0].offset);

	{
		InSequence seq;

		// Open PTP device
		EXPECT_CALL(m_libc_mock, open(StrEq(config[0].device),_,_))
			.WillOnce(Return(fake_fd));

		// Get current time calc relation between clocks
		EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
			.WillOnce(DoAll(SetArgPointee<1>((ptp_sys_offset_precise){ns2time<ptp_clock_time>(art1), ns2time<ptp_clock_time>(tsc1), {}}),
					Return(0)));
		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
			.WillOnce(Return(0));
		EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fd, _))
			.WillOnce(DoAll(SetArgPointee<1>((ptp_sys_offset_precise){ns2time<ptp_clock_time>(art2), ns2time<ptp_clock_time>(tsc2), {}}),
					Return(0)));

		// init
		EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, config[0].pin, PTP_FUNC_OUT, config[0].channel))
			.WillOnce(Return(TCC_E_SUCCESS));
		EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, config[0].channel, config[0].period, start_art, 0))
			.WillOnce(Return(TCC_E_SUCCESS));

		// Sleeping until signal interrupt
		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
			.WillOnce(Return(0))
			.WillOnce(Return(0))
			.WillOnce(Return(EINTR));

		// deinit
		EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fd, config[0].channel, 0, 0, 0))
			.WillOnce(Return(TCC_E_SUCCESS));
		EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fd, config[0].pin, PTP_FUNC_NONE, config[0].channel))
			.WillOnce(Return(TCC_E_SUCCESS));

		// Close PTP device
		EXPECT_CALL(m_libc_mock, close(fake_fd))
			.WillOnce(Return(0));
	}

	EXPECT_EQ(TCC_E_SUCCESS, tgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, tgpio_out_multiple_pin_positive)
{
	// Format: pin, offset (ns), period (ns), device
	std::vector<period_pin_config> config = {
		period_pin_config{ 1, ts2ns(1, 12), ts2ns(2, 34), 1, "/dev/ptp0"},
		period_pin_config{ 2, ts2ns(3, 56), ts2ns(4, 78), 2, "/dev/ptp0"}
	};
	std::vector<int> fake_fds = {123, 456};

	// calculate the tsc and art timestamps which will be get by PTP_SYS_OFFSET_PRECISE2 ioctl
	// base_time = begining of the next+1 second ({tsc1.tv_sec + sleep +2, 0})
	// start =  (base_time + offset from config), converted to the ART clock
	std::vector<uint64_t> tsc1 = {
		ts2ns(123456, 789), // crosstimestamp for the first device
		ts2ns(123456, 799), // crosstimestamp for the second device
	};
	std::vector<uint64_t> tsc2(config.size()), art1(config.size()), art2(config.size());
	std::vector<uint64_t> start_ts_art(config.size());
	for(size_t i = 0; i < tsc1.size(); i++) {
		calc_timestamps(tsc1[i], tsc2[i], art1[i], art2[i]);
		start_ts_art[i] = calc_start_art(tsc1[i], art1[i], config[i].offset);
	}

	{
		InSequence seq;

		for(size_t channel = 0; channel < config.size(); channel++) {
			// Open PTP device
			EXPECT_CALL(m_libc_mock, open(StrEq(config[channel].device),_,_))
				.WillOnce(Return(fake_fds[channel]));

			// Get current time calc relation between clocks
			EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fds[channel], _))
				.WillOnce(DoAll(SetArgPointee<1>((ptp_sys_offset_precise){ns2time<ptp_clock_time>(art1[channel]),
											  ns2time<ptp_clock_time>(tsc1[channel]), {}}),
						Return(0)));
		}

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
			.WillOnce(Return(0));

		// init
		for(size_t i = 0; i < config.size(); i++) {
			EXPECT_CALL(m_tgpio_mock, tgpio_get_sys_offset_precise(fake_fds[i], _))
				.WillOnce(DoAll(SetArgPointee<1>((ptp_sys_offset_precise){ns2time<ptp_clock_time>(art2[i]),
											  ns2time<ptp_clock_time>(tsc2[i]), {}}),
						Return(0)));
			EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fds[i], config[i].pin, PTP_FUNC_OUT, config[i].channel))
				.WillOnce(Return(TCC_E_SUCCESS));
			EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fds[i], config[i].channel, config[i].period, start_ts_art[i], 0))
			.WillOnce(Return(TCC_E_SUCCESS));
		}

		// Sleeping until signal interrupt
		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
			.WillOnce(Return(0))
			.WillOnce(Return(0))
			.WillOnce(Return(EINTR));

		// deinit
		for (size_t i = 0; i < config.size(); i++) {
			EXPECT_CALL(m_tgpio_mock, tgpio_perout_request(fake_fds[i], config[i].channel, 0, 0, 0))
				.WillOnce(Return(TCC_E_SUCCESS));
		}
		for (size_t i = 0; i < config.size(); i++) {
			EXPECT_CALL(m_tgpio_mock, tgpio_pin_setfunc(fake_fds[i], config[i].pin, PTP_FUNC_NONE, config[i].channel))
				.WillOnce(Return(TCC_E_SUCCESS));
			// Close PTP device
			EXPECT_CALL(m_libc_mock, close(fake_fds[i]))
				.WillOnce(Return(0));
		}
	}

	EXPECT_EQ(TCC_E_SUCCESS, tgpio_out(config.data(), config.size()));
}

///////////////////////////////////////////////
//            swgpio_out  tests              //
///////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_host, swgpio_out_config_null_negative)
{
	EXPECT_EQ(-TCC_E_BAD_PARAM, swgpio_out(nullptr, 3));
}

TEST_F(test_tcc_gpio_sample_host, swgpio_out_malloc_fails_negative)
{
	period_pin_config config = {1, 1000, 1000000};
	size_t pins_count = 1;

	EXPECT_CALL(m_alloc_mock, calloc(_, _))
		.WillOnce(ReturnNull());

	EXPECT_EQ(-TCC_E_MEMORY, swgpio_out(&config, pins_count));
}

TEST_F(test_tcc_gpio_sample_host, swgpio_out_export_fails_negative)
{
	std::vector<period_pin_config> config = {
		{ 1, 1 * BILLION + 123, 2 * BILLION + 456 } // pin, offset, period
	};

	// Get current time success
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));

	// GPIO init
	EXPECT_CALL(m_swgpio_mock, gpio_export(config[0].pin))
		.WillOnce(Return(-TCC_E_ERROR));

	// GPIO deinit
	EXPECT_CALL(m_swgpio_mock, gpio_unexport(config[0].pin))
		.WillOnce(Return(-TCC_E_ERROR));

	EXPECT_EQ(-TCC_E_ERROR, swgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, swgpio_out_set_direction_fails_negative)
{
	std::vector<period_pin_config> config = {
		{ 1, 1 * BILLION + 123, 2 * BILLION + 456 } // pin, offset, period
	};

	// Get current time success
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));

	// GPIO init
	EXPECT_CALL(m_swgpio_mock, gpio_export(config[0].pin))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_swgpio_mock, gpio_set_direction(config[0].pin, DIRECTION_OUTPUT))
		.WillOnce(Return(-TCC_E_ERROR));

	// GPIO deinit
	EXPECT_CALL(m_swgpio_mock, gpio_unexport(config[0].pin))
		.WillOnce(Return(-TCC_E_ERROR));

	EXPECT_EQ(-TCC_E_ERROR, swgpio_out(config.data(), config.size()));
}

TEST_F(test_tcc_gpio_sample_host, swgpio_out_gpio_open_fails_negative)
{
	std::vector<period_pin_config> config = {
		{ 1, 1 * BILLION + 123, 2 * BILLION + 456 } // pin, offset, period
	};

	// Get current time success
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Return(0));

	// GPIO init
	EXPECT_CALL(m_swgpio_mock, gpio_export(config[0].pin))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_swgpio_mock, gpio_set_direction(config[0].pin, DIRECTION_OUTPUT))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_swgpio_mock, gpio_open(config[0].pin))
		.WillOnce(Return(-1));

	// GPIO deinit
	EXPECT_CALL(m_swgpio_mock, gpio_unexport(config[0].pin))
		.WillOnce(Return(-TCC_E_ERROR));

	EXPECT_EQ(-TCC_E_NOT_AVAILABLE, swgpio_out(config.data(), config.size()));
}


TEST_F(test_tcc_gpio_sample_host, swgpio_out_positive)
{
	period_pin_config period_config[] = {{1, 1000, 2000}};
	size_t pin_count = 1;
	timespec now = {1000, 100};
	running = 1;

	// Get current time
	EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
		.WillOnce(Invoke([&](clockid_t clk_id, timespec* ts) {
			UNUSED(clk_id);
			*ts = now;
			return 0;
		}));

	EXPECT_CALL(m_swgpio_mock, gpio_export(period_config[0].pin))
			.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_swgpio_mock, gpio_set_direction(period_config[0].pin,
				DIRECTION_OUTPUT))
			.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_swgpio_mock, gpio_open(period_config[0].pin))
			.WillOnce(Return(1));

	EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
			.WillOnce(Return(0));

	EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
		.WillOnce(Invoke([&](int, int){
			running = 0;
			return TCC_E_SUCCESS;
		}));

	EXPECT_CALL(m_libc_mock, close(_))
			.WillOnce(Return(0));

	EXPECT_CALL(m_swgpio_mock, gpio_unexport(period_config[0].pin))
			.WillOnce(Return(TCC_E_SUCCESS));

	EXPECT_EQ(TCC_E_SUCCESS, swgpio_out(period_config, pin_count));

	running = 1;
}

TEST_F(test_tcc_gpio_sample_host, swgpio_out_multiple_pins_positive)
{
	int fake_fd[] = {111, 222};
	period_pin_config period_config[] = {{1, 100, 200}, {2, 200, 300}};
	size_t pin_count = 2;
	timespec now = {1000, 100};

	{
		InSequence seq;
		// Get current time
		EXPECT_CALL(m_libc_mock, clock_gettime(_, _))
			.WillOnce(Invoke([&](clockid_t clk_id, timespec* ts) {
				UNUSED(clk_id);
				*ts = now;
				return 0;
			}));

		for (size_t i = 0; i < pin_count; ++i) {
			EXPECT_CALL(m_swgpio_mock, gpio_export(period_config[i].pin))
					.WillOnce(Return(TCC_E_SUCCESS));
			EXPECT_CALL(m_swgpio_mock, gpio_set_direction(period_config[i].pin,
						DIRECTION_OUTPUT))
					.WillOnce(Return(TCC_E_SUCCESS));
			EXPECT_CALL(m_swgpio_mock, gpio_open(period_config[i].pin))
					.WillOnce(Return(fake_fd[i]));
		}

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
				.WillOnce(Invoke([&](clockid_t, int,const struct timespec *request,
								 struct timespec *){
							EXPECT_EQ(request->tv_sec, 1002);
							EXPECT_EQ(request->tv_nsec, 200);
							return 0;
				}));

		EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
			.WillOnce(Return(TCC_E_SUCCESS));

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
				.WillOnce(Invoke([&](clockid_t, int,const struct timespec *request,
								 struct timespec *){
							EXPECT_EQ(request->tv_sec, 1002);
							EXPECT_EQ(request->tv_nsec, 300);
							return 0;
				}));

		EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
			.WillOnce(Return(TCC_E_SUCCESS));

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
				.WillOnce(Invoke([&](clockid_t, int,const struct timespec *request,
								 struct timespec *){
							EXPECT_EQ(request->tv_sec, 1002);
							EXPECT_EQ(request->tv_nsec, 350);
							return 0;
				}));

		EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
			.WillOnce(Return(TCC_E_SUCCESS));

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
				.WillOnce(Invoke([&](clockid_t, int,const struct timespec *request,
								 struct timespec *){
							EXPECT_EQ(request->tv_sec, 1002);
							EXPECT_EQ(request->tv_nsec, 400);
							return 0;
				}));

		EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
			.WillOnce(Return(TCC_E_SUCCESS));

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
				.WillOnce(Invoke([&](clockid_t, int,const struct timespec *request,
								 struct timespec *){
							EXPECT_EQ(request->tv_sec, 1002);
							EXPECT_EQ(request->tv_nsec, 500);
							return 0;
				}));

		EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
			.WillOnce(Return(TCC_E_SUCCESS));

		EXPECT_CALL(m_libc_mock, clock_nanosleep(_, _, _, _))
				.WillOnce(Invoke([&](clockid_t, int,const struct timespec *request,
								 struct timespec *){
							EXPECT_EQ(request->tv_sec, 1002);
							EXPECT_EQ(request->tv_nsec, 500);
							return 0;
				}));

		EXPECT_CALL(m_swgpio_mock, gpio_write(_, _))
				.WillOnce(Invoke([&](int, int){
							running = 0;
							return TCC_E_SUCCESS;
				}));


		for (size_t i = 0; i < 2; ++i) {
			EXPECT_CALL(m_libc_mock, close(fake_fd[i]))
					.WillOnce(Return(0));

			EXPECT_CALL(m_swgpio_mock, gpio_unexport(period_config[i].pin))
					.WillOnce(Return(TCC_E_SUCCESS));
		}
	}

	running = 1;
	EXPECT_EQ(TCC_E_SUCCESS, swgpio_out(period_config, pin_count));
}

}
