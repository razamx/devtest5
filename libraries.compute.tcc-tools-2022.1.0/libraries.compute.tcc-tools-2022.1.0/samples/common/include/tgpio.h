/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TGPIO_H_
#define _TCC_TGPIO_H_

// Use as a workaround for ptp_clock.h issue on different kernels
#define KERNEL_HEADER_WORKAROUND

#ifdef KERNEL_HEADER_WORKAROUND
#include <sys/utsname.h>
#define PTP_PEROUT_FREQ_ADJ_5_4 (1 << 1)
#define PTP_PEROUT_FREQ_ADJ_5_10 (1 << 3)
#endif  //KERNEL_HEADER_WORKAROUND

#include "tgpio_defs.h"
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    PTP_FUNC_NONE = 0,
    PTP_FUNC_IN,
    PTP_FUNC_OUT
} ptp_func;

/* Set up Time-Aware GPIO pin function */
int tgpio_pin_setfunc(int fd, uint32_t pin, ptp_func func, uint32_t channel);

/* Control Time-Aware GPIO periodic output */
int tgpio_perout_request(int fd, uint32_t channel, uint64_t period_ns, uint64_t start_ns, unsigned int flags);

/* Control Time-Aware GPIO external timestamp request */
int tgpio_extts_request(int fd, uint32_t channel, int enable);

/* Get the number of events (edges) and timestamp of the last event */
int tgpio_get_event_count_tstamp(int fd, struct ptp_event_count_tstamp* event_count);

/* Get the timestamp of the TGPIO clock and system clock simultaneously */
int tgpio_get_sys_offset_precise(int fd, struct ptp_sys_offset_precise* offset);

#ifdef KERNEL_HEADER_WORKAROUND
char* get_kernel_version();
unsigned int GET_PTP_PEROUT_FREQ_ADJ();
#endif  //KERNEL_HEADER_WORKAROUND

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif  //_TCC_TGPIO_H_
