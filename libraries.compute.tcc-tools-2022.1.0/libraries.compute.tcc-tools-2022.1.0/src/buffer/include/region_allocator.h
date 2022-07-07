/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_REGION_ALLOCATOR_H_
#define _TCC_REGION_ALLOCATOR_H_

#include "allocator_interface.h"
#include "log/tcc_log.h"
#include "umm_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct region_allocator
{
    void* (*malloc)(allocator_interface_t* allocator, size_t size);
    void* (*calloc)(allocator_interface_t* allocator, size_t num, size_t size);
    void* (*realloc)(allocator_interface_t* allocator, void* ptr, size_t size);
    void (*free)(allocator_interface_t* allocator, void* ptr);
    void (*destroy)(allocator_interface_t* allocator);
    umm_malloc_t* instance;
} region_allocator_t;

void* region_allocator_malloc(allocator_interface_t* allocator, size_t size);
void* region_allocator_calloc(allocator_interface_t* allocator, size_t num, size_t size);
void* region_allocator_realloc(allocator_interface_t* allocator, void* ptr, size_t size);
void region_allocator_free(allocator_interface_t* allocator, void* ptr);
void region_allocator_destroy(allocator_interface_t* allocator);
allocator_interface_t* region_allocator_create(void* memory, size_t size);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_REGION_ALLOCATOR_H_
