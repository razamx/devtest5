/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_SWGPIO_H_
#define _TCC_SWGPIO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HIGH_SIGNAL "1"
#define LOW_SIGNAL "0"
#define SIGNAL_VALUE_SIZE 2

typedef enum
{
    DIRECTION_INPUT,
    DIRECTION_OUTPUT
} Direction;

int gpio_export(uint32_t pin);
int gpio_unexport(uint32_t pin);
int gpio_set_direction(uint32_t pin, Direction direction);
int gpio_open(uint32_t pin);
int gpio_write(int gpio_fd, int value);
int gpio_read(int gpio_fd);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif  // _TCC_SWGPIO_H_
