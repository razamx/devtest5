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

#include "SubprocessTest.hpp" // SubprocessTest, testing, SUBPROCESS_TEST_F
#include "tcc_cpuid_mock.hpp" // StrictMock, tcc_cpuid_mock
#include "tcc_time.h"
#include <gtest/gtest.h>
#include <functional>
#include <time_helpers.hpp>

namespace {
using namespace ::testing;

struct timespec timespec(__time_t tv_sec, __syscall_slong_t tv_nsec)
{
    return {tv_sec, tv_nsec};
}

class clk2timespec_fixture : public SubprocessTest<Test>
{
protected:
    StrictMock<tcc_cpuid_mock> m_tcc_cpuid_mock;
    std::function<int(unsigned int, struct registers* regs)> get_cpuid_fake_for_regs(unsigned int eax, unsigned int ebx ,unsigned int ecx)
    {
        return [=](unsigned int, struct registers* regs){
            regs->eax = eax;
            regs->ebx = ebx;
            regs->ecx = ecx;
            regs->edx = 0;
            return 1;
        };
    }
};

SUBPROCESS_TEST_F(clk2timespec_fixture, res_zero_negative)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull())).WillOnce(Return(0));
    EXPECT_EQ(tcc_clk2timespec(1000), timespec(0,0));
}

SUBPROCESS_TEST_F(clk2timespec_fixture, res_eax_zero_negative)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull()))
        .WillOnce(Invoke(get_cpuid_fake_for_regs(0,1,1)));
    EXPECT_EQ(tcc_clk2timespec(1000), timespec(0,0));
}

SUBPROCESS_TEST_F(clk2timespec_fixture, res_ebx_zero_negative)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull()))
        .WillOnce(Invoke(get_cpuid_fake_for_regs(1,0,1)));
    EXPECT_EQ(tcc_clk2timespec(1000), timespec(0,0));
}

SUBPROCESS_TEST_F(clk2timespec_fixture, res_ecx_zero_negative)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull()))
        .WillOnce(Invoke(get_cpuid_fake_for_regs(1,1,0)));
    EXPECT_EQ(tcc_clk2timespec(1000), timespec(0,0));
}

SUBPROCESS_TEST_F(clk2timespec_fixture, timespec2clk_negative)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull()))
        .WillOnce(Invoke(get_cpuid_fake_for_regs(1,1,0)));
    EXPECT_EQ(tcc_timespec2clk(timespec(6,172500000)), uint64_t(0));
}

SUBPROCESS_TEST_F(clk2timespec_fixture, clk2timespec_positive)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull()))
        .WillOnce(Invoke(get_cpuid_fake_for_regs(9,3000,6)));
    EXPECT_EQ(tcc_clk2timespec(12345), timespec(6,172500000));
    EXPECT_EQ(tcc_clk2timespec(54321), timespec(27,160500000));
}

SUBPROCESS_TEST_F(clk2timespec_fixture, timespec2clk_positive)
{
    EXPECT_CALL(m_tcc_cpuid_mock, tcc_cpuid(0x15,NotNull()))
        .WillOnce(Invoke(get_cpuid_fake_for_regs(9,3000,6)));
    EXPECT_EQ(tcc_timespec2clk(timespec(6,172500000)), uint64_t(12345));
    EXPECT_EQ(tcc_timespec2clk(timespec(27,160500000)), uint64_t(54321));
}

}
