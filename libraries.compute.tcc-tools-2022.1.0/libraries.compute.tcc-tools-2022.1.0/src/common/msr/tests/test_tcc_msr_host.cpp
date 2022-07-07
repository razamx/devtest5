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
#include <libc_mock.hpp>
#include "tcc_msr.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "gtest_helpers.hpp"

#define FAKE_CPU 13
#define FAKE_REG 42
#define FAKE_FD 777

#define TOSTR(s) TOSTR2(s)
#define TOSTR2(s) #s

namespace
{

    using namespace ::testing;

    class test_msr_simple_host: public Test
    {
    protected:
        uint64_t m_data;
    };

    TEST_F(test_msr_simple_host, rdmsr_data_nullprt_negative)
    {
        EXPECT_EQ(-1, rdmsr(FAKE_REG, FAKE_CPU, nullptr));
    }
    TEST_F(test_msr_simple_host, wrmsr_data_nullprt_negative)
    {
        EXPECT_EQ(-1, wrmsr(FAKE_REG, FAKE_CPU, nullptr));
    }

    class test_msr_host: public test_msr_simple_host
    {
    public:
        StrictMock<libc_mock> m_libc_mock;
        uint64_t m_data = 0;
    };
    
    TEST_F(test_msr_host, wrmsr_open_failed_negative)
    {
        EXPECT_CALL(m_libc_mock, open(StrEq("/dev/cpu/" TOSTR(FAKE_CPU) "/msr"),O_WRONLY , _))
            .WillOnce(RETURN_FILE_DESCRIPTOR(-1));
        EXPECT_EQ(-1, wrmsr(FAKE_REG, FAKE_CPU, &m_data));
    }

    TEST_F(test_msr_host, rdmsr_open_failed_negative){
        EXPECT_CALL(m_libc_mock, open(StrEq("/dev/cpu/" TOSTR(FAKE_CPU) "/msr"),O_RDONLY , _))
            .WillOnce(RETURN_FILE_DESCRIPTOR(-1));
        EXPECT_EQ(-1, rdmsr(FAKE_REG, FAKE_CPU, &m_data));
    }

    TEST_F(test_msr_host, wrmsr_pwrite_failed_negative)
    {
        EXPECT_CALL(m_libc_mock, open(StrEq("/dev/cpu/" TOSTR(FAKE_CPU) "/msr"),O_WRONLY , _))
            .WillOnce(RETURN_FILE_DESCRIPTOR(FAKE_FD));
        EXPECT_CALL(m_libc_mock, pwrite(FAKE_FD, &m_data, sizeof(m_data), FAKE_REG))
            .WillOnce(Return(-1));
        EXPECT_EQ(-1, wrmsr(FAKE_REG, FAKE_CPU, &m_data));
    }

    TEST_F(test_msr_host, rdmsr_read_failed_negative){
        EXPECT_CALL(m_libc_mock, open(StrEq("/dev/cpu/" TOSTR(FAKE_CPU) "/msr"),O_RDONLY , _))
            .WillOnce(RETURN_FILE_DESCRIPTOR(FAKE_FD));
        EXPECT_CALL(m_libc_mock, pread(FAKE_FD, &m_data, sizeof(m_data), FAKE_REG))
            .WillOnce(Return(-1));
        EXPECT_EQ(-1, rdmsr(FAKE_REG, FAKE_CPU, &m_data));
    }
    
    TEST_F(test_msr_host, wrmsr_success)
    {
        EXPECT_CALL(m_libc_mock, open(StrEq("/dev/cpu/" TOSTR(FAKE_CPU) "/msr"),O_WRONLY , _))
            .WillOnce(RETURN_FILE_DESCRIPTOR(FAKE_FD));
        EXPECT_CALL(m_libc_mock, pwrite(FAKE_FD, &m_data, sizeof(m_data), FAKE_REG))
            .WillOnce(Return(sizeof(m_data)));
        EXPECT_EQ(0, wrmsr(FAKE_REG, FAKE_CPU, &m_data));
    }

    TEST_F(test_msr_host, rdmsr_success){
        EXPECT_CALL(m_libc_mock, open(StrEq("/dev/cpu/" TOSTR(FAKE_CPU) "/msr"),O_RDONLY , _))
            .WillOnce(RETURN_FILE_DESCRIPTOR(FAKE_FD));
        EXPECT_CALL(m_libc_mock, pread(FAKE_FD, &m_data, sizeof(m_data), FAKE_REG))
            .WillOnce(Return(sizeof(m_data)));
        EXPECT_EQ(0, rdmsr(FAKE_REG, FAKE_CPU, &m_data));
    }

    //*/
} // namespace
