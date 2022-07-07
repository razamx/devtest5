/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_ALLOCATORS_MANAGER_H_
#define _TCC_ALLOCATORS_MANAGER_H_

#include "allocator_interface.h"
#include "memory_properties.h"
#include "tcc_config.h"
#include "tcc_region_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Helper macro to create for-each loop over all allocators from allocator manager
 * @param self         allocators_manager instance
 * @param requirements memory requirements for allocator
 * @param it           iterator
 *
 * Note: it should be initialized with NULL
 */
#define ALLOCATORS_FOREACH(self, requirements, it) \
    while (((it) = allocators_manager_get_next(self, requirements, it)) != NULL)

typedef struct allocators_manager allocators_manager_t;
typedef struct allocator_it allocator_it_t;

allocators_manager_t* allocators_manager_create(region_manager_t* region_manager, const tcc_config_t* config);
void allocators_manager_destroy(allocators_manager_t* self);

/**
 * @brief Get the next iterator which meets the given requirements
 * @param self allocators_manager instance
 * @param requirements Memory requirements
 * @param it Previous iterator
 * @return Iterator. NULL if no allocator meets requirements
 */
const allocator_it_t* allocators_manager_get_next(allocators_manager_t* self,
    memory_properties_t* requirements,
    const allocator_it_t* it);

/**
 * @brief Get pointer to the allocator which was used to allocate given pointer
 * @param self allocators_manager instance
 * @param ptr Pointer to memory
 * @return Pointer to the allocator which is used to allocate given pointer.
 *         Pointer cannot be NULL.
 */
allocator_interface_t* allocators_manager_get_alloc_by_ptr(allocators_manager_t* self, const void* ptr);

/// Get allocator stored in the iterator
allocator_interface_t* get_allocator_from_iterator(const allocator_it_t* it);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_ALLOCATORS_MANAGER_H_
