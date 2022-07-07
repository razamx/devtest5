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

/*
This is a benchmark to measure overhead of ITT notify API (ITT) with respect to
measurement library (ML).

See wiki for more details:

Measurement procedure:
    1. ML empty measurement
       ITT empty measurement
    2. Measure empty ML with ML
       Measure empty ITT with ML
    3. Validate:
           Measure workload with ML
           Measure workload with ITT
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <tcc/measurement.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/io.h>
#include <sched.h>
#include <errno.h>
#include "tcc/measurement_helpers.h"
#include "measurement.h"
#include "macros.h"

#define pr_err(s, ...) fprintf(stderr, s, ##__VA_ARGS__)
#define CPUID 3

void set_rt();
void empty_mapi();
void empty_itt();
void meta_mapi();
void meta_itt();
void workload_mapi();
void workload_itt();
void dump(struct tcc_measurement* measurement, const char* filename);
void set_measurements_buffers_env(const char* workload, int count);
void print_statistics(struct tcc_measurement* measurement);
void parse_args(int argc, char** argv);
void usage();


typedef enum
{
    MODE_UNKNOWN,
    MODE_EMPTY_MAPI,
    MODE_EMPTY_ITT,
    MODE_META_MAPI,
    MODE_META_ITT,
    MODE_WORKLOAD_MAPI,
    MODE_WORKLOAD_ITT
} MODE;

int iterations_count = -1;
int workload_iterations_count = -1;
int cumulative_count = -1;
MODE mode = MODE_UNKNOWN;
TCC_TIME_UNIT time_unit = TCC_TU_UNKNOWN;
const char* filename_prefix = "dump";


int main(int argc, char** argv)
{
    set_rt();

    parse_args(argc, argv);
    setenv("INTEL_LIBITTNOTIFY64", "libtcc_collector.so", 1);

    switch (mode) {
    case MODE_EMPTY_MAPI:
        empty_mapi();
        break;
    case MODE_EMPTY_ITT:
        empty_itt();
        break;
    case MODE_META_MAPI:
        meta_mapi();
        break;
    case MODE_META_ITT:
        meta_itt();
        break;
    case MODE_WORKLOAD_MAPI:
        workload_mapi();
        break;
    case MODE_WORKLOAD_ITT:
        workload_itt();
        break;
    default:
        return 1;
    }

    return 0;
}

void set_rt()
{
    cpu_set_t cpuset;
    struct sched_param sp;

    CPU_ZERO(&cpuset);
    CPU_SET(CPUID, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        pr_err("Failed to set affinity, error: %s\n", strerror(errno));
        exit(1);
    }

    memset(&sp, 0, sizeof(sp));
    sp.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0) {
        pr_err("Failed to set scheduler, error: %s\n", strerror(errno));
        exit(1);
    }

    if (iopl(3) < 0) {
        pr_err("Failed to set IO privilege level, error: %s\n", strerror(errno));
        exit(1);
    }
}

// Empty MAPI measurement
void empty_mapi()
{
    TCC_ERROR tcc_err;
    struct tcc_measurement measurement;
    if ((tcc_err = tcc_measurement_init(&measurement, iterations_count, "MAPI")) != TCC_E_SUCCESS) {
        pr_err("Failed to init TCC measurement, error: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    for (int iter = 0; iter < iterations_count; iter++) {
        asm volatile("cli");
        tcc_measurement_start(&measurement);
        tcc_measurement_end(&measurement, 1);
        asm volatile("sti");
    }

    print_statistics(&measurement);
    dump(&measurement, "empty_mapi");

    tcc_measurement_destroy(&measurement);
}

// Empty ITT measurement
void empty_itt()
{
    TCC_ERROR tcc_err;
    struct tcc_measurement* measurement;

    set_measurements_buffers_env("ITT", iterations_count);

    __itt_domain* domain = __itt_domain_create("TCC");
    __itt_string_handle* handle = __itt_string_handle_create("ITT");
    if (domain == NULL || handle == NULL) {
        pr_err("Failed to init ITT\n");
        exit(1);
    }

    for (int iter = 0; iter < iterations_count; iter++) {
        asm volatile("cli");
        __itt_task_begin(domain, __itt_null, __itt_null, handle);
        __itt_task_end(domain);
        asm volatile("sti");
    }

    if ((tcc_err = tcc_measurement_get(domain, handle, &measurement)) != TCC_E_SUCCESS) {
        pr_err("Failed to get measurement structure: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    print_statistics(measurement);
    dump(measurement, "empty_itt");
}

// Measure ML with ML
void meta_mapi()
{
    TCC_ERROR tcc_err;
    struct tcc_measurement meta_measurement;
    struct tcc_measurement measurement;

    if ((tcc_err = tcc_measurement_init(&meta_measurement, iterations_count, "META-MAPI")) != TCC_E_SUCCESS) {
        pr_err("Failed to init meta_measurement, error: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    if ((tcc_err = tcc_measurement_init(&measurement, 0, "MAPI")) != TCC_E_SUCCESS) {
        pr_err("Failed to init measurement, error: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    for (int iter = 0; iter < iterations_count; iter++) {
        asm volatile("cli");
        tcc_measurement_start(&meta_measurement);
        for (int cum_iter = 0; cum_iter < cumulative_count; cum_iter++) {
            tcc_measurement_start(&measurement);
            tcc_measurement_end(&measurement, 1);
        }
        tcc_measurement_end(&meta_measurement, cumulative_count);
        asm volatile("sti");
    }

    print_statistics(&meta_measurement);
    dump(&meta_measurement, "meta_mapi");

    tcc_measurement_destroy(&measurement);
    tcc_measurement_destroy(&meta_measurement);
}

// Measure ITT with ML
void meta_itt()
{
    TCC_ERROR tcc_err;
    struct tcc_measurement meta_measurement;

    if ((tcc_err = tcc_measurement_init(&meta_measurement, iterations_count, "META-MAPI")) != TCC_E_SUCCESS) {
        pr_err("Failed to init meta_measurement, error: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    set_measurements_buffers_env("ITT", iterations_count);
    __itt_domain* domain = __itt_domain_create("TCC");
    __itt_string_handle* handle = __itt_string_handle_create("ITT");
    if (domain == NULL || handle == NULL) {
        pr_err("Failed to init ITT\n");
        exit(1);
    }

    for (int iter = 0; iter < iterations_count; iter++) {
        asm volatile("cli");
        tcc_measurement_start(&meta_measurement);
        for (int cum_iter = 0; cum_iter < cumulative_count; cum_iter++) {
            __itt_task_begin(domain, __itt_null, __itt_null, handle);
            __itt_task_end(domain);
        }
        tcc_measurement_end(&meta_measurement, cumulative_count);
        asm volatile("sti");
    }

    print_statistics(&meta_measurement);
    dump(&meta_measurement, "meta_itt");
    tcc_measurement_destroy(&meta_measurement);
}

void workload_mapi()
{
    TCC_ERROR tcc_err;
    struct tcc_measurement measurement;
    uint64_t w_cnt = workload_iterations_count;

    if ((tcc_err = tcc_measurement_init(&measurement, iterations_count, "MAPI")) != TCC_E_SUCCESS) {
        pr_err("Failed to init measurement, error: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    for (int iter = 0; iter < iterations_count; iter++) {
        asm volatile("cli");
        tcc_measurement_start(&measurement);
        asm volatile("   movq %0, %%r15            \n"
                     "__workload_loop1:            \n"
                     "   dec %%r15                 \n"
                     "   jne __workload_loop1      \n"
                     :
                     : "g"(w_cnt)
                     : "%r15");
        tcc_measurement_end(&measurement, 1);
        asm volatile("sti");
    }

    print_statistics(&measurement);
    dump(&measurement, "workload_mapi");

    tcc_measurement_destroy(&measurement);
}

void workload_itt()
{
    TCC_ERROR tcc_err;
    struct tcc_measurement* measurement;
    uint64_t w_cnt = workload_iterations_count;

    set_measurements_buffers_env("ITT", iterations_count);

    __itt_domain* domain = __itt_domain_create("TCC");
    __itt_string_handle* handle = __itt_string_handle_create("ITT");
    if (domain == NULL || handle == NULL) {
        pr_err("Failed to init ITT\n");
        exit(1);
    }

    for (int iter = 0; iter < iterations_count; iter++) {
        asm volatile("cli");
        __itt_task_begin(domain, __itt_null, __itt_null, handle);
        asm volatile("   movq %0, %%r15            \n"
                     "__workload_loop2:            \n"
                     "   dec %%r15                 \n"
                     "   jne __workload_loop2      \n"
                     :
                     : "g"(w_cnt)
                     : "%r15");
        __itt_task_end(domain);
        asm volatile("sti");
    }

    if ((tcc_err = tcc_measurement_get(domain, handle, &measurement)) != TCC_E_SUCCESS) {
        pr_err("Failed to get measurement structure: %s\n", get_error_description(tcc_err));
        exit(1);
    }

    print_statistics(measurement);
    dump(measurement, "workload_itt");
}

void dump(struct tcc_measurement* measurement, const char* filename)
{
    char* name_with_prefix = (char*)malloc(strlen(filename) + strlen(filename_prefix) + 2);
    sprintf(name_with_prefix, "%s_%s", filename_prefix, filename);

    FILE* dump_file = fopen(name_with_prefix, "w");
    TCC_ERROR tcc_err;
    if (dump_file == NULL) {
        pr_err("Failed to open file %s\n", name_with_prefix);
        goto cleanup;
    }
    if ((tcc_err = tcc_measurement_print_history(measurement, dump_file, time_unit)) != TCC_E_SUCCESS) {
        pr_err("Failed to dump file: %s\n", get_error_description(tcc_err));
        goto cleanup;
    }

cleanup:
    free(name_with_prefix);
    if (dump_file != NULL) {
        fclose(dump_file);
    }
}

void set_measurements_buffers_env(const char* workload, int count)
{
    char str[255];
    sprintf(str, "%s:%d", workload, count);
    setenv("TCC_MEASUREMENTS_BUFFERS", str, 1);
}

void print_statistics(struct tcc_measurement* measurement)
{
    measurement->clk_avg /= measurement->clk_avgc;
    const char* time_unit_str = tcc_measurement_get_string_from_time_unit(time_unit);
    printf(
        "Min: %ld %s\n", tcc_measurement_convert_clock_to_time_units(measurement->clk_min, time_unit), time_unit_str);
    printf(
        "Avg: %ld %s\n", tcc_measurement_convert_clock_to_time_units(measurement->clk_avg, time_unit), time_unit_str);
    printf(
        "Max: %ld %s\n", tcc_measurement_convert_clock_to_time_units(measurement->clk_max, time_unit), time_unit_str);
}


void parse_args(int argc, char** argv)
{
    static struct option long_options[] = {{"help", no_argument, NULL, 'h'},
        {"prefix", required_argument, NULL, 'p'},
        {"mode", required_argument, NULL, 'm'},
        {"iterations", required_argument, NULL, 'i'},
        {"workload-iterations", required_argument, NULL, 'w'},
        {"cumulative-count", required_argument, NULL, 'c'},
        {"time-unit", required_argument, NULL, 'u'},
        {0, 0, 0, 0}};

    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "h:p:m:i:w:c:u:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 0:
            break;
        case 'm':
            if (strcmp(optarg, "empty-ml") == 0) {
                mode = MODE_EMPTY_MAPI;
            } else if (strcmp(optarg, "empty-itt") == 0) {
                mode = MODE_EMPTY_ITT;
            } else if (strcmp(optarg, "meta-ml") == 0) {
                mode = MODE_META_MAPI;
            } else if (strcmp(optarg, "meta-itt") == 0) {
                mode = MODE_META_ITT;
            } else if (strcmp(optarg, "workload-ml") == 0) {
                mode = MODE_WORKLOAD_MAPI;
            } else if (strcmp(optarg, "workload-itt") == 0) {
                mode = MODE_WORKLOAD_ITT;
            } else {
                pr_err("Invalid mode %s\n", optarg);
                exit(1);
            }
            break;
        case 'i':
            iterations_count = atoi(optarg);
            break;
        case 'w':
            workload_iterations_count = atoi(optarg);
            break;
        case 'c':
            cumulative_count = atoi(optarg);
            break;
        case 'u':
            time_unit = tcc_measurement_get_time_unit_from_string(optarg);
            if (time_unit == TCC_TU_UNKNOWN) {
                pr_err("Invalid time unit: %s\n", optarg);
                exit(1);
            }
            break;
        case 'p':
            filename_prefix = strdup(optarg);
            break;
        case 'h':
            usage();
            exit(0);
        default:
            puts("Cannot recognize input option");
            usage();
            exit(1);
        }
    }

    int fail = 0;
    if (mode == MODE_UNKNOWN) {
        pr_err("--mode required\n");
        fail = 1;
    }

    if (iterations_count == -1) {
        pr_err("--iterations required\n");
        fail = 1;
    }

    if (time_unit == TCC_TU_UNKNOWN) {
        pr_err("--time-unit required\n");
        fail = 1;
    }

    if (filename_prefix == NULL) {
        pr_err("--prefix required\n");
        fail = 1;
    }

    if ((mode == MODE_META_ITT || mode == MODE_META_MAPI) && cumulative_count == -1) {
        pr_err("--cumulative-count required in modes \"meta-ml\" and \"meta-itt\"\n");
        fail = 1;
    }

    if ((mode == MODE_WORKLOAD_MAPI || mode == MODE_WORKLOAD_ITT) && workload_iterations_count == -1) {
        pr_err("--workload-iterations required in modes \"workload-ml\" and \"workload-itt\"\n");
        fail = 1;
    }

    /*printf("Mode:              %d\n", mode);
    printf("Iterations:          %d\n", iterations_count);
    printf("Workload iterations: %d\n", workload_iterations_count);
    printf("Cumulative count:    %d\n", cumulative_count);
    printf("Time units:          %s\n", tcc_measurement_get_string_from_time_unit(time_unit));*/

    if (fail) {
        exit(1);
    }
}

void usage()
{
    printf(
        "ITT API overhead benchmark\n"
        "Usage:\n"
        "itt_benchmark -m <mode> -i <iterations> -p <filename prefix> [-w <workload iterations>] [-c <cumulative count>] [-u time unit]\n"
        "Options:\n"
        "    -m | --mode                operating mode: empty-ml, empty-itt, meta-ml, meta-itt, workload-ml, workload-itt\n"
        "    -i | --iterations          number of the iterations\n"
        "    -p | --prefix              dump filename prefix\n"
        "    -w | --workload-iterations number of iterations in every single workload (for workload-* modes)\n"
        "    -c | --cumulative-count    number of workload runs in one measurement (for meta-* modes)\n"
        "    -u | --time-unit           time unit (clk, ns, us)\n");
}
