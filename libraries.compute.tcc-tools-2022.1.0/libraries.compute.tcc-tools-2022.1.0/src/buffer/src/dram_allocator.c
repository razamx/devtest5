/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "dram_allocator.h"
#include "allocator_interface.h"

void* dram_allocator_malloc(allocator_interface_t* allocator, size_t size)
{
    TCC_TRACE_FUNC("allocator=%p, size=%zu", allocator, size);
    TCC_LOG_INFO("Calling native malloc with: size=%zu", size);
    return malloc(size);
}

void* dram_allocator_calloc(allocator_interface_t* allocator, size_t num, size_t size)
{
    TCC_TRACE_FUNC("allocator=%p num=%zu size=%zu", allocator, num, size);
    TCC_LOG_DEBUG("Calling native calloc with parameters: nmemb=%lu, size=%lu", num, size);
    return calloc(num, size);
}

void* dram_allocator_realloc(allocator_interface_t* allocator, void* ptr, size_t size)
{
    TCC_TRACE_FUNC("allocator=%p ptr=%p size=%zu", allocator, ptr, size);
    TCC_LOG_DEBUG("Calling native realloc with parameters: ptr=%p, size=%lu", ptr, size);
    return realloc(ptr, size);
}

void dram_allocator_free(allocator_interface_t* allocator, void* ptr)
{
    TCC_TRACE_FUNC("allocator=%p ptr=%p", allocator, ptr);
    TCC_LOG_DEBUG("Calling native free with: ptr=%p", ptr);
    free(ptr);
}

void dram_allocator_destroy(allocator_interface_t* allocator)
{
    TCC_TRACE_FUNC("self=%p", allocator);
    free(allocator);
}

allocator_interface_t* dram_allocator_create()
{
    TCC_TRACE_FUNC();

    allocator_interface_t* allocator = (allocator_interface_t*)malloc(sizeof(allocator_interface_t));
    if (allocator == NULL) {
        TCC_LOG_ERROR("Unable to allocate memory for allocator");
        return NULL;
    }

    allocator->malloc = dram_allocator_malloc;
    allocator->calloc = dram_allocator_calloc;
    allocator->realloc = dram_allocator_realloc;
    allocator->free = dram_allocator_free;
    allocator->destroy = dram_allocator_destroy;
    allocator->instance = NULL;

    return allocator;
}
