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

#ifndef GTEST_HELPERS_HPP
#define GTEST_HELPERS_HPP

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <functional>
#include <linux/ptp_clock.h>
#include "tcc_log.h"

////////////////////////////////////////////////////////////////////////////////
// HELPER MACROSES
////////////////////////////////////////////////////////////////////////////////

// Checks that dprintf() result matches expected string
#define dprintfStrEq(expect_str) Invoke([&](int, const char* format, va_list args)\
{ \
	char buffer[1024]; \
	vsnprintf(buffer, 1024, format, args); \
	std::string s(buffer); \
	EXPECT_STREQ(s.c_str(), expect_str); \
	return s.length(); \
})

// Checks that fprintf() result matches expected string
#define fprintfStrEq(expect_str) Invoke([&](FILE*, const char* format, va_list args)\
{ \
	char buffer[1024]; \
	vsnprintf(buffer, 1024, format, args); \
	std::string s(buffer); \
	EXPECT_STREQ(s.c_str(), expect_str); \
	return s.length(); \
})

//------------------------------------------------------------------------------
// PTP helpers
//------------------------------------------------------------------------------

#define BILLION 1000000000UL

#define EXPECT_PTP_PIN_DESC(expect) \
Invoke([&](int, unsigned int req, va_list args){ \
	EXPECT_EQ(PTP_PIN_SETFUNC2, req); \
	ptp_pin_desc* __desc = va_arg(args, ptp_pin_desc*); \
	EXPECT_EQ((expect).index, __desc->index); \
	EXPECT_EQ((expect).func, __desc->func);  \
	EXPECT_EQ((expect).chan, __desc->chan); \
	return 0; \
})

#define EXPECT_PTP_PEROUT_REQUEST(expect) \
Invoke([&](int, unsigned int req, va_list args){ \
	EXPECT_EQ(PTP_PEROUT_REQUEST2, req); \
	ptp_perout_request* __request = va_arg(args, ptp_perout_request*); \
	EXPECT_EQ((expect).start.sec, __request->start.sec); \
	EXPECT_EQ((expect).start.nsec, __request->start.nsec); \
	EXPECT_EQ((expect).period.sec, __request->period.sec); \
	EXPECT_EQ((expect).period.nsec, __request->period.nsec); \
	EXPECT_EQ((expect).index, __request->index);  \
	return 0; \
})

#define EXPECT_PTP_EXTTS_REQUEST(expect)\
Invoke([&](int, unsigned int req, va_list args){ \
	EXPECT_EQ(PTP_EXTTS_REQUEST2, req); \
	ptp_extts_request* __request = va_arg(args, ptp_extts_request*); \
	EXPECT_EQ((expect).index, __request->index); \
	EXPECT_EQ((expect).flags, __request->flags); \
	return 0; \
})

#define EXPECT_PTP_SYS_OFFSET_PRECISE(tsc, art)\
Invoke([&](int, unsigned int req, va_list args) { \
	EXPECT_EQ(PTP_SYS_OFFSET_PRECISE2, req); \
	ptp_sys_offset_precise* __offset = va_arg(args, ptp_sys_offset_precise*); \
	__offset->device.sec = art / BILLION; \
	__offset->device.nsec = art % BILLION; \
	__offset->sys_realtime.sec = tsc / BILLION; \
	__offset->sys_realtime.nsec = tsc % BILLION; \
	return 0; \
})

#define GET_PTP_EVENT_COUNT_TSTAMP(value)\
Invoke([&](int, unsigned int req, va_list args) {\
	EXPECT_EQ(PTP_EVENT_COUNT_TSTAMP2, req); \
	ptp_event_count_tstamp* __events = va_arg(args, ptp_event_count_tstamp*); \
        *__events = value; \
	return 0; \
})

////////////////////////////////////////////////////////////////////////////////
// CUSTOM MATCHERS
////////////////////////////////////////////////////////////////////////////////

// Matcher for comparing string as void* with std::steing
MATCHER_P(StrEqVoidPtr, expected, "")
{
	return std::string(static_cast<const char*>(arg)) == expected;
}

// Compare void* with const char*
MATCHER_P(CharEqVoidPtr, expected, "")
{
	return *static_cast<const char*>(arg) == expected;
}

// Matcher for comparing timespect*
MATCHER_P(TimespecPtrEq, expected, "")
{
	return arg->tv_sec == expected.tv_sec && arg->tv_nsec == expected.tv_nsec;
}

// Matcher for comparing timespect* with nanoseconds
MATCHER_P(TimespecPtrWithNsEq, expected, "")
{
	return arg->tv_sec == expected / BILLION && arg->tv_nsec == expected % BILLION;
}

#define EXPECT_ERRNO_EQ(code) ({EXPECT_EQ(errno, code) << "'"<<strerror(errno) << "' vs '" << strerror(code) << "'";})
#define ASSERT_ERRNO_EQ(code) ({ASSERT_EQ(errno, code) << "'"<<strerror(errno) << "' vs '" << strerror(code) << "'";})

#define RETURN_NEGATIVE_AND_SET_ERRNO(error_code) Invoke([](...){errno=error_code; return -1;})

#define RETURN_FILE_DESCRIPTOR(descriptor) Invoke([&](...) \
{ \
    if (descriptor != -1) { \
        EXPECT_CALL(m_libc_mock, close(descriptor)).WillOnce(Return(0)); \
    } else { \
        EXPECT_CALL(m_libc_mock, close(-1)) \
            .Times(testing::AnyNumber()) \
            .WillRepeatedly(Return(0)); \
    } \
    return descriptor; \
})

#endif // GTEST_HELPERS_HPP
