/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_OPCUA_UTILS_H_
#define _TCC_OPCUA_UTILS_H_

/**
 * @file opcua_utils.h
 * @brief This file defines the utility API used for logging and debug functionality
 */

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * This enumerator defines the possible log levels to output debug information
 */
typedef enum
{
    log_undef = 0, /**< Undefined or incorrect log level, used for uninitialized variables */
    log_error,     /**< The log level for reporting application errors */
    log_info,      /**< The log level for reporting informative notes */
    log_debug      /**< The log level for other debug information */
} log_level_t;

extern log_level_t log_level;

/**
 * Macro definition for extracting the basename of the filename
 */
#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)

/**
 * Macro to print logging information
 *
 * @param[in] log_wanted_level The log level for the current piece of information
 * @param[in] log_stream The stream device for the output
 * @param[in] log_text_mask The formatted logging information, can be followed by
 *                          optional arguments to be reflected in the output,
 *                          similar to \c printf formatting
 * @note Depending on the current \c log_level and requested log level \c log_wanted_level,
 *       the output will either be ignored or sent to the \c log_stream device
 */
#define print_log(log_wanted_level, log_stream, log_text_mask, ...) \
    if (log_level >= (log_wanted_level)) {                          \
        fprintf((log_stream), (log_text_mask), ##__VA_ARGS__);      \
    }

/**
 * Print error logging information
 *
 * @param[in] M The formatted logging information, can be followed by optional arguments
 *              to be reflected in the output, similar to \c printf formatting
 */
#define LOG_ERR(M, ...)                       \
    print_log(log_error,                      \
        stderr,                               \
        "[ERROR] (%s:%d: errno: %s) " M "\n", \
        __FILENAME__,                         \
        __LINE__,                             \
        strerror(errno),                      \
        ##__VA_ARGS__)

/**
 * Print informational logging information
 *
 * @param[in] M The formatted logging information, can be followed by optional arguments
 *              to be reflected in the output, similar to \c printf formatting
 */
#define LOG_INFO(M, ...) \
    print_log(log_info, stdout, "[INFO] (%s:%s:%d) " M "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

/**
 * Print debug logging information
 *
 * @param[in] M The formatted logging information, can be followed by optional arguments
 *              to be reflected in the output, similar to \c printf formatting
 */
#define LOG_DBG(M, ...) \
    print_log(log_debug, stdout, "[DEBUG] (%s:%s:%d) " M "\n", __FILENAME__, __func__, __LINE__, ##__VA_ARGS__)

/**
 * Run-time check for condition \c A to be true. If this check fails, report the error.
 *
 * @param[in] A The condition to be verified as \c true
 * @param[in] M  The formatted error information, can be followed by optional arguments
 *               to be reflected in the output, similar to \c printf formatting
 */
#define check(A, M, ...)           \
    if (!(A)) {                    \
        LOG_ERR(M, ##__VA_ARGS__); \
        errno = 0;                 \
        goto error;                \
    }

#ifdef __cplusplus
} /* extern "C" { */
#endif /* __cplusplus */

#endif /* _TCC_OPCUA_UTILS_H_ */
