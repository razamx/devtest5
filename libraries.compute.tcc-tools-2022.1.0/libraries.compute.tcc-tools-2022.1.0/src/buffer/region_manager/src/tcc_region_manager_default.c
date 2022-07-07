/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "tcc_log.h"
#include "tcc_time.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "tcc_region_manager_internal.h"

static int is_region_manager_created = 0;

typedef struct region_manager_t
{
    memory_properties_t dram_region;
} region_manager_t;

static region_manager_t region_manager;

region_manager_t* region_manager_default_create(const tcc_config_t* config)
{
    TCC_TRACE_FUNC("config=%p", config);

    if (is_region_manager_created) {
        TCC_LOG_ERROR("Region manager is already created");
        errno = EBUSY;
        goto error;
    }
    memset(&region_manager, 0, sizeof(region_manager));
    region_manager.dram_region.latency = 0;
    region_manager.dram_region.type = TCC_MEM_DRAM;
    fill_cpu_mask(&(region_manager.dram_region.mask));
    region_manager.dram_region.id = (unsigned int)-1;
    region_manager.dram_region.size = (size_t)-1;
    is_region_manager_created = 1;
    return &region_manager;
error:
    return NULL;
}

void region_manager_default_destroy(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    if (self != &region_manager) {
        TCC_LOG_WARNING("Wrong region manager pointer %p", self);
        errno = EINVAL;
        return;
    }
    is_region_manager_created = 0;
    memset(&region_manager, 0, sizeof(region_manager));
}
ssize_t region_manager_default_count(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    if (self != &region_manager) {
        TCC_LOG_WARNING("Wrong region manager pointer %p", self);
        errno = EINVAL;
        return -1;
    }
    return 1;
}

const memory_properties_t* region_manager_default_get(region_manager_t* self, size_t index)
{
    TCC_TRACE_FUNC("self=%p index=%zu", self, index);

    if (self != &region_manager) {
        TCC_LOG_WARNING("Wrong region manager pointer %p", self);
        errno = EINVAL;
        goto error;
    }
    if (0 < index) {
        TCC_LOG_WARNING("Index out of range. Try get index %zu, but have only 1 entry", index);
        errno = EINVAL;
        goto error;
    }
    return &(self->dram_region);
error:
    return NULL;
}

void* region_manager_default_mmap(region_manager_t* self, size_t index, size_t size)
{
    TCC_TRACE_FUNC("self=%p index=%zu size=%zu", self, index, size);
    int buffer_fd = -1;
    void* memory = NULL;
    if (self != &region_manager) {
        TCC_LOG_WARNING("Wrong region manager pointer %p", self);
        errno = EINVAL;
        goto end;
    }
    if (0 < index) {
        TCC_LOG_WARNING("Index out of range. Try get index %zu, but have only 1 entry", index);
        errno = EINVAL;
        goto end;
    }
    if (size == 0) {
        TCC_LOG_WARNING("Size is zero");
        errno = EINVAL;
        goto end;
    }
    buffer_fd = region_manager_get_dram_region_fd(size);
    if (buffer_fd < 0) {
        TCC_LOG_ERROR("Can't create descriptor");
        goto end;
    }
    memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, buffer_fd, 0);
    if (memory == MAP_FAILED) {
        TCC_LOG_ERROR("Failure mmap(NULL, %zu, PROT_READ | PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0):error %s(%i)",
            size,
            strerror(errno),
            errno);
        memory = NULL;
        goto end;
    }
end:
    if (buffer_fd >= 0) {
        close(buffer_fd);
    }
    return memory;
}
