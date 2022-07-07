/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TCC_CONFIG_H_
#define _TCC_TCC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "memory_properties.h"
#include <json-c/json.h>
#include <stdlib.h>

typedef struct tcc_config tcc_config_t;

typedef struct tcc_config_item
{
    enum tcc_memory_level level;
    size_t size;
} tcc_config_item_t;

/**
 * @brief Initialize tcc_config from config .ini-file
 * @param filename Full path to the config
 * @return tcc_config instance, NULL if unable to read config
 */
tcc_config_t* tcc_config_read(const char* filename);

/**
 * @brief Get the number of configurations in the config
 * @param config tcc_config instance
 * @return Number of configurations, 0 if config empty or NULL
 */
size_t tcc_config_count(const tcc_config_t* config);

/**
 * @brief Get the configuration item with given index
 * @param config tcc_config instance
 * @param index Index if item
 * @return Pointer to the configuration item or NULL if the config is NULL or
 *         index is out of range
 */
const tcc_config_item_t* tcc_config_get(const tcc_config_t* config, size_t index);

/**
 * @brief Destroy config and clean-up resources
 * @param config tcc_config instance
 */
void tcc_config_destroy(tcc_config_t* config);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_TCC_CONFIG_H_
