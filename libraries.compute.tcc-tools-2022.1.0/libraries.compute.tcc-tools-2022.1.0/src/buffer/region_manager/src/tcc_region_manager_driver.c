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
#include "errno.h"
#include "linux/tcc_buffer.h"
#include "memory_properties.h"
#include "rtct.h"
#include "tcc_driver_api.h"
#include "tcc_log.h"
#include "tcc_region_manager_internal.h"
#include "tcc_time.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#define UNUSED(x) ((void)(x))
typedef int (*qsort_comparator_t)(const void*, const void*);

#define MAX_INT_DECIMAL_DIGITS 10


typedef struct tcc_driver_region
{
    memory_properties_t prop;
} tcc_driver_region_t;


struct region_manager_t
{
    size_t count;
    tcc_driver_region_t* regions;
};

static int is_region_manager_created = 0;


static void tcc_driver_region_init(tcc_driver_region_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);
    memset(self, 0, sizeof(tcc_driver_region_t));
}

static int driver_regions_read(tcc_driver_region_t* regions, size_t count)
{
    TCC_TRACE_FUNC("count=%lu", count);
    for (size_t i = 0; i < count; i++) {
        tcc_driver_region_init(&(regions[i]));
        if (tcc_driver_get_memory_config(i, &(regions[i].prop)) < 0) {
            TCC_LOG_ERROR("Unable to get region with id %zu: %s(%i)", i, strerror(errno), errno);
            goto error;
        }
        TCC_LOG_DEBUG("read region with id %u", regions[i].prop.id);
    }
    return 0;
error:
    return -1;
}

static int64_t get_dram_latency_clk()
{
    TCC_TRACE_FUNC("void");

    void* rtct;
    size_t size;
    if (tcc_driver_read_rtct(&rtct, &size) < 0) {
        TCC_LOG_ERROR("Can't read rtct table");
        goto error;
    }
    int64_t clocks = rtct_get_dram_latency_clk(rtct, size);
    if (clocks < 0) {
        TCC_LOG_WARNING("Can't get dram latency from TCC buffer. Use hardcoded dran latency: %i cpu clocks",
            HARDCODED_DRAM_LATENCY_CLK);
        clocks = (int64_t)HARDCODED_DRAM_LATENCY_CLK;
    }
    free(rtct);
    return clocks;
error:
    return -1;
}

static int dram_region_init(tcc_driver_region_t* region)
{
    TCC_TRACE_FUNC("region=%p", region);

    tcc_driver_region_init(region);
    region->prop.type = TCC_MEM_DRAM;
    fill_cpu_mask(&(region->prop.mask));
    region->prop.id = (unsigned int)-1;
    region->prop.latency_clk = get_dram_latency_clk();
    region->prop.latency = tcc_clk2ns(region->prop.latency_clk);
    region->prop.size = (size_t)-1;
    return 0;
}

static int tcc_memory_region_compare(struct tcc_driver_region* first, struct tcc_driver_region* second)
{
    TCC_TRACE_FUNC("first=%p second=%p", first, second);
    return memory_properties_compare(&(first->prop), &(second->prop));
}

static void region_manager_driver_sort_regions(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);
    qsort(self->regions, self->count, sizeof(self->regions[0]), (qsort_comparator_t)tcc_memory_region_compare);
}

static void print_regions(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    for (size_t i = 0; i < self->count; i++) {
        TCC_LOG_DEBUG("region#%zu %s", i, memory_properties_to_str(&(self->regions[i].prop)));
    }
}

region_manager_t* region_manager_driver_create(const tcc_config_t* config)
{
    TCC_TRACE_FUNC("config=%p", config);

    region_manager_t* self = NULL;
    int region_count = -1;
    if (is_region_manager_created) {
        TCC_LOG_ERROR("Region manager is already created");
        errno = EBUSY;
        goto error;
    }
    self = (region_manager_t*)calloc(1, sizeof(region_manager_t));
    if (self == NULL) {
        TCC_LOG_ERROR("can't allocate memory for region manager");
        errno = EINVAL;
        goto error;
    }
    region_count = tcc_driver_get_region_count();
    if (region_count < 0) {
        TCC_LOG_ERROR("tcc_driver_get_region_count failure");
        goto error;
    }
    self->count = region_count + 1;
    self->regions = (tcc_driver_region_t*)calloc(self->count, sizeof(tcc_driver_region_t));
    if (self->regions == NULL) {
        TCC_LOG_ERROR("Can't allocate memory for regions: %s(%d)", strerror(errno), errno);
        goto error;
    }
    // Read all regions from driver and create one fake DRAM region. It is used only to understand the
    // latency
    // for DRAM
    if (region_count) {
        if (driver_regions_read(self->regions, region_count) < 0) {
            TCC_LOG_ERROR("Can't read regions from driver");
            goto error;
        }
    }
    if (dram_region_init(&(self->regions[self->count - 1])) < 0) {
        TCC_LOG_ERROR("Can't allocate fake region for DRAM latency");
        goto error;
    }
    region_manager_driver_sort_regions(self);
    is_region_manager_created = 1;
    TCC_LOG_DEBUG("Region manager created successfully:");
    print_regions(self);
    return self;
error:
    if (self) {
        free(self->regions);
    }
    free(self);
    return NULL;
}

void region_manager_driver_destroy(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    if (self == NULL) {
        TCC_LOG_WARNING("Parameter \"self\" is NULL");
        errno = EINVAL;
        goto error;
    }
    free(self->regions);
    free(self);
    is_region_manager_created = 0;
error:
    return;
}

ssize_t region_manager_driver_count(region_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    if (self == NULL) {
        TCC_LOG_WARNING("Parameter \"self\" is NULL");
        errno = EINVAL;
        goto error;
    }
    return self->count;
error:
    return -1;
}

const memory_properties_t* region_manager_driver_get(region_manager_t* self, size_t index)
{
    TCC_TRACE_FUNC("self=%p", self);

    if (self == NULL) {
        TCC_LOG_WARNING("Parameter \"self\" is NULL");
        errno = EINVAL;
        goto error;
    }
    if (index >= self->count) {
        TCC_LOG_WARNING("index out of range %zu(max %zu)", index, self->count);
        errno = EINVAL;
        goto error;
    }
    return &(self->regions[index].prop);
error:
    return NULL;
}

static int region_manager_driver_get_psram_fd(region_manager_t* self, size_t index, size_t size)
{
    int nodeid = tcc_driver_req_buffer(self->regions[index].prop.id, size);
    if (nodeid < 0) {
        TCC_LOG_ERROR("Failure for tcc_driver_req_buffer(index:%zu, size:%zu)", index, size);
        return -1;
    }

    size_t buffer_file_name_size = snprintf(NULL, 0, "%s%i", TCC_BUFFER_NAME, nodeid);
    char buffer_file_name[buffer_file_name_size + 1];
    sprintf(buffer_file_name, "%s%i", TCC_BUFFER_NAME, nodeid);

    int buffer_fd = open(buffer_file_name, O_RDWR);
    if (buffer_fd == -1) {
        TCC_LOG_ERROR("Failure open('%s', O_RDRW):error: %s(%i)", buffer_file_name, strerror(errno), errno);
        if (unlink(buffer_file_name) != 0) {
            TCC_LOG_WARNING("Failure unlink('%s%i'):error: %s(%i)", TCC_BUFFER_NAME, nodeid, strerror(errno), errno);
            TCC_LOG_FATAL("tcc_buffer leak detected. To avoid please run: 'rm %s'", buffer_file_name);
        }
        return -1;
    }
    return buffer_fd;
}

void* region_manager_driver_mmap(region_manager_t* self, size_t index, size_t size)
{
    TCC_TRACE_FUNC("self=%p index=%zu size=%zu", self, index, size);

    int buffer_fd = -1;
    void* memory = NULL;
    if (self == NULL) {
        TCC_LOG_WARNING("Parameter \"self\" is NULL");
        errno = EINVAL;
        goto end;
    }
    if (size == 0) {
        TCC_LOG_WARNING("Parameter \"size\" is zero");
        errno = EINVAL;
        goto end;
    }
    if (index >= self->count) {
        TCC_LOG_WARNING("index out of range %zu(max %zu)", index, self->count);
        errno = EINVAL;
        goto end;
    }
    if (self->regions[index].prop.type == TCC_MEM_DRAM) {
        TCC_LOG_DEBUG("Allocate memory from DRAM");
        buffer_fd = region_manager_get_dram_region_fd(size);
    } else {
        TCC_LOG_DEBUG("Allocate memory from PSRAM");
        buffer_fd = region_manager_driver_get_psram_fd(self, index, size);
    }

    if (buffer_fd < 0) {
        TCC_LOG_ERROR("Can't get descriptor for mmap");
        goto end;
    }
    memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, buffer_fd, 0);
    if (memory == MAP_FAILED) {
        TCC_LOG_ERROR("Failure mmap(NULL, %zu, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, %i, 0):error %s(%i)",
            size,
            buffer_fd,
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
