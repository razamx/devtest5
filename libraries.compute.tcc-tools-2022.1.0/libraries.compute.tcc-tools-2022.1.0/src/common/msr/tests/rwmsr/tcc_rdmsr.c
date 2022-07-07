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

#include "tcc_log.h"
#include "tcc_msr.h"
#include "tcc_rwmsr_common.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static void usage()
{
    ERR_MSG("tcc_rdmsr <cpuid> <reg>");
}

int main(int argc, char** argv)
{
    uint8_t cpuid;
    uint32_t reg;
    if (argc != 3) {
        ERR_MSG("Wrong params number!");
        usage();
        exit(1);
    }
    STR_TO_U(8, argv[1], cpuid);
    STR_TO_U(32, argv[2], reg);
    uint64_t value = 0;
    if (rdmsr(reg, cpuid, &value) != 0) {
        ERR_MSG("Can't read value from MSR 0x%x for cpuid %i", reg, cpuid)
    }
    printf("%lu\n", value);
    return 0;
error:
    exit(1);
}
