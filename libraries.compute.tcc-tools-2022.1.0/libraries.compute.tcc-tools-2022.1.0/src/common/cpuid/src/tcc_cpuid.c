/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include <cpuid.h>
#include <string.h>

#include <tcc_cpuid.h>

int tcc_cpuid(unsigned int leaf, struct registers* regs)
{
    memset(regs, 0, sizeof(struct registers));
    return __get_cpuid(leaf, &regs->eax, &regs->ebx, &regs->ecx, &regs->edx);
}

int tcc_cpuid_ex(unsigned int leaf, unsigned int sub_leaf, struct registers* regs)
{
    memset(regs, 0, sizeof(struct registers));
    return __get_cpuid_count(leaf, sub_leaf, &regs->eax, &regs->ebx, &regs->ecx, &regs->edx);
}
