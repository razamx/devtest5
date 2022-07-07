/* This sample is a simple self-contained example program demonstrating a Time-Aware GPIO (TGPIO) periodic output
scenario.
The sample generates a signal based on the specified period (1 second, for example). */

/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#pragma GCC diagnostic ignored "-Wunused-result"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "tcc/err_code.h"
#include "macros.h"
#include "args_parse.h"
#include "tgpio.h"

#define UNUSED(x) ((void)(x))
#define TGPIO_DEVICE "/dev/ptp0"
#define TCC_NSEC_PER_SEC (1000000000U)
#define SLEEP_TIME_USEC (1000000)
#define CROSSTIMESTAMP_SLEEP_TIME_USEC (1000)

struct settings_t
{
    unsigned int pin;
    unsigned int channel;
    const char* device;
    uint64_t period;
};

uint64_t ptptime2ns(struct ptp_clock_time);
struct ptp_clock_time ns2ptptime(uint64_t nsec);
void signal_handler(int);


int main(int argc, char** argv)
{
    struct ptp_sys_offset_precise prev_offset = {};
    struct ptp_sys_offset_precise cur_offset = {};

    /* Catch signal to enable correct shutdown by Ctrl+C interrupt */
    signal(SIGINT, signal_handler);

    settings_t sample_setting = {.pin = 0, .channel = 0, .device = TGPIO_DEVICE, .period = TCC_NSEC_PER_SEC};

    TCC_ARGS_PARSE_STATUS tcc_parse_status = parse_args(argc, argv, &sample_setting);
    if (tcc_parse_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    } else if (tcc_parse_status == TCC_PARSE_ERROR) {
        pr_err("Cannot recognize input option\n");
        print_usage();
        return -TCC_E_BAD_PARAM;
    }

    int fd = open(sample_setting.device, O_RDWR);
    if (fd < 0) {
        pr_err("Unable to open device %s: %s\n", sample_setting.device, strerror(errno));
        return -TCC_E_ERROR;
    }

    /* Set up pin's channel and output mode */
    struct ptp_pin_desc desc = {0};
    desc.index = sample_setting.pin;
    desc.func = 2;
    desc.chan = sample_setting.channel;
    ioctl(fd, PTP_PIN_SETFUNC2, &desc);

    /*
    Convert system time to the ART clock to set the start time
    correctly. Use hardware cross-timestamping to read ART and TSC clocks
    simultaneously. Two cross-timestamps with some delay between them provide the ability
    to calculate the clocks' speed ratio and convert timestamps from one to another.
    */

    /* Calculate relation between ART and TSC clocks.
       Use hardware cross-timestamping to get simultaneous ART and TSC timestamps*/
    ioctl(fd, PTP_SYS_OFFSET_PRECISE2, &prev_offset);
    usleep(CROSSTIMESTAMP_SLEEP_TIME_USEC);
    ioctl(fd, PTP_SYS_OFFSET_PRECISE2, &cur_offset);

    uint64_t art_cur_ns = ptptime2ns(cur_offset.device);
    uint64_t tsc_cur_ns = ptptime2ns(cur_offset.sys_realtime);
    long double art_to_tsc_ratio = (long double)(art_cur_ns - ptptime2ns(prev_offset.device)) /
                                   (tsc_cur_ns - ptptime2ns(prev_offset.sys_realtime));

    /* Base time */
    /* Measure offsets from the beginning of the current second + 2, i.e., zero nanoseconds */
    uint64_t tsc_basetime = (cur_offset.sys_realtime.sec + 2) * TCC_NSEC_PER_SEC;
    int64_t delta = tsc_basetime - tsc_cur_ns;
    uint64_t art_basetime = art_cur_ns + delta * art_to_tsc_ratio;

    /* Set up signal generation (period and start) for this channel
     * Note: TGPIO PTP API sets the half-period of the signal */
    struct ptp_perout_request request = {0};
    request.index = sample_setting.channel;
    request.period = ns2ptptime(sample_setting.period);
    request.start = ns2ptptime(art_basetime);
    if (ioctl(fd, PTP_PEROUT_REQUEST2, &request) == -1) {
        pr_err("Failed to start signal generation: %s\n", strerror(errno));
        close(fd);
        return -TCC_E_ERROR;
    }

    printf("Start periodic output.\n"
           "Pin %d, channel %d, period %ld nsec\n"
           "To interrupt, use Ctrl+C\n",
        sample_setting.pin,
        sample_setting.channel,
        sample_setting.period);

    while (usleep(SLEEP_TIME_USEC) != -1) {
        ;
    }
    printf("\nDone\n");

    /* Shut down TGPIO pin */
    request.start.sec = 0;
    request.start.nsec = 0;
    request.period.sec = 0;
    request.period.nsec = 0;
    ioctl(fd, PTP_PEROUT_REQUEST2, &request);
    desc.func = 0;
    ioctl(fd, PTP_PIN_SETFUNC2, &desc);
    close(fd);

    return TCC_E_SUCCESS;
}

__attribute__((const)) uint64_t ptptime2ns(struct ptp_clock_time time)
{
    return time.sec * TCC_NSEC_PER_SEC + time.nsec;
}

__attribute__((const)) struct ptp_clock_time ns2ptptime(uint64_t nsec)
{
    return (struct ptp_clock_time) {
        nsec / TCC_NSEC_PER_SEC,                  // seconds
        (unsigned int)(nsec % TCC_NSEC_PER_SEC),  // nanoseconds
        0                                         // reserved
    };
}

void signal_handler(int dummy)
{
    UNUSED(dummy);
}

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* setting)
{
    static const struct option opts[] = {{"help", no_argument, NULL, 'h'},
        {"pin", required_argument, NULL, 'p'},
        {"channel", required_argument, NULL, 'c'},
        {"device", required_argument, NULL, 'd'},
        {"period", required_argument, NULL, 't'},
        {NULL, 0, NULL, 0}};

    int index = 0;
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "h", opts, &index);
        switch (opt) {
        case 'p':
            setting->pin = (unsigned int)atoi(optarg);
            break;
        case 'c':
            setting->channel = (unsigned int)atoi(optarg);
            break;
        case 'd':
            setting->device = optarg;
            break;
        case 't':
            setting->period = strtoull(optarg, NULL, DECIMAL_NUM_SYS);
            break;
        case 'h':
            return TCC_PRINT_HELP;
        case -1:
            break;
        default:
            return TCC_PARSE_ERROR;
        }
    }
    return TCC_PARSE_SUCCESS;
}

void print_usage()
{
    printf("Usage: tcc_tgpio_basic_periodic_output [--pin N] [--channel N] [--device <device_name>] [--period N]\n"
           "Options:\n"
           "    --pin      Specify the output pin index. Default: 0.\n"
           "    --channel  Specify the channel for the output pin. Default: 0.\n"
           "    --device   Specify the TGPIO device. Default: \"/dev/ptp0\".\n"
           "    --period   Specify the output period in nanoseconds. Default: 1000000000 (1 second).\n");
}
