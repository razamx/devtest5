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

#ifndef __TCC_CPUID_MOCK_HPP__
#define __TCC_CPUID_MOCK_HPP__

#include "mock_common.hpp"

#include <gmock/gmock.h>

extern "C" {

#include <tcc_cpuid.h>

}

class tcc_cpuid_interface {

    public:
        virtual int tcc_cpuid(unsigned int leaf, struct registers* regs) = 0;
        virtual int tcc_cpuid_ex(unsigned int leaf, unsigned int sub_leaf, struct registers* regs) = 0;

        COMMON_MOCK_INTERFACE_DEFINITIONS(tcc_cpuid_interface);
};

class tcc_cpuid_mock: public tcc_cpuid_interface {

    public:
        MOCK_METHOD2(tcc_cpuid, int(unsigned int leaf, struct registers* regs));
        MOCK_METHOD3(tcc_cpuid_ex, int(unsigned int leaf, unsigned int sub_leaf, struct registers* regs));

        tcc_cpuid_mock();
};

#endif /* __TCC_CPUID_MOCK_HPP__ */
