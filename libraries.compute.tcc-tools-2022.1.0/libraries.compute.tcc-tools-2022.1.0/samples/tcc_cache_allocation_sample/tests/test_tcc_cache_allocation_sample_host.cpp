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

///Sample headers:
#define _GNU_SOURCE

#include <getopt.h>
#include <ittnotify/ittnotify.h>
#include <linux/random.h>
#include <sched.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/syscall.h> /* For SYS_xxx definitions */
#include <sys/sysinfo.h>
#include <unistd.h>

#include "args_parse.h"
#include "args_helper.hpp"
#include "macros.h"
#include "pointer_chasing.h"
#include "tcc/cache.h"
#include "tcc/measurement.h"
#include "tcc/measurement_helpers.h"

//include sample
#define main sample_main
extern "C"
{
#include "main.c"
}
#undef main

#include <gtest/gtest.h>
#include <vector>
#include "itt_notify_mock.hpp"

namespace
{
using namespace ::testing;

class tcc_cache_allocation_sample_init: public Test
{
protected:
    StrictMock<itt_notify_mock> m_itt_notify_mock;

    int call_tcc_cache_allocation_sample(::Args args)
    {
        return sample_main(args.count(), args);
    }
};

TEST_F(tcc_cache_allocation_sample_init, tcc_cache_allocation_sample_init)
{
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(StrEq("TCC"))).WillOnce(Return(nullptr));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(StrEq("Workload"))).WillOnce(Return(nullptr));
    EXPECT_EQ(-TCC_E_NOT_AVAILABLE, call_tcc_cache_allocation_sample({}));
}

TEST_F(tcc_cache_allocation_sample_init, tcc_cache_allocation_sample_bad_param)
{
    __itt_domain *test_domain = reinterpret_cast<__itt_domain *>(0x1234);
    __itt_string_handle *test_string_handle = reinterpret_cast<__itt_string_handle *>(0x2345);
    EXPECT_CALL(m_itt_notify_mock, __itt_domain_create(_)).WillOnce(Return(test_domain));
    EXPECT_CALL(m_itt_notify_mock, __itt_string_handle_create(_)).WillOnce(Return(test_string_handle));
    EXPECT_EQ(TCC_E_SUCCESS, call_tcc_cache_allocation_sample({"-h"}));
}

}  // namespace
