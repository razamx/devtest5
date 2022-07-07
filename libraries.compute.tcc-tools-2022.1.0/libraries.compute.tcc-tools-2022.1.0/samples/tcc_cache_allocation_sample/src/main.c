/* This sample is a self-contained example program demonstrating the cache allocation library.
The sample runs a workload (memory read access) and measures latency. */

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

#include <getopt.h>
#include <ittnotify/ittnotify.h>
#include <linux/random.h>
#include <sched.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/syscall.h> /* For SYS_xxx definitions */
#include <sys/sysinfo.h>
#include <unistd.h>

#include "args_parse.h"
#include "macros.h"
#include "pointer_chasing.h"
#include "tcc/cache.h"
#include "tcc/measurement.h"
#include "tcc/measurement_helpers.h"

#define COLLECTOR_LIBRARY_VAR_NAME "INTEL_LIBITTNOTIFY64"
#define TCC_COLLECTOR_NAME "libtcc_collector.so"

#define NOT_AVAILABLE_MESSAGE "N/A"

#define PAGE_SIZE (4096)
#define LINE_SIZE (64)
#define WORKLOAD_BUFFER_SIZE (95UL * 1024)
#define WORKLOAD_NUM_ACCESSES (512)
#define INTERNAL_STRESS_BUFFER_SIZE (32 * 1024 * 1024)
#define INTERNAL_STRESS_ITERATIONS (INTERNAL_STRESS_BUFFER_SIZE / LINE_SIZE / 6 + 1)
#define TCC_NSEC_PER_SEC (1000000000U)

enum OPT
{
    OPT_SLEEP,
    OPT_STRESS,
    OPT_NOSTRESS
};

enum stress_state
{
    INTERNAL,
    EXTERNAL,
    NOSTRESS
};

struct settings_t
{
    long latency;
    long iterations;
    struct timespec sleep;
    unsigned stress_state;
    unsigned collect_flag;
    long cpuid;
};

void print_params(settings_t sample_settings);
void print_statistics();

cache_line_node_t* run_workload_and_measure_cycles(void* start_addr,
    size_t size,
    uint iterations,
    settings_t sample_settings);

static __itt_domain* domain = NULL;
static __itt_string_handle* measurement = NULL;
static uint8_t internal_stress_buffer[INTERNAL_STRESS_BUFFER_SIZE] __attribute__((aligned(PAGE_SIZE)));


int main(int argc, char** argv)
{
    static const char measurement_name[] = "Workload";
    struct tcc_measurement* tcc_measurement_ptr = NULL;

    /* Initialize the ITT task handle to collect performance data for the sample workload */
    domain = __itt_domain_create("TCC");
    measurement = __itt_string_handle_create(measurement_name);
    if (!domain || !measurement) {
        printf("Unable to create ITT handles\n");
        return -TCC_E_NOT_AVAILABLE;
    }

    /* Parse params */
    settings_t sample_setting = {.latency = 0,
        .iterations = 100,
        .sleep = (struct timespec) {.tv_sec = 0, .tv_nsec = 0},
        .stress_state = INTERNAL,
        .collect_flag = 0,
        .cpuid = 3};

    int tcc_status = parse_args(argc, argv, &sample_setting);
    if (tcc_status == TCC_PARSE_ERROR) {
        print_usage();
        return -TCC_E_BAD_PARAM;
    }
    if (tcc_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    }

    /* Set up ittnotify environment variable to allow latency measurement */
    if (sample_setting.collect_flag) {
        setenv_if_not_set(COLLECTOR_LIBRARY_VAR_NAME, TCC_COLLECTOR_NAME);
    }

    print_params(sample_setting);

    /* Check the requested core is available on the system */
    int n_cores = get_nprocs();
    if (sample_setting.cpuid >= n_cores) {
        printf("This application is configured to run on core %ld, but the system has cores 0-%d only.\n"
               "Use a system that has core %ld.\n",
            sample_setting.cpuid,
            n_cores - 1,
            sample_setting.cpuid);
        return -TCC_E_NOT_AVAILABLE;
    }

    /* Lock all memory pages to avoid page faults */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    /* Initialize TCC libraries */
    /* Bind a process to the CPU specified by the cpuid variable (set process affinity) */
    if ((tcc_status = tcc_cache_init(sample_setting.cpuid)) != TCC_E_SUCCESS) {
        if (tcc_status == TCC_BUFFER_NOT_FOUND) {
            printf("TCC Driver not found. Regular memory is used.\n");
        } else {
            PRINT_ERROR_EXIT(tcc_cache_init(sample_setting.cpuid));
        }
    }

    /* Get tcc_measurement structure with collected data from ITT. Measurements will be printed if
     * the measurement library collector is solely
     * used */
    if ((tcc_status = tcc_measurement_get(domain, measurement, &tcc_measurement_ptr)) != TCC_E_SUCCESS) {
        if (tcc_status == -TCC_E_NOT_AVAILABLE && getenv(COLLECTOR_LIBRARY_VAR_NAME) != NULL) {
            printf("External collector is used.\nExecution will continue with no measurement results.\n");
        } else {
            printf("Collector is not set. Execution will continue with no measurement results.\n");
            printf("Use -c or --collect flag to enable collection or set environment variable manually.\n");
        }
    }

    /* Allocates memory according to the latency requirement */
    printf("Allocating memory according to the latency requirements\n");
    void* mem = tcc_cache_malloc(WORKLOAD_BUFFER_SIZE, sample_setting.latency);
    if (mem == NULL) {
        print_error_description(-TCC_E_MEMORY, "tcc_cache_malloc(BUFFER_SIZE, latency)", __LINE__, __FILE__);
        printf("Set TCC_LOG_LEVEL environment variable:\n");
        printf("export TCC_LOG_LEVEL=INFO\n");
        printf("Rerun the sample to obtain more detailed information.\n");
        return -TCC_E_MEMORY;
    }

    /* Run the application workload */
    /* Evaluate memory access latency */
    printf("Running workload. This may take a while, depending on sleep and iteration values\n");
    run_workload_and_measure_cycles(mem, WORKLOAD_BUFFER_SIZE, sample_setting.iterations, sample_setting);

    /* Releasing memory */
    printf("Deallocating memory\n");
    tcc_cache_free(mem);

    /* Print summary. This will work if only the measurement library collector is used */
    print_statistics();

    /* Finish working with TCC libraries */
    tcc_status = tcc_cache_finish();
    if (tcc_status != TCC_E_SUCCESS) {
        print_error_description(tcc_status, "tcc_cache_finish()", __LINE__, __FILE__);
        return -1;
    }

    return 0;
}


TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* sample_settings)
{
    static struct option long_options[] = {{"cpuid", required_argument, NULL, 'p'},
        {"latency", required_argument, NULL, 'l'},
        {"iterations", required_argument, NULL, 'i'},
        {"sleep", required_argument, NULL, OPT_SLEEP},
        {"stress", no_argument, NULL, OPT_STRESS},
        {"nostress", no_argument, NULL, OPT_NOSTRESS},
        {"collect", no_argument, NULL, 'c'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}};

    int flag_internal_stress = 0;
    int flag_external_stress = 0;
    int flag_no_stress = 0;
    int flag_latency = 0;
    int option_index = 0;
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "p:l:i:ch", long_options, &option_index);
        switch (opt) {
        case 'p': {
            if (optarg) {
                if ((sample_settings->cpuid = strtol(optarg, NULL, DECIMAL_NUM_SYS)) < 0) {
                    printf("Invalid cpuid value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case 'l': {
            if (optarg) {
                flag_latency = 1;
                if ((sample_settings->latency = strtol(optarg, NULL, DECIMAL_NUM_SYS)) <= 0) {
                    printf("Invalid latency value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case 'i': {
            if (optarg) {
                if ((sample_settings->iterations = strtol(optarg, NULL, DECIMAL_NUM_SYS)) <= 0) {
                    printf("Invalid iterations value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case OPT_SLEEP: {
            if (optarg) {
                flag_external_stress = 1;
                sample_settings->stress_state = EXTERNAL;
                if (strchr(optarg, '-') != NULL) {
                    printf("Invalid sleep value\n");
                    return TCC_PARSE_ERROR;
                }
                unsigned ns = strtoul(optarg, NULL, DECIMAL_NUM_SYS);
                sample_settings->sleep.tv_sec = (time_t)(ns / TCC_NSEC_PER_SEC);
                sample_settings->sleep.tv_nsec = ns % TCC_NSEC_PER_SEC;
            }
            break;
        }
        case OPT_STRESS: {
            flag_internal_stress = 1;
            sample_settings->stress_state = INTERNAL;
            break;
        }
        case OPT_NOSTRESS: {
            flag_no_stress = 1;
            sample_settings->stress_state = NOSTRESS;
            break;
        }
        case 'c':
            sample_settings->collect_flag = 1;
            break;
        case 'h':
            return TCC_PRINT_HELP;
        case -1:
            break;
        default:
            return TCC_PARSE_ERROR;
        }
    }
    if ((flag_external_stress & flag_no_stress) || (flag_internal_stress & flag_no_stress) ||
        (flag_internal_stress & flag_external_stress)) {
        printf("Do not use --stress,--sleep and --nostress options simultaneously\n");
        return TCC_PARSE_ERROR;
    }

    if (!flag_latency) {
        return TCC_PARSE_ERROR;
    }

    return TCC_PARSE_SUCCESS;
}


void print_params(settings_t sample_settings)
{
    const size_t len = 256;
    char stress_str[len];
    const char* collector_library = getenv(COLLECTOR_LIBRARY_VAR_NAME);

    snprintf(stress_str, len - 1, "internal");

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
    switch (sample_settings.stress_state) {
    case NOSTRESS: {
        snprintf(stress_str, len - 1, "off");
        break;
    }
    case EXTERNAL: {
        snprintf(stress_str,
            len - 1,
            "external (sleep %ld ns)",
            (sample_settings.sleep.tv_sec * TCC_NSEC_PER_SEC) + sample_settings.sleep.tv_nsec);
        break;
    }
    }
#pragma GCC diagnostic pop

    printf("Running with arguments:\n"
           "    cpuid = %ld,\n"
           "    latency = %ld ns,\n"
           "    stress = %s,\n"
           "    iterations = %ld,\n"
           "    collector = %s\n",
        sample_settings.cpuid,
        sample_settings.latency,
        stress_str,
        sample_settings.iterations,
        collector_library ? collector_library : NOT_AVAILABLE_MESSAGE);
}

void print_usage()
{
    printf("Usage: tcc_cache_allocation_sample -l N [--sleep N | --stress | --nostress]\n\
                                   [-p N] [-c] [-i N]\n\
\n\
Options:\n\
    --sleep N | --stress | --nostress\n\
    Select one of these options:\n\
    --sleep N                  Period between iterations in N nanoseconds.\n\
                               Select this option when you want to run\n\
                               your own noisy neighbor.\n\
    --stress                   Run the sample's provided noisy neighbor\n\
                               on core 3 (default).\n\
    --nostress                 Do not run the sample's provided noisy neighbor,\n\
                               and run workload iterations continuously\n\
                               without sleep in between.\n\
\n\
    -p | --cpuid N             Specify the requested core. Default: 3.\n\
    -l | --latency N           Specify the maximum tolerable latency\n\
                               for a single cache line access in N nanoseconds.\n\
    -i | --iterations N        Execute N iterations of the main loop to gather\n\
                               more precise timing statistics. Default: 100.\n\
    -c | --collect             Enable measurement result collection if\n\
                               INTEL_LIBITTNOTIFY64 was not set via environment.\n\
    -h | --help                Show this help message and exit.\n");
}

void print_statistics()
{
    int tcc_sts = 0;
    struct tcc_measurement* tcc_measurement_ptr = NULL;
    tcc_sts = tcc_measurement_get(domain, measurement, &tcc_measurement_ptr);
    if (TCC_E_SUCCESS == tcc_sts) {
        tcc_measurement_get_avg(tcc_measurement_ptr);
        printf("\n*** Statistics for workload ****************************\n");

        printf("    Minimum total latency: %.0f CPU cycles (%ld ns)\n",
            tcc_measurement_ptr->clk_min,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_ptr->clk_min).tv_nsec);
        printf("    Maximum total latency: %.0f CPU cycles (%ld ns)\n",
            tcc_measurement_ptr->clk_max,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_ptr->clk_max).tv_nsec);
        printf("    Average total latency: %.0f CPU cycles (%ld ns)\n",
            tcc_measurement_ptr->clk_result,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_ptr->clk_result).tv_nsec);
        printf("\n");
        printf("    Minimum latency per buffer access: %.0f CPU cycles (%ld ns)\n",
            tcc_measurement_ptr->clk_min / WORKLOAD_NUM_ACCESSES,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_ptr->clk_min).tv_nsec / WORKLOAD_NUM_ACCESSES);
        printf("    Maximum latency per buffer access: %.0f CPU cycles (%ld ns)\n",
            tcc_measurement_ptr->clk_max / WORKLOAD_NUM_ACCESSES,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_ptr->clk_max).tv_nsec / WORKLOAD_NUM_ACCESSES);
        printf("    Average latency per buffer access: %.0f CPU cycles (%ld ns)\n",
            tcc_measurement_ptr->clk_result / WORKLOAD_NUM_ACCESSES,
            tcc_measurement_convert_clock_to_timespec(tcc_measurement_ptr->clk_result).tv_nsec / WORKLOAD_NUM_ACCESSES);

        printf("********************************************************\n\n");
    }
}

/* Measure latency of accessing memory */

cache_line_node_t* run_workload_and_measure_cycles(void* start_addr,
    size_t size,
    uint iterations,
    settings_t sample_settings)
{
    cache_line_node_t* internal_stress_pointer_chasing = NULL;
    if (sample_settings.stress_state == INTERNAL) {  // Init only if internal buffer enabled
        internal_stress_pointer_chasing =
            pointer_chase_create_random(internal_stress_buffer, sizeof(internal_stress_buffer), random);
        if (internal_stress_pointer_chasing == NULL) {
            return NULL;
        }
    }
    cache_line_node_t* workload_pointer_chasing = pointer_chase_create_random(start_addr, size, random);
    if (workload_pointer_chasing == NULL) {
        return NULL;
    }
    while (iterations--) {
        unsigned state = sample_settings.stress_state;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"
        switch (state) {
        case INTERNAL:
            internal_stress_pointer_chasing =
                pointer_chase_run_read_write_workload(internal_stress_pointer_chasing, INTERNAL_STRESS_ITERATIONS);
            break;
        case EXTERNAL:
            nanosleep(&sample_settings.sleep, NULL);
            break;
        case NOSTRESS:
            break;
        }
#pragma GCC diagnostic pop
        __itt_task_begin(domain, __itt_null, __itt_null, measurement);
        workload_pointer_chasing = pointer_chase_run_read_workload(workload_pointer_chasing, WORKLOAD_NUM_ACCESSES);
        __itt_task_end(domain);
    }
    return workload_pointer_chasing;
}
