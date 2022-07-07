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
#include "log/tcc_log.h"
#include "tcc_region_manager_internal.h"
#include <errno.h>
#include <linux/tcc_buffer.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>


#define CALL_METHOD(name, ...)                         \
    ({                                                 \
        if (self == NULL) {                            \
            TCC_LOG_WARNING("Region manager is NULL"); \
            goto error;                                \
        }                                              \
        self->vtable->name(self->self, ##__VA_ARGS__); \
    })

typedef struct region_manager_vtable_t
{
    region_manager_t* (*region_manager_create)(const tcc_config_t* config);
    void (*region_manager_destroy)(region_manager_t* self);
    ssize_t (*region_manager_count)(region_manager_t* self);
    const memory_properties_t* (*region_manager_get)(region_manager_t* self, size_t index);
    void* (*region_manager_mmap)(region_manager_t* self, size_t index, size_t size);
} region_manager_vtable_t;

static region_manager_vtable_t region_manager_default_vtable = {region_manager_default_create,
    region_manager_default_destroy,
    region_manager_default_count,
    region_manager_default_get,
    region_manager_default_mmap};
static region_manager_vtable_t region_manager_driver_vtable = {region_manager_driver_create,
    region_manager_driver_destroy,
    region_manager_driver_count,
    region_manager_driver_get,
    region_manager_driver_mmap};

struct region_manager_t
{
    region_manager_t* self;
    int tcc_buffer_exists;
    region_manager_vtable_t* vtable;
};

static int tcc_buffer_exist()
{
    TCC_TRACE_FUNC("void");
    struct stat buf;
    if (stat(TCC_BUFFER_NAME, &buf) < 0) {
        if (errno == ENOENT) {
            TCC_LOG_DEBUG("'%s' does not exist", TCC_BUFFER_NAME);
        } else {
            TCC_LOG_ERROR("stat:%s(%i)", strerror(errno), errno);
        }
        goto error;
    }
    TCC_LOG_DEBUG("'%s' exist", TCC_BUFFER_NAME);
    return 1;
error:
    return 0;
}

region_manager_t* region_manager_create(const tcc_config_t* config)
{
    TCC_TRACE_FUNC("config=%p", config);

    region_manager_t* self = calloc(1, sizeof(struct region_manager_t));
    if (self == NULL) {
        TCC_LOG_ERROR("Can't allocate memory for region manager:%s(%i)", strerror(errno), errno);
        goto error;
    }
    if (tcc_buffer_exist() > 0) {
        TCC_LOG_INFO("pSRAM allocation method: Real-time Configuration Driver");
        self->tcc_buffer_exists = 1;
        self->vtable = &region_manager_driver_vtable;
    } else {
        TCC_LOG_WARNING("Can't find pSRAM allocation method. Use DRAM memory");
        self->tcc_buffer_exists = 0;
        self->vtable = &region_manager_default_vtable;
    }
    TCC_LOG_TRACE("errno:%i", errno);
    self->self = self->vtable->region_manager_create(config);
    if (self->self == NULL) {
        TCC_LOG_ERROR("Can't initialize region manager!");
        goto error;
    }
    return self;
error:
    free(self);
    return NULL;
}

void region_manager_destroy(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    CALL_METHOD(region_manager_destroy);
    memset(self, 0, sizeof(region_manager_t));
    free(self);
error:
    return;
}
ssize_t region_manager_count(region_manager_t* self)
{
    return CALL_METHOD(region_manager_count);
error:
    return -1;
}
const memory_properties_t* region_manager_get(region_manager_t* self, size_t index)
{
    return CALL_METHOD(region_manager_get, index);
error:
    return NULL;
}
void* region_manager_mmap(region_manager_t* self, size_t index, size_t size)
{
    return CALL_METHOD(region_manager_mmap, index, size);
error:
    return NULL;
}

__attribute__((pure)) int is_tcc_buffer_exists(region_manager_t* self)
{
    return self->tcc_buffer_exists;
}