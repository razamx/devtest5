/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/**
 * @file measurement.h
 * @brief Intel(R) Time Coordinated Computing Tools (Intel® TCC Tools) measurement library
 *
 * The measurement library enables you to analyze the behavior of real-time
 * applications. You can use the library to instrument your code and gather
 * various latency statistics in CPU clock cycles, nanoseconds, or
 * microseconds. In addition, you can react to deadline violations and store
 * latency values in a shared memory ring buffer to be processed by an external
 * application.
 *
 * The library is intended specifically for analysis of isochronous cyclic
 * workloads. An isochronous cyclic workload is a code sequence in your
 * real-time application that runs repeatedly (“cyclic workload”) and has a
 * deadline that every cycle must meet (“isochronous”). Also, you can analyze
 * parts of the workload that have their own deadlines, such as data input,
 * processing, and output. For one-time measurements, the library is not
 * recommended.
 *
 * The library works in conjunction with the Instrumentation and Tracing
 * Technology API (ITT API). Use the ITT
 * API
 * __itt_task_begin() and __itt_task_end() to instrument the code and the measurement library to perform analysis of the
 * results.
 *
 * Key concepts:
 *
 * - Measured sequence: The code sequence you want to analyze. It can be the whole isochronous cyclic workload or any
 * part of it.
 *
 * - Measured block: A measured sequence instrumented with the ITT API. A measured block consists of the API call
 * __itt_task_begin(), followed by one or more iterations of the measured sequence, followed by the API call
 * __itt_task_end(). The begin function uses the CPU’s timestamp counter (TSC) to collect the start time of the
 * measured block. The end function uses the TSC to collect the end time of the measured block, calculating the
 * difference between the start and end timestamps in order to get the latency of the measured block.
 *
 * The begin and end functions together add minimal overhead. To reduce the relative cost of
 * the functions, you can run the measured sequence multiple times within the measured block. This can be useful,
 * for example, when your data processing code contains an inner loop with multiple iterations of the measured
 * sequence. In this case, the latency of the measured block is the sum of multiple iterations of the measured sequence.
 *
 * For details, see the Developer Guide.
 */

#include "tcc/err_code.h"
#include "time.h"
#include <ittnotify/ittnotify.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef _TCC_MEASUREMENT_H_
#define _TCC_MEASUREMENT_H_

#ifdef NO_TCC_MEASUREMENT
#define USE_TCC_MEASUREMENT 0
#else
#define USE_TCC_MEASUREMENT 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TCC_MEASUREMENT_NAME_MAX 128
#define unlikely(expr) __builtin_expect(!!(expr), 0)

struct tcc_measurement;

/**
 * @brief Deadline violation callback
 *
 * Defines a callback function of this type to be called when the latency of the measured sequence exceeds the deadline.
 * @param [in] context Pointer to the measurement structure associated with the callback.
 * Use this to retrieve additional information such as measurement name.
 * @param [in] latency Latency value that triggered the deadline violation.
 */
typedef void (*deadline_callback_t)(struct tcc_measurement* context, uint64_t latency);

/**
 * @brief Measurement ring buffer structure
 *
 * The structure stores the internal state data for a lock-free ring buffer with the ability to track a deadline
 * violation.
 * The fields of the structure are used internally by the measurement library. Don't manipulate the fields directly.
 */
struct tcc_measurement_buffer
{
    uint64_t buffer_size;  /**< History buffer size */
    uint64_t writer_index; /**< Index of the recorded measurement  */
    uint64_t reader_index; /**< Index of the read measurement   */
};
/**
 * @brief Measurement structure
 *
 * The internal measurement structure associated with the specific measurement identified by Instrumentation and Tracing
 * Technology API (ITT API) domain and task name.
 * Contains the start timestamp of the most recent measured block, measurement statistics, and measurement history.
 * The structure is automatically allocated and initialized when you use __itt_task_begin() or tcc_measurement_get().
 * You can access the structure by using tcc_measurement_get(). The structure is automatically destroyed when you
 * exit the application. The fields of the structure are used internally by the measurement library. Don't manipulate
 * the fields directly.
 */
struct tcc_measurement
{
    uint64_t clk_last_diff;              /**< Last measured block duration */
    volatile uint64_t clk_start;         /**< Start Timestamp Counter (TSC) of a measured block */
    uint64_t clk_avgc;                   /**< Cumulative number of processed iterations of the measured sequence to
                                              calculate the average latency of the measured sequence */
    double clk_min;                      /**< Latency of the fastest measured sequence iteration */
    double clk_max;                      /**< Latency of the slowest measured sequence iteration */
    double clk_avg;                      /**< Cumulative sum of latency measurements to calculate an average */
    double clk_result;                   /**< Average latency */
    double cost;                         /**< Latency of __itt_task_begin() and __itt_task_end() */
    char name[TCC_MEASUREMENT_NAME_MAX]; /**< Name of the measurement */
    int64_t shared_memory_id;            /**< Shared memory identifier */
    bool is_full;                        /**< Flag indicating whether the buffer is full */
    deadline_callback_t deadline_notify; /**< Function to be called after a deadline violation */
    bool enable_deadline;                /**< Flag indicating whether deadline monitoring is active */
    uint64_t deadline;                   /**< Maximum latency allowed for one measured sequence,
                                             in CPU clock cycles */
    uint64_t* measurements;              /**< Array of measurement results */
    bool shared_flag; /**< Flag indicating whether shared memory should be used to store the measurement history */
    struct tcc_measurement_buffer* measurement_buffer; /**< Measurement buffer state structure */
};

/**
 * @brief  Time units for the measurement results output
 *
 * Measurement results can be printed using the following time units:
 */
enum time_units
{
    TCC_TU_CLK = 0, /**< Time in CPU clock cycles */
    TCC_TU_NS,      /**< Time in nanoseconds */
    TCC_TU_US,      /**< Time in microseconds */
    TCC_TU_UNKNOWN  /**< Unknown units. Returned by tcc_measurement_get_time_unit_from_env()
                         if it fails to get a valid time unit from the environment. */
};

typedef enum time_units TCC_TIME_UNIT;

#if USE_TCC_MEASUREMENT

/* Timestamp counter (TSC) conversion */

/**
 * @brief Converts timestamp counter (TSC) value to struct timespec.
 *
 * @param [in] clk Duration measured in CPU clock cycles
 * @return Duration converted to time units if the request was successful or zero if the request failed
 */
struct timespec tcc_measurement_convert_clock_to_timespec(uint64_t clk);

/**
 * @brief Converts struct timespec to timestamp counter (TSC) value.
 *
 * @param [in] ts Duration in time units
 * @return Duration converted to CPU clock cycles if the request was successful or zero if the request failed
 */
uint64_t tcc_measurement_convert_timespec_to_clock(struct timespec ts);

/**
 * @brief Converts timestamp counter (TSC) value to the given time units.
 *
 * @param [in] clk Duration measured in CPU clock cycles
 * @param [in] timeunit Target time units
 * @return Duration converted to time units if the request was successful or zero if the request failed
 * @return Source value when timeunit has an invalid value
 */
uint64_t tcc_measurement_convert_clock_to_time_units(uint64_t clk, TCC_TIME_UNIT timeunit);

/**
 * @brief Converts time in given time units to timestamp counter (TSC) value.
 *
 * @param [in] time_interval Duration in time units
 * @param [in] timeunit Source time units
 * @return Duration converted to CPU clock cycles if the request was successful or zero if the request failed
 * @return Source value when timeunit has an invalid value
 */
uint64_t tcc_measurement_convert_time_units_to_clock(uint64_t time_interval, TCC_TIME_UNIT timeunit);

/* Ring buffer operations */
/**
 * @brief Determines whether a measurement ring buffer exists in shared memory.
 *
 * Used in monitoring mode (when the measurement structure in the real-time application is initialized with shared_flag
 * = true).
 * This function is intended to be used in a separate reader application that monitors measurements from the real-time
 * application.
 * Call this function before attempting to read measurements with tcc_measurement_buffer_get().
 *
 * @param [in] name String identifier of the measurement
 * @retval 1 Success, shared memory exists
 * @retval 0 Shared memory doesn't exist
 */
int tcc_measurement_buffer_check(const char* name);

/**
 * @brief Allocates the measurement ring buffer in shared memory or attaches to the existing one.
 *
 * Used in monitoring mode (when the measurement structure in the real-time application is initialized with shared_flag
 * = true).
 * This function is intended to be used in a separate reader application that monitors measurements from the real-time
 * application.
 * Call this function to initialize the buffer.
 *
 * @param [in] history_size Maximum number of measurements to store in the buffer. If the total
 *        number of measurements exceeds this value, the library overwrites previous measurements.
 * @param [in] name String identifier of the measurement
 * @param [in,out] buffer Pointer to the measurement ring buffer structure
 * @retval non-negative Success, shared memory id
 * @retval -TCC_E_MEMORY Failed to attach shared memory buffer (::TCC_E_MEMORY)
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 */
int64_t tcc_measurement_buffer_init(size_t history_size, const char* name, struct tcc_measurement_buffer** buffer);

/**
 * @brief Reads a value from the array of measurements in the buffer initialized by tcc_measurement_buffer_init().
 *
 * Used in monitoring mode (when the measurement structure in the real-time application is initialized with shared_flag
 * = true).
 * This function is intended to be used in a separate reader application that monitors measurements from the real-time
 * application.
 * Call this function after tcc_measurement_buffer_check() determines that the buffer exists.
 *
 * @param [in,out] buffer Pointer to the measurement ring buffer structure
 * @retval >0 Success, value from buffer
 * @retval 0 Buffer is empty. You need to wait for the writer application to input the data.
 */
uint64_t tcc_measurement_buffer_get(struct tcc_measurement_buffer* buffer);

/* tcc_measurement structure operations */

/**
 * @brief Retrieves the measurement structure by ITT domain and task name.
 *
 * It's important to call this function and make sure it returns success before running any other operations with the
 * measurement structure.
 *
 * @param [in] domain ITT domain created by __itt_domain_create()
 * @param [in] handle ITT string handle for task name
 * @param [out] measurement Pointer to the measurement structure
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 * @retval -TCC_E_BAD_PARAM Wrong or incompatible parameter (::TCC_E_BAD_PARAM).
 */
int tcc_measurement_get(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement);

/**
 * @brief Calculates the average latency for the measured sequence.
 *
 * The calculated average latency is also stored in the clk_result field of the measurement structure.
 *
 * @param [in,out] measurement Pointer to the measurement structure
 *
 * @return Calculated average latency for the measured sequence
 * @retval 0 if no code measurement done so far
 */
static __attribute__((always_inline)) inline double tcc_measurement_get_avg(struct tcc_measurement* measurement)
{
    double avg_c = 0.0;

    if (measurement->clk_avgc > 0) {
        avg_c = (measurement->clk_avg / ((double)measurement->clk_avgc));
    }
    measurement->clk_result = avg_c;
    return avg_c;
}

/**
 * @brief Prints a summary of the measurement data to a file.
 *
 * @param [in,out] measurement Pointer to the measurement structure
 * @param [in] out Pointer assigned to the output file
 * @param [in] timeunit Time unit for printing measurement data
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_ACCESS  Access rights or I/O problem (::TCC_E_ACCESS)
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 * @retval -TCC_E_BAD_PARAM Wrong or incompatible parameter (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_print_summary(struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit);

/**
 * @brief Prints the measurement data history to a file and clears already accumulated values.
 *
 * @param [in] measurement Pointer assigned to the measurement structure
 * @param [in] out Pointer assigned to the output file
 * @param [in] timeunit Time unit for printing measurement data
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_ACCESS Access rights or I/O problem (::TCC_E_ACCESS)
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 * @retval -TCC_E_BAD_PARAM Wrong or incompatible parameter (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_print_history(struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit);

/**
 * @brief Prints the measurement data to the console.
 *
 * @param [in] measurement Pointer assigned to the measurement structure
 * @param [in] timeunit Time unit for printing measurement data
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_ACCESS Access rights or I/O problem (::TCC_E_ACCESS)
 * @retval -TCC_E_BAD_PARAM Wrong or incompatible parameter (::TCC_E_BAD_PARAM)
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 */
int tcc_measurement_print(struct tcc_measurement* measurement, TCC_TIME_UNIT timeunit);

/**
 * @brief Prints the last measurement result to a file.
 *
 * @param [in] measurement Pointer assigned to the measurement structure
 * @param [in] out Pointer assigned to the output file
 * @param [in] timeunit Time unit for printing measurement data
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_ACCESS Access rights or I/O problem (::TCC_E_ACCESS)
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 * @retval -TCC_E_BAD_PARAM Wrong or incompatible parameter (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_print_last_record(struct tcc_measurement* measurement, FILE* out, TCC_TIME_UNIT timeunit);

/**
 * @brief Sets a deadline value and callback for the specified measurement structure.
 *
 * @param [in] measurement Pointer assigned to the measurement structure
 * @param [in] deadline Maximum tolerable latency for the measured sequence, in CPU clock cycles
 * @param [in] deadline_callback Function to be called after deadline violation
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 * @retval -TCC_E_BAD_PARAM Wrong or incompatible parameter (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_set_deadline(struct tcc_measurement* measurement,
    uint64_t deadline,
    deadline_callback_t deadline_callback);

#else /* USE_TCC_MEASUREMENT == 0 */

static inline int tcc_measurement_get(__attribute__((unused)) __itt_domain* domain,
    __attribute__((unused)) __itt_string_handle* handler,
    __attribute__((unused)) struct tcc_measurement** measurement)
{
    return -TCC_E_NOT_AVAILABLE;
}

static inline struct timespec tcc_measurement_convert_clock_to_timespec(__attribute__((unused)) uint64_t clk_delta)
{
    struct timespec ret = {0, 0};
    return ret;  // Zero Time
}

static inline uint64_t tcc_measurement_convert_timespec_to_clock(__attribute__((unused)) struct timespec ts_delta)
{
    return 0;  // Error
}

static inline uint64_t tcc_measurement_convert_clock_to_time_units(uint64_t clk,
    __attribute__((unused)) TCC_TIME_UNIT timeunit)
{
    return clk;
}

static inline uint64_t tcc_measurement_convert_time_units_to_clock(uint64_t time_interval,
    __attribute__((unused)) TCC_TIME_UNIT timeunit)
{
    return time_interval;
}

static inline int tcc_measurement_buffer_check(__attribute__((unused)) const char* name)
{
    return 0;  // Shared memory doesn't exist
}

static inline int64_t tcc_measurement_buffer_init(__attribute__((unused)) size_t history_size,
    __attribute__((unused)) const char* name,
    __attribute__((unused)) struct tcc_measurement_buffer** buffer)
{
    return -TCC_E_NOT_AVAILABLE;
}

static inline uint64_t tcc_measurement_buffer_get(__attribute__((unused)) struct tcc_measurement_buffer* buffer)
{
    return 0;  // Buffer is empty
}

static inline double tcc_measurement_get_avg(__attribute__((unused)) struct tcc_measurement* measurement)
{
    return 0.;
}

static inline int tcc_measurement_print_summary(__attribute__((unused)) struct tcc_measurement* measurement,
    __attribute__((unused)) FILE* out,
    __attribute__((unused)) TCC_TIME_UNIT timeunit)
{
    return -TCC_E_NOT_AVAILABLE;
}

static inline int tcc_measurement_print_history(__attribute__((unused)) struct tcc_measurement* measurement,
    __attribute__((unused)) FILE* out,
    __attribute__((unused)) TCC_TIME_UNIT timeunit)
{
    return -TCC_E_NOT_AVAILABLE;
}

static inline int tcc_measurement_print(__attribute__((unused)) struct tcc_measurement* measurement,
    __attribute__((unused)) TCC_TIME_UNIT timeunit)
{
    return -TCC_E_NOT_AVAILABLE;
}
static inline int tcc_measurement_print_last_record(__attribute__((unused)) struct tcc_measurement* measurement,
    __attribute__((unused)) FILE* out,
    __attribute__((unused)) TCC_TIME_UNIT timeunit)
{
    return -TCC_E_NOT_AVAILABLE;
}
static inline int tcc_measurement_set_deadline(__attribute__((unused)) struct tcc_measurement* measurement,
    __attribute__((unused)) uint64_t deadline,
    __attribute__((unused)) deadline_callback_t deadline_callback)
{
    return -TCC_E_NOT_AVAILABLE;
}

#endif /* USE_TCC_MEASUREMENT */

#ifdef __cplusplus
}
#endif

#endif  // _TCC_MEASUREMENT_H_
