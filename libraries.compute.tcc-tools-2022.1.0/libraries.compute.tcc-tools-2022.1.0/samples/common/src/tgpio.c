/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "tgpio.h"
#include "macros.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int tgpio_pin_setfunc(int fd, uint32_t pin, ptp_func func, uint32_t channel)
{
    if (func > PTP_FUNC_OUT || func < PTP_FUNC_NONE) {
        pr_err("Invalid function %u\n", func);
        return -TCC_E_BAD_PARAM;
    }

    struct ptp_pin_desc desc = {0};
    desc.index = pin;
    desc.func = func;
    desc.chan = channel;

    if (ioctl(fd, PTP_PIN_SETFUNC2, &desc) < 0) {
        pr_err("PTP_PIN_SETFUNC2 failed: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }
    return TCC_E_SUCCESS;
}

int tgpio_perout_request(int fd, uint32_t channel, uint64_t period_ns, uint64_t start_ns, unsigned int flags)
{
    struct ptp_perout_request request = {0};
    request.index = channel;
    request.start.sec = start_ns / BILLION;
    request.start.nsec = start_ns % BILLION;
    request.period.sec = period_ns / BILLION;
    request.period.nsec = period_ns % BILLION;
    request.flags = flags;

    if (ioctl(fd, PTP_PEROUT_REQUEST2, &request) < 0) {
        pr_err("PTP_PEROUT_REQUEST2 failed: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }
    return TCC_E_SUCCESS;
}

int tgpio_extts_request(int fd, uint32_t channel, int enable)
{
    struct ptp_extts_request request;
    request.index = channel;
    request.flags = enable ? PTP_ENABLE_FEATURE : 0;

    if (ioctl(fd, PTP_EXTTS_REQUEST2, &request) < 0) {
        pr_err("PTP_EXTTS_REQUEST2 failed: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }
    return TCC_E_SUCCESS;
}

int tgpio_get_event_count_tstamp(int fd, struct ptp_event_count_tstamp* event_count)
{
    if (ioctl(fd, PTP_EVENT_COUNT_TSTAMP2, event_count) < 0) {
        pr_err("PTP_EVENT_COUNT_TSTAMP2 failed: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }
    return TCC_E_SUCCESS;
}

int tgpio_get_sys_offset_precise(int fd, struct ptp_sys_offset_precise* offset)
{
    if (ioctl(fd, PTP_SYS_OFFSET_PRECISE2, offset) < 0) {
        pr_err("Failed to read cross-timestamp: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }
    return TCC_E_SUCCESS;
}

// Use as a workaround for ptp_clock.h issue on different kernels
#ifdef KERNEL_HEADER_WORKAROUND

#include <sys/utsname.h>

//#define __snprintf(...) (snprintf(__VA_ARGS__) < 0 ? abort() : (void)0)
#define PTP_PEROUT_FREQ_ADJ_5_4 (1 << 1)
#define PTP_PEROUT_FREQ_ADJ_5_10 (1 << 3)

char* get_kernel_version()
{
    struct utsname sysinfo;
    static char* ret = NULL;
    free(ret);
    ret = NULL;

    if (uname(&sysinfo) == -1) {
        return ret;
    }

    char* end_release = strrchr(sysinfo.release, '.');
    if (end_release == NULL) {
        return ret;
    }

    size_t kernel_version_length = end_release - sysinfo.release;
    if (kernel_version_length > 4) {
        return ret;
    }

    const size_t buffer_size = 5;
    char* kernel_version = calloc(buffer_size, sizeof(char));
    if (!kernel_version) {
        return ret;
    }

    *end_release = '\0';
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(kernel_version, buffer_size, "%s", sysinfo.release);
#pragma GCC diagnostic pop

    ret = kernel_version;
    return ret;
}

unsigned int GET_PTP_PEROUT_FREQ_ADJ()
{
    unsigned int freq_adj_flag = (unsigned int)PTP_PEROUT_FREQ_ADJ;
    char* kernel_version_str = get_kernel_version();

    if (kernel_version_str == NULL) {
        return freq_adj_flag;
    }

    if (!strcmp(kernel_version_str, "5.4")) {
        freq_adj_flag = (unsigned int)PTP_PEROUT_FREQ_ADJ_5_4;
    } else {
        freq_adj_flag = (unsigned int)PTP_PEROUT_FREQ_ADJ_5_10;
    }

    return freq_adj_flag;
}

#endif  //KERNEL_HEADER_WORKAROUND
