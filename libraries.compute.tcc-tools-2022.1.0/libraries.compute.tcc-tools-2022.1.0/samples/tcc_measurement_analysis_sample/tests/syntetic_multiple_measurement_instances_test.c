/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#define _GNU_SOURCE
#include "macros.h"
#include <getopt.h>
#include <ittnotify/ittnotify.h>
#include <linux/random.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h> /* For SYS_xxx definitions */
#include <tcc/cache.h>
#include <tcc/measurement.h>
#include <tcc/measurement_helpers.h>
#include <unistd.h>

/* The size is 64KB, in the format of: ([L2 cache instance size in KB]/[granularity]*[bytes in KB]) / 2. */
#define BUFFER_SIZE ((1024 / 8 * 1024) / 2)

int print_latency_histogram(char* measurements);
int parse_params(const int argc, char* const* argv);
void print_params();
void run_workload_and_measure_cycles(void* start_addr,
    size_t size,
    uint iter,
    __itt_domain* domain,
    __itt_string_handle* measurement_1,
    __itt_string_handle* measurement_2,
    __itt_string_handle* measurement_3);

__itt_domain* g_domain;
__itt_string_handle* measurement_cycle;
__itt_string_handle* measurement_sensor;
__itt_string_handle* measurement_model;

/* Print statistics for the collected measurement data if TCC collector is loaded*/
struct tcc_measurement* tcc_measurement_cycle_ptr;
struct tcc_measurement* tcc_measurement_model_ptr;
struct tcc_measurement* tcc_measurement_sensor_ptr;

void print_usage();

char* buffer;
int latency = -1;
int sleep_flag = 1;
int verbose_flag = 0;
long iterations = 1000;
int cpuid = 3;
int stress_cpuid = 2;
int deadline_raw = 0;  // in time units
int deadline = 0;      // in clk
TCC_TIME_UNIT time_unit;

/* Default sleep value is 10 milliseconds */
struct timespec sleep_timeout = (struct timespec) {.tv_sec = 0, .tv_nsec = 10000000};

/* Callback function for the deadline monitoring. Called when iteration latency exceeds the deadline.
 * The measured latency value is printed in CPU clock cycles. */
static void notify_deadline(__attribute__((unused)) struct tcc_measurement* measurement, uint64_t value)
{
    printf("Deadline: %lu CPU cycles (%ld nsec)\n", value, tcc_measurement_convert_clock_to_timespec(value).tv_nsec);
}

int main(int argc, char* argv[])
{
    int tcc_sts = 0;

    if (!parse_params(argc, argv)) {
        print_usage();
        return 0;
    }

    print_params();

    /* Lock all memory pages to avoid page faults */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    /* Initialize TCC libraries */
    /* Bind a process to the CPU specified by the cpuid variable (set process affinity) */
    /* As the target CPU supports Level 2 (L2) cache distributed among pairs of cores,
     * the same part of L2 cache should be used by the process and the API to ensure
     * the correct behavior of pseudo-lock */
    tcc_sts = tcc_cache_init(cpuid);
    if (tcc_sts != TCC_E_SUCCESS) {
        printf("tcc_cache_init() error: %d\n", tcc_sts);
        return 1;
    }

    /* Initialize the measurement structure to collect performance data for the sample workload */
    /* Read the buffer size from the environment if the tcc_hotspot tool launched the application */
    static const char measurement_cycle_name[] = "Cycle";
    static const char measurement_model_name[] = "Model";
    static const char measurement_sensor_name[] = "Sensor";

    g_domain = __itt_domain_create("TCC");
    measurement_cycle = __itt_string_handle_create(measurement_cycle_name);
    measurement_model = __itt_string_handle_create(measurement_model_name);
    measurement_sensor = __itt_string_handle_create(measurement_sensor_name);
    if (!g_domain || !measurement_cycle || !measurement_model || !measurement_sensor) {
        printf("Failed to create ITT handles\n");
        exit(-1);
    }

    /* Get tcc_measurement structure with collected data from ITT. This will work if only TCC collector is used */
    if (tcc_measurement_get(g_domain, measurement_cycle, &tcc_measurement_cycle_ptr) == TCC_E_SUCCESS &&
        tcc_measurement_get(g_domain, measurement_model, &tcc_measurement_model_ptr) == TCC_E_SUCCESS &&
        tcc_measurement_get(g_domain, measurement_sensor, &tcc_measurement_sensor_ptr) == TCC_E_SUCCESS) {
        tcc_sts = 0;
        tcc_sts |= tcc_measurement_set_deadline(tcc_measurement_cycle_ptr, deadline, notify_deadline);
        tcc_sts |= tcc_measurement_set_deadline(tcc_measurement_model_ptr, deadline, notify_deadline);
        tcc_sts |= tcc_measurement_set_deadline(tcc_measurement_sensor_ptr, deadline, notify_deadline);
        if (tcc_sts != TCC_E_SUCCESS) {
            printf("Unable to setup deadline\n");
        }
    } else {
        printf("Unable to get tcc_measurement structures\n");
    }

    printf("Allocates memory according to the latency\n");
    void* mem = tcc_cache_malloc(BUFFER_SIZE, latency);
    if (mem == NULL) {
        print_error_description(-TCC_E_MEMORY, "tcc_cache_malloc(BUFFER_SIZE, latency)", __LINE__, __FILE__);
        return 1;
    }

    /* Run the application workload */
    /* Evaluate memory access latency for the pseudo-locked cache region */
    printf("Run workload. This may take a while, depending on sleep and iteration values\n");
    run_workload_and_measure_cycles(
        mem, BUFFER_SIZE, iterations, g_domain, measurement_cycle, measurement_sensor, measurement_model);

    printf("Deallocates the memory\n");
    tcc_cache_free(mem);

    if (tcc_measurement_cycle_ptr && tcc_measurement_model_ptr && tcc_measurement_sensor_ptr) {
        printf("%s: Maximum measured latency: %.0f CPU cycles (%ld nsec)\n",
            measurement_cycle_name,
            tcc_measurement_cycle_ptr->clk_max,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_cycle_ptr->clk_max).tv_nsec);
        printf("%s: Maximum measured latency: %.0f CPU cycles (%ld nsec)\n",
            measurement_sensor_name,
            tcc_measurement_sensor_ptr->clk_max,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_sensor_ptr->clk_max).tv_nsec);
        printf("%s: Maximum measured latency: %.0f CPU cycles (%ld nsec)\n",
            measurement_model_name,
            tcc_measurement_model_ptr->clk_max,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_model_ptr->clk_max).tv_nsec);
    }

    /* Finish working with TCC libraries */
    /* A non-0 parameter value specifies that the persistent pseudo-locked cache regions should be removed */
    tcc_sts = tcc_cache_finish(0);
    if (tcc_sts != TCC_E_SUCCESS) {
        printf("tcc_finish() error: %d\n", tcc_sts);
        return 1;
    }

    /* Application exits too fast. We use sleep here for the shared memory reader to read measurement results */
    sleep(1);
    return 0;
}

/**
 * run_workload_and_measure_cycles - Measure latency of accessing memory
 */
void run_workload_and_measure_cycles(void* start_addr,
    __attribute__((unused)) size_t size,
    uint iter,
    __itt_domain* domain,
    __itt_string_handle* measurement_1,
    __itt_string_handle* measurement_2,
    __itt_string_handle* measurement_3)
{
    unsigned long i;

    while (iter--) {
        // tcc_measurement_start(measurement_3);
        __itt_task_begin(domain, __itt_null, __itt_null, measurement_3);
        if (sleep_flag) {
            nanosleep(&sleep_timeout, NULL);
        }
        // tcc_measurement_end(measurement_3,2);
        __itt_task_end(domain);

        // tcc_measurement_start(measurement_1);
        __itt_task_begin(domain, __itt_null, __itt_null, measurement_1);
        for (i = 0; i < 128; i += 32) {
            asm volatile("mov (%0,%1,1), %%eax" : : "r"(start_addr), "r"(i) : "%eax", "memory");
        }
        // tcc_measurement_end(measurement_1,1);
        __itt_task_end(domain);

        // tcc_measurement_start(measurement_2);
        __itt_task_begin(domain, __itt_null, __itt_null, measurement_2);
        asm volatile("mov (%0,%1,1), %%eax" : : "r"(start_addr), "r"(i) : "%eax", "memory");
        // tcc_measurement_end(measurement_2,1);
        __itt_task_end(domain);
    }
}

int parse_params(const int argc, char* const* argv)
{
    latency = -1;  // User should pass latency as an input parameter
    while (1) {
        static struct option long_options[] = {{"latency", required_argument, NULL, 4},
            {"sleep", required_argument, &sleep_flag, 1},
            {"deadline", required_argument, NULL, 3},
            {"iterations", required_argument, NULL, 2},
            {"nosleep", no_argument, &sleep_flag, 0},
            {"verbose", no_argument, &verbose_flag, 1},
            {0, 0, 0, 0}};
        int option_index = 0;
        int c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 0:
            if (strcmp(long_options[option_index].name, "sleep") == 0 && optarg) {
                sleep_timeout.tv_sec = (time_t)(atoll(optarg) / 1000000000);
                sleep_timeout.tv_nsec = atoll(optarg) % 1000000000;
            }
            break;
        case 2:
            if (optarg) {
                iterations = atol(optarg);
            }
            break;
        case 3:
            if (optarg) {
                deadline_raw = atoi(optarg);
                char* units_str = strchr(optarg, ',');
                if (units_str) {
                    units_str++;
                }
                time_unit = tcc_measurement_get_time_unit_from_string(units_str);
                if (time_unit == TCC_TU_UNKNOWN) {
                    printf("Unknown time unit\n");
                    return 0;
                }
                deadline = tcc_measurement_convert_time_units_to_clock(deadline_raw, time_unit);
            }
            break;
        case 4:
            if (optarg) {
                latency = atoi(optarg);
                if (0 >= latency) {
                    printf("Input latency %d must be a positive integer\n", latency);
                    print_usage();
                    return 0;
                }
            }
            break;
        }
    }
    if (0 >= latency) {
        printf("Must spectify input latency\n");
        return 0;
    }
    if (-1 == sleep_flag) {
        printf("Must spectify sleep flag\n");
        return 0;
    }

    return 1;
}

void print_params()
{
    const size_t len = 256;
    char sleep_str[len];
    if (sleep_flag == 0) {
        snprintf(sleep_str, len - 1, "no sleep");
    } else if (sleep_timeout.tv_sec != 0) {
        snprintf(sleep_str, len - 1, "%ld sec %ld ns", sleep_timeout.tv_sec, sleep_timeout.tv_nsec);
    } else {
        snprintf(sleep_str, len - 1, "%.3f us", (double)sleep_timeout.tv_nsec / 1000.);
    }


    printf("Running with arguments:\n"
           "    latency = %d,\n"
           "    sleep = %s,\n"
           "    iterations = %ld,\n"
           "    verbose = %s\n"
           "    deadline = %d %s\n",
        latency,
        sleep_str,
        iterations,
        verbose_flag ? "true" : "false",
        deadline_raw,
        tcc_measurement_get_string_from_time_unit(time_unit));
}

void print_usage()
{
    printf("Usage: payload_syntetic_multiple_measurement_instances_target\n"
           "                              --latency N \n"
           "                             [--sleep N | --nosleep]\n"
           "                             [--iterations N]\n"
           "                             [--verbose]\n"
           "                             [--deadline N[,us|,ns]]\n");
}
