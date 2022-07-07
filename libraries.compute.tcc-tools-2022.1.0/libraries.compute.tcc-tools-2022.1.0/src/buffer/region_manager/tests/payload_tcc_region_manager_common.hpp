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

#ifndef PAYLOAD_TCC_REGION_MANAGER_COMMON_HPP
#define PAYLOAD_TCC_REGION_MANAGER_COMMON_HPP

#define CACHE_LINE_SIZE 64
#define PAGE_SIZE 4096
#define DRIVER_NAME "tcc_buffer"
#define PATH_TO_KERNEL_MODULE ""
#define STRESS_BUFFER_SIZE (32 * 1024 * 1024)
#define MEASUREMENT_NAME "psram_performance_test"
#define MIN_PERF_DEVIATION 3.5

#endif //PAYLOAD_TCC_REGION_MANAGER_COMMON_HPP
