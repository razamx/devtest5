/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef _TCC_MEMORY_PROPERTIES_H_
#define _TCC_MEMORY_PROPERTIES_H_

#include <sched.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tcc_memory_level
{
    TCC_MEM_UNKNOWN = 0,
    TCC_MEM_L1,
    TCC_MEM_L2,
    TCC_MEM_L3,
    TCC_MEM_EDRAM,
    TCC_MEM_DRAM
};

/**
 * @brief Memory region properties
 */
typedef struct
{
    unsigned int id;
    cpu_set_t mask;
    unsigned int latency;
    unsigned int latency_clk;
    enum tcc_memory_level type;
    size_t size_drv;
    size_t size;
} memory_properties_t;

int memory_properties_compare(const void* a, const void* b);

/**
 * @brief Return string name of the tcc_memory_level
 */
const char* tcc_memory_level_to_str(enum tcc_memory_level level);

/**
 * @brief Return string binary representation of the cpu_set_t
 * WARNING: Global static function is returned, do not store this
 * pointer
 */
const char* cpu_set_to_str(const cpu_set_t* cpuset);

/**
 * @brief Return string representation of the memory_properties
 * WARNING: Global static function is returned, do not store this
 * pointer
 */
const char* memory_properties_to_str(const memory_properties_t* properties);

#ifdef __cplusplus
}
#endif

#endif  // _TCC_MEMORY_PROPERTIES_H_
