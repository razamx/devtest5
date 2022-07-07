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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* See feature_test_macros(7) */
#endif              // _GNU_SOURCE
#include "benchmark_common.h"
#include "pointer_chasing.h"
#include "workload.h"
#include "params.h"
#include "tcc_log.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include "bench_measurement.h"

#define COLLECTOR_LIBRARY_VAR_NAME "INTEL_LIBITTNOTIFY64"
#define TCC_COLLECTOR_NAME "libtcc_collector.so"

#define MEASUREMENT_START bench_measurement_begin()
#define MEASUREMENT_END bench_measurement_end()

#define ACCESSES_PER_ITERATION_FOR_PTC_CHASE_RW_STRESS 6

#define UNUSED(var) ((void)(var))

// /////////////////////////////////////////////////////////////////////////////
//    WORKLOADS
// /////////////////////////////////////////////////////////////////////////////

#pragma GCC push_options
#pragma GCC optimize("O0")

typedef struct internal_workload_t
{
    cache_line_node_t* lines;
    size_t size;
} internal_workload_t;

#define PRINT_PROGRESS           \
    if (iterations % 100 == 0) { \
        fputc('|', stderr);      \
    }

static void internal_stress()
{
    static cache_line_node_t* internal_stress_chasing = NULL;
    if (internal_stress_chasing == NULL) {
        internal_stress_chasing = (cache_line_node_t*)aligned_alloc(PAGE_SIZE, control.stress_buffer_size);
        if (internal_stress_chasing == NULL) {
            fprintf(stderr, "Can't allocate memory for stress buffer");
            exit(1);
        }
        if (control.random_seed) {
            internal_stress_chasing =
                pointer_chase_create_random(internal_stress_chasing, control.stress_buffer_size, random);
        } else {
            internal_stress_chasing = pointer_chase_create_linear(internal_stress_chasing, control.stress_buffer_size);
        }

        internal_stress_chasing = pointer_chase_run_read_write_workload(internal_stress_chasing,
            control.stress_buffer_accesses / ACCESSES_PER_ITERATION_FOR_PTC_CHASE_RW_STRESS + 1);
    }
    internal_stress_chasing = pointer_chase_run_read_workload(internal_stress_chasing, control.stress_buffer_accesses);
}
#define WORKLOAD_BEGIN
void workload_measurement(__attribute__((unused)) cache_line_node_t* lines)
{
    TCC_TRACE_FUNC();
    unsigned iterations = control.iterations;
    while (iterations--) {
        if (control.sleep) {
            usleep(control.sleep);
        }
        MEASUREMENT_START;
        MEASUREMENT_END;
        PRINT_PROGRESS;
    }
}

void workload_pointer_chasing_with_internal_stress(cache_line_node_t* lines)
{
    TCC_TRACE_FUNC();
    register cache_line_node_t* line = lines;
    unsigned iterations = control.iterations;
    while (iterations--) {
        if (control.sleep) {
            usleep(control.sleep);
        }
        internal_stress();
        register size_t read_iterations = control.accesses;
        MEASUREMENT_START;
        line = pointer_chase_run_read_workload(line, read_iterations);
        MEASUREMENT_END;
        PRINT_PROGRESS;
    }
}

void workload_pointer_chasing(cache_line_node_t* lines)
{
    TCC_TRACE_FUNC();
    register cache_line_node_t* line = lines;
    unsigned iterations = control.iterations;
    while (iterations--) {
        if (control.sleep) {
            usleep(control.sleep);
        }
        register size_t read_iterations = control.accesses;
        MEASUREMENT_START;
        line = pointer_chase_run_read_workload(line, read_iterations);
        MEASUREMENT_END;
        PRINT_PROGRESS;
    }
}

void workload_driver_team(cache_line_node_t* lines)
{
    TCC_TRACE_FUNC();
    unsigned iterations = control.iterations;
    while (iterations--) {
        if (control.sleep) {
            usleep(control.sleep);
        }
        internal_stress();
        MEASUREMENT_START;
        void* test_addr = lines;
        long int buffer_size = (long int)control.buffer_size;
        for (long int i = 0; i < buffer_size; i += LINE_SIZE) {
            asm volatile("mov (%0,%1,1), %%eax\n\t" : : "r"(test_addr), "r"(i) : "%eax", "memory");
        }
        MEASUREMENT_END;
        PRINT_PROGRESS;
    }
}

#pragma GCC pop_options

// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////

#define WORKLOAD(name)         \
    {                          \
#name, workload_##name \
    }

workload_t workloads[] = {
    WORKLOAD(pointer_chasing_with_internal_stress),
    WORKLOAD(pointer_chasing),
    WORKLOAD(measurement),
    WORKLOAD(driver_team),
};

const workload_t* workload_get_by_id(size_t id)
{
    TCC_TRACE_FUNC("td:%zu", id)
    if (id >= sizeof(workloads) / sizeof(workload_t)) {
        TCC_LOG_WARNING("id %zu is out of range", id);
        return NULL;
    }
    return &workloads[id];
}

const workload_t* workload_get_by_name(const char* name)
{
    TCC_TRACE_FUNC();
    size_t i = 0;
    while (workloads[i].name != NULL) {
        if (strcmp(name, workloads[i].name) == 0) {
            TCC_LOG_DEBUG("Found workload:%s", workloads[i].name);
            return &workloads[i];
        }
        i++;
    }
    TCC_LOG_WARNING("Can't find workload with name:'%s'", name);
    return NULL;
}

int workload_run(const workload_t* workload, cache_line_node_t* lines)
{
    if (workload == NULL) {
        TCC_LOG_WARNING("Workload is null");
        goto error;
    }
    if (lines == NULL) {
        TCC_LOG_WARNING("Lines is NULL");
        goto error;
    }
    if (bench_measurement_init(WORKLOAD_NAME, control.cpuid, control.collect_counters) < 0) {
        TCC_LOG_ERROR("Can't init measurement");
        goto error;
    }
    workload->func(lines);
    return 0;
error:
    return -1;
}
