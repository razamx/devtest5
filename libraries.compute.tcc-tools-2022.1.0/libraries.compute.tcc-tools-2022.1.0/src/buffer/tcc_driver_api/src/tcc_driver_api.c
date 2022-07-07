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
#include "tcc_driver_api.h"
#include "tcc_log.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/tcc_buffer.h>
#include <sched.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <tcc_time.h>
#include <unistd.h>

#define UNUSED(x) ((void)(x))
#ifndef STATIC
#define STATIC static
#endif

enum tcc_memory_level chache_level_map[] = {[RGN_UNKNOWN] = TCC_MEM_UNKNOWN,
    [RGN_L1] = TCC_MEM_L1,
    [RGN_L2] = TCC_MEM_L2,
    [RGN_L3] = TCC_MEM_L3,
    [RGN_EDRAM] = TCC_MEM_EDRAM,
    [RGN_MALLOC] = TCC_MEM_DRAM,
    TCC_MEM_UNKNOWN};

STATIC int tcc_buffer_fd = -1;

static int init_tcc_buffer_fd()
{
    TCC_TRACE_FUNC();

    if (tcc_buffer_fd != -1) {
        return tcc_buffer_fd;
    }
    tcc_buffer_fd = open(TCC_BUFFER_NAME, O_RDWR);
    if (tcc_buffer_fd == -1) {
        TCC_LOG_ERROR("Unable to open driver %s: %s(%i)", TCC_BUFFER_NAME, strerror(errno), errno);
        return -1;
    }
    return tcc_buffer_fd;
}

int tcc_driver_get_region_count()
{
    TCC_TRACE_FUNC();

    int buffer_count = 0;
    if (init_tcc_buffer_fd() < 0) {
        goto error;
    }
    if (ioctl(tcc_buffer_fd, TCC_GET_REGION_COUNT, &buffer_count) < 0) {
        TCC_LOG_ERROR("Unable to get region count, fd=%d: %s(%i)", tcc_buffer_fd, strerror(errno), errno);
        goto error;
    }
    return buffer_count;
error:
    return -1;
}

int tcc_driver_get_memory_config(unsigned int region_id, memory_properties_t* out)
{
    TCC_TRACE_FUNC("region_id=%u, out=%p", region_id, out);

    if (out == NULL) {
        TCC_LOG_ERROR("Parameter \"out\" is NULL");
        errno = EINVAL;
        goto error;
    }
    if (init_tcc_buffer_fd() < 0) {
        goto error;
    }
    struct tcc_buf_mem_config_s region_config;
    memset(&region_config, 0, sizeof(region_config));
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    region_config.cpu_mask_p = &cpu_set;
    region_config.id = region_id;
    if (ioctl(tcc_buffer_fd, TCC_GET_MEMORY_CONFIG, &region_config) < 0) {
        TCC_LOG_ERROR("Unable to get memory config, fd=%d, config(%p)={region_id=%u}: %s(%i)",
            tcc_buffer_fd,
            (const void*)out,
            region_id,
            strerror(errno),
            errno);
        goto error;
    }
    if (region_config.cpu_mask_p != &cpu_set) {
        TCC_LOG_ERROR(
            "Real-time Communication Driver set cpu_mask to %p, expected: %p", region_config.cpu_mask_p, &cpu_set);
        goto error;
    }
    TCC_LOG_DEBUG("Region config: id=%i, latancy=%u, size=%zu, type=%i, ways=%i",
        region_config.id,
        region_config.latency,
        region_config.size,
        region_config.type,
        region_config.ways);
    size_t pagesize = (size_t)getpagesize();
    out->id = region_config.id;
    out->latency = tcc_clk2ns(region_config.latency);
    out->latency_clk = region_config.latency;
    out->size_drv = region_config.size;
    out->size = (out->size_drv / pagesize) * pagesize;
    out->type = chache_level_map[region_config.type];
    out->mask = *(cpu_set_t*)region_config.cpu_mask_p;
    if (out->size_drv != out->size) {
        TCC_LOG_WARNING("Region size not aligned for id:%u. Align %zu to %zu", out->id, out->size_drv, out->size);
    }
    return 0;
error:
    return -1;
}

int tcc_driver_req_buffer(unsigned int region_id, unsigned int size)
{
    TCC_TRACE_FUNC("region_id%u size=%u", region_id, size);
    if (size == 0) {
        TCC_LOG_ERROR("size is zero");
        errno = EINVAL;
        goto error;
    }
    if (init_tcc_buffer_fd() < 0) {
        goto error;
    }
    struct tcc_buf_mem_req_s data;
    memset(&data, 0, sizeof(data));
    data.devnode = -1;
    data.id = region_id;
    data.size = size;
    if (ioctl(tcc_buffer_fd, TCC_REQ_BUFFER, &data) < 0) {
        TCC_LOG_ERROR("Unable to request buffer, fd=%d, data={region_id=%u, size=%zu}: %s(%i)",
            tcc_buffer_fd,
            data.id,
            data.size,
            strerror(errno),
            errno);
        goto error;
    }
    TCC_LOG_DEBUG("Allocated %zu bytes (requested %u) for region_id: %u (requested %u), dev_node=%u",
        data.size,
        size,
        data.id,
        region_id,
        data.devnode);
    return (int)data.devnode;
error:
    return -1;
}

int tcc_driver_read_rtct(void** rtct, size_t* size)
{
    TCC_TRACE_FUNC("rtct=%p size=%p", rtct, size);

    if (rtct == NULL) {
        TCC_LOG_ERROR("Parameter \"rtct\" is NULL");
        errno = EINVAL;
        return -1;
    }
    if (size == NULL) {
        TCC_LOG_ERROR("Parameter \"size\" is NULL");
        errno = EINVAL;
        return -1;
    }
    *rtct = NULL;
    *size = 0;
    if (init_tcc_buffer_fd() < 0) {
        goto error;
    }
    if (ioctl(tcc_buffer_fd, TCC_QUERY_RTCT_SIZE, size) < 0) {
        TCC_LOG_ERROR("Unable to read RTCT size from driver(fd=%i): %s(%i)", tcc_buffer_fd, strerror(errno), errno);
        goto error;
    }
    TCC_LOG_DEBUG("The RTCT size is %zu", *size);
    *rtct = calloc(1, *size);
    if (*rtct == NULL) {
        TCC_LOG_ERROR("Can't allocate memory for RTCT table:%s(%i)", strerror(errno), errno);
        goto error;
    }
    if (ioctl(tcc_buffer_fd, TCC_GET_RTCT, (unsigned int*)*rtct) < 0) {
        TCC_LOG_ERROR(
            "Unable to read RTCT from driver(fd=%i, size=%zu): %s(%i)", tcc_buffer_fd, *size, strerror(errno), errno);
        goto error;
    }
    return 0;
error:
    free(*rtct);
    *rtct = NULL;
    return -1;
}
