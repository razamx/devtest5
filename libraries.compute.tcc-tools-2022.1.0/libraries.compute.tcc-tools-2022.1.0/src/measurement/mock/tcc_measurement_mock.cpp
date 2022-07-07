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
#include "measurement.h"
#include "tcc_measurement_mock.hpp"
#include <stdarg.h>

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(measurement_mock)

void measurement_mock::init()
{
    ON_CALL(*this, tcc_measurement_print_history(_, _, _)).WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
    ON_CALL(*this, tcc_measurement_get_time_unit_from_env()).WillByDefault(Return(TCC_TU_UNKNOWN));
    ON_CALL(*this, tcc_measurement_buffer_init(_, _, _)).WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
    ON_CALL(*this, tcc_measurement_get_dump_file_from_env()).WillByDefault(ReturnNull());
    ON_CALL(*this, tcc_measurement_shared_memory_flag_from_env()).WillByDefault(Return(0));
    ON_CALL(*this, tcc_measurement_get_buffer_size_from_env(_)).WillByDefault(Return(0));
    ON_CALL(*this, tcc_measurement_init_ex(_, _, _, _, _, _, _)).WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
    ON_CALL(*this, tcc_measurement_get(_, _, _)).WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
    ON_CALL(*this, tcc_measurement_destroy(_)).WillByDefault(Return(TCC_E_SUCCESS));
    ON_CALL(*this, tcc_measurement_convert_clock_to_timespec(_)).WillByDefault(Return((timespec){0, 0}));
    ON_CALL(*this, tcc_measurement_buffer_get(_)).WillByDefault(Return(0)); //buffer empty
    ON_CALL(*this, tcc_measurement_buffer_check(_)).WillByDefault(Return(0)); //shared memory does not exist
    ON_CALL(*this, tcc_measurement_convert_clock_to_time_units(_,_)).WillByDefault(Return(0));

    ON_CALL(*this, tcc_measurement_get_deadline_from_env(_)).WillByDefault(Return(0));
    ON_CALL(*this, tcc_measurement_convert_time_units_to_clock(_,_)).WillByDefault(Return(0));
    ON_CALL(*this, tcc_measurement_get_string_from_time_unit(_)).WillByDefault(ReturnNull());
}

int tcc_measurement_print_history(struct tcc_measurement* measurement, FILE* dump_file, TCC_TIME_UNIT timeunit)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_print_history)(measurement, dump_file, timeunit);
}

TCC_TIME_UNIT tcc_measurement_get_time_unit_from_env()
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_get_time_unit_from_env)();
}

const char* tcc_measurement_get_dump_file_from_env()
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_get_dump_file_from_env)();
}

int tcc_measurement_shared_memory_flag_from_env()
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_shared_memory_flag_from_env)();
}

size_t tcc_measurement_get_buffer_size_from_env(const char* name)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_get_buffer_size_from_env)(name);
}

int tcc_measurement_init_ex(struct tcc_measurement* measurement,
    size_t history_size,
    uint64_t deadline,
    deadline_callback_t deadline_callback,
    bool shared_flag,
    const char* name,
    ...)
{
    va_list args;
    va_start(args, name);
    int status = measurement_mock::get().tcc_measurement_init_ex(
        measurement, history_size, deadline, deadline_callback, shared_flag, name, args);
    va_end(args);
    return status;
}

int64_t tcc_measurement_buffer_init(size_t history_size, const char* name, struct tcc_measurement_buffer** buffer)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_buffer_init)(history_size, name, buffer);
}

void __tcc_measurement_store_measure(struct tcc_measurement* measurement, const unsigned count)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, __tcc_measurement_store_measure)(measurement, count);
}

int tcc_measurement_destroy(struct tcc_measurement* measurement)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_destroy)(measurement);
}

int tcc_measurement_get(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_get)(domain, handle, measurement);
}

struct timespec tcc_measurement_convert_clock_to_timespec(uint64_t clk)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_convert_clock_to_timespec(clk));
}
uint64_t tcc_measurement_buffer_get(struct tcc_measurement_buffer* buffer)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_buffer_get)(buffer);
}

int tcc_measurement_buffer_check(const char* name)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_buffer_check)(name);
}
uint64_t tcc_measurement_convert_clock_to_time_units(uint64_t clk, TCC_TIME_UNIT timeunit)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_convert_clock_to_time_units)(clk, timeunit);
}

uint64_t tcc_measurement_get_deadline_from_env(const char* name)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_get_deadline_from_env)(name);
}
uint64_t tcc_measurement_convert_time_units_to_clock(uint64_t time_interval, TCC_TIME_UNIT timeunit)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_convert_time_units_to_clock)(time_interval, timeunit);
}
const char* tcc_measurement_get_string_from_time_unit(TCC_TIME_UNIT time_units)
{
    return ALLOW_REGULAR_MOCK(measurement_mock, tcc_measurement_get_string_from_time_unit)(time_units);
}
