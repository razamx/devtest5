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
#ifndef CONFIGURE_PMC_H
#define CONFIGURE_PMC_H

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define BIT_RANGE(begin, end) ((end) - (begin) + 1)

struct IA32_PERFEVTSELx_msr
{
    union
    {
        struct
        {
            uint8_t event_select;  // Event select field (bits 0 through 7)
            uint8_t umask;         // Unit mask (UMASK) field (bits 8 through 15)

            //Flag register:
            uint8_t usr : 1;             // (user mode) flag (bit 16)
            uint8_t os : 1;              // (os mode) flag (bit 17)
            uint8_t e : 1;               // E (edge detect) flag (bit 18)
            uint8_t pc : 1;              // PC (pin control) flag (bit 19)
            uint8_t apic_interrupt : 1;  // INT(APIC interrupt enable) flag (bit 20)
            uint8_t en : 1;              // EN (Enable Counters) Flag (bit 22)

            uint8_t
                any_thread : 1;  // Bit 21 (AnyThread) of IA32_PERFEVTSELx is supported in architectural performance monitoring version 3 for
            // processor core comprising of two or more logical processors. When set to 1, it enables counting the associated
            // event conditions (including matching the thread’s CPL with the OS/USR setting of IA32_PERFEVTSELx)
            // occurring across all logical processors sharing a processor core. When bit 21 is 0, the counter only increments
            // the associated event conditions (including matching the thread’s CPL with the OS/USR setting of
            // IA32_PERFEVTSELx) occurring in the logical processor which programmed the IA32_PERFEVTSELx MSR

            uint8_t inv : 1;  // INV (invert) flag (bit 23)

            uint8_t cmask;  // Counter mask (CMASK) field (bits 24 through 31)
            uint32_t reserved_bits;
        };
        uint64_t data;
    };
};

int IA32_PERFEVTSELx_msr_write(uint8_t msr_num, uint8_t cpu_core, struct IA32_PERFEVTSELx_msr value);

#endif  // CONFIGURE_PMC_H
