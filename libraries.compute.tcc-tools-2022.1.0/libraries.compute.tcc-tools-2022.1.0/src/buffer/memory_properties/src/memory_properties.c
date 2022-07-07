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
#include "memory_properties.h"
#include "tcc_log.h"
#include <sched.h>
#include <stdio.h>
#include <inttypes.h>
#include "tcc_time.h"

#define PRINT_CPU_SET_NUM_CPUS 8
#define PRINT_BUFFER_SIZE 4096
#define CPU_SET_BUFFER_SIZE (PRINT_CPU_SET_NUM_CPUS + 1)


static const char* memory_level_names[] = {[TCC_MEM_UNKNOWN] = "TCC_MEM_UNKNOWN",
    [TCC_MEM_L1] = "TCC_MEM_L1",
    [TCC_MEM_L2] = "TCC_MEM_L2",
    [TCC_MEM_L3] = "TCC_MEM_L3",
    [TCC_MEM_EDRAM] = "TCC_MEM_EDRAM",
    [TCC_MEM_DRAM] = "TCC_MEM_DRAM"};

static char print_buffer_cpu_set[CPU_SET_BUFFER_SIZE] = {0};
static char print_buffer_memory_properties[PRINT_BUFFER_SIZE] = {0};

__attribute__((const)) const char* tcc_memory_level_to_str(enum tcc_memory_level level)
{
    if (level < TCC_MEM_UNKNOWN || level > TCC_MEM_DRAM) {
        level = TCC_MEM_UNKNOWN;
    }
    return memory_level_names[level];
}

const char* cpu_set_to_str(const cpu_set_t* cpuset)
{
    if (cpuset == NULL) {
        return "NULL";
    }

    size_t cpu_cnt = PRINT_CPU_SET_NUM_CPUS;
    size_t i = 0;
    for (; i < cpu_cnt; i++) {
        char symbol = CPU_ISSET(i, cpuset) ? '1' : '0';
        print_buffer_cpu_set[i] = symbol;
    }
    print_buffer_cpu_set[i] = '\0';
    return print_buffer_cpu_set;
}

const char* memory_properties_to_str(const memory_properties_t* properties)
{
    if (properties == NULL) {
        return "NULL";
    }

    snprintf(print_buffer_memory_properties,
        PRINT_BUFFER_SIZE,
        "\"id\":%u, "
        "\"mask\":\"%s\", "
        "\"latency\":%u, "
        "\"latency_clc\":%u, "
        "\"level\":\"%s\", "
        "\"size_drv\":%zu, "
        "\"size\":%zu",
        properties->id,
        cpu_set_to_str(&properties->mask),
        properties->latency,
        properties->latency_clk,
        tcc_memory_level_to_str(properties->type),
        properties->size_drv,
        properties->size);
    return print_buffer_memory_properties;
}

int memory_properties_compare(const void* a, const void* b)
{
    TCC_TRACE_FUNC("a=%p b=%p", a, b);

    if (a == NULL || b == NULL) {
        TCC_LOG_ERROR("One or more parameters is NULL: a=%p, b=%p", a, b);
        return 0;
    }

    const memory_properties_t* pa = (const memory_properties_t*)a;
    const memory_properties_t* pb = (const memory_properties_t*)b;

    TCC_LOG_TRACE("a=%s", memory_properties_to_str(pa));
    TCC_LOG_TRACE("b=%s", memory_properties_to_str(pb));

    if (pa->latency > pb->latency) {
        return -1;
    }
    if (pa->latency < pb->latency) {
        return 1;
    }
    size_t a_msk_len = CPU_COUNT(&pa->mask);
    size_t b_msk_len = CPU_COUNT(&pb->mask);
    if (a_msk_len < b_msk_len) {
        return -1;
    }
    if (a_msk_len > b_msk_len) {
        return 1;
    }
    return 0;
}
