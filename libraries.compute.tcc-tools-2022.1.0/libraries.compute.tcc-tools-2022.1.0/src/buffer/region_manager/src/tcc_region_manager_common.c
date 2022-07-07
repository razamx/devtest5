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
#include "tcc_region_manager_internal.h"
#include "memory_properties.h"
#include "tcc_log.h"

#include <sched.h>
#include <stddef.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

void fill_cpu_mask(cpu_set_t* cpuset)
{
    CPU_ZERO(cpuset);
    for (int i = 0; i < get_nprocs(); i++) {
        CPU_SET(i, cpuset);
    }
}

int region_manager_get_dram_region_fd(size_t size)
{
    TCC_TRACE_FUNC();
    int fd = -1;
    fd = memfd_create("cache_dram_region", 0);
    if (fd == -1) {
        TCC_LOG_ERROR("Cant create memory file descriptor, %s(%i)", strerror(errno), errno);
        goto error;
    };
    if (ftruncate(fd, size) < 0) {
        TCC_LOG_ERROR("Can't populate memory for size %zu, %s(%i)", size, strerror(errno), errno);
        goto error;
    }
    return fd;
error:
    if (fd >= 0) {
        close(fd);
    }
    return -1;
}