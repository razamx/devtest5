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

#include <gtest/gtest.h>
#include "tcc_log.c"
#include "libc_mock.hpp"
#include <string>

namespace
{
    using namespace testing;

    class test_tcc_log_host : public Test
    {
    protected:
        StrictMock<libc_mock> log_mock;
        void SetUp() override {}
    };

    TEST_F(test_tcc_log_host, make_time_stamp_is_correct_if_usec_1e7)
    {
        EXPECT_CALL(log_mock, gettimeofday(_, _))
            .WillOnce(Invoke([](timeval *__restrict__ __tv, void *__restrict__ __tz) {
                __tv->tv_sec = 0;
                __tv->tv_usec = 1100000;
                return 0;
            }));
        EXPECT_CALL(log_mock, localtime(_))
            .WillOnce(Invoke([](const time_t *__timer) {
                timeval tv;
                tv.tv_sec = 1;
                tv.tv_usec = 1100000;
                return __real_localtime(&tv.tv_sec);
            }));
        char *p = make_time_stamp();
        ASSERT_EQ("1970.01.01 03:00:01.100", std::string(p));
        free(p);
    }

    TEST_F(test_tcc_log_host, make_time_stamp_is_correct_if_usec_1e3)
    {
        EXPECT_CALL(log_mock, gettimeofday(_, _))
            .WillOnce(Invoke([](timeval *__restrict__ __tv, void *__restrict__ __tz) {
                __tv->tv_sec = 0;
                __tv->tv_usec = 1001;
                return 0;
            }));
        EXPECT_CALL(log_mock, localtime(_))
            .WillOnce(Invoke([](const time_t *__timer) {
                timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 1001;
                return __real_localtime(&tv.tv_sec);
            }));
        char *p = make_time_stamp();
        ASSERT_EQ("1970.01.01 03:00:00.001", std::string(p));
        free(p);
    }

    TEST_F(test_tcc_log_host, make_time_stamp_is_correct_if_local_time_null)
    {
        EXPECT_CALL(log_mock, gettimeofday(_, _))
            .WillOnce(Invoke([](timeval *__restrict__ __tv, void *__restrict__ __tz) {
                __tv->tv_sec = 0;
                __tv->tv_usec = 0;
                return 0;
            }));
        EXPECT_CALL(log_mock, localtime(_))
            .WillOnce(ReturnNull());
        char *p = make_time_stamp();
        ASSERT_EQ("TIMESTAMP", std::string(p));
        free(p);
    }

    TEST_F(test_tcc_log_host, set_log_level_is_correct_if_log_level_null)
    {
        ASSERT_NO_THROW(set_log_level(NULL));
    }

    TEST_F(test_tcc_log_host, tcc_log_set_level_is_correct_if_level_error_given)
    {
        int old_level = tcc_log_level;
        ASSERT_EQ(old_level, tcc_log_set_level(LOG_ERROR));
        ASSERT_EQ(LOG_ERROR, tcc_log_level);
    }
}
