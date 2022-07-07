/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include <fcntl.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "macros.h"
#include "swgpio.h"

#define STR_BUF_LENGTH 255


int gpio_export(uint32_t pin)
{
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        return -TCC_E_NOT_AVAILABLE;
    }
    int err = dprintf(fd, "%d", pin);
    close(fd);
    if (err < 0) {
        pr_err("Failed to export pin %d\n", pin);
        return -TCC_E_BAD_PARAM;
    }
    return TCC_E_SUCCESS;
}

int gpio_unexport(uint32_t pin)
{
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        return -TCC_E_NOT_AVAILABLE;
    }
    int err = dprintf(fd, "%d", pin);
    close(fd);
    if (err < 0) {
        pr_err("Failed to unexport pin %d\n", pin);
        return -TCC_E_BAD_PARAM;
    }
    return TCC_E_SUCCESS;
}

int gpio_set_direction(uint32_t pin, Direction direction)
{
    char fname[PATH_MAX];
    int err = 0;
    int ret = TCC_E_SUCCESS;
    sprintf(fname, "/sys/class/gpio/gpio%d/direction", pin);
    int fd = open(fname, O_WRONLY);
    if (fd == -1) {
        return -TCC_E_NOT_AVAILABLE;
    }

    if (direction == DIRECTION_INPUT) {
        err = dprintf(fd, "in");
    } else if (direction == DIRECTION_OUTPUT) {
        err = dprintf(fd, "out");
    } else {
        pr_err("Invalid direction: %d\n", direction);
        ret = -TCC_E_BAD_PARAM;
    }

    if (err < 0) {
        pr_err("Failed to setup gpio%d direction: %s\n", pin, strerror(errno));
        ret = -TCC_E_ERROR;
    }

    close(fd);
    return ret;
}

int gpio_open(uint32_t pin)
{
    char fname[PATH_MAX];
    sprintf(fname, "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(fname, O_RDWR | O_SYNC);
    if (fd < 0) {
        pr_err("Failed to open gpio%d file descriptor: %s\n", pin, strerror(errno));
    }
    return fd;
}

int gpio_write(int gpio_fd, int value)
{
    const char* str = value == 0 ? LOW_SIGNAL : HIGH_SIGNAL;

    if (write(gpio_fd, (const void*)str, SIGNAL_VALUE_SIZE) < 0) {
        pr_err("Failed to write to pin\n");
        return -TCC_E_BAD_PARAM;
    }
    return TCC_E_SUCCESS;
}

int gpio_read(int gpio_fd)
{
    char buf = '0';

    if (lseek(gpio_fd, 0, SEEK_SET) < 0) {
        pr_err("Failed lseek to read from pin\n");
        return -TCC_E_BAD_PARAM;
    }

    if (read(gpio_fd, (void*)&buf, 1) < 0) {
        pr_err("Failed to read from pin\n");
        return -TCC_E_BAD_PARAM;
    }

    return buf - '0';
}
