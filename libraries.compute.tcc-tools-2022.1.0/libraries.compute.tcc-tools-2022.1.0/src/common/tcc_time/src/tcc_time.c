/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#include "tcc_cpuid.h"
#include "tcc_log.h"
#include "tcc_time.h"
#include <inttypes.h>  // PRIu64, PRIu32
#include <stdio.h>

static unsigned long nominal_tsc_freq = 0;
static int freq_init_flag = 0;

static void init_tsc_freq()
{
    TCC_LOG_TRACE("void");

    freq_init_flag = 1;
    struct registers reg;
    int res = tcc_cpuid(0x15, &reg);
    if (res) {
        TCC_LOG_TRACE("%" PRIu32 ",%" PRIu32 ",%" PRIu32, reg.eax, reg.ebx, reg.ecx);
        /* This won't work everywhere */
        if (reg.eax == 0) {
            TCC_LOG_ERROR("Register 'eax' has zero value");
            return;
        }
        if (reg.ebx == 0) {
            TCC_LOG_ERROR("Register 'ebx' has zero value");
            return;
        }
        if (reg.ecx == 0) {
            TCC_LOG_ERROR("Register 'ecx' has zero value");
            return;
        }
        nominal_tsc_freq = (unsigned long)reg.ecx * reg.ebx / reg.eax;
    }
}

struct timespec tcc_clk2timespec(uint64_t clk)
{
    TCC_LOG_TRACE("%" PRIu64, clk);

    struct timespec ret = {0};
    if (freq_init_flag == 0) {
        init_tsc_freq();
    }
    if (nominal_tsc_freq != 0) {
        ret.tv_sec = clk / nominal_tsc_freq;
        ret.tv_nsec = (clk % nominal_tsc_freq) * TCC_NSEC_PER_SEC / nominal_tsc_freq;
    }
    return ret;
}

uint64_t tcc_timespec2clk(struct timespec ts)
{
    TCC_LOG_TRACE("%ld.%lu", ts.tv_sec, ts.tv_nsec);

    uint64_t ret = 0;
    if (freq_init_flag == 0) {
        init_tsc_freq();
    }
    if (nominal_tsc_freq != 0) {
        ret = ts.tv_sec * nominal_tsc_freq + ts.tv_nsec * nominal_tsc_freq / TCC_NSEC_PER_SEC;
    }
    return ret;
}


__attribute__((const)) uint64_t tcc_timespec2ns(struct timespec ts)
{
    return TCC_NSEC_PER_SEC * ts.tv_sec + ts.tv_nsec;
}

struct timespec tcc_ns2timespec(uint64_t ns)
{
    struct timespec ts = {ns / TCC_NSEC_PER_SEC, ns % TCC_NSEC_PER_SEC};
    return ts;
}
