/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "tcc_region_manager.h"

#ifndef _TCC_TCC_REGION_MANAGER_INTERNAL_H_
#define _TCC_TCC_REGION_MANAGER_INTERNAL_H_

#define HARDCODED_DRAM_LATENCY_CLK 12402

// Region_manager_default
#ifdef __cplusplus
extern "C" {
#endif

region_manager_t* region_manager_driver_create(const tcc_config_t* config);
void region_manager_driver_destroy(region_manager_t* self);
ssize_t region_manager_driver_count(region_manager_t* self);
const memory_properties_t* region_manager_driver_get(region_manager_t* self, size_t index);
void* region_manager_driver_mmap(region_manager_t* self, size_t index, size_t size);

region_manager_t* region_manager_default_create(const tcc_config_t* config);
void region_manager_default_destroy(region_manager_t* self);
ssize_t region_manager_default_count(region_manager_t* self);
const memory_properties_t* region_manager_default_get(region_manager_t* self, size_t index);
void* region_manager_default_mmap(region_manager_t* self, size_t index, size_t size);

void fill_cpu_mask(cpu_set_t* cpuset);
int region_manager_get_dram_region_fd(size_t size);

#ifdef __cplusplus
}
#endif

#endif  // _TCC_TCC_REGION_MANAGER_INTERNAL_H_
