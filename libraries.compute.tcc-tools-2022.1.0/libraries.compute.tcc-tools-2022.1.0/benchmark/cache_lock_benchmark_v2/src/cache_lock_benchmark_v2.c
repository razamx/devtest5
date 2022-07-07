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
#define _GNU_SOURCE /* See feature_test_macros(7) */
#include "benchmark_common.h"
#include "tcc_log.h"
#include "buffer_allocator.h"
#include "pointer_chasing.h"
#include "workload.h"
#include "params.h"
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/io.h>
#include "benchmark_common.h"
#include "tcc_region_manager.h"

#define DISABLE_INTERRUPTS() \
    {                        \
        asm volatile("cli"); \
    }
#define ENABLE_INTERRUPTS()  \
    {                        \
        asm volatile("sti"); \
    }

#define TCC_PRINT_PARAM_TO_FILE(file, name)                     \
    ({                                                          \
        if (fprintf(file, "-%s_%u", #name, control.name) < 0) { \
            TCC_LOG_ERROR("fprintf:%s", strerror(errno));       \
            goto error;                                         \
        }                                                       \
    })

// /////////////////////////////////////////////////////////////////////////////
//    INITIALISATION
// /////////////////////////////////////////////////////////////////////////////

static int set_afinity(unsigned cpuid)
{
    cpu_set_t cpuset;
    /* Bind a process to certain CPU (set process affinity) specified by cpuid variable */
    /* As target CPU supports L2 cache distributed among pairs of cores,
     * the same part of L2 cache should be used by the process and TCC API to ensure the correct behavior of pseudo-lock */
    CPU_ZERO(&cpuset);
    CPU_SET(cpuid, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) < 0) {
        TCC_LOG_ERROR("sched_setaffinity:%s", strerror(errno));
        return -1;
    }
    return 0;
}

static int init()
{
    int status = -1;
    char* tcc_measurement_buffers_val = NULL;
    char* dump_file_name = get_dump_file_name();
    if (dump_file_name == NULL) {
        TCC_LOG_ERROR("get_dump_file_name");
        goto end;
    }
    if (setenv("TCC_MEASUREMENTS_DUMP_FILE", dump_file_name, 0)) {
        TCC_LOG_ERROR("setenv:%s", strerror(errno));
        goto end;
    }
    printf("TCC_MEASUREMENTS_DUMP_FILE=%s\n", getenv("TCC_MEASUREMENTS_DUMP_FILE"));
    if (asprintf(&tcc_measurement_buffers_val, "%s:%lu", WORKLOAD_NAME, control.iterations) < 0) {
        TCC_LOG_ERROR("asprintf:%s", strerror(errno));
        goto end;
    }
    if (setenv("TCC_MEASUREMENTS_BUFFERS", tcc_measurement_buffers_val, 0)) {
        TCC_LOG_ERROR("setenv:%s", strerror(errno));
        goto end;
    }
    printf("TCC_MEASUREMENTS_BUFFERS=%s\n", getenv("TCC_MEASUREMENTS_BUFFERS"));
    if (set_afinity(control.cpuid) != 0) {
        TCC_LOG_ERROR("set_afinity");
        goto end;
    }

    if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        TCC_LOG_ERROR("mlockall:%s", strerror(errno));
        goto end;
    }

    if (control.interrupts) {
        TCC_LOG_WARNING("Interupts enabled");
        status = 0;
        goto end;
    } else if (iopl(3) < 0) {
        printf("Unable to set IO privilege level, error: %s\n", strerror(errno));
        goto end;
    } else {
        DISABLE_INTERRUPTS();
    }
    status = 0;
end:
    free(tcc_measurement_buffers_val);
    return status;
}


int main(int argc, char** argv)
{
    if (parse_params(argc, argv) != 0) {
        TCC_LOG_ERROR("Can't parse params");
        bench_printf("wrong usage!\n");
        goto error;
    }
    bench_printf("Initializing...\n");
    if (init() != 0) {
        TCC_LOG_ERROR("Cannot initialize benchmark");
        goto error;
    }
    void* buffer = NULL;
    buffer = buffer_alloc(control.region_index, control.buffer_size);
    if (buffer == NULL) {
        bench_printf("Can't allocate buffer memory");
        goto error;
    }
    cache_line_node_t* lines = NULL;
    if (control.random_seed) {
        srandom(control.random_seed);
        lines = pointer_chase_create_random(buffer, control.buffer_size, random);
    } else {
        lines = pointer_chase_create_linear(buffer, control.buffer_size);
    }
    bench_printf("Workload start...\n");
    if (workload_run(control.workload, lines) != 0) {
        bench_printf("Can't run workload");
    }
    bench_printf("\nWorkload done\n");

    bench_printf("Measure data will be stored to:%s\n", getenv("TCC_MEASUREMENTS_DUMP_FILE"));

    allocate_buffer_deinit();
    return 0;
error:
    bench_printf("benchmark error!\n");
    return -1;
}
