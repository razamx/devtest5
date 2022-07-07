/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/**
 * @file err_code.h
 * @brief Intel(R) Time Coordinated Computing Tools (Intel® TCC Tools) error codes
 */

#ifndef _TCC_ERR_CODE_H_
#define _TCC_ERR_CODE_H_


/**
 * @brief Error codes
 *
 * The API returns negative values for these codes.
 */
typedef enum TCC_ERROR_enum
{
    TCC_E_SUCCESS = 0,    /**< Default return code. No error. */
    TCC_BUFFER_NOT_FOUND, /**< Real-time configuration driver (tcc_buffer) is unavailable */
    TCC_E_ACCESS,         /**< Access rights or I/O problem */
    TCC_E_MEMORY,         /**< Memory allocation problem */
    TCC_E_NOT_AVAILABLE,  /**< Resource is not available or insufficient */
    TCC_E_FATAL,          /**< Internal SDK problem */
    TCC_E_BAD_PARAM,      /**< Wrong or incompatible parameter */
    TCC_E_ERROR           /**< General error */
} TCC_ERROR;

#endif  // _TCC_ERR_CODE_H_
