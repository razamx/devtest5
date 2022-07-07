/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TIME_CONVERT_H_
#define _TCC_TIME_CONVERT_H_

#include "tgpio_defs.h"
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BILLION 1000000000UL

uint64_t ptptime2ns(struct ptp_clock_time time);
struct ptp_clock_time ns2ptptime(uint64_t time_ns);
uint64_t timespec2ns(struct timespec time);
struct timespec ns2timespec(uint64_t time_ns);

/**
 * @brief Get the clock ID from the file descriptor
 * @param fd File descriptor of the clock device
 * @return Clock id
 */
clockid_t get_clockid_from_fd(int fd);

/**
 * @brief Converts the timestamp related to ART to the timestamp related to
 *        the CLOCK_REALTIME
 * @param offset_prev First cross-timestamp
 * @param offset_cur Second cross-timestamp
 * @param art_timestamp ART-related timestamp
 * @return CLOCK_REALTIME-related timestamp
 */
uint64_t convert_art_to_sys(struct ptp_sys_offset_precise* offset_prev,
    struct ptp_sys_offset_precise* offset_cur,
    uint64_t art_timestamp);

/**
 * @brief Converts the timestamp related to CLOCK_REALTIME to the timestamp
 *        related to the ART
 * @param offset_prev First cross-timestamp
 * @param offset_cur Second cross-timestamp
 * @param art_timestamp CLOCK_REALTIME-related timestamp
 * @return ART-related timestamp
 */
uint64_t convert_sys_to_art(struct ptp_sys_offset_precise* offset_prev,
    struct ptp_sys_offset_precise* offset_cur,
    uint64_t sys_timestamp);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif  // _TCC_TIME_CONVERT_H_
