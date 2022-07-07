/* This sample is a simple self-contained example program demonstrating a Time-Aware GPIO (TGPIO) input scenario.
The sample prints a timestamp when an edge is detected, indicating a change in the TGPIO input pin state. */

/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

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

#include "args_parse.h"
#include "macros.h"
#include "tcc/err_code.h"
#include "tgpio.h"
#include "tgpio_defs.h"

#define TCC_NSEC_PER_SEC (1000000000U)
#define TIME_BUFFER_SIZE (1024)
#define SLEEP_TIME_USEC (100)
#define UNUSED(x) ((void)(x))
#define TGPIO_DEVICE "/dev/ptp0"

struct settings_t
{
    unsigned int pin;
    unsigned int channel;
    const char* device;
};

void signal_handler(int);
const char* convert_to_human_readable(time_t seconds);
uint64_t ptptime2ns(struct ptp_clock_time);

int running = 1;


int main(int argc, char** argv)
{
    settings_t sample_setting = {
        .pin = 0,
        .channel = 0,
        .device = TGPIO_DEVICE,
    };

    struct ptp_sys_offset_precise initial_offset = {0};
    struct ptp_sys_offset_precise current_offset = {0};
    struct ptp_event_count_tstamp events = {0};

    /* Catch signal to enable correct shutdown by Ctrl+C interrupt */
    signal(SIGINT, signal_handler);

    TCC_ARGS_PARSE_STATUS tcc_parse_status = parse_args(argc, argv, &sample_setting);
    if (tcc_parse_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    } else if (tcc_parse_status == TCC_PARSE_ERROR) {
        pr_err("Cannot recognize input option\n");
        print_usage();
        return -TCC_E_BAD_PARAM;
    }

    /* Configure pin */
    int fd = open(sample_setting.device, O_RDWR);
    if (fd < 0) {
        pr_err("Unable to open device %s: %s\n", sample_setting.device, strerror(errno));
        return -TCC_E_ERROR;
    }

    struct ptp_pin_desc desc = {0};
    desc.index = sample_setting.pin;
    desc.func = 1; /* Set external timestamp auxiliary function */
    desc.chan = sample_setting.channel;
    ioctl(fd, PTP_PIN_SETFUNC2, &desc);

    /* Enable external timestamp (input) mode */
    struct ptp_extts_request request = {0};
    request.index = sample_setting.channel;
    request.flags = PTP_ENABLE_FEATURE | PTP_EXTTS_EDGES; /* Detects both rising and falling edges */
    if (ioctl(fd, PTP_EXTTS_REQUEST2, &request) == -1) {
        pr_err("Failed to start signal generation: %s\n", strerror(errno));
        close(fd);
        return -TCC_E_ERROR;
    }

    /* Set up channel for event count*/
    events.index = sample_setting.channel;

    /* Get current ART and TSC timestamp using HW cross-timestamping to
       calculate relation between clocks */
    ioctl(fd, PTP_SYS_OFFSET_PRECISE2, &initial_offset);

    printf("Start input sample.\n"
           "Pin %d, channel %d\n"
           "To interrupt, use Ctrl+C\n",
        sample_setting.pin,
        sample_setting.channel);

    /* Get the current event count */
    if (ioctl(fd, PTP_EVENT_COUNT_TSTAMP2, &events) < 0) {
        pr_err("PTP_EVENT_COUNT_TSTAMP2 failed: %s\n", strerror(errno));
        close(fd);
        return -TCC_E_ERROR;
    }
    uint64_t previous_count = events.event_count;

    while (running) {
        /* Update the event counter */
        ioctl(fd, PTP_EVENT_COUNT_TSTAMP2, &events);

        /* When the event count has changed from the original value */
        if (events.event_count != previous_count) {
            /* We should convert ART timestamp read from TGPIO to display
            correct timestamps for captured edges. We use hardware
            cross-timestamping to read ART and TSC clocks simultaneously.
            Two cross-timestamps with some delay between allow to
            calculate the clocks' speed ratio and convert timestamps
            from one to another */

            /* Get current ART and PCS timestamps using HW cross-timestamping
            and calculate ART clock and system clock speed ratio */
            ioctl(fd, PTP_SYS_OFFSET_PRECISE2, &current_offset);
            uint64_t art_cur_ns = ptptime2ns(current_offset.device);
            uint64_t tsc_cur_ns = ptptime2ns(current_offset.sys_realtime);
            long double art_to_tsc_ratio = (long double)(art_cur_ns - ptptime2ns(initial_offset.device)) /
                                           (tsc_cur_ns - ptptime2ns(initial_offset.sys_realtime));

            /* Convert TGPIO event timestamp  to the system clock */
            int64_t delta = ptptime2ns(events.device_time) - ptptime2ns(initial_offset.device);
            uint64_t tsc_event = ptptime2ns(initial_offset.sys_realtime) + delta / art_to_tsc_ratio;

            printf("Detected edge on channel %u at %s.%09lu \n",
                events.index,
                convert_to_human_readable(tsc_event / TCC_NSEC_PER_SEC),
                tsc_event % TCC_NSEC_PER_SEC);

            previous_count = events.event_count;
        }

        /* Poll frequency 10 KHz. Even if signal edge occurs when application is
        sleeping, it will be captured by TGPIO HW and application will get the
        precise timestamp of the edge after resuming on the next iteration*/
        usleep(SLEEP_TIME_USEC);
    }
    printf("\nDone\n");

    /* Shut down TGPIO pin */
    request.flags = 0;
    ioctl(fd, PTP_EXTTS_REQUEST2, &request);
    desc.func = 0;
    desc.chan = 0;
    ioctl(fd, PTP_PIN_SETFUNC2, &desc);
    close(fd);

    return TCC_E_SUCCESS;
}

void signal_handler(int dummy)
{
    UNUSED(dummy);
    running = 0;
}

__attribute__((const)) uint64_t ptptime2ns(struct ptp_clock_time time)
{
    return time.sec * TCC_NSEC_PER_SEC + time.nsec;
}

const char* convert_to_human_readable(time_t seconds)
{
    static char time_string[TIME_BUFFER_SIZE];
    struct tm* hr_time = localtime(&seconds);
    if (hr_time == NULL) {
        fprintf(stderr, "Unable to convert time.");
        exit(EXIT_FAILURE);
    }
    strftime(time_string, TIME_BUFFER_SIZE, "%X", hr_time);
    return time_string;
}

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* setting)
{
    static const struct option opts[] = {{"help", no_argument, NULL, 'h'},
        {"pin", required_argument, NULL, 'p'},
        {"channel", required_argument, NULL, 'c'},
        {"device", required_argument, NULL, 'd'},
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
    printf("Usage: tcc_tgpio_basic_input [--pin N] [--channel N] [--device <device_name>]\n"
           "Options:\n"
           "    --pin      Specify the input pin index. Default: 0.\n"
           "    --channel  Specify the channel for the input pin. Default: 0.\n"
           "    --device   Specify the TGPIO device. Default: \"/dev/ptp0\".\n");
}
