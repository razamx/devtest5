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

#include "libc_mock.hpp"
#include "alloc_mock.hpp"
#include "tcc/err_code.h"
#include "swgpio.h"
#include "gtest_helpers.hpp"
#include "time_helpers.hpp"

///////////////////////////////////////////////////////////////////////////////

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;



//hack to compensate fprintf() calls in implementation
#define EXPECT_STDERR()EXPECT_CALL(m_libc_mock, fprintf(stderr,_,_)).Times(AnyNumber())

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
//               gpio_export tests             //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_export_can_not_open_export_P1_negative)
{
	EXPECT_CALL(m_libc_mock,open(_,_,_)).WillOnce(Return(-1));
	ASSERT_EQ(-TCC_E_NOT_AVAILABLE, gpio_export(10));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_export_can_not_write_to_export_P1_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock,open(_,_,_)).WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock,dprintf(fake_fd,_,_)).WillOnce(Return(-1));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(-TCC_E_BAD_PARAM, gpio_export(10));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_export_write_pin_to_export_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock,open(StrEq(gpio_export_path),_,_)).WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, dprintf(fake_fd,_,_)).WillOnce(dprintfStrEq("10"));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(TCC_E_SUCCESS, gpio_export(10));
}

/////////////////////////////////////////////////
//              gpio_unexport tests            //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_unexport_can_not_open_unexport_P1_negative)
{
	EXPECT_CALL(m_libc_mock,open(_,_,_)).WillOnce(Return(-1));
	ASSERT_EQ(-TCC_E_NOT_AVAILABLE, gpio_unexport(10));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_unexport_can_not_write_to_export_P1_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock,open(_,_,_)).WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, dprintf(fake_fd, _, _)).WillOnce(Return(-1));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(-TCC_E_BAD_PARAM, gpio_unexport(10));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_unexport_write_pin_to_unexport_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock,open(StrEq(gpio_unexport_path), _, _)).WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, dprintf(fake_fd, _, _)).WillOnce(dprintfStrEq("10"));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(TCC_E_SUCCESS, gpio_unexport(10));
}

/////////////////////////////////////////////////
//          gpio_set_direction tests           //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_set_direction_can_not_open_P1_negative)
{
	EXPECT_CALL(m_libc_mock,open(_,_,_)).WillOnce(Return(-1));
	ASSERT_EQ(-TCC_E_NOT_AVAILABLE, gpio_set_direction(10, DIRECTION_INPUT));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_set_direction_can_not_write_P1_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(_,_,_)).WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, dprintf(fake_fd,_,_)).WillOnce(Return(-1));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(-TCC_E_ERROR, gpio_set_direction(10, DIRECTION_INPUT));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_set_direction_in_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq("/sys/class/gpio/gpio10/direction"),_,_))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, dprintf(fake_fd,_,_)).WillOnce(dprintfStrEq("in"));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(TCC_E_SUCCESS, gpio_set_direction(10, DIRECTION_INPUT));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_set_direction_out_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq("/sys/class/gpio/gpio10/direction"),_,_))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock, dprintf(fake_fd,_,_)).WillOnce(dprintfStrEq("out"));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	ASSERT_EQ(TCC_E_SUCCESS, gpio_set_direction(10, DIRECTION_OUTPUT));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_set_direction_bad_direction_P1_negative)
{
	int fake_fd = 123;
	Direction bad_direction = (Direction)456;
	EXPECT_CALL(m_libc_mock, open(StrEq("/sys/class/gpio/gpio10/direction"),_,_))
		.WillOnce(Return(fake_fd));
	EXPECT_CALL(m_libc_mock,close(fake_fd)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, fprintf(_,_,_)).WillOnce(Return(0)); // workaround for logging
	ASSERT_EQ(-TCC_E_BAD_PARAM, gpio_set_direction(10, bad_direction));
}

/////////////////////////////////////////////////
//               gpio_open tests               //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_open_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, open(StrEq("/sys/class/gpio/gpio10/value"),_,_))
		.WillOnce(Return(fake_fd));
	EXPECT_EQ(fake_fd, gpio_open(10));
}

/////////////////////////////////////////////////
//               gpio_write tests              //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_write_P1_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, write(fake_fd, _, _)).WillOnce(Return(-1));
	EXPECT_EQ(-TCC_E_BAD_PARAM, gpio_write(fake_fd, 1));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_write_1_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, write(fake_fd, StrEqVoidPtr("1"), 2)).WillOnce(Return(1));
	EXPECT_EQ(TCC_E_SUCCESS, gpio_write(fake_fd, 1));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_write_0_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, write(fake_fd, StrEqVoidPtr("0"), 2)).WillOnce(Return(1));
	EXPECT_EQ(TCC_E_SUCCESS, gpio_write(fake_fd, 0));
}

/////////////////////////////////////////////////
//               gpio_read tests               //
/////////////////////////////////////////////////

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_read_lseek_fails_P1_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, lseek(fake_fd, _, _)).WillOnce(Return(-1));
	EXPECT_EQ(-TCC_E_BAD_PARAM, gpio_read(fake_fd));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_read_lseek_read_P1_negative)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, lseek(fake_fd, _, _)).WillOnce(Return(0));
	EXPECT_CALL(m_libc_mock, read(fake_fd, _, _)).WillOnce(Return(-1));
	EXPECT_EQ(-TCC_E_BAD_PARAM, gpio_read(fake_fd));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_read_lseek_1_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, lseek(fake_fd, 0, SEEK_SET)).WillOnce(Return(1));
	EXPECT_CALL(m_libc_mock, read(fake_fd, NotNull(), 1))
		.WillOnce(Invoke([](int fd, void* ptr, size_t size) {
			*static_cast<char*>(ptr) = '1';
			return 1;
		}));
	EXPECT_EQ(1, gpio_read(fake_fd));
}

TEST_F(test_tcc_gpio_sample_helpers_host, gpio_read_lseek_0_P1_positive)
{
	int fake_fd = 123;
	EXPECT_CALL(m_libc_mock, lseek(fake_fd, 0, SEEK_SET)).WillOnce(Return(1));
	EXPECT_CALL(m_libc_mock, read(fake_fd, NotNull(), 1))
		.WillOnce(Invoke([](int fd, void* ptr, size_t size) {
			*static_cast<char*>(ptr) = '0';
			return 1;
		}));
	EXPECT_EQ(0, gpio_read(fake_fd));
}

}
