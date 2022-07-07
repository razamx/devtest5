/* This sample is a simple self-contained example program that generates a signal as close to the
 * required frequency as possible (with the smallest period and phase errors) by changing Time-Aware
 * GPIO (TGPIO) period repeatedly.
 */

/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

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

#include "args_parse.h"
#include "macros.h"
#include "tcc/err_code.h"
#include "tgpio.h"
#include "tgpio_defs.h"
#include "time_convert.h"


#define TGPIO_DEVICE "/dev/ptp0"
#define UNUSED(x) ((void)(x))
#define TCC_NSEC_PER_SEC (1000000000U)
#define TCC_NSEC_PER_MSEC (1000000U)
#define TCC_NSEC_PER_USEC (1000U)
#define TIME_BUFFER_SIZE (1024)

/* Short option equivalents */
#define OPT_PIN 1
#define OPT_CHANNEL 2
#define OPT_DEVICE 3
#define OPT_PERIOD 4
#define OPT_SLEEP 5
#define OPT_FREQUENCY 6

struct settings_t
{
    unsigned int pin;
    unsigned int channel;
    const char* device;
    uint64_t period;
    uint64_t sleep_time;
    float proportional_gain;
    float integral_gain;
};

int generate_signal(settings_t* setting);
void signal_handler(int dummy);
const char* timestamp_to_str(double timestmap);

int running = 1;


int main(int argc, char** argv)
{
    /* Catch signal to enable correct shutdown by Ctrl+C interrupt */
    signal(SIGINT, signal_handler);

    settings_t sample_setting = {.pin = 0,
        .channel = 0,
        .device = TGPIO_DEVICE,
        .proportional_gain = 0,
        .integral_gain = 0,
        .period = 76896,         // ~13004.6 Hz
        .sleep_time = 1000000};  // 1 kHz

    TCC_ARGS_PARSE_STATUS tcc_parse_status = parse_args(argc, argv, &sample_setting);
    if (tcc_parse_status == TCC_PARSE_ERROR) {
        print_usage();
        return -TCC_E_BAD_PARAM;
    }
    if (tcc_parse_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    }
    return generate_signal(&sample_setting);
}

int generate_signal(settings_t* setting)
{
    int ret = TCC_E_SUCCESS;
    struct ptp_event_count_tstamp event_count = {0};

#ifdef KERNEL_HEADER_WORKAROUND
    unsigned int workaround_freq_adj_flag = GET_PTP_PEROUT_FREQ_ADJ();
#endif  //KERNEL_HEADER_WORKAROUND

    /* Open TGPIO device */
    int fd = open(setting->device, O_RDWR);
    if (fd < 0) {
        pr_err("Unable to open %s device: %s\n", setting->device, strerror(errno));
        ret = -TCC_E_NOT_AVAILABLE;
        goto out_ret;
    }

    /* Set up pin's channel and output mode */
    if ((ret = tgpio_pin_setfunc(fd, setting->pin, PTP_FUNC_OUT, setting->channel)) != TCC_E_SUCCESS) {
        goto out;
    }

    /* Set up channel for event count*/
    event_count.index = setting->channel;

    /* Get current time*/
    struct ptp_sys_offset_precise cur_offset = {0};
    if ((ret = tgpio_get_sys_offset_precise(fd, &cur_offset)) != TCC_E_SUCCESS) {
        goto out;
    }

    uint64_t t_start = (cur_offset.device.sec + 2) * BILLION;  // Start timestamp with some offset
    uint64_t t_prev = t_start;                                 // Timestamp from previous loop
    uint64_t cnt_prev = 0;                                     // Number of edges from previous loop
    uint64_t period_expected = setting->period;                // Desired period
    double period_control = setting->period;                   // Period to passed to the TGPIO

    printf("Start generating signal.\n"
           "To interrupt, use Ctrl+C\n");

    /* Start */
    if ((ret = tgpio_perout_request(fd, setting->channel, period_control, t_start, 0)) != TCC_E_SUCCESS) {
        goto out;
    }

    while (running) {
        struct timespec sleep_time = ns2timespec(setting->sleep_time);
        nanosleep(&sleep_time, NULL);

        /* Get the number of output edges and timestamp of the latest edge */
        if ((ret = tgpio_get_event_count_tstamp(fd, &event_count)) != TCC_E_SUCCESS) {
            goto out;
        }

        uint64_t t_real = ptptime2ns(event_count.device_time);
        uint64_t cnt_real = event_count.event_count;

        /* Skip control loop before generation starts */
        if (cnt_real <= cnt_prev || t_real <= t_prev) {
            continue;
        }

        /* Calculate the period of the real signal and get period error */
        uint64_t period_real = 2 * (t_real - t_prev) / (cnt_real - cnt_prev);

        /* Calculate the timestamp of the corresponding edge of the expected signal and time error */
        uint64_t t_expected = t_start + (cnt_real - 1) * (period_expected / 2);

        /* Calculate time (phase) and period errors */
        int64_t t_err = t_expected - t_real;
        int64_t period_err = period_expected - period_real;

        /* Calculate the new period by PI controller */
        double adjustment = (double)(setting->proportional_gain * period_err + setting->integral_gain * t_err);
        period_control += adjustment;
        printf("Period adjustment: %s\n", timestamp_to_str(adjustment));
        printf("Period           : %s\n", timestamp_to_str(period_control));

        /* Update period
           PTP_PEROUT_FREQ_ADJ flag is used to update period on-the-fly */

#ifndef KERNEL_HEADER_WORKAROUND
        if ((ret = tgpio_perout_request(fd, setting->channel, period_control, 0, PTP_PEROUT_FREQ_ADJ)) !=
            TCC_E_SUCCESS) {
            goto out;
        }
#else
        if ((ret = tgpio_perout_request(fd, setting->channel, period_control, 0, workaround_freq_adj_flag)) !=
            TCC_E_SUCCESS) {
            goto out;
        }
#endif  //KERNEL_HEADER_WORKAROUND

        cnt_prev = cnt_real;
        t_prev = t_real;
    }

out:
    tgpio_perout_request(fd, setting->channel, 0, 0, 0);
    tgpio_pin_setfunc(fd, setting->pin, PTP_FUNC_NONE, setting->channel);
    close(fd);
out_ret:
    return ret;
}

void signal_handler(int dummy)
{
    UNUSED(dummy);
    running = 0;
}

const char* timestamp_to_str(double timestmap)
{
    static char buffer[TIME_BUFFER_SIZE];
    if (timestmap > TCC_NSEC_PER_SEC) {
        snprintf(buffer, sizeof(buffer), "%lf s", timestmap / (double)TCC_NSEC_PER_SEC);
    } else if (timestmap > TCC_NSEC_PER_MSEC) {
        snprintf(buffer, sizeof(buffer), "%lf ms", timestmap / (double)TCC_NSEC_PER_MSEC);
    } else if (timestmap > TCC_NSEC_PER_USEC) {
        snprintf(buffer, sizeof(buffer), "%lf us", timestmap / (double)TCC_NSEC_PER_USEC);
    } else {
        snprintf(buffer, sizeof(buffer), "%lf ns", timestmap);
    }
    return buffer;
}


TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* setting)
{
    static const struct option opts[] = {{"help", no_argument, NULL, 'h'},
        {"pin", required_argument, NULL, OPT_PIN},
        {"channel", required_argument, NULL, OPT_CHANNEL},
        {"device", required_argument, NULL, OPT_DEVICE},
        {"period", required_argument, NULL, 'T'},
        {"sleep", required_argument, NULL, 's'},
        {"frequency", required_argument, NULL, 'f'},
        {"proportional", required_argument, NULL, 'p'},
        {"integral", required_argument, NULL, 'i'},
        {0, 0, 0, 0}};
    int index = 0;
    int opt = 0;
    int frequency_flag = 0;
    int period_flag = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "hp:i:T:f:s:", opts, &index);
        switch (opt) {
        case OPT_PIN:
            setting->pin = (unsigned int)strtoul(optarg, NULL, DECIMAL_NUM_SYS);
            break;
        case OPT_CHANNEL:
            setting->channel = (unsigned int)strtoul(optarg, NULL, DECIMAL_NUM_SYS);
            break;
        case OPT_DEVICE:
            setting->device = optarg;
            break;
        case 'T':
            setting->period = strtoull(optarg, NULL, DECIMAL_NUM_SYS);
            period_flag = 1;
            break;
        case 's':
            setting->sleep_time = strtoull(optarg, NULL, DECIMAL_NUM_SYS);
            break;
        case 'f':
            setting->period = (unsigned int)BILLION / strtoul(optarg, NULL, DECIMAL_NUM_SYS);
            frequency_flag = 1;
            break;
        case 'p':
            setting->proportional_gain = strtof(optarg, NULL);
            break;
        case 'i':
            setting->integral_gain = strtof(optarg, NULL);
            break;
        case 'h':
            return TCC_PRINT_HELP;
        case -1:
            break;
        default:
            return TCC_PARSE_ERROR;
        }
    }
    if (period_flag & frequency_flag) {
        printf("Do not use --period and --frequency options simultaneously\n");
        return TCC_PARSE_ERROR;
    }
    return TCC_PARSE_SUCCESS;
}

void print_usage()
{
    printf(
        "Usage: tcc_tgpio_frequency_discipline_sample [--pin N] [--channel N] [--device <device_name>] [--period N | "
        "--frequency N] [--sleep N] [--proportional N] [--integral N]\n"
        "Options:\n"
        "    --pin               Specify the output pin index. Default: 0.\n"
        "    --channel           Specify the channel for the output pin. Default: 0.\n"
        "    --device            Specify the TGPIO device. Default: \"/dev/ptp0\".\n"
        "    -T, --period        Specify the output period in nanoseconds. Default: 76896.\n"
        "    -f, --frequency     Specify the output frequency in hertz. Default: 13004.\n"
        "    -s, --sleep         Specify the interval in nanoseconds between changing the output period. Default:\n"
        "                        1000000.\n"
        "    -p, --proportional  Specify the proportional gain of the PI controller. Default: 0.\n"
        "    -i, --integral      Specify the integral gain of the PI controller. Default: 0.\n");
}
