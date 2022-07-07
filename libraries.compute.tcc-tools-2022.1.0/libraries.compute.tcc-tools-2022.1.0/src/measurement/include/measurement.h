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
 * The library is intended for analysis of isochronous cyclic workloads and parts of these workloads such as data input,
 * processing, and output. An isochronous cyclic workload is a code sequence that runs multiple times (iterations), and
 * each iteration
 * must be executed within a defined period. The defined period is also known as maximum tolerable latency or deadline
 * and is the same for each iteration. Correspondingly, each part of the cyclic workload has its own deadline. The
 * maximum
 * measured latency value among all iterations is known as the worst-case execution time (WCET). The library enables
 * you to measure WCET and other latency statistics in CPU clock cycles and time units (nanoseconds or microseconds).
 * In addition, you can use the library to react on the deadline violations and/or store the latency values in a shared
 * memory
 * ring buffer to be processed by an external application.
 *
 * Key concepts:
 *
 * - Measured sequence: Code sequence that you want to analyze. It can be the whole cycle or any part of it.
 *
 * - Measured block: Block of code consisting of one or multiple iterations of the measured sequence and the
 * measurement library functions needed to measure the latency. A measured block contains tcc_measurement_start(),
 * followed by
 * the workload code, followed by tcc_measurement_end() or tcc_measurement_end_ex(). The start function uses the CPU's
 * timestamp counter (TSC) to collect the start time of the measured block. Either end function uses the TSC to collect
 * the end
 * time of the measured block, and calculates the difference between the start and end timestamps to get the latency of
 * the measured block.
 *
 * The start and end functions together add about 200 nanoseconds of latency. To reduce the relative cost of the
 * functions,
 * you can run the measured sequence multiple times within the measured block. This can be useful, for example,
 * when your data processing code contains an inner loop with multiple iterations of the measured sequence.
 * In this case, the latency of the measured block is the sum of multiple iterations of the measured sequence. The
 * library calculates
 * the latency statistics of the measured sequence by dividing the latency of the measured block by the number of
 * iterations.
 *
 */

#include "tcc/err_code.h"
#include "tcc/measurement.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifndef _TCC_MEASUREMENT_INTERNAL_H_
#define _TCC_MEASUREMENT_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gets the Timestamp Counter (TSC) value for the start of the measured block.
 *
 * This function prevents out-of-order execution before reading TSC.
 * The function uses the LFENCE instruction:
 *   - No out-of-order (OOO) execution
 *   - Load buffers (LB) are empty after LFENCE
 *   - No deliberate restrictions on store buffers (SB), some stores may drain though
 *
 * Other options to prevent OOO are:
 *   - cpuid; affects LB and SB (both get emptied)
 *   - Forced branch miss-prediction; no effect on LB or SB but
 *     loads/stores may drain
 *
 * When the measured block has very high latency, preventing OOO
 * may not be required, and RDTSCP instruction may be enough.
 *
 * @return TSC value
 */
static __attribute__((always_inline)) inline uint64_t __tsc_start(void)
{
    uint32_t cycles_high;
    uint32_t cycles_low;

#ifdef __x86_64__
    asm volatile("lfence\n\t"
                 "rdtscp\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 : "=r"(cycles_high), "=r"(cycles_low)
                 :
                 : "%rax", "%rdx");
#else
    asm volatile("lfence\n\t"
                 "rdtscp\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 : "=r"(cycles_high), "=r"(cycles_low)
                 :
                 : "%eax", "%edx");
#endif
    return (((uint64_t)cycles_high << 32) | cycles_low);
}

/**
 * @brief Gets the Timestamp Counter (TSC) value for the end of the measured block.
 *
 * No out-of-order (OOO) prevention required. RDTSCP is used here, which makes sure
 * all previous instructions retire before reading TSC.
 *
 * @return TSC value
 */
static __attribute__((always_inline)) inline uint64_t __tsc_end(void)
{
    uint32_t cycles_high;
    uint32_t cycles_low;

#ifdef __x86_64__
    asm volatile("rdtscp\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 : "=r"(cycles_high), "=r"(cycles_low)
                 :
                 : "%rax", "%rdx");
#else
    asm volatile("rdtscp\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 : "=r"(cycles_high), "=r"(cycles_low)
                 :
                 : "%eax", "%edx");
#endif
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

/**
 * @brief Starts a measured block.

 * Collects the current timestamp from the CPU's timestamp counter (TSC) and puts the value in the clk_start field of
 * the measurement structure.
 *
 * tcc_measurement_start() and tcc_measurement_end() or tcc_measurement_end_ex() can be called multiple times with
 different
 * measurement structures. For example, when a real-time application contains multiple consecutive stages, you can
 measure
 * each stage individually. This can help to isolate the biggest source of jitter among the stages.
 *
 * @param [in,out] measurement Pointer to the measurement structure
 */
static __attribute__((always_inline)) inline void tcc_measurement_start(struct tcc_measurement* measurement)
{
    measurement->clk_start = __tsc_start();
}

/**
 * @brief Store measurement results to measurement structure
 *
 * @param [in/out] measurement Pointer to the measurement structure
 * @param [in] count Number of measured sequence iterations processed within the measured block.
 */
void __tcc_measurement_store_measure(struct tcc_measurement* measurement, const unsigned count);

/**
 * @brief Stops a measured block.
 *
 * Collects the current timestamp from the CPU's timestamp counter (TSC) and puts the value in the clk_last_diff
 * field of the measurement structure. Provides tcc_measurement_end() capabilities and the ability to set
 * a different start timestamp from the one collected by tcc_measurement_start().
 *
 * @param [in,out] measurement Pointer to the measurement structure
 * @param [in] count Number of measured sequence iterations processed within the measured block.
 *        This allows the library to calculate the average latency for the measured sequence.
 * @param [in] clk_start Start Timestamp Counter (TSC) value. This is useful when using one
 *        start TSC reading for multiple different measurements.
 */
static __attribute__((always_inline)) inline void tcc_measurement_end_ex(struct tcc_measurement* measurement,
    const unsigned count,
    const uint64_t clk_start)
{
    uint64_t diff = __tsc_end() - clk_start;
    /* Store raw TSC difference in clk_last_diff */
    measurement->clk_last_diff = diff;
    __tcc_measurement_store_measure(measurement, count);
}

/**
 * @brief Stops the measured block.
 *
 * Collects the current timestamp from the CPU's timestamp counter (TSC) and puts the value in the clk_last_diff
 * field of the measurement structure.
 *
 * @param [in,out] measurement Pointer to the measurement structure
 * @param [in] count Number of measured sequence iterations processed within the measured block.
 *        This allows the library to calculate the average latency for the measured sequence.
 */
//
static __attribute__((always_inline)) inline void tcc_measurement_end(struct tcc_measurement* measurement,
    const unsigned count)
{
    tcc_measurement_end_ex(measurement, count, measurement->clk_start);
}

/**
 * @brief Initializes the measurement structure.
 *
 * You should use tcc_measurement_destroy() to free allocated resources.
 *
 * @param [in,out] measurement Pointer to the measurement structure
 * @param [in] history_size Maximum number of measurements to store in the measurement history. If the total
 *        number of measurements exceeds the history_size value, the library overwrites previous measurements.
 * @param [in] name String identifier of the measurement
 * @param [in] ... Variadic arguments depending on \a name format. For example, tcc_measurement_init(measurement, 100,
 * "Workload:%d", 1000)
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_MEMORY Failed to allocate history buffer (::TCC_E_MEMORY)
 * @retval -TCC_E_BAD_PARAM \a measurement is NULL (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_init(struct tcc_measurement* measurement, size_t history_size, const char* name, ...);

/**
 * @brief Initializes the measurement structure.
 *
 * Provides tcc_measurement_init() capabilities and enables deadline monitoring.
 * You should use tcc_measurement_destroy() to free allocated resources.
 *
 * @param [in,out] measurement Pointer to the measurement structure
 * @param [in] history_size Maximum number of measurements to store in the measurement history. If the total
 *        number of measurements exceeds the history_size value, the library overwrites previous measurements.
 * @param [in] deadline Maximum tolerable latency for the measured sequence, in CPU clock cycles
 * @param [in] function_notify Function to be called after deadline violation
 * @param [in] shared_flag Flag to indicate whether shared memory should be used to store the measurement history
 * @param [in] name String identifier of the measurement
 * @param [in] ... Variadic arguments depending on \a name format. For example, tcc_measurement_init_ex(measurement,
 * 100, 100000, function, true, "Workload:%d", 1000)
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_MEMORY Failed to allocate history buffer (::TCC_E_MEMORY)
 * @retval -TCC_E_BAD_PARAM \a measurement is NULL (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_init_ex(struct tcc_measurement* measurement,
    size_t history_size,
    uint64_t deadline,
    deadline_callback_t function_notify,
    bool shared_flag,
    const char* name,
    ...);


/**
 * @brief Destroys the measurement structure.
 *
 * @param [in] measurement Pointer to the measurement structure
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_MEMORY Failed to free history buffer (::TCC_E_MEMORY)
 * @retval -TCC_E_BAD_PARAM \a measurement is NULL (::TCC_E_BAD_PARAM)
 */
int tcc_measurement_destroy(struct tcc_measurement* measurement);

/**
 * @brief Helper function to extract the shared memory usage flag from the environment variable.
 *
 * retval Is shared memory flag is set
 */
int tcc_measurement_shared_memory_flag_from_env();

#ifdef __cplusplus
}
#endif

#endif  // _TCC_MEASUREMENT_INTERNAL_H_
