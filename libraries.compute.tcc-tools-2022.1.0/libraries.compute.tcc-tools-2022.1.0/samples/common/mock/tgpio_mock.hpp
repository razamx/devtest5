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

#ifndef TGPIO_MOCK_H
#define TGPIO_MOCK_H

#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "tgpio.h"
#include "tcc/err_code.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

struct tgpio_interface {
	virtual int tgpio_pin_setfunc(int fd, uint32_t pin, ptp_func func, uint32_t channel) = 0;
	virtual int tgpio_perout_request(int fd, uint32_t channel,  uint64_t period_ns, uint64_t start_ns, unsigned int flags) = 0;
	virtual int tgpio_extts_request(int fd, uint32_t channel, int enable) = 0;
	virtual int tgpio_get_event_count_tstamp(int fd, ptp_event_count_tstamp* event_count) = 0;
	virtual int tgpio_get_sys_offset_precise(int fd, ptp_sys_offset_precise* offset) = 0;

	COMMON_MOCK_INTERFACE_DEFINITIONS(tgpio_interface);
};

struct tgpio_mock: public tgpio_interface {
	MOCK_METHOD4(tgpio_pin_setfunc, int(int fd, uint32_t pin, ptp_func func, uint32_t channel));
	MOCK_METHOD5(tgpio_perout_request, int(int fd, uint32_t channel,  uint64_t period_ns, uint64_t start_ns, unsigned int flags));
	MOCK_METHOD3(tgpio_extts_request, int(int fd, uint32_t channel, int enable));
	MOCK_METHOD2(tgpio_get_event_count_tstamp, int(int fd, ptp_event_count_tstamp* event_count));
	MOCK_METHOD2(tgpio_get_sys_offset_precise, int(int fd, ptp_sys_offset_precise* offset));

	tgpio_mock() {
		ON_CALL(*this, tgpio_pin_setfunc(_, _, _, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, tgpio_perout_request(_, _, _, _, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, tgpio_extts_request(_, _, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, tgpio_get_event_count_tstamp(_, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
		ON_CALL(*this, tgpio_get_sys_offset_precise(_, _))
			.WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
	}
};

#endif
