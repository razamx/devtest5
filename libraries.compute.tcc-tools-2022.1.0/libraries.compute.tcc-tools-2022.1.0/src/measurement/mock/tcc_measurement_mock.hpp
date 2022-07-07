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

#ifndef TCC_MEASUREMENT_MOCK_HPP
#define TCC_MEASUREMENT_MOCK_HPP

#include "gmock/gmock.h"
#include "measurement.h"
#include "tcc/measurement_helpers.h"
#include <ittnotify/ittnotify.h>
#include "mock_common.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

struct measurement_mock{
	MOCK_METHOD3(tcc_measurement_print_history, int(struct tcc_measurement* measurement, FILE* dump_file,  TCC_TIME_UNIT timeunit));
	MOCK_METHOD0(tcc_measurement_get_time_unit_from_env, TCC_TIME_UNIT());
	MOCK_METHOD0(tcc_measurement_get_dump_file_from_env, const char*());
	MOCK_METHOD0(tcc_measurement_shared_memory_flag_from_env, int ());
	MOCK_METHOD1(tcc_measurement_get_buffer_size_from_env, size_t(const char* name));
	MOCK_METHOD7(tcc_measurement_init_ex, int(struct tcc_measurement* measurement, size_t history_size, uint64_t deadline,
				    deadline_callback_t deadline_callback,
				    bool shared_flag, const char *name, va_list list));
	MOCK_METHOD3(tcc_measurement_buffer_init, int64_t(size_t history_size, const char* name, struct tcc_measurement_buffer** buffer));
	MOCK_METHOD2(__tcc_measurement_store_measure, void(struct tcc_measurement* measurement, const unsigned count));
	MOCK_METHOD3(tcc_measurement_get, int(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement));
	MOCK_METHOD1(tcc_measurement_destroy, int(struct tcc_measurement* measurement));
    MOCK_METHOD1(tcc_measurement_convert_clock_to_timespec, struct timespec (uint64_t clk));
	MOCK_METHOD1(tcc_measurement_buffer_get, uint64_t(struct tcc_measurement_buffer* buffer));
	MOCK_METHOD1(tcc_measurement_buffer_check, int(const char* name));
	MOCK_METHOD2(tcc_measurement_convert_clock_to_time_units, uint64_t(uint64_t clk, TCC_TIME_UNIT timeunit));

	MOCK_METHOD1(tcc_measurement_get_deadline_from_env, uint64_t (const char* name));
	MOCK_METHOD2(tcc_measurement_convert_time_units_to_clock, uint64_t (uint64_t time_interval, TCC_TIME_UNIT timeunit));
	MOCK_METHOD1(tcc_measurement_get_string_from_time_unit, const char* (TCC_TIME_UNIT time_units));

	COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(measurement_mock)
};

#endif // TCC_MEASUREMENT_MOCK_HPP

