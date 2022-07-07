/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TCC_MSR_H_
#define _TCC_TCC_MSR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int rdmsr(uint32_t reg, uint8_t cpu, uint64_t* data);

int wrmsr(uint32_t reg, uint8_t cpu, const uint64_t* data);

#ifdef __cplusplus
}
#endif

#endif  // _TCC_TCC_MSR_H_
