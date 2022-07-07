/* This sample is a simple self-contained example program demonstrating a Time-Aware GPIO (TGPIO) one shot output
scenario.
The sample generates a pulse after a specified delay (1 second, for example). */

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

struct settings_t
{
    unsigned int pin;
    unsigned int channel;
    const char* device;
    uint64_t start;
};

struct ptp_clock_time ns2ptptime(uint64_t nsec);
uint64_t ptptime2ns(struct ptp_clock_time);


int main(int argc, char** argv)
{
    struct ptp_sys_offset_precise cur_offset = {0};
    int fd;

    settings_t sample_setting = {.pin = 0, .channel = 0, .device = TGPIO_DEVICE, .start = TCC_NSEC_PER_SEC};

    TCC_ARGS_PARSE_STATUS tcc_parse_status = parse_args(argc, argv, &sample_setting);
    if (tcc_parse_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    } else if (tcc_parse_status == TCC_PARSE_ERROR) {
        pr_err("Cannot recognize input option\n");
        print_usage();
        return -TCC_E_BAD_PARAM;
    }

    fd = open(sample_setting.device, O_RDWR);
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

    /* Read current timestamp */
    ioctl(fd, PTP_SYS_OFFSET_PRECISE2, &cur_offset);

    /* Convert the ART timestamp to nanoseconds for
     * setting up single shot output request */
    uint64_t art_cur_ns = ptptime2ns(cur_offset.device);

    /* Set up oneshot for this channel */
    struct ptp_perout_request request = {0};
    request.start = ns2ptptime(art_cur_ns + sample_setting.start);
    request.index = sample_setting.channel;
    request.flags = PTP_PEROUT_ONE_SHOT;  // Set mode flag to one shot
    if (ioctl(fd, PTP_PEROUT_REQUEST2, &request) == -1) {
        pr_err("Failed to start signal generation: %s\n", strerror(errno));
        close(fd);
        return -TCC_E_ERROR;
    }

    printf("Single shot output requested on:\n"
           "Pin %d, channel %d, delay %ld nsec\n",
        sample_setting.pin,
        sample_setting.channel,
        sample_setting.start);

    close(fd);

    return TCC_E_SUCCESS;
}

__attribute__((const)) struct ptp_clock_time ns2ptptime(uint64_t nsec)
{
    return (struct ptp_clock_time) {
        nsec / TCC_NSEC_PER_SEC,                  // seconds
        (unsigned int)(nsec % TCC_NSEC_PER_SEC),  // nanoseconds
        0                                         // reserved
    };
}

__attribute__((const)) uint64_t ptptime2ns(struct ptp_clock_time time)
{
    return time.sec * TCC_NSEC_PER_SEC + time.nsec;
}

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* setting)
{
    static const struct option opts[] = {{"help", no_argument, NULL, 'h'},
        {"pin", required_argument, NULL, 'p'},
        {"channel", required_argument, NULL, 'c'},
        {"device", required_argument, NULL, 'd'},
        {"start", required_argument, NULL, 's'},
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
        case 's':
            setting->start = strtoull(optarg, NULL, DECIMAL_NUM_SYS);
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
    printf("Usage: tcc_tgpio_basic_oneshot_output [--pin N] [--channel N] [--device <device_name>] [--start N]\n"
           "Options:\n"
           "    --pin      Specify the output pin index. Default: 0.\n"
           "    --channel  Specify the channel for the output pin. Default: 0.\n"
           "    --device   Specify the TGPIO device. Default: \"/dev/ptp0\".\n"
           "    --start    Specify the output delay in nanoseconds. Default: 1000000000 (1 second).\n");
}
