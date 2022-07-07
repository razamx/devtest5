/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_CONFIG_FINDER_H_
#define _TCC_CONFIG_FINDER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Search for configuration file in several locations and return path
 *        to first found file
 * @return String with full path to the file. The pointer should be freed after
 *         usage to avoid memory leak
 */
const char* get_available_config_file();

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_CONFIG_FINDER_H_
