
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

#include "tcc_msr_mock.hpp"
#include "tcc_log.h"

using namespace ::testing;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(tcc_msr_mock)

int rdmsr(uint32_t reg, uint8_t cpu, uint64_t* data)
{
    return ALLOW_REGULAR_MOCK(tcc_msr_mock, rdmsr)(reg, cpu, data);
}

int wrmsr(uint32_t reg, uint8_t cpu, const uint64_t* data)
{
    return ALLOW_REGULAR_MOCK(tcc_msr_mock, wrmsr)(reg, cpu, data);
}

void tcc_msr_mock::init()
{
    ON_CALL(*this, wrmsr(_, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, rdmsr(_, _, _)).WillByDefault(Return(-1));
}