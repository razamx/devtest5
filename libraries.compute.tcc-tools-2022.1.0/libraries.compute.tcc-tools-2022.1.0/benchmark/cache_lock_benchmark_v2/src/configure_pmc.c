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

#include "tcc_msr.h"
#include "tcc_log.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include "configure_pmc.h"

#define PRINT_ERROR(format, ...)          \
    TCC_LOG_ERROR(format, ##__VA_ARGS__); \
    fprintf(stderr, format "\n", ##__VA_ARGS__)

#define IA32_PERFEVTSEL0_MSR (0x186)

int IA32_PERFEVTSELx_msr_write(uint8_t msr_num, uint8_t cpu_core, struct IA32_PERFEVTSELx_msr value)
{
    TCC_LOG_DEBUG("Store msr: %xH", IA32_PERFEVTSEL0_MSR + msr_num);
    TCC_LOG_DEBUG("  name   IA32_PERFEVTSEL%u", msr_num);
    TCC_LOG_DEBUG("  core:  %u", cpu_core);
    TCC_LOG_DEBUG("  value: 0x%016lx", value.data);
    TCC_LOG_TRACE("    event_select: %02xH", value.event_select);
    TCC_LOG_TRACE("    umask:        %02xH", value.umask);
    TCC_LOG_TRACE("    usr:          %s", value.usr ? "true" : "false");
    TCC_LOG_TRACE("    os:           %s", value.os ? "true" : "false");
    TCC_LOG_TRACE("    E:            %s", value.e ? "true" : "false");
    TCC_LOG_TRACE("    PC:           %s", value.pc ? "true" : "false");
    TCC_LOG_TRACE("    INT:          %s", value.apic_interrupt ? "true" : "false");
    TCC_LOG_TRACE("    EN:           %s", value.en ? "true" : "false");
    TCC_LOG_TRACE("    AnyThread:    %s", value.any_thread ? "true" : "false");
    TCC_LOG_TRACE("    INV:          %s", value.inv ? "true" : "false");
    TCC_LOG_TRACE("    cmask:        %u", value.cmask);
    return wrmsr(IA32_PERFEVTSEL0_MSR + msr_num, cpu_core, &(value.data));
}
