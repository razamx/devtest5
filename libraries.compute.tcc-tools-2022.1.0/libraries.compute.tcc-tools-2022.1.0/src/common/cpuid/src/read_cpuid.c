/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "tcc_cpuid.h"

static void print_regs(struct registers* regs)
{
    printf("eax=0x%08x ebx=0x%08x ecx=0x%08x edx=0x%08x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage:\n");
        printf("read_cpuid <leaf (hex)> <subleaf (hex)>\n");
        return -1;
    }

    unsigned int leaf = strtoul(argv[1], NULL, 16);
    unsigned int subleaf = strtoul(argv[2], NULL, 16);
    struct registers regs = {0};

    if (!tcc_cpuid_ex(leaf, subleaf, &regs)) {
        printf("Failed to read CPUID\n");
        return -1;
    }

    print_regs(&regs);

    return 0;
}
