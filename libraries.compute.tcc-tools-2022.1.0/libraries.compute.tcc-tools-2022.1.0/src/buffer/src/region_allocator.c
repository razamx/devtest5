/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "region_allocator.h"
#include "allocator_interface.h"
#include <errno.h>
#include <string.h>
#include <tcc/err_code.h>


#define UNUSED(x) ((void)(x))

void* region_allocator_malloc(allocator_interface_t* allocator, size_t size)
{
    TCC_TRACE_FUNC("allocator=%p size=%zu", allocator, size);
    TCC_LOG_INFO("Calling region malloc with: size=%zu", size);
    return umm_malloc(size, ((region_allocator_t*)allocator)->instance);
}

void* region_allocator_calloc(allocator_interface_t* allocator, size_t num, size_t size)
{
    TCC_TRACE_FUNC("allocator=%p num=%zu size=%zu", allocator, num, size);
    TCC_LOG_DEBUG("Calling region calloc with parameters: nmemb=%zu, size=%zu", num, size);
    return umm_calloc(num, size, ((region_allocator_t*)allocator)->instance);
}

void* region_allocator_realloc(allocator_interface_t* allocator, void* ptr, size_t size)
{
    TCC_TRACE_FUNC("allocator=%p ptr=%p size=%zu", allocator, ptr, size);
    TCC_LOG_DEBUG("Calling region realloc with parameters: ptr=%p, size=%zu", ptr, size);
    return umm_realloc(ptr, size, ((region_allocator_t*)allocator)->instance);
}

void region_allocator_free(allocator_interface_t* allocator, void* ptr)
{
    TCC_TRACE_FUNC("allocator=%p ptr=%p", allocator, ptr);
    TCC_LOG_DEBUG("Calling region free with: ptr=%p", ptr);
    umm_free(ptr, ((region_allocator_t*)allocator)->instance);
}

void region_allocator_destroy(allocator_interface_t* allocator)
{
    TCC_TRACE_FUNC("allocator=%p", allocator);

    if (allocator != NULL) {
        TCC_LOG_TRACE("free umm_allocator, instance=%p", allocator->instance);
        free(allocator->instance);
        allocator->instance = NULL;
    }
    free(allocator);
}

allocator_interface_t* region_allocator_create(void* memory, size_t size)
{
    TCC_TRACE_FUNC("memory=%p size=%zu", memory, size);

    allocator_interface_t* allocator = (allocator_interface_t*)malloc(sizeof(allocator_interface_t));
    if (allocator == NULL) {
        TCC_LOG_ERROR("Unable to allocate memory for allocator");
        return NULL;
    }

    allocator->malloc = region_allocator_malloc;
    allocator->calloc = region_allocator_calloc;
    allocator->realloc = region_allocator_realloc;
    allocator->free = region_allocator_free;
    allocator->destroy = region_allocator_destroy;
    allocator->instance = (umm_malloc_t*)calloc(1, sizeof(umm_malloc_t));
    if (allocator->instance == NULL) {
        TCC_LOG_ERROR("calloc():error:%s(%i)", strerror(errno), errno);
        goto error;
    }
    if (umm_init(allocator->instance, memory, size) != TCC_E_SUCCESS) {
        TCC_LOG_ERROR("umm_init error");
        goto error;
    }
    return allocator;
error:
    free(allocator->instance);
    free(allocator);
    return NULL;
}
