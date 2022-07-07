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
#ifndef PARAMS_H
#define PARAMS_H
#include "workload.h"

struct control_data
{
    unsigned long cpuid;
    unsigned long region_index;
    unsigned long buffer_size;
    unsigned long sleep;
    unsigned long random_seed;
    unsigned long iterations;
    unsigned long accesses;
    unsigned long internal_stress;
    unsigned long interrupts;
    unsigned long collect_counters;
    unsigned long preallocate_size;
    unsigned long stress_buffer_size;
    unsigned long stress_buffer_accesses;
    const workload_t* workload;
};

extern struct control_data control;
int parse_params(int argc, char** argv);
char* get_dump_file_name();

#endif  // PARAMS_H
