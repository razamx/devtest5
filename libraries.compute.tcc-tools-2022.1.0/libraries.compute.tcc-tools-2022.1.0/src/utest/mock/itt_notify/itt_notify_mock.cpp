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

#include "itt_notify_mock.hpp"
#include "tcc_log.h"

using namespace ::testing;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(itt_notify_mock)

__itt_domain* __itt_domain_create(const char* str)
{
    return ALLOW_REGULAR_MOCK(itt_notify_mock, __itt_domain_create)(str);
}

__itt_string_handle* __itt_string_handle_create(const char* str)
{
    return ALLOW_REGULAR_MOCK(itt_notify_mock, __itt_string_handle_create)(str);
}
void __itt_task_begin(const __itt_domain* domain, __itt_id taskid, __itt_id parentid, __itt_string_handle* name)
{
    ALLOW_REGULAR_MOCK(itt_notify_mock, __itt_task_begin)(domain, taskid, parentid, name);
}

void __itt_task_end(const __itt_domain* domain)
{
    ALLOW_REGULAR_MOCK(itt_notify_mock, __itt_task_end)(domain);
}

void itt_notify_mock::init()
{
    ON_CALL(*this, __itt_domain_create(_)).WillByDefault(ReturnNull());
    ON_CALL(*this, __itt_string_handle_create(_)).WillByDefault(ReturnNull());
}