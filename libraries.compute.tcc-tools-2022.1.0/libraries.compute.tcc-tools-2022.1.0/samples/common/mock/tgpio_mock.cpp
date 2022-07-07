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

#include "tgpio_mock.hpp"

COMMON_MOCK_INTERFACE_IMPLEMENTATION(tgpio_interface)

int tgpio_pin_setfunc(int fd, uint32_t pin, ptp_func func, uint32_t channel)
{
	return tgpio_interface::get().tgpio_pin_setfunc(fd, pin, func, channel);
}

int tgpio_perout_request(int fd, uint32_t channel,  uint64_t period_ns, uint64_t start_ns, unsigned int flags)
{
	return tgpio_interface::get().tgpio_perout_request(fd, channel, period_ns, start_ns, flags);
}

int tgpio_extts_request(int fd, uint32_t channel, int enable)
{
	return tgpio_interface::get().tgpio_extts_request(fd, channel, enable);
}

int tgpio_get_event_count_tstamp(int fd, ptp_event_count_tstamp* event_count)
{
	return tgpio_interface::get().tgpio_get_event_count_tstamp(fd, event_count);
}

int tgpio_get_sys_offset_precise(int fd, ptp_sys_offset_precise* offset)
{
	return tgpio_interface::get().tgpio_get_sys_offset_precise(fd, offset);
}
