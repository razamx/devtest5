/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "swgpio_mock.hpp"

COMMON_MOCK_INTERFACE_IMPLEMENTATION(swgpio_interface)

int gpio_export(uint32_t pin)
{
	return swgpio_interface::get().gpio_export(pin);
}

int gpio_unexport(uint32_t pin)
{
	return swgpio_interface::get().gpio_unexport(pin);
}

int gpio_set_direction(uint32_t pin, Direction direction)
{
	return swgpio_interface::get().gpio_set_direction(pin, direction);
}

int gpio_open(uint32_t pin)
{
	return swgpio_interface::get().gpio_open(pin);
}

int gpio_write(int gpio_fd, int value)
{
	return swgpio_interface::get().gpio_write(gpio_fd, value);
}

int gpio_read(int gpio_fd)
{
	return swgpio_interface::get().gpio_read(gpio_fd);
}
