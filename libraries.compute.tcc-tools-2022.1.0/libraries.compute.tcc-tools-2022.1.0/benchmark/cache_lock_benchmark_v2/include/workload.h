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
#ifndef WORKLOAD_H
#define WORKLOAD_H
#include "pointer_chasing.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*workload_func_t)(cache_line_node_t* lines);

typedef struct workload_t
{
    const char* name;
    workload_func_t func;
} workload_t;

const workload_t* workload_get_by_name(const char* name);
const workload_t* workload_get_by_id(size_t id);
int workload_run(const workload_t* workload, cache_line_node_t* lines);

#ifdef __cplusplus
}
#endif

#endif  // WORKLOAD_H
