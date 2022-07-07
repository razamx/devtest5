/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "tcc_msr.h"
#include "tcc_log.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


static const char* get_msr_file_name(uint8_t cpu)
{
    static char msr_file_name[] = "/dev/cpu/-xxx/msr";
    snprintf(msr_file_name, sizeof(msr_file_name), "/dev/cpu/%d/msr", cpu);
    return msr_file_name;
}

static int check_params_and_open_descriptor(__attribute__((unused)) uint32_t reg,
    uint8_t cpu,
    const uint64_t* data,
    int flags)
{
    TCC_TRACE_FUNC();
    int fd = -1;

    if (data == NULL) {
        TCC_LOG_WARNING("Data is NULL");
        errno = EINVAL;
        goto error;
    }
    const char* msr_file_name = get_msr_file_name(cpu);
    fd = open(msr_file_name, flags);
    if (fd == -1) {
        TCC_LOG_ERROR("Unable to open file %s: %s(%d)", msr_file_name, strerror(errno), errno);
        goto error;
    }
    return fd;
error:
    close(fd);
    return -1;
}

int rdmsr(uint32_t reg, uint8_t cpu, uint64_t* data)
{
    TCC_TRACE_FUNC("reg:0x%x cpu:%i data:%p", reg, cpu, data);
    int ret = -1;
    int fd = check_params_and_open_descriptor(reg, cpu, data, O_RDONLY);
    if (fd == -1) {
        TCC_LOG_WARNING("Can't get msr descriptor");
        goto out;
    }
    if (pread(fd, (void*)data, sizeof(*data), reg) != sizeof(*data)) {
        TCC_LOG_ERROR("Unable to read file %s: %s(%d)", get_msr_file_name(cpu), strerror(errno), errno);
        goto out;
    }
    ret = 0;
out:
    close(fd);
    return ret;
}

int wrmsr(uint32_t reg, uint8_t cpu, const uint64_t* data)
{
    TCC_TRACE_FUNC("reg:0x%x cpu:%i %p(%" PRIu64 ")", reg, cpu, data, data ? *data : 0);
    int ret = -1;
    int fd = check_params_and_open_descriptor(reg, cpu, data, O_WRONLY);
    if (fd == -1) {
        TCC_LOG_WARNING("Can't get msr descriptor");
        goto out;
    }
    TCC_LOG_INFO("Write value 0x%lx to MSR 0x%x for cpu %i", *data, reg, cpu);
    if (pwrite(fd, (const void*)data, sizeof(*data), reg) != sizeof(*data)) {
        TCC_LOG_ERROR("Unable to read file %s: %s(%d)", get_msr_file_name(cpu), strerror(errno), errno);
        goto out;
    }
    ret = 0;
out:
    close(fd);
    return ret;
}
