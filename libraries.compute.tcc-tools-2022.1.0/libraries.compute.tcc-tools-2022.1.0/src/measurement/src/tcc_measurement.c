/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#undef NO_TCC_MEASUREMENT  // Not needed for measurement library

#include "common/cpuid/include/tcc_cpuid.h"
#include "log/tcc_log.h"
#include "measurement.h"
#include "tcc/measurement.h"
#include "tcc/measurement_helpers.h"

#include "version.h"
#include <cpuid.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h> /* PRIu64 */
#include <limits.h>   /* ULONG_MAX */
#include <stdarg.h>   /* va_start(), va_end() */
#include <stdint.h>
#include <stdio.h>  /* vsnprintf() */
#include <stdlib.h> /* getenv(), atoll() */
#include <string.h> /* memset(), strtok_r() */
#include <time.h>


#define UNUSED(x) (void)(x)
#define TCC_MEASUREMENTS_CONFIG_ENV "TCC_MEASUREMENTS_BUFFERS"
#define TCC_MEASUREMENTS_TIME_UNIT_ENV "TCC_MEASUREMENTS_TIME_UNIT"
#define TCC_SHARED_MEMORY_FLAG_ENV "TCC_USE_SHARED_MEMORY"
#define INITIAL_WRITE_INDEX UINT64_MAX

static unsigned long nominal_tsc_freq = 0;
static int freq_init_flag = 0;
struct tcc_measurement_buffer default_ring;

static void init_tsc_freq()
{
    TCC_TRACE_FUNC("void");

    freq_init_flag = 1;
    struct registers reg;
    int res = tcc_cpuid(0x15, &reg);
    if (res) {
        TCC_LOG_TRACE("%" PRIu32 ",%" PRIu32 ",%" PRIu32, reg.eax, reg.ebx, reg.ecx);
        /* This won't work everywhere */
        if (reg.eax == 0) {
            TCC_LOG_ERROR("Register 'eax' has zero value");
            return;
        }
        if (reg.ebx == 0) {
            TCC_LOG_ERROR("Register 'ebx' has zero value");
            return;
        }
        if (reg.ecx == 0) {
            TCC_LOG_ERROR("Register 'ecx' has zero value");
            return;
        }
        nominal_tsc_freq = (unsigned long)reg.ecx * reg.ebx / reg.eax;
    }
}

struct timespec tcc_measurement_convert_clock_to_timespec(uint64_t clk)
{
    TCC_TRACE_FUNC("clk=%lu", clk);

    struct timespec ret = {0};
    if (freq_init_flag == 0) {
        init_tsc_freq();
    }
    if (nominal_tsc_freq != 0) {
        double nsec = 1.e9 * clk / (unsigned)nominal_tsc_freq;
        ret.tv_sec = nsec / 1.e9;
        ret.tv_nsec = nsec - ret.tv_sec * 1.e9;
    }
    return ret;
}

uint64_t tcc_measurement_convert_timespec_to_clock(struct timespec ts)
{
    TCC_TRACE_FUNC("ts={tv_sec=%ld tv_nsec=%lu}", ts.tv_sec, ts.tv_nsec);

    uint64_t ret = 0;
    if (freq_init_flag == 0) {
        init_tsc_freq();
    }
    if (nominal_tsc_freq != 0) {
        ret = (ts.tv_sec * 1000000000 + ts.tv_nsec) * nominal_tsc_freq / 1000000000;
    }
    return ret;
}

const char* tcc_measurement_get_string_from_time_unit(TCC_TIME_UNIT time_units)
{
    TCC_TRACE_FUNC("time_units=%d", time_units);

    switch (time_units) {
    case TCC_TU_CLK:
        return "clk";
        break;
    case TCC_TU_NS: {
        return "ns";
        break;
    }
    case TCC_TU_US: {
        return "us";
        break;
    }
    default:
        TCC_LOG_WARNING("Invalid time unit");
        return "invalid";
        break;
    }
}

TCC_TIME_UNIT tcc_measurement_get_time_unit_from_string(const char* string)
{
    TCC_TRACE_FUNC("string=%s", SAFENULLSTR(string));

    if (string == NULL) {
        return TCC_TU_UNKNOWN;
    }
    if (strncasecmp(string, "clk", 4) == 0) {
        return TCC_TU_CLK;
    }
    if (strncasecmp(string, "ns", 4) == 0) {
        return TCC_TU_NS;
    }
    if (strncasecmp(string, "us", 4) == 0) {
        return TCC_TU_US;
    }
    TCC_LOG_WARNING("Invalid time unit string");
    return TCC_TU_UNKNOWN;
}

#if USE_TCC_MEASUREMENT
static const double __measurement_cost = 0;

// Measurement settings from environment
struct tcc_measurement_props
{
    char* name;
    size_t buffer_size;
    uint64_t deadline;
};

// parse uint64_t from string with exact match required
static int parse_uint64_t(char* string, uint64_t* result)
{
    TCC_TRACE_FUNC("string=%s result=%p", SAFENULLSTR(string), result);

    if (string == NULL || result == NULL) {
        TCC_LOG_ERROR("One or more parameters are NULL: string=%p, result=%p", string, result);
        return -TCC_E_BAD_PARAM;
    }

    uint64_t value;
    int cnt;
    int parsed;
    parsed = sscanf(string, "%lu%n", &value, &cnt);
    if (parsed != 1 || (size_t)cnt != strlen(string)) {
        TCC_LOG_ERROR("uint64_t from string \"%s\" parsing error: string is not a number", string);
        return -TCC_E_BAD_PARAM;
    }
    *result = value;
    return TCC_E_SUCCESS;
}

// parse measurement config string, format: name:buffer_size[:deadline]
static int tcc_parse_measurement(char* string, struct tcc_measurement_props* props)
{
    TCC_TRACE_FUNC("string=%s props=%p", SAFENULLSTR(string), props);

    if (string == NULL || props == NULL) {
        TCC_LOG_ERROR("One or more parameters are NULL: string=%p, props=%p", string, props);
        return -TCC_E_BAD_PARAM;
    }

    const char* separator_fields = ":";
    char* token_field = strsep(&string, separator_fields);
    int field_index = 0;
    int err = 0;

    // iterate over fields in measurement
    while (token_field != NULL) {
        switch (field_index++) {
        case 0:
            props->name = token_field;
            break;
        case 1:
            if (parse_uint64_t(token_field, &props->buffer_size) != 0) {
                TCC_LOG_ERROR("Format error: invalid buffer size");
                err = -TCC_E_BAD_PARAM;
            }
            break;
        case 2:
            if (parse_uint64_t(token_field, &props->deadline) != 0) {
                TCC_LOG_ERROR("Format error: invalid deadline");
                err = -TCC_E_BAD_PARAM;
            }
            break;
        default:
            TCC_LOG_ERROR("Format error: invalid number of fields");
            err = -TCC_E_BAD_PARAM;
            break;
        }

        token_field = strsep(&string, separator_fields);
    }
    if (err == 0) {
        TCC_LOG_DEBUG("Measurement config parsed: name=%s, buffer_size=%lu, deadline=%lu",
            SAFENULLSTR(props->name),
            props->buffer_size,
            props->deadline);
    }
    return err;
}

// Remove leading and trailing whitespaces
// Returns new allocated string
static char* strtrim(char* string)
{
    TCC_TRACE_FUNC("string=%s", SAFENULLSTR(string));

    if (string == NULL) {
        TCC_LOG_ERROR("Parameter \"name\" is NULL");
        return NULL;
    }

    char* substr = string;
    char* start = NULL;  // pointer to the first non-space symbol
    char* stop = NULL;   // pointer to the last non-space symbol
    while (*substr != '\0') {
        int is_space = isspace(*substr);
        if (start == NULL && !is_space) {
            start = substr;
        }
        if (!is_space && *substr != '\0') {
            stop = substr;
        }
        substr++;
    }

    size_t new_size;  // size of the string without 0-symbol

    if (start == NULL || stop == NULL) {
        // No non-whitespace symbols found, result is empty string
        new_size = 0;
    } else {
        new_size = (uintptr_t)stop - (uintptr_t)start + 1;
    }

    char* trimmed = (char*)malloc(new_size + 1);
    if (trimmed == NULL) {
        TCC_LOG_ERROR("Failed to allocate memory: %s(%d)", strerror(errno), errno);
        return NULL;
    }

    if (start != NULL) {
        memcpy(trimmed, start, new_size);
    }
    trimmed[new_size] = '\0';
    return trimmed;
}

// Find config of the measurement with given name in string
static int tcc_parse_measurement_props_from_string(char* string, const char* name, struct tcc_measurement_props* props)
{
    TCC_TRACE_FUNC("string=%s name=%s props=%p", SAFENULLSTR(string), SAFENULLSTR(name), props);

    int err = TCC_E_SUCCESS;
    const char* separator_measurements = ",";
    char* saveptr;
    char* token_measurement;

    // 2 in 1: trim whitespaces and allocate new string, because strtok and
    // strsep will modify the string
    char* string_trimmed = strtrim(string);
    if (string_trimmed == NULL) {
        TCC_LOG_ERROR("Failed to duplicate string: %s(%d)", strerror(errno), errno);
        return -TCC_E_MEMORY;
    }

    token_measurement = strtok_r(string_trimmed, separator_measurements, &saveptr);
    // iterate over measurements in string
    while (token_measurement != NULL) {
        TCC_LOG_TRACE("token: %s", token_measurement);
        int parsing_err = tcc_parse_measurement(token_measurement, props);
        if (parsing_err != TCC_E_SUCCESS) {
            // Ignore format error for measurements with different name
            TCC_LOG_ERROR("Format error: invalid measurement config format");
        }
        if (strcmp(props->name, name) == 0) {
            if (parsing_err == TCC_E_SUCCESS) {
                TCC_LOG_DEBUG("Measurement config find: name=%s, buffer_size=%lu, deadline=%lu",
                    SAFENULLSTR(props->name),
                    props->buffer_size,
                    props->deadline);
                err = TCC_E_SUCCESS;
                goto out;
            } else {
                TCC_LOG_ERROR("Measurement config find: invalid measurement config format");
                err = -TCC_E_BAD_PARAM;
                goto out;
            }
        }
        token_measurement = strtok_r(NULL, separator_measurements, &saveptr);
    }
    TCC_LOG_ERROR("Config for measurement with name \"%s\" not found", SAFENULLSTR(name));
    err = -TCC_E_BAD_PARAM;
out:
    free(string_trimmed);
    return err;
}

// Find config of the measurement with the given name in the environment variable
static int tcc_parse_measurement_props_from_env(const char* name, struct tcc_measurement_props* props)
{
    TCC_TRACE_FUNC("name=%s props=%p", SAFENULLSTR(name), props);

    char* config_env = getenv(TCC_MEASUREMENTS_CONFIG_ENV);
    if (!config_env) {
        TCC_LOG_TRACE("%s var is not set", TCC_MEASUREMENTS_CONFIG_ENV);
        return TCC_E_NOT_AVAILABLE;
    }
    TCC_LOG_TRACE("%s=%s", TCC_MEASUREMENTS_CONFIG_ENV, config_env);
    return tcc_parse_measurement_props_from_string(config_env, name, props);
}

size_t tcc_measurement_get_buffer_size_from_env(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));

    if (name == NULL) {
        TCC_LOG_ERROR("Parameter \"name\" is NULL");
        return -TCC_E_BAD_PARAM;
    }

    struct tcc_measurement_props props = {0};
    int status = tcc_parse_measurement_props_from_env(name, &props);
    if (status == TCC_E_SUCCESS) {
        TCC_LOG_DEBUG("Buffer size: %lu", props.buffer_size);
        return props.buffer_size;
    }
    if (status == TCC_E_NOT_AVAILABLE) {
        TCC_LOG_DEBUG("Unable to get buffer size from TCC_MEASUREMENTS_BUFFERS environment variable.");
        return 0;
    }
    return 0;
}

uint64_t tcc_measurement_get_deadline_from_env(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));

    if (name == NULL) {
        TCC_LOG_ERROR("Parameter \"name\" is NULL");
        return -TCC_E_BAD_PARAM;
    }

    struct tcc_measurement_props props = {0};
    int status = tcc_parse_measurement_props_from_env(name, &props);
    if (status == TCC_E_SUCCESS) {
        TCC_LOG_DEBUG("Deadline: %lu", props.deadline);
        return props.deadline;
    }
    if (status == TCC_E_NOT_AVAILABLE) {
        TCC_LOG_DEBUG("Unable to get deadline from TCC_MEASUREMENTS_BUFFERS environment variable.");
        return 0;
    }
    return 0;
}

int tcc_measurement_shared_memory_flag_from_env()
{
    TCC_TRACE_FUNC("void");

    int shm_flag = 0;
    const char* use_shm = getenv(TCC_SHARED_MEMORY_FLAG_ENV);
    if (use_shm == NULL) {
        return 0;
    }
    if (strncasecmp(use_shm, "true", 6) == 0) {
        shm_flag = 1;
    } else if (strncasecmp(use_shm, "false", 6) == 0) {
        shm_flag = 0;
    } else {
        TCC_LOG_WARNING(
            "Environment variable %s format error. Used default value \"false\".", TCC_SHARED_MEMORY_FLAG_ENV);
    }
    TCC_LOG_DEBUG("Shared memory flag: %d", shm_flag);
    return shm_flag;
}

uint64_t tcc_measurement_buffer_get(struct tcc_measurement_buffer* buffer)
{
    TCC_TRACE_FUNC("buffer=%p", buffer);

    uint64_t value;
    uint64_t* measurements = (uint64_t*)(buffer + 1);
    if (buffer->reader_index == buffer->writer_index) {
        return 0;
    }
    buffer->reader_index = (buffer->reader_index + 1) % buffer->buffer_size;
    value = measurements[buffer->reader_index];
    return value;
}

static uint64_t get_identifier_shared_memory(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));

    uint64_t hash = 5545;
    int character = *name;
    while (character) {
        hash = ((hash << 5) + hash) + character;
        character = *name++;
    }
    return hash;
}

static int get_buffer_id(const char* name)
{
    return shmget(get_identifier_shared_memory(name), sizeof(struct tcc_measurement_buffer), IPC_EXCL | 0666);
}

static int check_shm_size_or_remove(size_t history_size, const char* name)
{
    int64_t shm_id;
    struct shmid_ds buf;

    shm_id = get_buffer_id(name);
    if (shm_id > 0) {
        if (shmctl(shm_id, IPC_STAT, &buf) < 0) {
            TCC_LOG_ERROR("Failed to get shared memory size: %s", strerror(errno));
            return -1;
        }
        size_t length = ((size_t)buf.shm_segsz - sizeof(struct tcc_measurement_buffer)) / sizeof(uint64_t);

        if (length != history_size) {
            TCC_LOG_WARNING("Wrong size of buffer. Deleting...");
            if (shmctl(shm_id, IPC_RMID, NULL) < 0) {
                TCC_LOG_ERROR("Failed to delete the shared memory: %s", strerror(errno));
                return -1;
            }
        }
    }
    return 0;
}

int tcc_measurement_buffer_check(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));

    if (get_buffer_id(name) < 0) {
        return 0;
    }
    return 1;
}

int64_t tcc_measurement_buffer_init(size_t history_size, const char* name, struct tcc_measurement_buffer** buffer)
{
    TCC_TRACE_FUNC("history_size=%lu name=%s buffer=%p", history_size, SAFENULLSTR(name), buffer);

    int64_t shm_id;
    uint64_t* shared_memory;

    if (check_shm_size_or_remove(history_size, name) < 0) {
        return -TCC_E_MEMORY;
    }

    if ((shm_id = shmget(get_identifier_shared_memory(name),
             history_size * sizeof(uint64_t) + sizeof(struct tcc_measurement_buffer),
             IPC_CREAT | 0666)) < 0) {
        TCC_LOG_ERROR("Failed to get shared memory segment: %s(%d)", strerror(errno), errno);
        return -TCC_E_MEMORY;
    }
    if ((shared_memory = shmat(shm_id, NULL, 0)) == (uint64_t*)-1) {
        TCC_LOG_ERROR("Failed to attach shared memory segment to the address space of the process: %s(%d)",
            strerror(errno),
            errno);
        return -TCC_E_MEMORY;
    }
    *buffer = (struct tcc_measurement_buffer*)shared_memory;
    return shm_id;
}

uint64_t tcc_measurement_convert_clock_to_time_units(uint64_t clk, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("clk=%lu timeunit=%d", clk, timeunit);

    switch (timeunit) {
    case TCC_TU_CLK:
        return clk;
        break;
    case TCC_TU_NS: {
        struct timespec time_spec = tcc_measurement_convert_clock_to_timespec(clk);
        return time_spec.tv_sec * 1000000000 + time_spec.tv_nsec;
        break;
    }
    case TCC_TU_US: {
        struct timespec time_spec = tcc_measurement_convert_clock_to_timespec(clk);
        return time_spec.tv_sec * 1000000 + time_spec.tv_nsec / 1000;
        break;
    }
    case TCC_TU_UNKNOWN:
        return clk;
        break;
    default:
        break;
    }
    return clk;
}

uint64_t tcc_measurement_convert_time_units_to_clock(uint64_t time_interval, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("time_interval=%lu timeunit=%d", time_interval, timeunit);

    if (freq_init_flag == 0) {
        init_tsc_freq();
    }
    switch (timeunit) {
    case TCC_TU_CLK:
        return time_interval;
        break;
    case TCC_TU_NS: {
        return 1e-9 * time_interval * (unsigned)nominal_tsc_freq;
        break;
    }
    case TCC_TU_US: {
        return 1e-6 * time_interval * (unsigned)nominal_tsc_freq;
        break;
    }
    case TCC_TU_UNKNOWN:
        return time_interval;
        break;
    default:
        break;
    }
    return time_interval;
}

static int measurement_init(struct tcc_measurement* measurement,
    uint64_t deadline,
    deadline_callback_t function_notify,
    bool shared_flag,
    size_t history_size,
    const char* name,
    ...)
{
    TCC_TRACE_FUNC("measurement=%p deadline=%lu function_notify=%p"
                   "shared_flag=%d history_size=%lu name=%s",
        measurement,
        deadline,
        function_notify,
        shared_flag,
        history_size,
        SAFENULLSTR(name));

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }

    va_list ap;

    va_start(ap, name);
    memset(measurement->name, 0, sizeof(measurement->name));
    vsnprintf(measurement->name, sizeof(measurement->name) - 1, name, ap);
    va_end(ap);

    measurement->clk_last_diff = 0;
    measurement->clk_avg = 0;
    measurement->clk_avgc = 0;
    measurement->clk_result = 0.0;
    measurement->clk_max = 0.0;
    measurement->clk_min = (double)ULONG_MAX;
    measurement->cost = __measurement_cost;
    measurement->is_full = false;
    measurement->shared_memory_id = 0;
    measurement->shared_flag = shared_flag;
    measurement->deadline_notify = function_notify;
    measurement->enable_deadline = false;
    measurement->deadline = deadline;

    default_ring.buffer_size = 0;
    default_ring.writer_index = INITIAL_WRITE_INDEX;
    default_ring.reader_index = INITIAL_WRITE_INDEX;
    measurement->measurement_buffer = &default_ring;

    measurement->measurements = NULL;
    if (deadline != 0 && *measurement->deadline_notify) {
        measurement->enable_deadline = true;
    }
    if (history_size) {
        if (shared_flag) {
            measurement->shared_memory_id =
                tcc_measurement_buffer_init(history_size, measurement->name, &measurement->measurement_buffer);
            if (measurement->shared_memory_id < 0) {
                return -TCC_E_MEMORY;
            }
        } else {
            measurement->measurement_buffer = (struct tcc_measurement_buffer*)malloc(
                history_size * sizeof(uint64_t) + sizeof(struct tcc_measurement_buffer));
            if (measurement->measurement_buffer == NULL) {
                return -TCC_E_MEMORY;
            }
        }
        measurement->measurements = (uint64_t*)(measurement->measurement_buffer + 1);
        measurement->measurement_buffer->buffer_size = history_size;
        measurement->measurement_buffer->writer_index = INITIAL_WRITE_INDEX;
        measurement->measurement_buffer->reader_index = INITIAL_WRITE_INDEX;
    }
    return TCC_E_SUCCESS;
}

int tcc_measurement_init(struct tcc_measurement* measurement, size_t history_size, const char* name, ...)
{
    TCC_TRACE_FUNC("measurement=%p history_size=%lu name=%s", measurement, history_size, SAFENULLSTR(name));

    va_list arg;
    int return_val;
    va_start(arg, name);
    return_val = measurement_init(measurement, 0, NULL, false, history_size, name, arg);
    va_end(arg);
    return return_val;
}

int tcc_measurement_init_ex(struct tcc_measurement* measurement,
    size_t history_size,
    uint64_t deadline,
    deadline_callback_t function_notify,
    bool shared_flag,
    const char* name,
    ...)
{
    TCC_TRACE_FUNC("measurement=%p history_size=%lu deadline=%lu "
                   "function_notify=%p shared_flag=%d name=%s",
        measurement,
        history_size,
        deadline,
        function_notify,
        shared_flag,
        SAFENULLSTR(name));

    va_list arg;
    int return_val;
    va_start(arg, name);
    return_val = measurement_init(measurement, deadline, function_notify, shared_flag, history_size, name, arg);
    va_end(arg);
    return return_val;
}

static double clock_double_convert_to_time_units(double clk, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("clk=%lf timeunit=%d", clk, timeunit);

    switch (timeunit) {
    case TCC_TU_CLK:
        return clk;
        break;
    case TCC_TU_NS: {
        struct timespec time_spec = tcc_measurement_convert_clock_to_timespec(clk);
        return time_spec.tv_sec * 1.e9 + time_spec.tv_nsec;
        break;
    }
    case TCC_TU_US: {
        struct timespec time_spec = tcc_measurement_convert_clock_to_timespec(clk);
        return time_spec.tv_sec * 1.e6 + time_spec.tv_nsec * 1.e-3;
        break;
    }
    case TCC_TU_UNKNOWN:
        return clk;
        break;
    default:
        break;
    }
    return clk;
}

int tcc_measurement_destroy(struct tcc_measurement* measurement)
{
    TCC_TRACE_FUNC("measurement=%p", measurement);

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }

    if (measurement->measurements == NULL) {
        return TCC_E_SUCCESS;
    }

    if (measurement->shared_flag == false) {
        free((uint64_t*)measurement->measurement_buffer);
    } else {
        if (shmctl(measurement->shared_memory_id, IPC_RMID, NULL) < 0) {
            return -TCC_E_MEMORY;
        }
    }
    return TCC_E_SUCCESS;
}

/**
 * @brief Print Last Measurement
 *
 * @param measurement pointer to TSC profile structure
 */
int tcc_measurement_print_last_record(struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("measurement=%p out=%p timeunit=%d", measurement, out, timeunit);

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }
    if (out == NULL) {
        return -TCC_E_NOT_AVAILABLE;
    }
    if (timeunit == TCC_TU_UNKNOWN) {
        return -TCC_E_BAD_PARAM;
    }
    if (fprintf(out,
            "{\"measurement\":\"%s\","
            "\"duration\":%lu}\n",
            measurement->name,
            (unsigned long)tcc_measurement_convert_clock_to_time_units(measurement->clk_last_diff, timeunit)) < 0) {
        return -TCC_E_ACCESS;
    }
    return TCC_E_SUCCESS;
}

static int __tcc_measurement_history_print(const struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("measurement=%p out=%p timeunit=%d", measurement, out, timeunit);

    size_t number_of_measurement_to_write = 0;
    size_t start_index = 0;
    if (measurement->measurement_buffer->writer_index == INITIAL_WRITE_INDEX) {
        return TCC_E_SUCCESS;
    }
    if (measurement->is_full) {
        number_of_measurement_to_write = measurement->measurement_buffer->buffer_size;
        start_index =
            (measurement->measurement_buffer->writer_index + 1) % measurement->measurement_buffer->buffer_size;
    } else {
        number_of_measurement_to_write = measurement->measurement_buffer->writer_index + 1;
    }
    if (number_of_measurement_to_write == 0) {
        return TCC_E_SUCCESS;
    }

    if (fprintf(out,
            "%" PRIu64,
            tcc_measurement_convert_clock_to_time_units(measurement->measurements[start_index], timeunit)) < 0) {
        return -TCC_E_ACCESS;
    }

    for (size_t i = 1; i < number_of_measurement_to_write; ++i) {
        size_t current_index = (start_index + i) % measurement->measurement_buffer->buffer_size;
        if (fprintf(out,
                ", %" PRIu64,
                tcc_measurement_convert_clock_to_time_units(measurement->measurements[current_index], timeunit)) < 0) {
            return -TCC_E_ACCESS;
        }
    }
    return TCC_E_SUCCESS;
}

TCC_TIME_UNIT tcc_measurement_get_time_unit_from_env()
{
    TCC_TRACE_FUNC("void");

    const char* time_unit = getenv(TCC_MEASUREMENTS_TIME_UNIT_ENV);
    if (time_unit == NULL) {
        TCC_LOG_DEBUG("Unable to get time unit from TCC_MEASUREMENTS_TIME_UNIT environment variable.");
        return TCC_TU_UNKNOWN;
    }
    return tcc_measurement_get_time_unit_from_string(time_unit);
}

const char* tcc_measurement_get_dump_file_from_env()
{
    TCC_TRACE_FUNC("void");

    static const char env_var_name[] = "TCC_MEASUREMENTS_DUMP_FILE";
    const char* dump_filename = getenv(env_var_name);
    if (dump_filename == NULL) {
        TCC_LOG_DEBUG("Unable to get dump file from TCC_MEASUREMENTS_DUMP_FILE environment variable.");
    }
    return dump_filename;
}

int tcc_measurement_print_history(struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("measurement=%p dump_file=%p timeunit=%d", measurement, out, timeunit);

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }
    if (out == NULL) {
        return -TCC_E_NOT_AVAILABLE;
    }
    if (timeunit == TCC_TU_UNKNOWN) {
        return -TCC_E_BAD_PARAM;
    }
    if (fprintf(out, "%s: ", measurement->name) < 0) {
        return -TCC_E_ACCESS;
    }
    if (__tcc_measurement_history_print(measurement, out, timeunit) < 0) {
        return -TCC_E_ACCESS;
    }
    if (fprintf(out, "\n") < 0) {
        return -TCC_E_ACCESS;
    }
    return TCC_E_SUCCESS;
}

int tcc_measurement_print_summary(struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("measurement=%p out=%p timeunit=%d", measurement, out, timeunit);

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }
    if (out == NULL) {
        return -TCC_E_NOT_AVAILABLE;
    }
    if (timeunit == TCC_TU_UNKNOWN) {
        return -TCC_E_BAD_PARAM;
    }
    tcc_measurement_get_avg(measurement);
    const char* time_units = tcc_measurement_get_string_from_time_unit(timeunit);
    if (fprintf(out,
            "{\"measurement\":\"%s\","
            "\"time units\":\"%s\","
            "\"count\":%lu,"
            "\"avg duration\":%.3f,"
            "\"min duration\":%.3f,"
            "\"max duration\":%.3f,"
            "\"delta\":%.3f,"
            "\"measurements_history\": [",
            measurement->name,
            time_units,
            (unsigned long)measurement->clk_avgc,
            clock_double_convert_to_time_units(measurement->clk_result, timeunit),
            clock_double_convert_to_time_units(measurement->clk_min, timeunit),
            clock_double_convert_to_time_units(measurement->clk_max, timeunit),
            clock_double_convert_to_time_units(measurement->clk_max - measurement->clk_min, timeunit)) < 0) {
        return -TCC_E_ACCESS;
    }
    if (__tcc_measurement_history_print(measurement, out, timeunit) < 0) {
        return -TCC_E_ACCESS;
    }
    if (fprintf(out, "]}\n") < 0) {
        return -TCC_E_ACCESS;
    }
    return TCC_E_SUCCESS;
}

int tcc_measurement_print(struct tcc_measurement* measurement, TCC_TIME_UNIT timeunit)
{
    TCC_TRACE_FUNC("measurement=%p timeunit=%d", measurement, timeunit);

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }
    if (timeunit == TCC_TU_UNKNOWN) {
        return -TCC_E_BAD_PARAM;
    }
    tcc_measurement_get_avg(measurement);
    const char* time_units = tcc_measurement_get_string_from_time_unit(timeunit);
    printf("[%s] Iterations %lu; iteration duration [%s]: "
           "avg=%.3f min=%.3f max=%.3f jitter=%.3f\n",
        measurement->name,
        (unsigned long)measurement->clk_avgc,
        time_units,
        clock_double_convert_to_time_units(measurement->clk_result, timeunit),
        clock_double_convert_to_time_units(measurement->clk_min, timeunit),
        clock_double_convert_to_time_units(measurement->clk_max, timeunit),
        clock_double_convert_to_time_units(measurement->clk_max - measurement->clk_min, timeunit));
    return TCC_E_SUCCESS;
}

int tcc_measurement_set_deadline(struct tcc_measurement* measurement,
    uint64_t deadline,
    deadline_callback_t deadline_callback)
{
    TCC_TRACE_FUNC("measurement=%p deadline=%lu deadline_callback=%p", measurement, deadline, deadline_callback);

    if (measurement == NULL) {
        return -TCC_E_BAD_PARAM;
    }

    measurement->deadline_notify = deadline_callback;
    measurement->enable_deadline = false;
    measurement->deadline = deadline;

    if (deadline != 0 && *measurement->deadline_notify) {
        measurement->enable_deadline = true;
    }

    return TCC_E_SUCCESS;
}

/**
 * @brief Puts a value into the array of measurement results.
 *
 * @param [in] value Value to put in the array of measurement results
 * @param [in,out] buffer Pointer to the measurement ring buffer structure
 */
static __attribute__((always_inline)) inline void __measurement_buffer_put(struct tcc_measurement_buffer* buffer,
    uint64_t value)
{
    uint64_t* measurements = (uint64_t*)(buffer + 1);
    if (((buffer->writer_index + 1) % buffer->buffer_size) == buffer->reader_index) {
        return;
    }
    uint64_t new_index = (buffer->writer_index + 1) % buffer->buffer_size;
    measurements[new_index] = value;
    buffer->writer_index = new_index;
}

void __tcc_measurement_store_measure(struct tcc_measurement* measurement, const unsigned count)
{
    uint64_t diff = measurement->clk_last_diff;
    uint64_t clk_diff = diff;
    /* Update cumulative values for average calculation */
    measurement->clk_avgc += count;
    measurement->clk_avg += (diff - measurement->cost);
    /* Use TSC difference divided by iteration count for remaining operations */
    clk_diff = clk_diff / count;
    if (unlikely(measurement->measurements)) {
        __measurement_buffer_put(measurement->measurement_buffer, clk_diff);
        if (((measurement->measurement_buffer->writer_index + 1) % measurement->measurement_buffer->buffer_size) == 0) {
            measurement->is_full = true;
        }
    }

    if (unlikely(measurement->enable_deadline && clk_diff > measurement->deadline)) {
        (*measurement->deadline_notify)(measurement, clk_diff);
    }

    if (clk_diff < measurement->clk_min) {
        measurement->clk_min = clk_diff;
    }
    if (clk_diff > measurement->clk_max) {
        measurement->clk_max = clk_diff;
    }
}

#endif /* USE_TCC_MEASUREMENT */
