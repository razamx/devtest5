/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TCC_CPUID_H_
#define _TCC_TCC_CPUID_H_

struct registers
{
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
};

/**
 * The function checks if cpuid is supported and returns 1 for valid cpuid
 * information or 0 for unsupported cpuid leaf.
 */
int tcc_cpuid(unsigned int leaf, struct registers* regs);
/**
 * The function checks if cpuid is supported and returns 1 for valid cpuid
 * information or 0 for unsupported cpuid leaf.
 */
int tcc_cpuid_ex(unsigned int leaf, unsigned int sub_leaf, struct registers* regs);

#endif  // _TCC_TCC_CPUID_H_
