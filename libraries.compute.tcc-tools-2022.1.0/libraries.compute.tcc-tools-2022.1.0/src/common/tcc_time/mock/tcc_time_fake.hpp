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

#ifndef CLK2TIMESPEC_MOCK_HPP_INCLUDED
#define CLK2TIMESPEC_MOCK_HPP_INCLUDED

#include "mock_common.hpp"
#include <gmock/gmock.h>
#include "tcc_cpuid_mock.hpp"

template<typename TestBase, unsigned int cpu_freq = 1000000000>
struct tcc_time_fake: public TestBase
{
protected:
	tcc_time_fake()
    {
        EXPECT_CALL(TestBase::m_tcc_cpuid_mock, tcc_cpuid(0x15, ::testing::NotNull()))
			.Times(::testing::Between(0,1))
			.WillRepeatedly(Invoke([&](unsigned int, struct registers* regs){
				memset(regs, 0, sizeof(*regs));
				regs->ecx = cpu_freq;
				regs->eax = 1;
				regs->ebx = 1;
				return 1;
			}));
    }
};
#endif // CLK2TIMESPEC_MOCK_HPP_INCLUDED
