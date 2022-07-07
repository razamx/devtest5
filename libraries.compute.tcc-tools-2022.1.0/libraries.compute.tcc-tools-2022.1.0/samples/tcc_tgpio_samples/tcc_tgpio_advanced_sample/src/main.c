/* This sample is a self-contained example program demonstrating scenarios with software GPIO and Time-Aware GPIO
 * (TGPIO). */

/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sched.h>
#include <search.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "args_parse.h"
#include "macros.h"
#include "swgpio.h"
#include "tcc/err_code.h"
#include "tgpio.h"
#include "tgpio_defs.h"
#include "tgpio_sample.h"
#include "time_convert.h"

#define RT_SCHED_PRIORITY (99)

int swgpio_out(period_pin_config* config, size_t pins_count);
int tgpio_out(period_pin_config* config, size_t pins_count);
int parse_periods_config(const char* file, period_pin_config** config, size_t* pins_count, Mode mode);
int open_and_check_file(const char* file, FILE** fconf, size_t* pins_count);
size_t get_line_count(FILE* file);
size_t get_next_pin(sw_gpio_output* pins, size_t pins_count);
int set_rt(void);
void signal_handler(int);

static settings_t sample_settings = {
    .signal_file = NULL,
    .scenario = SCENARIO_UNKNOWN,
    .mode = MODE_UNKNOWN,
};
int running = 1;


int main(int argc, char** argv)
{
    int tcc_status = parse_args(argc, argv, &sample_settings);
    if (tcc_status != TCC_PARSE_SUCCESS) {
        print_usage();
        return -TCC_E_BAD_PARAM;
    }

    if (sample_settings.scenario != SCENARIO_UNKNOWN) {
        if (signal(SIGINT, signal_handler) == SIG_ERR) {
            pr_err("Can't catch SIGINT\n");
            return -TCC_E_ERROR;
        }
        if (set_rt() != TCC_E_SUCCESS) {
            return -TCC_E_ERROR;
        };
    }

    if (sample_settings.scenario == SCENARIO_OUTPUT) {
        period_pin_config* config;
        size_t pins_count;
        if (parse_periods_config(sample_settings.signal_file, &config, &pins_count, sample_settings.mode) !=
            TCC_E_SUCCESS) {
            return -TCC_E_ERROR;
        }

        if (sample_settings.mode == MODE_TGPIO) {
            tcc_status = tgpio_out(config, pins_count);
        } else {
            tcc_status = swgpio_out(config, pins_count);
        }

        free(config);
    }

    return tcc_status;
}

int swgpio_out(period_pin_config* config, size_t pins_count)
{
    int ret = TCC_E_SUCCESS;

    if (config == NULL) {
        return -TCC_E_BAD_PARAM;
    }

    /* Init GPIO */
    sw_gpio_output* pins = (sw_gpio_output*)calloc(pins_count, sizeof(sw_gpio_output));
    if (pins == NULL) {
        pr_err("Failed to allocate memory for pins data: %s\n", strerror(errno));
        return -TCC_E_MEMORY;
    }

    /* Get beginning time */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;
    ts.tv_nsec = 0;
    uint64_t current_timestamp = timespec2ns(ts);

    for (size_t i = 0; i < pins_count; i++) {
        if ((ret = gpio_export(config[i].pin)) != TCC_E_SUCCESS) {
            goto out;
        }

        if ((ret = gpio_set_direction(config[i].pin, DIRECTION_OUTPUT)) != TCC_E_SUCCESS) {
            goto out;
        }

        pins[i].fd = gpio_open(config[i].pin);
        if (pins[i].fd < 0) {
            ret = -TCC_E_NOT_AVAILABLE;
            goto out;
        }
        pins[i].current_value = 0;
        pins[i].next_edge_time = current_timestamp + config[i].offset + config[i].period / 2;
    }

    printf("Start software GPIO output scenario.\n"
           "To interrupt, use Ctrl+C\n");

    /* Generate signal */
    struct timespec remain;
    while (running) {
        size_t next_pin = get_next_pin(pins, pins_count);
        ts = ns2timespec(pins[next_pin].next_edge_time);
        if (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, &remain) != 0) {
            goto out;
        }

        pins[next_pin].current_value = !pins[next_pin].current_value;
        pins[next_pin].next_edge_time += config[next_pin].period / 2;

        if ((ret = gpio_write(pins[next_pin].fd, pins[next_pin].current_value)) != TCC_E_SUCCESS) {
            goto out;
        }
    }

out:
    /* Deinit */
    for (size_t i = 0; i < pins_count; i++) {
        if (pins[i].fd > 0) {
            close(pins[i].fd);
        }

        gpio_unexport(config[i].pin);
    }

    free(pins);
    return ret;
}

int tgpio_out(period_pin_config* config, size_t pins_count)
{
    int ret = TCC_E_SUCCESS;
    struct timespec one_second;

    one_second.tv_sec = 1;
    one_second.tv_nsec = 0;

    if (config == NULL || pins_count == 0) {
        return -TCC_E_BAD_PARAM;
    }

    /* Init TGPIO */
    tgpio_output* pins = (tgpio_output*)calloc(pins_count, sizeof(tgpio_output));
    if (pins == NULL) {
        pr_err("Failed to allocate memory for pins data: %s\n", strerror(errno));
        return -TCC_E_MEMORY;
    }

    for (size_t i = 0; i < pins_count; i++) {
        /* Open TGPIO device */
        pins[i].fd = open(config[i].device, O_WRONLY);  // get_device_descriptor(pins, pins_count, config[i].device);
        if (pins[i].fd < 0) {
            pr_err("Failed to open %s device: %s\n", config[i].device, strerror(errno));
            ret = -TCC_E_NOT_AVAILABLE;
            goto out;
        }

        /* Get cross-timestamp to calculate ART - system clock relation */
        if ((ret = tgpio_get_sys_offset_precise(pins[i].fd, &pins[i].offset_prev)) != TCC_E_SUCCESS) {
            goto out;
        }
    }


    clock_nanosleep(CLOCK_MONOTONIC, 0, &one_second, NULL);


    /* Measure offsets from the beginning of the current second + 2, i.e., zero nanoseconds */
    uint64_t sys_basetime = (pins[0].offset_prev.sys_realtime.sec + 1 + 2) * BILLION;

    /* Enable TGPIO signal generation */
    for (size_t i = 0; i < pins_count; i++) {
        /* Get cross-timestamp to calculate ART - system clock relation */
        if ((ret = tgpio_get_sys_offset_precise(pins[i].fd, &pins[i].offset_cur)) != TCC_E_SUCCESS) {
            goto out;
        }

        /* Convert start timestamp from system time to ART */
        uint64_t start_sys = sys_basetime + config[i].offset;
        uint64_t start_art = convert_sys_to_art(&pins[i].offset_prev, &pins[i].offset_cur, start_sys);


        /* Set up TGPIO pin to the periodic output mode*/
        if ((ret = tgpio_pin_setfunc(pins[i].fd, config[i].pin, PTP_FUNC_OUT, config[i].channel)) != TCC_E_SUCCESS) {
            goto out;
        }

        /* Enable periodic output (use index as channel)
         * Note: TGPIO PTP API sets the half-period of the signal */
        if ((ret = tgpio_perout_request(pins[i].fd, config[i].channel, config[i].period, start_art, 0)) !=
            TCC_E_SUCCESS) {
            goto out;
        }
    }


    printf("Start TGPIO output scenario.\n"
           "To interrupt, use Ctrl+C\n");

    /* Wait for signal, hardware runs output period generation automatically */
    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &one_second, NULL) != EINTR) {
        ;
    }

out:
    /* Disable TGPIO signal generation
     * Note: Cannot unite loops */
    for (size_t i = 0; i < pins_count; i++) {
        tgpio_perout_request(pins[i].fd, config[i].channel, 0, 0, 0);
    }
    for (size_t i = 0; i < pins_count; i++) {
        tgpio_pin_setfunc(pins[i].fd, config[i].pin, PTP_FUNC_NONE, config[i].channel);
        close(pins[i].fd);
    }
    free(pins);
    return ret;
}

/*
 * Parses config with format for GPIO
 * <pin> <offset, ns> <period, ns>
 * <pin> <offset, ns> <period, ns>
 * ...
 *
 * Parses config with format for TGPIO
 * <pin> <offset, ns> <period, ns> <channel> <device>
 * <pin> <offset, ns> <period, ns> <channel> <device>
 * ...
 */
int parse_periods_config(const char* file, period_pin_config** config, size_t* pins_count, Mode mode)
{
    int ret = TCC_E_SUCCESS;
    FILE* fconf = NULL;

    if (file == NULL || config == NULL || pins_count == NULL) {
        pr_err("One or more parameters are NULL: "
               "file=%p, config=%p, pins_count=%p\n",
            file,
            config,
            pins_count);
        ret = -TCC_E_BAD_PARAM;
        goto out;
    }

    if ((ret = open_and_check_file(file, &fconf, pins_count)) != TCC_E_SUCCESS) {
        goto out;
    }

    *config = (period_pin_config*)malloc(*pins_count * sizeof(period_pin_config));
    if (*config == NULL) {
        ret = -TCC_E_MEMORY;
        goto out;
    }

    period_pin_config* cfg = *config;
    for (size_t i = 0; i < *pins_count; i++) {
        int cnt = 0;
        if (mode == MODE_SW_GPIO) {
            cnt = fscanf(fconf, "%u %lu %lu\n", &(cfg[i].pin), &(cfg[i].offset), &(cfg[i].period));
        } else if (mode == MODE_TGPIO) {
            cnt = fscanf(fconf,
                "%u %lu %lu %u %" STRINGIFY(MAX_DEVICE_NAME_LEN) "s\n",
                &(cfg[i].pin),
                &(cfg[i].offset),
                &(cfg[i].period),
                &(cfg[i].channel),
                cfg[i].device);
        }
        int expected_cnt = mode == MODE_SW_GPIO ? 3 : 5;
        if (cnt != expected_cnt) {
            pr_err("Config parsing error: invalid format at line %lu\n", i + 1);
            free(*config);
            *config = NULL;
            *pins_count = 0;
            ret = -TCC_E_BAD_PARAM;
            goto out;
        }
    }

out:
    if (fconf != NULL) {
        fclose(fconf);
    }
    return ret;
}

int open_and_check_file(const char* file, FILE** fconf, size_t* pins_count)
{
    *fconf = fopen(file, "r");
    if (*fconf == NULL) {
        pr_err("Couldn't open file %s\n", file);
        return -TCC_E_BAD_PARAM;
    }

    *pins_count = get_line_count(*fconf);

    if (*pins_count == 0) {
        pr_err("Config parsing error: file is empty\n");
        fclose(*fconf);
        return -TCC_E_BAD_PARAM;
    }
    return TCC_E_SUCCESS;
}

size_t get_line_count(FILE* file)
{
    char tmp;
    size_t lines = 0;

    tmp = (char)getc(file);
    while (tmp != EOF) {
        if (tmp == '\n') {
            ++lines;
        }
        tmp = (char)getc(file);
    }
    fseek(file, 0L, SEEK_SET);

    return lines;
}

__attribute__((pure)) size_t get_next_pin(sw_gpio_output* pins, size_t pins_count)
{
    size_t index = 0;
    uint64_t min = pins[index].next_edge_time;
    for (size_t i = 1; i < pins_count; i++) {
        if (pins[i].next_edge_time < min) {
            index = i;
            min = pins[i].next_edge_time;
        }
    }

    return index;
}

__attribute__((const)) double nsec2sec(uint64_t time_nsec)
{
    return (double)time_nsec / (double)BILLION;
}

int set_rt()
{
    cpu_set_t cpuset;
    struct sched_param sp = {.sched_priority = RT_SCHED_PRIORITY};

    /* Lock all memory pages to avoid page faults */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    CPU_ZERO(&cpuset);
    CPU_SET(CPUID, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        pr_err("Failed to set affinity, error: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }

    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
        pr_err("Failed to set scheduler, error: %s\n", strerror(errno));
        return -TCC_E_ERROR;
    }
    return TCC_E_SUCCESS;
}

void signal_handler(int dummy)
{
    UNUSED(dummy);
    running = 0;
}

int parse_args(int argc, char** argv, settings_t* settings)
{
    static const struct option opts[] = {{"help", no_argument, NULL, 'h'},
        {"mode", required_argument, NULL, 'm'},
        {"signal_file", required_argument, NULL, 'c'},
        {NULL, 0, NULL, 0}};

    /* Available: output scenario */
    settings->scenario = SCENARIO_OUTPUT;

    int fail = 0;
    int index = 0;
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "hm:c:", opts, &index);
        switch (opt) {
        case 'm':
            if (!strcmp(optarg, "soft")) {
                settings->mode = MODE_SW_GPIO;
            } else if (!strcmp(optarg, "tgpio")) {
                settings->mode = MODE_TGPIO;
            } else {
                printf("Error. --mode can only be: soft or tgpio, and cannot be: %s\n", optarg);
                fail = 1;
            }
            break;
        case 'c':
            settings->signal_file = optarg;
            break;
        case 'h':
            settings->scenario = SCENARIO_UNKNOWN;
            print_usage();
            return TCC_E_SUCCESS;
        case -1:
            break;
        default:
            return -TCC_E_BAD_PARAM;
        }
    }

    if (settings->scenario == SCENARIO_UNKNOWN) {
        printf("--scenario is required\n");
        fail = 1;
    }
    if (settings->mode == MODE_UNKNOWN) {
        printf("--mode is required\n");
        fail = 1;
    }

    if (!fail) {
        if (settings->scenario == SCENARIO_OUTPUT) {
            if (settings->signal_file == NULL) {
                printf("--signal_file is required for output mode.\n");
                fail = 1;
            }
        }
    }

    if (fail) {
        printf("\n");
        print_usage();
        return -TCC_E_BAD_PARAM;
    }

    return TCC_E_SUCCESS;
}

void print_usage()
{
    printf("Usage: tcc_tgpio_advanced_sample --mode <mode> --signal_file <path>\n"
           "Options:\n"
           "    -m, --mode         Specify the mode (soft or tgpio) to run.\n"
           "    -c, --signal_file  Specify the path to the signal config file.\n"
           "Example:\n"
           "Output scenario:\n"
           "tcc_tgpio_advanced_sample --mode tgpio --signal_file signal.txt\n");
}
