/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TCC_REGION_MANAGER_H_
#define _TCC_TCC_REGION_MANAGER_H_

#include <inttypes.h>

#include "memory_properties.h"
#include "tcc_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct region_manager_t region_manager_t;

typedef struct region
{
    memory_properties_t properties;
    void* user_data;
} region_t;

typedef struct mmap_settings
{
    void* addr;
    int prot;
    int flags;
} mmap_settings_t;

region_manager_t* region_manager_create(const tcc_config_t* config);
void region_manager_destroy(region_manager_t* self);
int is_tcc_buffer_exists(region_manager_t* self);


/**
 * @brief Get region count
 * @param self     Region_manager instance
 * @return         Regions number. -1 if region_manager is not valid
 */
ssize_t region_manager_count(region_manager_t* self);

/**
 * @brief Get region instance
 * @param self     Region_manager instance
 * @param index    Index of the region
 * @return         Pointer to memory properties. NULL if the index is out of range
 */
const memory_properties_t* region_manager_get(region_manager_t* self, size_t index);

/**
 * @brief Get size memory from the region specified by iterator it
 * @param self     Region_manager instance
 * @param index    Index of the region
 * @param size     Size of the memory to mmap
 * @return         Pointer to the memory. NULL if memory cannot be allocated
 */
void* region_manager_mmap(region_manager_t* self, size_t index, size_t size);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_TCC_REGION_MANAGER_H_
