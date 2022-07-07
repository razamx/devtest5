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
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h> /* For SYS_xxx definitions */
#include <sys/mman.h>    /* mlockall */
#include <stdarg.h>
#include <sched.h>
#include <linux/random.h>
#include <getopt.h>
#include "tcc/cache.h"
#include "tcc/measurement_helpers.h"
#include "measurement.h"
#include "macros.h"

//4 page sizes block
#define BUFFER_BLOCK_SIZE (4 * 4096)

static int parse_params(const int argc, char* const* argv);
static int init();
static void workload(struct tcc_measurement* measurement);

struct tcc_measurement g_measurement;

// 512K of function calls, 8k of functions

static unsigned int functions_size = BUFFER_BLOCK_SIZE;

#define CALL_LENGTH 12
#define CALLS_NUM (1 << 20)
#define CALLS_SIZE (CALLS_NUM * 16)

typedef void (*func)(void);

// Simple function w/o parameters with single memory write inside
static unsigned char myproc[] = {0x48, 0xc7, 0xc1, 0x11, 0x11, 0x11, 0x11, 0xc3};

static unsigned char calls[CALLS_SIZE] __attribute__((aligned(0x1000)));
static unsigned char* functions = NULL;

void print_usage();

size_t buffer_size = BUFFER_BLOCK_SIZE;
void* buffer = NULL;
int sleep_flag = -1;
int cumulative_flag = -1;
long iterations = 100;
int time_units = TCC_TU_CLK;
int cpuid = 3;
int latency = 150;

struct timespec sleep_timeout = (struct timespec) {.tv_sec = 0, .tv_nsec = 1000000};

int main(int argc, char* argv[])
{
    if (!parse_params(argc, argv)) {
        print_usage();
        return 1;
    }

    mlockall(MCL_CURRENT | MCL_FUTURE);

    /* Initialize tcc libraries */
    /* Bind a process to the CPU specified by the cpuid variable (set process affinity) */
    PRINT_ERROR_EXIT(tcc_cache_init(cpuid));

    static const char measurement_name[] = "Workload";
    PRINT_ERROR_EXIT(tcc_measurement_init(
        &g_measurement, tcc_measurement_get_buffer_size_from_env(measurement_name), measurement_name));

    printf("Allocates memory according to the latency\n");
    int page_size = getpagesize();
    buffer = tcc_cache_malloc(functions_size + page_size, latency);
    if (buffer == NULL) {
        print_error_description(-TCC_E_MEMORY, "tcc_cache_malloc(BUFFER_SIZE, latency)", __LINE__, __FILE__);
        return 1;
    }

    functions = (void*)(((uintptr_t)buffer) & ~(page_size - 1));
    if (0 != init()) {
        tcc_cache_free(functions);
        tcc_cache_finish();
        return 1;
    }

    for (long count = iterations; count > 0; count--) {
        workload(&g_measurement);
        if (sleep_flag) {
            nanosleep(&sleep_timeout, NULL);
        }
    }

    printf("Deallocates the memory\n");
    tcc_cache_free(buffer);

    PRINT_ERROR(tcc_measurement_print(&g_measurement, TCC_TU_CLK));
    const char* dump_file_name = tcc_measurement_get_dump_file_from_env();
    if (dump_file_name) {
        FILE* dump_file = fopen(dump_file_name, "w");
        PRINT_ERROR(tcc_measurement_print_history(&g_measurement, dump_file, time_units));
        fclose(dump_file);
    }

    PRINT_ERROR(tcc_measurement_destroy(&g_measurement));
    PRINT_ERROR(tcc_cache_finish());

    return 0;
}


int parse_params(const int argc, char* const* argv)
{
    while (1) {
        static struct option long_options[] = {{"sleep", required_argument, &sleep_flag, 1},
            {"nosleep", no_argument, &sleep_flag, 0},
            {"iterations", required_argument, NULL, 2},
            {"buffersize", required_argument, NULL, 3},
            {"time_units", required_argument, NULL, 4},
            {"latency", required_argument, NULL, 5},
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
                functions_size = atol(optarg) * BUFFER_BLOCK_SIZE;
            }
            break;
        case 4:
            if (optarg) {
                int t_units = atoi(optarg);
                if (t_units == TCC_TU_CLK || t_units == TCC_TU_NS || t_units == TCC_TU_US) {
                    time_units = t_units;
                } else {
                    printf("Incorrect time units\n");
                    return 0;
                }
            }
            break;
        case 5:
            if (optarg) {
                latency = atoi(optarg);
                if (0 >= latency) {
                    printf("Input latency %d must be a positive integer\n", latency);
                    print_usage();
                    return 0;
                }
            }
            break;
        default:
            puts("Incorrect parameter passed");
            return 0;
        }
    }

    return (sleep_flag == -1) ? 0 : 1;
}

void print_usage()
{
    printf("Usage: icache_benchmark      --latency [N] \n"
           "                             --sleep [N]  | --nosleep\n"
           "                             [--iterations [N]]  :: default 10000\n"
           "                             [--buffersize [size in blocks of 128Kb ]] :: default 1\n"
           "                             [--time_units [UNITS]] :: default 0\n"
           "                                            UNITS : clocks       : 0\n"
           "                                                    nanoseconds  : 1\n"
           "                                                    microseconds : 2\n");
}


/////////////////////////////////

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
static void build_calls(int len, unsigned N)
{
    for (int i = 0; i < len; i++) {
        unsigned char* current = calls + i * CALL_LENGTH;
        unsigned char* random_function_address = functions + 64 * (rand() % N);

        *(current + 0) = 0x48;  // prefix
        *(current + 1) = 0xb8;  // MOV imm64, %rax
        *(intptr_t*)(current + 2) = (intptr_t)random_function_address;
        *(current + 10) = 0xff;
        *(current + 11) = 0xd0;  // callq  *%rax
    }

    *(calls + len * CALL_LENGTH) = 0xc3;
}
#pragma GCC diagnostic pop

static int init()
{
    static volatile unsigned int i = 0;
    unsigned int functions_number = functions_size / 64;

    if (mprotect(functions, functions_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Cannot make functions memory Read/Write/Executable: %s\n", strerror(errno));
        return -1;
    }

    if (mprotect(calls, CALLS_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Cannot make calls memory Read/Write/Executable: %s\n", strerror(errno));
        return -2;
    }

    for (i = 0; i < functions_number; i++) {
        memcpy((unsigned char*)functions + i * 64, myproc, sizeof(myproc));
    }

    build_calls(CALLS_NUM, functions_number);
    return 0;
}

static void workload(struct tcc_measurement* measurement)
{
    tcc_measurement_start(measurement);
    ((func)calls)();
    tcc_measurement_end(measurement, 1);
}
