/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#include <stddef.h>
#include <stdint.h>

#ifndef _TCC_RTCT_H_
#define _TCC_RTCT_H_

#ifdef __cplusplus
extern "C" {
#endif

int64_t rtct_get_dram_latency_clk(void* rtct, size_t size);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_RTCT_H_
