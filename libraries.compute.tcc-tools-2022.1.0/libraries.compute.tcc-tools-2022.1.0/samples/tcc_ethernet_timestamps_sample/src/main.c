/* This sample is an example program that demonstrates how Intel platform features
can improve time synchronization quality between the system clock (CLOCK_REALTIME) and a
Precision Time Protocol Hardware Clock (PHC). The sample uses a Time-Aware GPIO (TGPIO)
pin to generate single pulses and the pulse per second (PPS) pin of a network controller
to generate periodic signals. */

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
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "args_parse.h"
#include "macros.h"
#include "tgpio.h"
#include "time_convert.h"

#define CROSS_TIMESTAMP_SLEEP 50000
#define UNUSED(x) ((void)(x))

int running = 1;
int verbose_output = 0;

struct device_settings
{
    char* device;
    unsigned int pin;
};

struct settings_t
{
    struct device_settings tgpio;
    struct device_settings pps;
    uint64_t period;
};

int open_fd(struct device_settings* device);
int run(settings_t* settings);
void signal_handler(int dummy);
uint64_t get_start_sys(uint64_t start_sec);
void usage(void);

int main(int argc, char** argv)
{
    settings_t settings = {};
    TCC_ARGS_PARSE_STATUS status = parse_args(argc, argv, &settings);
    if (status == TCC_PARSE_ERROR) {
        usage();
        return -TCC_E_BAD_PARAM;
    }
    if (status == TCC_PRINT_HELP) {
        usage();
        return TCC_E_SUCCESS;
    }

    return run(&settings);
}

int run(settings_t* settings)
{
    int ret = TCC_E_SUCCESS;
    int fd_tgpio = -1;
    int fd_pps = -1;
    struct ptp_sys_offset_precise offset_prev = {0};
    struct ptp_sys_offset_precise offset_cur = {0};

    /* Catch signal to enable correct shutdown by Ctrl+C interrupt */
    signal(SIGINT, signal_handler);

    /* Enable TGPIO and Ethernet controller SDP (software definable pin) */
    if ((fd_tgpio = open_fd(&settings->tgpio)) == -1) {
        pr_err("Unable to initialize the TGPIO device: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
        goto out;
    }
    if ((fd_pps = open_fd(&settings->pps)) == -1) {
        pr_err("Unable to initialize the PPS device: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
        goto out;
    }
    if ((ret = tgpio_pin_setfunc(fd_tgpio, settings->tgpio.pin, PTP_FUNC_OUT, settings->tgpio.pin)) != TCC_E_SUCCESS) {
        pr_err("tgpio_pin_setfunc failed for TGPIO: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
        goto out;
    }

    /* Get first timestamp for periodic signal */
    struct timespec first_ts = {0};
    if ((ret = clock_gettime(CLOCK_REALTIME, &first_ts)) != TCC_E_SUCCESS) {
        pr_err("Unable to get first timestamp: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
        goto out;
    }
    offset_prev.sys_realtime.sec = first_ts.tv_sec;
    offset_prev.sys_realtime.nsec = first_ts.tv_nsec;

    /* Measure offsets from the beginning of the current second + 2, i.e., zero nanoseconds */
    uint64_t start_sys = get_start_sys(offset_prev.sys_realtime.sec);
    uint64_t start_sys_prev = 0;

    /* Enable periodic signal generation */
    if ((ret = tgpio_perout_request(fd_pps, settings->pps.pin, settings->period, start_sys, 0)) != TCC_E_SUCCESS) {
        pr_err("Unable to start signal generation on PPS: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
        goto out;
    }
    printf("Started PPS signal generation. "
           "Pin/channel %" PRIu32 ", period %" PRIu64 " nsec\n",
        settings->pps.pin,
        settings->period);

    if (verbose_output) {
        printf("offset prev sys_realtime: %" PRIu64 "s, %" PRIu64 "ns\n",
            (uint64_t)offset_prev.sys_realtime.sec,
            (uint64_t)offset_prev.sys_realtime.nsec);
        printf("start  sys              : %" PRIu64 "ns\n", start_sys);
    }

    printf("To interrupt, use Ctrl+C\n\n");

    while (running) {
        /* Get current ART and TSC timestamp using HW cross-timestamping to
        calculate relation between clocks */
        if ((ret = tgpio_get_sys_offset_precise(fd_tgpio, &offset_prev)) != TCC_E_SUCCESS) {
            pr_err("Unable to get cross-timestamp on TGPIO: %s\n", strerror(errno));
            ret = -TCC_E_ERROR;
            goto out;
        }
        usleep(CROSS_TIMESTAMP_SLEEP);
        if ((ret = tgpio_get_sys_offset_precise(fd_tgpio, &offset_cur)) != TCC_E_SUCCESS) {
            pr_err("Unable to get cross-timestamp on TGPIO: %s\n", strerror(errno));
            ret = -TCC_E_ERROR;
            goto out;
        }

        /* Convert start timestamp from system time to ART */
        uint64_t start_art = convert_sys_to_art(&offset_prev, &offset_cur, start_sys);

        /* Set up TGPIO to generate single edge at the same time as periodic output edge */
        if ((ret = tgpio_perout_request(
                 fd_tgpio, settings->tgpio.pin, settings->period, start_art, PTP_PEROUT_ONE_SHOT)) != TCC_E_SUCCESS) {
            pr_err("Unable to start signal generation on TGPIO: %s\n", strerror(errno));
            ret = -TCC_E_ERROR;
            goto out;
        }
        printf("Generated single pulse on TGPIO. Start time: %" PRIu64 "ns\n", start_art);

        if (verbose_output) {
            printf("offset prev sys_realtime: %" PRIu64 "s, %" PRIu64 "ns\n",
                (uint64_t)offset_prev.sys_realtime.sec,
                (uint64_t)offset_prev.sys_realtime.nsec);
            printf("offset prev device      : %" PRIu64 "s, %" PRIu64 "ns\n",
                (uint64_t)offset_prev.device.sec,
                (uint64_t)offset_prev.device.nsec);
            printf("offset cur  sys_realtime: %" PRIu64 "s, %" PRIu64 "ns\n",
                (uint64_t)offset_cur.sys_realtime.sec,
                (uint64_t)offset_cur.sys_realtime.nsec);
            printf("offset cur  device      : %" PRIu64 "s, %" PRIu64 "ns\n",
                (uint64_t)offset_cur.device.sec,
                (uint64_t)offset_cur.device.nsec);
            printf("start  sys              : %" PRIu64 "ns\n", start_sys);
            printf("start  art              : %" PRIu64 "ns\n", start_art);
            printf("\n");
        }

        start_sys_prev = start_sys;
        start_sys += settings->period / 2;

        /* Wait for next periodic output edge to generate next single pulse */
        struct timespec start_sys_ts = ns2timespec(start_sys_prev);
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &start_sys_ts, NULL);
    }

    printf("\nDone\n");

out:
    /* deinit */
    if ((ret = tgpio_perout_request(fd_pps, settings->pps.pin, 0, 0, 0)) != TCC_E_SUCCESS) {
        pr_err("Unable to stop signal generation on PPS: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
    }
    if ((ret = tgpio_pin_setfunc(fd_tgpio, settings->tgpio.pin, PTP_FUNC_NONE, settings->tgpio.pin)) != TCC_E_SUCCESS) {
        pr_err("tgpio_pin_setfunc failed for TGPIO: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
    }
    if ((ret = close(fd_tgpio)) != 0) {
        pr_err("Unable to deinitialize the TGPIO device: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
    }
    if ((ret = close(fd_pps)) != 0) {
        pr_err("Unable to deinitialize the PPS device: %s\n", strerror(errno));
        ret = -TCC_E_ERROR;
    }
    return ret;
}

void signal_handler(int dummy)
{
    UNUSED(dummy);
    running = 0;
}

int open_fd(struct device_settings* device)
{
    int fd = open(device->device, O_WRONLY);
    if (fd == -1) {
        pr_err("Unable to open %s device: %s\n", device->device, strerror(errno));
        return -1;
    }
    return fd;
}

__attribute__((const)) uint64_t get_start_sys(uint64_t start_sec)
{
    return (start_sec + 2) * BILLION;
}

__attribute__((pure)) int is_next_arg_flag(int argc, char** argv)
{
    char* argument = (optind < argc) ? argv[optind] : NULL;
    return argument != NULL && strlen(argument) > 0 && argument[0] != '-';
}

void parse_device(int argc, char** argv, struct device_settings* device)
{
    device->device = optarg;
    if (is_next_arg_flag(argc, argv)) {
        device->pin = (unsigned int)strtoul(argv[optind], NULL, DECIMAL_NUM_SYS);
        optind++;
    }
}

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* setting)
{
    static const struct option opts[] = {{"help", no_argument, NULL, 'h'},
        {"verbose", no_argument, NULL, 'v'},
        {"tgpio", required_argument, NULL, 't'},
        {"pps", required_argument, NULL, 'p'},
        {"period", required_argument, NULL, 'T'},
        {0, 0, 0, 0}};

    int index = 0;
    int opt = 0;

    setting->period = BILLION;

    while (opt != -1) {
        opt = getopt_long(argc, argv, "hvt:p:T:", opts, &index);
        switch (opt) {
        case 't':
            parse_device(argc, argv, &setting->tgpio);
            break;
        case 'p':
            parse_device(argc, argv, &setting->pps);
            break;
        case 'T':
            setting->period = (uint64_t)strtoul(optarg, NULL, 10);
            break;
        case 'v':
            verbose_output = 1;
            break;
        case 'h':
            return TCC_PRINT_HELP;
        case '?':
            return TCC_PARSE_ERROR;
        default:
            break;
        }
    }

    if (setting->tgpio.device == NULL) {
        printf("--tgpio <device_name> [pin] option is required\n");
        return TCC_PARSE_ERROR;
    }
    if (setting->pps.device == NULL) {
        printf("--pps <device_name> [pin] option is required\n");
        return TCC_PARSE_ERROR;
    }

    return TCC_PARSE_SUCCESS;
}

void usage()
{
    printf(
        "Usage:\n"
        "tcc_ethernet_timestamps_sample --tgpio <device_name> [pin]\n"
        "                               --pps <device_name> [pin]\n"
        "                               --period [value]\n"
        "                               --verbose\n"
        "Options:\n"
        "    -t | --tgpio      Specify the output TGPIO device (required) and pin index (optional). Default pin: 0.\n"
        "    -p | --pps        Specify the output PPS device (required) and pin index (optional). Default pin: 0.\n"
        "    -T | --period     Specify the output period in nanoseconds. Default: 1000000000 (1 second).\n"
        "    -v | --verbose    Enable verbose output mode.\n");
}
