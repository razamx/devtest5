/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_PDATASOURCE_H_
#define _TCC_PDATASOURCE_H_

/**
 * @file pdatasource.h
 * @brief Defines the API for accessing data source storage.
 *
 * This file defines the API for creating, deleting, and accessing data source
 * storage by producers and consumers.
 * Cache allocation library capabilities are used to allocate memory for data storage.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Prepare data source storage according to the provided memory access latency
 * using the cache allocation library and pointer chasing workload logic
 * @param[in] tcc_latency The memory access latency to be used by the cache allocation library
 * @param[in] tcc_affinity The affinity for the datasource workload
 * @note Terminates the application if the initialization steps fail
 */
int prepare_datasource(uint64_t tcc_latency, int tcc_affinity);
/**
 * Destroy data source storage
 */
void destroy_datasource(void);
/**
 * Put a pointer to the data into data source storage
 * @param[in] data The pointer to the data provided by the caller
 * @note A linked list is used internally as data source storage
 */
void put_value_in_datasource(void* data);
/**
 * Get the pointer to the data from data source storage
 * @note A linked list is used internally as data source storage
 */
void* get_value_from_datasource(void);
/**
 * Print and save statistics to the file about usage of data source storage
 */
void print_datasource_statistic(void);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif  // _TCC_PDATASOURCE_H_
