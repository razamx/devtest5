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

#include "tcc_cpuid_mock.hpp"

using ::testing::_;
using ::testing::Return;

COMMON_MOCK_INTERFACE_IMPLEMENTATION(tcc_cpuid_interface)

tcc_cpuid_mock::tcc_cpuid_mock() {
    ON_CALL(*this, tcc_cpuid(_,_))
        .WillByDefault(Return(0));
    ON_CALL(*this, tcc_cpuid_ex(_,_,_))
        .WillByDefault(Return(0));
}

int tcc_cpuid(unsigned int leaf, struct registers* regs) {
    return tcc_cpuid_interface::get().tcc_cpuid(leaf, regs);
}

int tcc_cpuid_ex(unsigned int leaf, unsigned int sub_leaf, struct registers* regs) {
    return tcc_cpuid_interface::get().tcc_cpuid_ex(leaf, sub_leaf, regs);
}
