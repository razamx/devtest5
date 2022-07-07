/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef _TCC_MACROS_H_
#define _TCC_MACROS_H_

#include "tcc/cache.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BILLION 1000000000UL

/**
 * @file macros.h
 * @brief Helper macros for lightweight error handling in code samples
 *
 * Prints a description of the error from the enum.
 */

static const char* get_error_description(int code)
{
    switch (-code) {
    case TCC_E_SUCCESS:
        return "Default return code. No error";
    case TCC_E_ACCESS:
        return "Access rights or IO problem";
    case TCC_E_MEMORY:
        return "Memory allocation problem";
    case TCC_E_NOT_AVAILABLE:
        return "Resource is not available or insufficient";
    case TCC_E_FATAL:
        return "Internal SDK problem";
    case TCC_E_BAD_PARAM:
        return "Wrong or incompatible parameter";
    default:
        return "Unclassified error";
    }
}

/**
 * @brief Prints an error message with information about the error
 *
 * @param[in] code Error return code
 * @param[in] func_name Function that returned an error
 * @param[in] line Line number in the file where an error occurred
 * @param[in] file_name File name where an error occurred
 */
static __attribute__((always_inline)) inline int print_error_description(int code,
    const char* func_name,
    int line,
    const char* file_name)
{
    /*static char* error_usage[] = {
           [TCC_E_SUCCESS]       = "Default return code. No error.",
           [TCC_E_ACCESS]        = "Access rights or IO problem",
           [TCC_E_MEMORY]        = "Memory allocation problem",
           [TCC_E_NOT_AVAILABLE] = "Resource is not available or insufficient",
           [TCC_E_FATAL]         = "Internal SDK problem",
           [TCC_E_BAD_PARAM]     = "Wrong or incompatible parameter"
    };*/

    printf("Function %s failed with code %d \nfrom %s:%d.\nError description : %s\n",
        func_name,
        code,
        file_name,
        line,
        get_error_description(code));
    return code;
}
/**
 * @brief Macro for critical error handling.
 *
 * Runs the function. If an error occurs, prints the error description, cleans up the resources, and exits the
 * application with returned error code.
 * It is used when the application cannot continue after error.
 */
#define PRINT_ERROR_EXIT(code)                                            \
    do {                                                                  \
        int ret_code;                                                     \
        if ((ret_code = (code)) < 0) {                                    \
            print_error_description(ret_code, #code, __LINE__, __FILE__); \
            tcc_cache_finish();                                           \
            exit(ret_code);                                               \
        }                                                                 \
    } while (0)
/**
 * @brief Macro for non-critical error handling.
 *
 * Runs the function. If an error occurs, prints the error description.
 * It is used when the application can continue after error.
 */
#define PRINT_ERROR(code)                                                 \
    do {                                                                  \
        int ret_code;                                                     \
        if ((ret_code = (code)) < 0) {                                    \
            print_error_description(ret_code, #code, __LINE__, __FILE__); \
        }                                                                 \
    } while (0)

/**
 * @brief Sets an environment variable if it was not set before.
 */
inline static int setenv_if_not_set(const char* var_name, const char* value)
{
    if (setenv(var_name, value, 0) != 0) {
        fprintf(stderr, "setenv: can't set %s to '%s': %s(%i)\n", var_name, value, strerror(errno), errno);
        return -1;
    }
    return 0;
}

#define pr_err(s, ...) fprintf(stderr, s, ##__VA_ARGS__) /* Error message output */


#endif  // _TCC_MACROS_H_
