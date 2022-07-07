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
#include "buffer_allocator.h"
#include "stddef.h"
#include "tcc_log.h"
#include "tcc_config_finder.h"
#include "tcc_config.h"
#include "tcc_region_manager.h"
#include "benchmark_common.h"
#include <stdlib.h>
#include "memory_properties.h"

region_manager_t* region_manager = NULL;

int allocate_buffer_init()
{
    if (region_manager == NULL) {
        region_manager = region_manager_create(tcc_config_read(get_available_config_file()));
        if (region_manager == NULL) {
            TCC_LOG_ERROR("Can't create region_manager");
            return -1;
        }
    }
    return 0;
}

void allocate_buffer_deinit()
{
    TCC_TRACE_FUNC();
    region_manager_destroy(region_manager);
}

void* buffer_alloc(size_t index, size_t size)
{
    if (allocate_buffer_init() < 0) {
        goto error;
    }
    const memory_properties_t* prop = region_manager_get(region_manager, index);
    if (prop == NULL) {
        TCC_LOG_ERROR("Unknown index %zu", index);
        goto error;
    }
    TCC_LOG_INFO("Allocate buffer from:#%zi %s\n", index, memory_properties_to_str(prop));
    printf("Allocate buffer from:#%zi %s\n", index, memory_properties_to_str(prop));
    void* buffer = NULL;
    if (prop->type == TCC_MEM_DRAM) {
        buffer = aligned_alloc(PAGE_SIZE, size);
    } else {
        buffer = region_manager_mmap(region_manager, index, size);
    }
    if (buffer == NULL) {
        TCC_LOG_ERROR("Can't allocate buffer");
        goto error;
    }
    return buffer;
error:
    region_manager_destroy(region_manager);
    return NULL;
}
