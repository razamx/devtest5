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

#ifndef SWGPIO_MOCK_H
#define SWGPIO_MOCK_H

#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "swgpio.h"
#include "tcc/err_code.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

struct swgpio_interface {
	virtual int gpio_export(uint32_t pin) = 0;
	virtual int gpio_unexport(uint32_t pin) = 0;
	virtual int gpio_set_direction(uint32_t pin, Direction direction) = 0;
	virtual int gpio_open(uint32_t pin) = 0;
	virtual int gpio_write(int gpio_fd, int value) = 0;
	virtual int gpio_read(int gpio_fd) = 0;
	COMMON_MOCK_INTERFACE_DEFINITIONS(swgpio_interface);
};

struct swgpio_mock: public swgpio_interface {
	MOCK_METHOD1(gpio_export, int(uint32_t pin));
	MOCK_METHOD1(gpio_unexport, int(uint32_t pin));
	MOCK_METHOD2(gpio_set_direction, int(uint32_t pin, Direction direction));
	MOCK_METHOD1(gpio_open, int(uint32_t pin));
	MOCK_METHOD2(gpio_write, int(int gpio_fd, int value));
	MOCK_METHOD1(gpio_read, int(int gpio_fd));

	swgpio_mock() {
		ON_CALL(*this, gpio_export(_))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, gpio_unexport(_))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, gpio_set_direction(_, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, gpio_open(_))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, gpio_write(_, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, gpio_read(_))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
	}
};


#endif
