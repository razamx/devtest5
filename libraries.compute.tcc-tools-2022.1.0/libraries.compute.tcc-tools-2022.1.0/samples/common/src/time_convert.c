/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "time_convert.h"
#include "tgpio_defs.h"
#include <time.h>

__attribute__((const)) uint64_t ptptime2ns(struct ptp_clock_time time)
{
    return time.sec * BILLION + time.nsec;
}

__attribute__((const)) uint64_t timespec2ns(struct timespec time)
{
    return time.tv_sec * BILLION + time.tv_nsec;
}

struct ptp_clock_time ns2ptptime(uint64_t time_ns)
{
    return (struct ptp_clock_time) {.sec = time_ns / BILLION, .nsec = time_ns % BILLION};
}

__attribute__((const)) struct timespec ns2timespec(uint64_t time_ns)
{
    return (struct timespec) {.tv_sec = time_ns / BILLION, .tv_nsec = time_ns % BILLION};
}

__attribute__((const)) clockid_t get_clockid_from_fd(int fd)
{
    return (~(clockid_t)fd << 3) | 3;
}

__attribute__((pure)) static long double calc_art_to_tsc_ratio(struct ptp_sys_offset_precise* offset_prev,
    struct ptp_sys_offset_precise* offset_cur)
{
    uint64_t art_prev_ns = ptptime2ns(offset_prev->device);
    uint64_t sys_prev_ns = ptptime2ns(offset_prev->sys_realtime);
    uint64_t art_cur_ns = ptptime2ns(offset_cur->device);
    uint64_t sys_cur_ns = ptptime2ns(offset_cur->sys_realtime);
    return (long double)(art_cur_ns - art_prev_ns) / (sys_cur_ns - sys_prev_ns);
}

__attribute__((pure)) uint64_t convert_art_to_sys(struct ptp_sys_offset_precise* offset_prev,
    struct ptp_sys_offset_precise* offset_cur,
    uint64_t art_timestamp)
{
    long double art_to_tsc_ratio = calc_art_to_tsc_ratio(offset_prev, offset_cur);
    int64_t delta = ptptime2ns(offset_cur->device) - art_timestamp;
    return ptptime2ns(offset_cur->sys_realtime) - delta / art_to_tsc_ratio;
}

__attribute__((pure)) uint64_t convert_sys_to_art(struct ptp_sys_offset_precise* offset_prev,
    struct ptp_sys_offset_precise* offset_cur,
    uint64_t sys_timestamp)
{
    long double art_to_tsc_ratio = calc_art_to_tsc_ratio(offset_prev, offset_cur);
    int64_t delta = ptptime2ns(offset_cur->sys_realtime) - sys_timestamp;
    return ptptime2ns(offset_cur->device) - delta * art_to_tsc_ratio;
}
