/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef _TCC_TCC_TIME_H_
#define _TCC_TCC_TIME_H_

#include <stdint.h>  // uint64_t
#include <time.h>    // struct timespec

#ifdef __cplusplus
extern "C" {
#endif

#define TCC_NSEC_PER_SEC (1000000000U)
#define TCC_NSEC_PER_MSEC (1000000U)
#define TCC_USEC_PER_SEC (1000000U)
#define TCC_NSEC_PER_USEC (1000U)
#define TCC_USEC_PER_MSEC (1000U)
#define TCC_NSEC_PER_USEC (1000U)

/**
 * @brief Convert cpu clocks to timespec
 **/
struct timespec tcc_clk2timespec(uint64_t clk);

/**
 * @brief Convert timespec to cpu clocks
 **/
uint64_t tcc_timespec2clk(struct timespec ts);

/**
 * @brief Convert nanoseconds to timespec
 **/
struct timespec tcc_ns2timespec(uint64_t ns);

/**
 * @brief Convert nanoseconds to cpu clocks
 **/
uint64_t tcc_timespec2ns(struct timespec ts);

/**
 * @brief Convert cpu clocks to nanoseconds
 **/
#define tcc_clk2ns(clk) tcc_timespec2ns(tcc_clk2timespec(clk))

/**
 * @brief Convert nanoseconds to cpu clocks
 **/
#define tcc_ns2clk(ns) tcc_timespec2clk(tcc_ns2timespec(ns))

#ifdef __cplusplus
}
#endif

#endif  // _TCC_TCC_TIME_H_
