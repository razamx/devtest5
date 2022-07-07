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
#ifndef BENCHMARK_COMMON_H
#define BENCHMARK_COMMON_H
#include <stdio.h>

#define WORKLOAD_NAME "tcc_benchmark"
#define LINE_SIZE 64
#define PAGE_SIZE 4096
#define DEFAULT_STRESS_BUFFER_SIZE (12 * 1024 * 1024)

#define bench_printf(message, ...)            \
    {                                         \
        printf(message "", ##__VA_ARGS__);    \
        TCC_LOG_INFO(message, ##__VA_ARGS__); \
    }

#endif  // BENCHMARK_COMMON_H
