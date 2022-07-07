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
#include "params.h"
#include "workload.h"
#include "tcc_log.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "benchmark_common.h"

#define START_OF_XX_CENTURY 1900

#define RANDOM_SEED 42

// TODO remove params name duplication

static struct control_data default_control = {0};
struct control_data control = {0};

static void init_default()
{
    default_control.cpuid = 3;
    default_control.region_index = 0;
    default_control.buffer_size = 128 * 1024;   // NOLINT(readability-magic-numbers)
    default_control.sleep = 1000;               // NOLINT(readability-magic-numbers)
    default_control.random_seed = RANDOM_SEED;  // NOLINT(readability-magic-numbers)
    default_control.iterations = 1000;          // NOLINT(readability-magic-numbers)
    default_control.accesses = 100;             // NOLINT(readability-magic-numbers)
    default_control.interrupts = 1;
    default_control.stress_buffer_size = DEFAULT_STRESS_BUFFER_SIZE;
    default_control.collect_counters = 1;
    default_control.stress_buffer_accesses = DEFAULT_STRESS_BUFFER_SIZE / LINE_SIZE;
    default_control.workload = workload_get_by_id(0);
}

static void print_usage()
{
    init_default();
#define PRINT_NUMB_PARAM_USAGE(name, description) \
    bench_printf("    [--%s %%value%%] :: default %lu\n", #name, default_control.name)
#define PRINT_BOOL_PARAM_USAGE(name, description) \
    bench_printf(                                 \
        "    [--%s %%value%%], %s :: default %s\n", #name, description, default_control.name ? "ENABLED" : "DISABLED")
    bench_printf("Usage: cache_lock_benchmark_v2\n");
    PRINT_NUMB_PARAM_USAGE(cpuid, "");
    PRINT_NUMB_PARAM_USAGE(region_index, "");
    PRINT_NUMB_PARAM_USAGE(buffer_size, "");
    PRINT_NUMB_PARAM_USAGE(sleep, ", in microseconds");
    PRINT_NUMB_PARAM_USAGE(random_seed, ", 0 for random seed mean that random disabled");
    PRINT_NUMB_PARAM_USAGE(iterations, "");
    PRINT_NUMB_PARAM_USAGE(accesses, ",how many memory reads done by iteration");
    PRINT_BOOL_PARAM_USAGE(interrupts, "");
    PRINT_BOOL_PARAM_USAGE(collect_counters, "");
    PRINT_NUMB_PARAM_USAGE(stress_buffer_size, "");
    PRINT_NUMB_PARAM_USAGE(stress_buffer_accesses, ", number af memory reads for workload buffer for each iteration");
    bench_printf("    [--workload %%name%%] :: default %s\n", default_control.workload->name);
    bench_printf("        Possible workloads:\n");
    size_t i = 0;
    while (workload_get_by_id(i) != NULL) {
        printf("              %s\n", workload_get_by_id(i++)->name);
    }
    printf("To set output file use TCC_MEASUREMENTS_DUMP_FILE environment variable\n");
#undef PRINT_NUMB_PARAM_USAGE
#undef PRINT_BOOL_PARAM_USAGE
}

static void print_params()
{
#define PRINT_NUMB_PARAM(name) bench_printf("%-21s %lu\n", #name "=", control.name)
#define PRINT_BOOL_PARAM(name) bench_printf("%-21s %s\n", #name "=", control.name ? "ENABLED" : "DISABLED")

    PRINT_NUMB_PARAM(cpuid);
    PRINT_NUMB_PARAM(region_index);
    PRINT_NUMB_PARAM(buffer_size);
    PRINT_NUMB_PARAM(sleep);
    PRINT_NUMB_PARAM(random_seed);
    PRINT_NUMB_PARAM(iterations);
    PRINT_NUMB_PARAM(accesses);
    PRINT_BOOL_PARAM(interrupts);
    PRINT_BOOL_PARAM(collect_counters);
    PRINT_NUMB_PARAM(stress_buffer_size);
    PRINT_NUMB_PARAM(stress_buffer_accesses);
    bench_printf("workload =           %s\n", control.workload->name);
#undef PRINT_NUMB_PARAM
#undef PRINT_BOOL_PARAM
}

int parse_params(int argc, char** argv)
{
    init_default();
    TCC_TRACE_FUNC();
    control = default_control;
    TCC_LOG_INFO("%s", argv[0]);
    for (int i = 1; i < argc; i++) {
        TCC_LOG_INFO("       %s", argv[i]);
    }

    enum flags
    {
        flag_cpuid = 1,
        flag_region_index,
        flag_buffer_size,
        flag_sleep,
        flag_random_seed,
        flag_iterations,
        flag_accesses,
        flag_interrupts,
        flag_collect_counters,
        flag_stress_buffer_size,
        flag_stress_buffer_accesses,
        flag_workload
    };
    static struct option long_options[] = {
#define BENCH_OPTARG(name) {#name, required_argument, NULL, flag_##name}
        BENCH_OPTARG(cpuid),
        BENCH_OPTARG(region_index),
        BENCH_OPTARG(buffer_size),
        BENCH_OPTARG(sleep),
        BENCH_OPTARG(random_seed),
        BENCH_OPTARG(iterations),
        BENCH_OPTARG(accesses),
        BENCH_OPTARG(interrupts),
        BENCH_OPTARG(collect_counters),
        BENCH_OPTARG(stress_buffer_size),
        BENCH_OPTARG(stress_buffer_accesses),
        BENCH_OPTARG(workload),
        {0, 0, NULL, 0}
#undef BENCH_OPTARG
    };

#define PARSE_NUMB_PARAM(name)                                                            \
    case flag_##name: {                                                                   \
        TCC_LOG_WARNING("Parse param '%s'", #name);                                       \
        if (optarg) {                                                                     \
            control.name = strtoul(optarg, NULL, 0);                                      \
            if (errno == ERANGE || errno == EINVAL) {                                     \
                TCC_LOG_ERROR("Can't parse string '%s' for " #name " parameter", optarg); \
                goto error;                                                               \
            }                                                                             \
            TCC_LOG_DEBUG("Parse control." #name "=%lu from '%s'", control.name, optarg); \
            break;                                                                        \
        }                                                                                 \
        TCC_LOG_ERROR("can't parse parameter for " #name);                                \
        goto error;                                                                       \
    }

#define PARSE_BOOL_FOR_STR(name, string, value) \
    ({                                          \
        if (strcmp(optarg, string) == 0) {      \
            control.name = value;               \
            break;                              \
        }                                       \
    })

#define PARSE_BOOL_PARAM(name)                                                            \
    case flag_##name: {                                                                   \
        TCC_LOG_WARNING("Parse param '%s'", #name);                                       \
        if (optarg) {                                                                     \
            PARSE_BOOL_FOR_STR(name, "ENABLE", 1);                                        \
            PARSE_BOOL_FOR_STR(name, "enable", 1);                                        \
            PARSE_BOOL_FOR_STR(name, "ON", 1);                                            \
            PARSE_BOOL_FOR_STR(name, "on", 1);                                            \
            PARSE_BOOL_FOR_STR(name, "DISABLE", 0);                                       \
            PARSE_BOOL_FOR_STR(name, "disable", 0);                                       \
            PARSE_BOOL_FOR_STR(name, "OFF", 0);                                           \
            PARSE_BOOL_FOR_STR(name, "off", 0);                                           \
            control.name = strtoul(optarg, NULL, 0);                                      \
            if (errno == ERANGE || errno == EINVAL) {                                     \
                TCC_LOG_ERROR("Can't parse string '%s' for " #name " parameter", optarg); \
                goto error;                                                               \
            }                                                                             \
            TCC_LOG_DEBUG("Parse control." #name "=%lu from '%s'", control.name, optarg); \
            break;                                                                        \
        }                                                                                 \
        TCC_LOG_ERROR("can't parse parameter for " #name);                                \
        goto error;                                                                       \
    }


    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            PARSE_NUMB_PARAM(cpuid)
            PARSE_NUMB_PARAM(region_index)
            PARSE_NUMB_PARAM(buffer_size)
            PARSE_NUMB_PARAM(sleep)
            PARSE_NUMB_PARAM(random_seed)
            PARSE_NUMB_PARAM(iterations)
            PARSE_NUMB_PARAM(accesses)
            PARSE_BOOL_PARAM(interrupts)
            PARSE_BOOL_PARAM(collect_counters)
            PARSE_NUMB_PARAM(stress_buffer_size)
            PARSE_NUMB_PARAM(stress_buffer_accesses)
        case flag_workload: {
            TCC_LOG_WARNING("Parse param 'workload'");
            if (optarg) {
                const workload_t* workload = workload_get_by_name(optarg);
                if (workload == NULL) {
                    TCC_LOG_WARNING("Can't find workload");
                    goto error;
                }
                control.workload = workload;
                break;
            }
            TCC_LOG_ERROR("can't parse workload name");
            goto error;
        }
        default:
            bench_printf("?? getopt returned character code 0%o ??\n", c);
            goto error;
        }
    }
#undef PARSE_NUMB_PARAM
#undef PARSE_BOOL_PARAM
#undef PARSE_BOOL_FOR_STR
    if (optind < argc) {
        bench_printf("non-option ARGV-elements: \n");
        while (optind < argc) {
            printf("%s ", argv[optind++]);
        }
        bench_printf("\n");
        goto error;
    }
    if (control.buffer_size / LINE_SIZE == 0) {
        bench_printf("buffer size %lu bytes is too small. Buffer should be more than 64 bytes\n", control.buffer_size);
        goto error;
    }
    print_params();
    return 0;
error:
    print_usage();
    return -1;
}

char* get_dump_file_name()
{
    int status = 0;
    char* buffer = NULL;
    size_t size = 0;
    FILE* file = open_memstream(&buffer, &size);
    if (file == NULL) {
        TCC_LOG_ERROR("open_memstream:%s", strerror(errno));
        goto error;
    }
    {
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        if (fprintf(file,
                "%04u%02u%02u%02u%02u%02u",
                timeinfo->tm_year + START_OF_XX_CENTURY,
                timeinfo->tm_mon,
                timeinfo->tm_mday,
                timeinfo->tm_hour,
                timeinfo->tm_min,
                timeinfo->tm_sec) < 0) {
            TCC_LOG_ERROR("fprintf:%s", strerror(errno));
            goto error;
        }
    }
    status |= 0 > fprintf(file, "wrk_%s", control.workload->name);
    status |= 0 > fprintf(file, "-cpu%lu", control.cpuid);
    status |= 0 > fprintf(file, "rgn%lu", control.region_index);
    status |= 0 > fprintf(file, "buf%lu", control.buffer_size);
    status |= 0 > fprintf(file, "slp%lu", control.sleep);
    status |= 0 > fprintf(file, "acs%lu", control.accesses);
    status |= 0 > fprintf(file, "itr%lu", control.iterations);
    status |= 0 > fprintf(file, "rnd%lu", control.random_seed);
    status |= 0 > fprintf(file, "int%lu", control.interrupts);
    status |= 0 > fprintf(file, "strez%lu", control.stress_buffer_size);
    status |= 0 > fprintf(file, "strea%lu", control.stress_buffer_accesses);
    status |= 0 > fprintf(file, ".txt");
    if (status != 0) {
        TCC_LOG_ERROR("Can't create file name");
        goto error;
    }
    if (fflush(file) == EOF) {
        TCC_LOG_ERROR("fprintf:%s", strerror(errno));
        goto error;
    }
    fclose(file);
    return buffer;
error:
    if (file) {
        fclose(file);
    }
    free(buffer);
    return NULL;
}
