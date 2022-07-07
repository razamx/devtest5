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
#include "args_helper.hpp"
#include "libc_mock.hpp"
#include "args_parse.h"
#include "tcc/err_code.h"

#define main sample_main
#include "tgpio_basic_oneshot_output.c"
#undef main

namespace
{
using namespace testing;

class test_tcc_tgpio_basic_oneshot_output : public Test
{
protected:
    StrictMock<libc_mock> m_libc_mock;
    settings_t sample_settings;

    void SetUp() final
    {
        optind = 0;
        sample_settings = {
            .pin = 0,
            .channel = 0,
            .device = TGPIO_DEVICE,
            .start = TCC_NSEC_PER_SEC
        };
    }
};

////////////////////////////////////////////////////////////////////////////////
// COMMAND LINE ARGUMENTS PARSING TESTS
////////////////////////////////////////////////////////////////////////////////
TEST_F(test_tcc_tgpio_basic_oneshot_output, parse_help_positive) {
    ::Args args {"--help"};
    EXPECT_EQ(TCC_PRINT_HELP, parse_args(args.count(), args, &sample_settings));
    EXPECT_EQ(sample_settings.pin, 0);
    EXPECT_EQ(sample_settings.channel, 0);
    EXPECT_EQ(sample_settings.device, TGPIO_DEVICE);
    EXPECT_EQ(sample_settings.start, TCC_NSEC_PER_SEC);
}

////////////////////////////////////////////////////////////////////////////////
// ERROR HANDLING TESTS
////////////////////////////////////////////////////////////////////////////////
TEST_F(test_tcc_tgpio_basic_oneshot_output, wrong_device_open_negative) {
    ::Args args {"--device", "/nonexistent/device"};
    EXPECT_CALL(m_libc_mock, open(StrEq("/nonexistent/device"), O_RDWR, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(m_libc_mock, fprintf(_, StrEq("Unable to open device %s: %s\n"), _))
        .WillOnce(Return(0));
    EXPECT_EQ(-TCC_E_ERROR, sample_main(args.count(), args));
}

TEST_F(test_tcc_tgpio_basic_oneshot_output, signal_generation_ioctl_negative) {
    int fake_fd = 123;
    ::Args args {"--device", "/wrong/device"};
    EXPECT_CALL(m_libc_mock, open(StrEq("/wrong/device"), O_RDWR, _))
        .WillOnce(Return(fake_fd));
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, _, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, PTP_PEROUT_REQUEST2, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(m_libc_mock, fprintf(_, StrEq("Failed to start signal generation: %s\n"), _))
        .WillOnce(Return(0));
    EXPECT_CALL(m_libc_mock, close(fake_fd))
        .WillOnce(Return(0));
    EXPECT_EQ(-TCC_E_ERROR, sample_main(args.count(), args));
}

TEST_F (test_tcc_tgpio_basic_oneshot_output, defaults_positive) {
    int fake_fd = 123;
    ::Args args {};
    EXPECT_CALL(m_libc_mock, open(StrEq(TGPIO_DEVICE), O_RDWR, _))
        .WillOnce(Return(fake_fd));
    EXPECT_CALL(m_libc_mock, ioctl(fake_fd, _, _))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(m_libc_mock, close(fake_fd))
        .WillOnce(Return(0));
    EXPECT_EQ(TCC_E_SUCCESS, sample_main(args.count(), args));
}

}  // namespace
