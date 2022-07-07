/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef _TCC_TCC_DRIVER_API_H_
#define _TCC_TCC_DRIVER_API_H_

#include "memory_properties.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief tcc_driver_get_region_count
 * @return region count on succes or negative on failure
 */
int tcc_driver_get_region_count();
/**
 * @brief tcc_driver_get_memory_config
 * @param region_id
 * @param out
 * @return 0 on success, negative on failure
 */
int tcc_driver_get_memory_config(unsigned int region_id, memory_properties_t* out);

/**
 * @brief tcc_driver_req_buffer
 * @param region_id region to allocate
 * @param size requred size
 * @return dev node id or negative number
 */
int tcc_driver_req_buffer(unsigned int region_id, unsigned int size);

/**
 * @brief read RTCT table from driver
 * The pointer should be freed after usage to avoid memory leak
 * @return the pointer to RTCT structure on success or NULL on failure
 */
int tcc_driver_read_rtct(void** rtct, size_t* size);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_TCC_DRIVER_API_H_
