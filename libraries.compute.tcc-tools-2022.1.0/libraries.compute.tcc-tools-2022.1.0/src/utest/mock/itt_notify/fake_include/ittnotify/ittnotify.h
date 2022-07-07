
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
#ifndef FAKE_ITT_NOTIFY
#define FAKE_ITT_NOTIFY

#include <ittnotify/ittnotify.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ___itt_domain __itt_domain;
typedef struct ___itt_string_handle __itt_string_handle;
typedef struct ___itt_id
{
    int nothing;
} __itt_id;
static const __itt_id __itt_null = {0};

__itt_domain* __itt_domain_create(const char* str);
__itt_string_handle* __itt_string_handle_create(const char* str);
void __itt_task_begin(const __itt_domain* domain, __itt_id taskid, __itt_id parentid, __itt_string_handle* name);
void __itt_task_end(const __itt_domain* domain);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_ITT_NOTIFY
