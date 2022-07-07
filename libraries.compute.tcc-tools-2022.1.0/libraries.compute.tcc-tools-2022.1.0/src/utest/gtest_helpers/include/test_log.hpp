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

#ifndef TEST_LOG_HPP
#define TEST_LOG_HPP

#include "tcc_log.h"

#define TEST_LOG_TRACE(fmt, ...) TCC_LOG_TRACE("TEST:" fmt, ## __VA_ARGS__)
#define TEST_LOG_DEBUG(fmt, ...) TCC_LOG("TEST:" fmt, ## __VA_ARGS__)
#define TEST_LOG_INFO(fmt, ...) TCC_LOG("TEST:" fmt, ## __VA_ARGS__)
#define TEST_LOG_WARNING(fmt, ...) TCC_LOG("TEST:" fmt, ## __VA_ARGS__)
#define TEST_LOG_ERROR(fmt, ...) TCC_LOG("TEST:" fmt, ## __VA_ARGS__)
#define TEST_LOG_FATAL(fmt, ...) TCC_LOG("TEST:" fmt, ## __VA_ARGS__)
#define TEST_TRACE_FUNC(...) TCC_TRACE_FUNC("TEST:" __VA_ARGS__)

#endif // TEST_LOG_HPP
