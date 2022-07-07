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

#include <gtest/gtest.h>
#include <iostream>
#include <cstdarg>
#include <string>
#include <linux/ptp_clock.h>
#include <vector>
#include <sys/utsname.h>

#include "libc_mock.hpp"
#include "alloc_mock.hpp"
#include "tcc/err_code.h"
#include "tgpio.h"
#include "gtest_helpers.hpp"
#include "time_helpers.hpp"

///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;



//hack to compensate fprintf() calls in implementation
#define EXPECT_STDERR() EXPECT_CALL(m_libc_mock, fprintf(stderr, _, _)).Times(AnyNumber())

namespace
{
using namespace testing;

class test_tcc_gpio_sample_helpers_host : public Test
{
protected:
    StrictMock<libc_mock> m_libc_mock;
    NiceMock<alloc_mock> m_alloc_mock;
    std::string gpio_export_path = "/sys/class/gpio/export";
    std::string gpio_unexport_path = "/sys/class/gpio/unexport";

    void SetUp() override
    {
        EXPECT_STDERR();
    }
};

/////////////////////////////////////////////////
//         tgpio_pin_setfunc tests             //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_pin_setfunc_bad_func_negative)
{
    int fake_fd = 123;
    EXPECT_EQ(-TCC_E_BAD_PARAM, tgpio_pin_setfunc(fake_fd, 0, static_cast<ptp_func>(3), 0));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_pin_setfunc_ioctl_fails_negative)
{
    int fake_fd = 123;
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _)).WillOnce(Return(-1));
    EXPECT_EQ(-TCC_E_ERROR, tgpio_pin_setfunc(fake_fd, 0, PTP_FUNC_IN, 0));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_pin_setfunc_positive)
{
    int fake_fd = 123;
    ptp_pin_desc desc = {{}, 1, PTP_FUNC_OUT, 3};  // name, pin, func, channel

    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PIN_SETFUNC2, _)).WillOnce(EXPECT_PTP_PIN_DESC(desc));

    EXPECT_EQ(TCC_E_SUCCESS, tgpio_pin_setfunc(fake_fd, 1, PTP_FUNC_OUT, 3));
}

/////////////////////////////////////////////////
//       tgpio_perout_request tests            //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_perout_request_ioctl_fails_negative)
{
    int fake_fd = 123;
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _)).WillOnce(Return(-1));
    EXPECT_EQ(-TCC_E_ERROR, tgpio_perout_request(fake_fd, 1, ts2ns(1, 0), ts2ns(2, 0), 0));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_perout_request_positive)
{
    int fake_fd = 123;
    ptp_perout_request request = {{2, 2000}, {1, 1000}, 1};  // start (s, ns), period (s, ns), channel

    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _)).WillOnce(EXPECT_PTP_PEROUT_REQUEST(request));

    EXPECT_EQ(TCC_E_SUCCESS, tgpio_perout_request(fake_fd, 1, ts2ns(1, 1000), ts2ns(2, 2000), 0));
}

/////////////////////////////////////////////////
//        tgpio_extts_request tests            //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_extts_request_ioctl_fails_negative)
{
    int fake_fd = 123;
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EXTTS_REQUEST2, _)).WillOnce(Return(-1));
    EXPECT_EQ(-TCC_E_ERROR, tgpio_extts_request(fake_fd, 1, 1));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_extts_request_enable_0_positive)
{
    int fake_fd = 123;
    ptp_extts_request request = {1, 0};  // channel, flags
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EXTTS_REQUEST2, _)).WillOnce(EXPECT_PTP_EXTTS_REQUEST(request));

    EXPECT_EQ(TCC_E_SUCCESS, tgpio_extts_request(fake_fd, 1, 0));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_extts_request_enable_1_positive)
{
    int fake_fd = 123;
    ptp_extts_request request = {1, PTP_ENABLE_FEATURE};  // channel, flags
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EXTTS_REQUEST2, _)).WillOnce(EXPECT_PTP_EXTTS_REQUEST(request));

    EXPECT_EQ(TCC_E_SUCCESS, tgpio_extts_request(fake_fd, 1, 1));
}

/////////////////////////////////////////////////
//    tgpio_get_event_count_tstamp tests       //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_get_event_count_tstamp_ioctl_negative)
{
    int fake_fd = 123;
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EVENT_COUNT_TSTAMP2, _)).WillOnce(Return(-1));
    ptp_event_count_tstamp event = {0};
    EXPECT_EQ(-TCC_E_ERROR, tgpio_get_event_count_tstamp(fake_fd, &event));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_get_event_count_tstamp_positive)
{
    int fake_fd = 123;
    ptp_event_count_tstamp expected = {
        0,
    };
    expected.device_time = {123, 456};
    expected.event_count = 789;

    ptp_event_count_tstamp event = {0};
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_EVENT_COUNT_TSTAMP2, _)).WillOnce(GET_PTP_EVENT_COUNT_TSTAMP(expected));

    EXPECT_EQ(TCC_E_SUCCESS, tgpio_get_event_count_tstamp(fake_fd, &event));
    EXPECT_EQ(expected.index, event.index);
    EXPECT_TRUE(expected.device_time == event.device_time);
    EXPECT_TRUE(expected.event_count == event.event_count);
}

/////////////////////////////////////////////////
//    tgpio_get_event_count_tstamp tests       //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_get_sys_offset_precise_ioctl_negative)
{
    int fake_fd = 123;
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _)).WillOnce(Return(-1));
    ptp_sys_offset_precise offset;
    EXPECT_EQ(-TCC_E_ERROR, tgpio_get_sys_offset_precise(fake_fd, &offset));
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_get_sys_offset_precise_positive)
{
    int fake_fd = 123;
    ptp_clock_time device_expected = {12, 34};
    ptp_clock_time realtime_expected = {56, 78};
    ptp_sys_offset_precise offset;
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_SYS_OFFSET_PRECISE2, _))
        .WillOnce(Invoke([&](int, unsigned int, va_list args) {
            ptp_sys_offset_precise* offset = va_arg(args, ptp_sys_offset_precise*);
            offset->device.sec = device_expected.sec;
            offset->device.nsec = device_expected.nsec;
            offset->sys_realtime.sec = realtime_expected.sec;
            offset->sys_realtime.nsec = realtime_expected.nsec;
            return 0;
        }));

    EXPECT_EQ(TCC_E_SUCCESS, tgpio_get_sys_offset_precise(fake_fd, &offset));
    EXPECT_EQ(device_expected, offset.device);
    EXPECT_EQ(realtime_expected, offset.sys_realtime);
}

/////////////////////////////////////////////////
//    tgpio_GET_PTP_PEROUT_FREQ_ADJ tests      //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_negative_uname)
{
    EXPECT_CALL(m_libc_mock, uname(_)).WillOnce(Return(-1));
    EXPECT_EQ(2, GET_PTP_PEROUT_FREQ_ADJ());
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_negative_alloc_kv)
{
    EXPECT_CALL(m_alloc_mock, calloc(_, _)).WillOnce(ReturnNull());
    EXPECT_EQ(2, GET_PTP_PEROUT_FREQ_ADJ());
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_negative_strrchr_null)
{
    EXPECT_CALL(m_libc_mock, strrchr(_, _)).WillOnce(ReturnNull());
    EXPECT_EQ(2, GET_PTP_PEROUT_FREQ_ADJ());
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_wrong_length)
{
    std::string bad_kernel("5.100500.bad_kernel");
    EXPECT_CALL(m_libc_mock, uname(_)).WillOnce(Invoke([&](struct utsname* buf) {
        strcpy(buf->release, bad_kernel.c_str());
        return 0;
    }));
    EXPECT_EQ(2, GET_PTP_PEROUT_FREQ_ADJ());
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_kernel_5_4)
{
    std::string good_kernel_5_4("5.4.good_kernel");
    EXPECT_CALL(m_libc_mock, uname(_)).WillOnce(Invoke([&](struct utsname* buf) {
        strcpy(buf->release, good_kernel_5_4.c_str());
        return 0;
    }));
    EXPECT_EQ(2, GET_PTP_PEROUT_FREQ_ADJ());
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_kernel_5_10)
{
    std::string good_kernel_5_10("5.10.good_kernel");
    EXPECT_CALL(m_libc_mock, uname(_)).WillOnce(Invoke([&](struct utsname* buf) {
        strcpy(buf->release, good_kernel_5_10.c_str());
        return 0;
    }));
    EXPECT_EQ(8, GET_PTP_PEROUT_FREQ_ADJ());
}

TEST_F(test_tcc_gpio_sample_helpers_host, tgpio_GET_PTP_PEROUT_FREQ_ADJ_kernel_5_20)
{
    std::string bad_kernel_5_5("5.5.bad_kernel");
    EXPECT_CALL(m_libc_mock, uname(_)).WillOnce(Invoke([&](struct utsname* buf) {
        strcpy(buf->release, bad_kernel_5_5.c_str());
        return 0;
    }));
    EXPECT_EQ(8, GET_PTP_PEROUT_FREQ_ADJ());
}

}  // namespace
