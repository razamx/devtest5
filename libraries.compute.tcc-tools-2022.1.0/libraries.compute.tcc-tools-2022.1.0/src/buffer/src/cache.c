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
#include "tcc/cache.h"
#include "allocators_manager.h"
#include "memory_properties.h"
#include "tcc/err_code.h"
#include "tcc_config.h"
#include "tcc_config_finder.h"
#include "tcc_log.h"
#include "tcc_region_manager.h"
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#define UNUSED(x) ((void)(x))

#ifndef STATIC
#define STATIC static
#endif

#define CONFIG_USR_PATH "/usr/share/tcc_tools/config"
#define TCC_CONFIG_FILE ".tcc.config"

#define CHECK_INITIALIZED()                                                  \
    ({                                                                       \
        if (tcc_cache_state == NULL) {                                       \
            TCC_LOG_ERROR("TCC Cache isn't initialized");                    \
            return NULL;                                                     \
        }                                                                    \
        if (is_tcc_buffer_exists(tcc_cache_state->region_manager) == 0) {    \
            TCC_LOG_WARNING("TCC Driver not found. Regular memory is used"); \
        }                                                                    \
    })

#define GET_BUFFER(latency, method, ...)                                          \
    ({                                                                            \
        void* ptr = NULL;                                                         \
        const allocator_it_t* it = NULL;                                          \
        memory_properties_t* requirements = NULL;                                 \
        requirements = get_requrement(latency);                                   \
        if (requirements == NULL) {                                               \
            TCC_LOG_ERROR("Unable to get requirements");                          \
            return NULL;                                                          \
        }                                                                         \
        ALLOCATORS_FOREACH(tcc_cache_state->allocators_manager, requirements, it) \
        {                                                                         \
            allocator_interface_t* allocator = get_allocator_from_iterator(it);   \
            ptr = allocator->method(allocator, __VA_ARGS__);                      \
            if (ptr) {                                                            \
                break;                                                            \
            }                                                                     \
        }                                                                         \
        if (ptr == NULL) {                                                        \
            TCC_LOG_WARNING("Unable to allocate cache");                          \
        }                                                                         \
        TCC_LOG_DEBUG("Allocated cache: %p", ptr);                                \
        ptr;                                                                      \
    })

struct tcc_cache
{
    uint64_t latency;
    tcc_config_t* config;
    region_manager_t* region_manager;
    allocators_manager_t* allocators_manager;
    memory_properties_t requirements_tmp;
};

STATIC struct tcc_cache* tcc_cache_state = NULL;

static int get_cpu_affinity(cpu_set_t* cpuset)
{
    TCC_TRACE_FUNC("cpuset=%s", cpu_set_to_str(cpuset));

    int status = sched_getaffinity(getpid(), sizeof(cpu_set_t), cpuset);
    if (status == -1) {
        TCC_LOG_ERROR("sched_getaffinity(%i):error:%s(%i)", getpid(), strerror(errno), errno);
    }

    return status;
}

static int set_cpu_affinity(int cpuid)
{
    TCC_TRACE_FUNC("cpuid=%d", cpuid);

    int nprocs = get_nprocs();
    if (cpuid < -1 || cpuid >= nprocs) {
        TCC_LOG_ERROR("Index %i out of range, max:%i", cpuid, nprocs);
        return -TCC_E_BAD_PARAM;
    }
    if (cpuid == -1) {
        TCC_LOG_INFO("cpuid set as -1. Use current affinity");
        return TCC_E_SUCCESS;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuid, &cpuset);
    if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset) == -1) {
        TCC_LOG_ERROR("sched_setaffinity(pid%i, cpuid:%i)):error:%s(%i)", getpid(), cpuid, strerror(errno), errno);
        return -TCC_E_ERROR;
    }
    return 0;
}

int tcc_cache_set_default_latency(uint64_t latency)
{
    TCC_TRACE_FUNC("latency=%" PRIu64, latency);
    if (tcc_cache_state == NULL) {
        TCC_LOG_ERROR("TCC Buffer isn't initialized");
        return -TCC_E_NOT_AVAILABLE;
    }
    tcc_cache_state->latency = latency;
    return TCC_E_SUCCESS;
}

int tcc_cache_init(int8_t cpuid)
{
    TCC_TRACE_FUNC("cpuid=%i" PRId64, cpuid);

    int status;
    uint64_t default_latency = 0;

    if (tcc_cache_state != NULL) {
        TCC_LOG_ERROR("TCC Buffer is already initialized");
        status = -TCC_E_NOT_AVAILABLE;
        goto error;
    }
    status = set_cpu_affinity(cpuid);
    if (status < 0) {
        TCC_LOG_ERROR("Unable to set affinity: %s(%i)", strerror(errno), errno);
        goto error;
    }

    status = -TCC_E_ERROR;
    tcc_cache_state = calloc(1, sizeof(struct tcc_cache));
    if (tcc_cache_state == NULL) {
        TCC_LOG_ERROR("Unable to allocate memory for state: %s(%i)", strerror(errno), errno);
        goto error;
    }

    tcc_cache_state->latency = default_latency;
    const char* config_path = get_available_config_file();
    if (config_path == NULL) {
        TCC_LOG_ERROR("Unable find available config: %s(%i)", strerror(errno), errno);
        goto error;
    }
    TCC_LOG_INFO("Read config file: '%s'", config_path);
    tcc_cache_state->config = tcc_config_read(config_path);
    if (tcc_cache_state->config == NULL) {
        TCC_LOG_ERROR("Unable to read config file '%s': %s(%i)", config_path, strerror(errno), errno);
        goto error;
    }
    tcc_cache_state->region_manager = region_manager_create(tcc_cache_state->config);
    if (tcc_cache_state->region_manager == NULL) {
        TCC_LOG_ERROR("Unable to create region_manager: %s(%i)", strerror(errno), errno);
        goto error;
    }
    tcc_cache_state->allocators_manager =
        allocators_manager_create(tcc_cache_state->region_manager, tcc_cache_state->config);
    if (tcc_cache_state->allocators_manager == NULL) {
        TCC_LOG_ERROR("Unable to create allocators_manager: %s(%i)", strerror(errno), errno);
        goto error;
    }
    if (is_tcc_buffer_exists(tcc_cache_state->region_manager) == 0) {
        TCC_LOG_WARNING("TCC Driver not found. Regular memory is used");
        goto tcc_buffer_not_exists;
    }
    return TCC_E_SUCCESS;
error:
    tcc_cache_finish();
    return status;
tcc_buffer_not_exists:
    return TCC_BUFFER_NOT_FOUND;
}

int tcc_cache_finish()
{
    TCC_TRACE_FUNC();

    if (tcc_cache_state == NULL) {
        TCC_LOG_ERROR("TCC Buffer isn't initialized");
        return TCC_E_SUCCESS;
    }
    allocators_manager_destroy(tcc_cache_state->allocators_manager);
    region_manager_destroy(tcc_cache_state->region_manager);
    tcc_config_destroy(tcc_cache_state->config);
    free(tcc_cache_state);
    tcc_cache_state = NULL;
    return TCC_E_SUCCESS;
}

static memory_properties_t* get_requrement(uint64_t latency)
{
    TCC_TRACE_FUNC("latency=%" PRIu64, latency);

    memset(&(tcc_cache_state->requirements_tmp), 0, sizeof(memory_properties_t));
    tcc_cache_state->requirements_tmp.latency = latency > UINT_MAX ? UINT_MAX : latency;
    if (get_cpu_affinity(&(tcc_cache_state->requirements_tmp.mask)) == -1) {
        TCC_LOG_ERROR("Unable to get affinity: %s(%i)", strerror(errno), errno);
        goto error;
    }
    return &(tcc_cache_state->requirements_tmp);
error:
    return NULL;
}

static void* tcc_buffer_malloc_internal(size_t size, uint64_t latency)
{
    TCC_TRACE_FUNC("size=%zu, latency=%" PRId64, size, latency);
    return GET_BUFFER(latency, malloc, size);
}

void* tcc_cache_malloc(size_t size, uint64_t latency)
{
    TCC_TRACE_FUNC("size=%zu latency=%" PRIu64, size, latency);
    CHECK_INITIALIZED();
    return tcc_buffer_malloc_internal(size, latency);
}

void* tcc_cache_malloc_default(size_t size)
{
    TCC_TRACE_FUNC("size=%zu", size);
    CHECK_INITIALIZED();
    if (tcc_cache_state->latency == 0) {
        TCC_LOG_ERROR("Default latency wasn't provided");
    }
    return tcc_buffer_malloc_internal(size, tcc_cache_state->latency);
}

static void* tcc_buffer_calloc_internal(size_t num, size_t size, uint64_t latency)
{
    TCC_TRACE_FUNC("num=%zu size=%zu latency=%" PRIu64, num, size, latency);
    return GET_BUFFER(latency, calloc, num, size);
}

void* tcc_cache_calloc(size_t num, size_t size, uint64_t latency)
{
    TCC_TRACE_FUNC("num=%zu size=%zu latency=%" PRIu64, num, size, latency);
    CHECK_INITIALIZED();
    return tcc_buffer_calloc_internal(num, size, latency);
}

void* tcc_cache_calloc_default(size_t num, size_t size)
{
    TCC_TRACE_FUNC("num=%zu size=%zu", num, size);
    CHECK_INITIALIZED();
    if (tcc_cache_state->latency == 0) {
        TCC_LOG_ERROR("Default latency wasn't provided");
    }
    return tcc_buffer_calloc_internal(num, size, tcc_cache_state->latency);
}

void* tcc_cache_realloc(void* ptr, size_t size)
{
    TCC_TRACE_FUNC("ptr=%p size=%zu", ptr, size);

    CHECK_INITIALIZED();
    allocator_interface_t* allocator = allocators_manager_get_alloc_by_ptr(tcc_cache_state->allocators_manager, ptr);
    if (allocator == NULL) {
        TCC_LOG_FATAL("allocators_manager_get_alloc_by_ptr(%p, %p) failed: %s(%i)",
            tcc_cache_state->allocators_manager,
            ptr,
            strerror(errno),
            errno);
        return NULL;
    }
    void* new_ptr = allocator->realloc(allocator, ptr, size);
    if (new_ptr == NULL) {
        TCC_LOG_ERROR("Unable reallocate buffer: %s(%i)", strerror(errno), errno);
    }
    return new_ptr;
}

void tcc_cache_free(void* ptr)
{
    TCC_TRACE_FUNC("ptr=%p", ptr);

    if (tcc_cache_state == NULL) {
        TCC_LOG_ERROR("TCC Buffer isn't initialized");
        return;
    }
    allocator_interface_t* allocator = allocators_manager_get_alloc_by_ptr(tcc_cache_state->allocators_manager, ptr);
    if (allocator == NULL) {
        TCC_LOG_FATAL("allocators_manager_get_alloc_by_ptr(%p, %p) failed: %s(%i)",
            tcc_cache_state->allocators_manager,
            ptr,
            strerror(errno),
            errno);
        return;
    }
    allocator->free(allocator, ptr);
}
