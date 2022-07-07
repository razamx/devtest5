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
#include "tcc/err_code.h"
#include "tgpio.h"
#include "tgpio_defs.h"
#include "time_convert.h"
#include <iostream>
#include <cstdarg>
#include <linux/ptp_clock.h>
#include <fcntl.h>
#include <vector>
#include <string>

extern "C" {
	#include "args_parse.h"
}

#include "gtest_helpers.hpp"


struct settings_t
{
	unsigned int pin;
	unsigned int channel;
	const char* device;
	uint64_t period;
	uint64_t sleep_time;
	float proportional_gain;
	float integral_gain;
};

extern int running;
extern "C" {
	int generate_signal(settings_t* settings);
};

///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

//hack to compensate fprintf() calls in implementation
#define EXPECT_STDERR()EXPECT_CALL(m_libc_mock, fprintf(stderr,_,_)).Times(AnyNumber())
#define BILLION 1000000000UL


////////////////////////////////////////////////////////////////////////////////
// TEST FIXTURES
////////////////////////////////////////////////////////////////////////////////

namespace
{
using namespace testing;

class test_tcc_frequency_discipline_sample_argparse : public Test
{

protected:
	settings_t sample_setting;
	void SetUp() override
	{
		optind = 0;
		sample_setting = init_settings();
	}

	settings_t init_settings()
	{
		return (settings_t){
			.pin = 0,
			.channel = 0,
			.device = "/dev/ptp0",
			.period = 1,
			.sleep_time = 100000000,
			.proportional_gain = 0,
			.integral_gain = 0,
		};
	}
};

class test_tcc_frequency_discipline_sample : public Test
{

protected:
	StrictMock<libc_mock> m_libc_mock;
	void SetUp() override
	{
		EXPECT_STDERR();
		running = 1;
	}
};

////////////////////////////////////////////////////////////////////////////////
// COMMAND LINE ARGUMENTS PARSING TESTS
////////////////////////////////////////////////////////////////////////////////


TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_help_positive)
{
	std::vector<const char*> argv = {"path/to/sample", "--help"};
	EXPECT_EQ(TCC_PRINT_HELP, parse_args(argv.size(), (char**)argv.data(), &sample_setting));

}

TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_help_short_positive)
{
	std::vector<const char*> argv = {"path/to/sample", "--h"};
	EXPECT_EQ(TCC_PRINT_HELP, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
}

TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_generic_with_period_positive)
{
	std::vector<const char*> argv = {"path/to/sample",
				   "--pin", "123",
				   "--channel", "456",
				   "--device", "/dev/ptp123",
				   "--sleep", "2000001000",
				   "--period", "100000",
				   "--proportional", "0.5",
				   "--integral", "0.01"};
	ASSERT_EQ(TCC_PARSE_SUCCESS, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
	EXPECT_EQ(123, sample_setting.pin);
	EXPECT_EQ(456, sample_setting.channel);
	EXPECT_EQ("/dev/ptp123", sample_setting.device);
	EXPECT_EQ(2000001000, sample_setting.sleep_time);
	EXPECT_EQ(100000, sample_setting.period);
	EXPECT_FLOAT_EQ(0.5, sample_setting.proportional_gain);
	EXPECT_FLOAT_EQ(0.01, sample_setting.integral_gain);
}


TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_generic_with_frequency_positive)
{
	std::vector<const char*> argv = {"path/to/sample",
				   "--pin", "123",
				   "--channel", "456",
				   "--device", "/dev/ptp123",
				   "--sleep", "2000001000",
				   "--frequency", "10000",
				   "--proportional", "0.5",
				   "--integral", "0.01"};
	ASSERT_EQ(TCC_PARSE_SUCCESS, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
	EXPECT_EQ(123, sample_setting.pin);
	EXPECT_EQ(456, sample_setting.channel);
	EXPECT_EQ("/dev/ptp123", sample_setting.device);
	EXPECT_EQ(2000001000, sample_setting.sleep_time);
	EXPECT_EQ(100000, sample_setting.period);
	EXPECT_FLOAT_EQ(0.5, sample_setting.proportional_gain);
	EXPECT_FLOAT_EQ(0.01, sample_setting.integral_gain);
}

TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_generic_negative)
{
	std::vector<const char*> argv = {"path/to/sample",
				   "--pin", "123",
				   "--channel", "456",
				   "--device", "/dev/ptp123",
				   "--sleep", "2000001000",
				   "--frequency", "10000",
				   "--period", "100000",
				   "--proportional", "0.5",
				   "--integral", "0.01"};
	ASSERT_EQ(TCC_PARSE_ERROR, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
}

TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_proportional_short_positive)
{
	std::vector<const char*> argv = {"path/to/sample",
				   "--pin", "123",
				   "--channel", "456",
				   "--device", "/dev/ptp123",
				   "--sleep", "2000001000",
				   "-p", "0.5",
				   "--integral", "0.01"};
	ASSERT_EQ(TCC_PARSE_SUCCESS, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
	EXPECT_FLOAT_EQ(0.5, sample_setting.proportional_gain);
}

TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_integral_short_positive)
{
	std::vector<const char*> argv = {"path/to/sample",
				   "--pin", "123",
				   "--channel", "456",
				   "--device", "/dev/ptp123",
				   "--sleep", "2000001000",
				   "--proportional", "0.5",
				   "-i", "0.01"};
	ASSERT_EQ(TCC_PARSE_SUCCESS, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
	EXPECT_FLOAT_EQ(0.01, sample_setting.integral_gain);
}

// Tests if parsing is successful and keeps default settings without any cmd options
TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_defautl_values_positive)
{
	settings_t expected = sample_setting;
	std::vector<const char*> argv = {"path/to/sample"};
	ASSERT_EQ(TCC_PARSE_SUCCESS, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
	EXPECT_EQ(expected.pin, sample_setting.pin);
	EXPECT_EQ(expected.channel, sample_setting.channel);
	EXPECT_EQ(expected.device, sample_setting.device);
	EXPECT_EQ(expected.sleep_time, sample_setting.sleep_time);
	EXPECT_FLOAT_EQ(expected.proportional_gain, sample_setting.proportional_gain);
	EXPECT_FLOAT_EQ(expected.integral_gain, sample_setting.integral_gain);
}

// Tests that parsing doesn't accept invalid parameters' values
TEST_F(test_tcc_frequency_discipline_sample_argparse, parse_invalid_values_negative)
{
	std::vector<std::vector<const char*>> invalid_values = {
		// TODO: Add checks in sample and enable this test cases
		//{"--pin", "bad_number"},
		//{"--channel", "bad_number"},
		{"--device", nullptr},
		//{"--sleep", "bad_number"},
		//{"--proportional", "bad_number"},
		//{"--integral", "bad_number"}
	};

	for(const auto& args: invalid_values) {
		sample_setting = init_settings();
		optind = 0;
		std::cout << "Testing invalid options: ";
		std::vector<const char*> argv {"/path/from/sample"};
		for(const char* arg: args) {
			if(arg != nullptr) {
				std::cout << arg << " ";
				argv.push_back(arg);
			} else {
				std::cout << "<missing> ";
			}
		}
		std::cout << std::endl;

		EXPECT_EQ(TCC_PARSE_ERROR, parse_args(argv.size(), (char**)argv.data(), &sample_setting));
	}
}

////////////////////////////////////////////////////////////////////////////////
// FREQUENCY DISCIPLINE TEST
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_frequency_discipline_sample, signal_generation_positive)
{
	/* Manual calculation
	 * It's easier to manually calculate everything that implements
	 * the calculation algorithm in the test case which is close to the algorithm
	 * in the sample, especially since the algorithm can contain errors.
	 * -----------------------------------------------------------------
	 * Requested period     900 (everything in ns)
	 * Possible  period     1000
	 * Current time         1s 500 ns
	 * Start time           3s 0 ns
	 * Sleep                2250
	 *
	 * Iteration #1
	 * Time after sleep     3s 2250 ns (2.5 expected periods)
	 * Expected edges       5 (2.5 periods)
	 * Real edges           4 (2 periods)
	 * Last edge tmstmp     start + 2 * 1000 = 3s 2000 ns
	 * Last expected tmstmp start + 1.5 * 900 = 3s 1350
	 * Phase error          1350 - 2000 = -650
	 * Period error         900 - 1000 = -100
	 * P                    0.5
	 * I                    0.01
	 * Adjustment           P*100 + I*(-200) = -57
	 * New period           900 - 57 = 843 (truncated to int)
	 * New start time       3s 2500 ns
	 *
	 */

	/* NOTE:
	   Number of edges should be bigger. I.e. 5 edges == 2 periods,
	 * 6 edges = 4.5 periods etc and real period calculation should be:
	 * uint64_t period_real = 2 * (t_real - t_prev) / (cnt_real - cnt_prev - 1);
	 * But with this formula I got worse results, than with this one:
	 * uint64_t period_real = 2 * (t_real - t_prev) / (cnt_real - cnt_prev);
	 * I don't know why, but I decided to keep it and assume as correct approach.
	 * Test is designed for approach #2
	 */

	settings_t settings = {
		.pin = 3,
		.channel = 4,
		.device = "/dev/ptp0",
		.period = 900,
		.sleep_time = 0,
		.proportional_gain = 0.5,
		.integral_gain = 0.01,
	};

	int fake_fd = 123;
	ptp_pin_desc desc_out = {{}, settings.pin, 2, settings.channel};
	// timespec fake_current_time = {1, 500};
	uint64_t fake_current_time = ptptime2ns({1, 500});
	// ptp_sys_offset_precise fake_current_time = {
	// 	.device = {1, 500},
	// 	.sys_realtime = {1, 500}, // doesn't matter
	// 	.sys_monoraw = {1, 500}}; // doesn't matter

	// ptp_event_count_tstamp fake_event_count_1 = {0,};
	// fake_event_count_1.device_time = {123, 456};
	// fake_event_count_1.event_count = 100;
	ptp_perout_request request_1 = {{3, 0}, {0, 900}, settings.channel};
	ptp_event_count_tstamp fake_event_count_2 = {0,};
	fake_event_count_2.device_time = {3, 2000};
	fake_event_count_2.event_count = 4;
	ptp_perout_request request_2 = {{0, 0}, {0, 843}, settings.channel, PTP_PEROUT_FREQ_ADJ};
	// some value, just to prevent division by zero, it isn' correct
	ptp_perout_request request_disable = {{0,0}, {0,0}, settings.channel};
	ptp_event_count_tstamp fake_event_count_3 = {0,};
	fake_event_count_3.device_time = {3, 4750};
	fake_event_count_3.event_count = 10;
	ptp_pin_desc desc_disable = {{}, settings.pin, PTP_FUNC_NONE, settings.channel};

	{
		InSequence seq;

		// Open ptp device
		EXPECT_CALL(m_libc_mock, open(StrEq(settings.device), O_RDWR, _))
			.WillOnce(Return(fake_fd));
		// Setup TGPIO as output
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
			.WillOnce(EXPECT_PTP_PIN_DESC(desc_out));
		// Get current PTP clock time
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
			.WillOnce(EXPECT_PTP_SYS_OFFSET_PRECISE(
				fake_current_time, fake_current_time));
		// Enable signal generation with given expected period
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
			.WillOnce(EXPECT_PTP_PEROUT_REQUEST(request_1));

		// Iteration #1
		// Sleep
		EXPECT_CALL(m_libc_mock, nanosleep(TimespecPtrWithNsEq(settings.sleep_time), nullptr))
			.WillOnce(Return(0));
		// Get new number of edges
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EVENT_COUNT_TSTAMP2, _))
			.WillOnce(GET_PTP_EVENT_COUNT_TSTAMP(fake_event_count_2));
		// Update signal period
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
			.WillOnce(EXPECT_PTP_PEROUT_REQUEST(request_2));

		// Iteration #2
		// Sleep
		EXPECT_CALL(m_libc_mock, nanosleep(TimespecPtrWithNsEq(settings.sleep_time), nullptr))
			.WillOnce(Return(0));
		// Get new number of edges (not interesting)
		EXPECT_CALL(m_libc_mock, ioctl(_, _, _))
			.WillOnce(GET_PTP_EVENT_COUNT_TSTAMP(fake_event_count_3));
		// Update signal period (not interesting)
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
			.WillOnce(Invoke([&](int, unsigned long, va_list){
				running = 0; // Emulate interrupt handle
				return 0;
			}));

		// Disable TGPIO
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
			.WillOnce(EXPECT_PTP_PEROUT_REQUEST(request_disable));
		EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
			.WillOnce(EXPECT_PTP_PIN_DESC(desc_disable));
		// Close file
		EXPECT_CALL(m_libc_mock, close(fake_fd))
			.WillOnce(Return(0));
	}

	EXPECT_EQ(TCC_E_SUCCESS, generate_signal(&settings));
}


TEST_F(test_tcc_frequency_discipline_sample, open_ptp_device_fails_negative)
{
	settings_t settings = {
		0,                 // pin
		0,                 // channel
		"/dev/ptp0",       // device
		1000000000,        // period
		100000000,         // sleep time
		0,                 // proportional gain
		0,                 // integral gain
	};

	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(-1));

	EXPECT_EQ(-TCC_E_NOT_AVAILABLE, generate_signal(&settings));
}

TEST_F(test_tcc_frequency_discipline_sample, call_ioctl_setfunc_fails_negative)
{
	int fake_fd = 123;
	settings_t settings = {
		0,                 // pin
		0,                 // channel
		"/dev/ptp0",       // device
		1000000000,        // period
		100000000,         // sleep time
		0,                 // proportional gain
		0,                 // integral gain
	};


	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
		.WillOnce(Return(-1))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, generate_signal(&settings));

}

TEST_F(test_tcc_frequency_discipline_sample, call_sys_offset_fails_negative)
{
	int fake_fd = 123;
	settings_t settings = {
		0,                 // pin
		0,                 // channel
		"/dev/ptp0",       // device
		1000000000,        // period
		100000000,         // sleep time
		0,                 // proportional gain
		0,                 // integral gain
	};


	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
		.WillOnce(Return(-TCC_E_ERROR));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, generate_signal(&settings));

}

TEST_F(test_tcc_frequency_discipline_sample, call_ioctl_perout_request_fails_negative)
{
	int fake_fd = 123;
	settings_t settings = {
		0,                 // pin
		0,                 // channel
		"/dev/ptp0",       // device
		1000000000,        // period
		100000000,         // sleep time
		0,                 // proportional gain
		0,                 // integral gain
	};


	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
		.WillOnce(Return(-1))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, generate_signal(&settings));

}

TEST_F(test_tcc_frequency_discipline_sample, call_ioctl_event_count_in_loop_fails_negative)
{
	int fake_fd = 123;
	settings_t settings = {
		0,                 // pin
		0,                 // channel
		"/dev/ptp0",       // device
		1000000000,        // period
		100000000,         // sleep time
		0,                 // proportional gain
		0,                 // integral gain
	};


	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EVENT_COUNT_TSTAMP2, _))
		.WillOnce(Return(-1));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, nanosleep(_, _))
			.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, generate_signal(&settings));

}

TEST_F(test_tcc_frequency_discipline_sample, call_ioctl_perout_request_in_loop_fails_negative)
{
	int fake_fd = 123;
	ptp_event_count_tstamp event_count = {0,};
	event_count.device_time = {3, 10000100};
	event_count.event_count = 100 + 1000000;



	uint64_t fake_current_time = ptptime2ns({1, 100}); // current time from clock_gettime
	settings_t settings = {
		3,                   // pin
		4,                   // channel
		"/dev/ptp0",         // device
		900,                 // period
		100000000,           // sleep time
		0.1,                 // proportional gain
		0.5,                 // integral gain
	};


	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
	.WillOnce(EXPECT_PTP_SYS_OFFSET_PRECISE(
		fake_current_time, fake_current_time));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EVENT_COUNT_TSTAMP2, _))
		.WillOnce(GET_PTP_EVENT_COUNT_TSTAMP(event_count));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(-1))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, nanosleep(_, _))
			.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(-TCC_E_ERROR, generate_signal(&settings));

}

TEST_F(test_tcc_frequency_discipline_sample, cycle_with_async_time_cnt_positive)
{
	int fake_fd = 123;
	ptp_event_count_tstamp event_count = {0,};
	event_count.device_time = {1, 10000100};
	event_count.event_count = 100 + 1000000;



	uint64_t fake_current_time = ptptime2ns({1, 100}); // current time from clock_gettime
	settings_t settings = {
		3,                   // pin
		4,                   // channel
		"/dev/ptp0",         // device
		900,                 // period
		100000000,           // sleep time
		0.1,                 // proportional gain
		0.5,                 // integral gain
	};


	EXPECT_CALL(m_libc_mock, open(_,_,_))
		.WillOnce(Return(fake_fd));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
	.WillOnce(EXPECT_PTP_SYS_OFFSET_PRECISE(
		fake_current_time, fake_current_time));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EVENT_COUNT_TSTAMP2, _))
		.WillOnce(GET_PTP_EVENT_COUNT_TSTAMP(event_count));

	EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
		.WillOnce(Return(0))
		.WillOnce(Return(0));

	EXPECT_CALL(m_libc_mock, nanosleep(_, _))
		.WillOnce(Invoke([&](const struct timespec *, const struct timespec *){
			running = 0; // Emulate interrupt handle
			return 0;
		}));

	EXPECT_CALL(m_libc_mock, close(fake_fd))
		.WillOnce(Return(0));

	EXPECT_EQ(TCC_E_SUCCESS, generate_signal(&settings));

}

}
