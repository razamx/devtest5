/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_PCHECK_H_
#define _TCC_PCHECK_H_

/**
 * @file pcheck.h
 * @brief Defines the debug utility macros
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* These macros are defined in 'assert.h',
   see 2 variants in different platforms, use either of them */
#ifndef __ASSERT_FUNCTION
#ifdef __ASSERT_FUNC
#define __ASSERT_FUNCTION __ASSERT_FUNC
#else
#define __ASSERT_FUNCTION __PRETTY_FUNCTION__
#endif /* __ASSERT_FUNC */
#endif /* __ASSERT_FUNCTION */

/**
 * Convert a macro expression value to a string literal
 * @param[in] arg The macro expression, value of which will be converted to a string
 * @return The string literal representing the value of the input macro expression
 */
#define stringify(arg) stringify_helper(arg)

/**
 * This is a helper macro to implement conversion of macro definitions to string values
 * @param[in] arg The macro expression to be converted to a string
 * @return The input macro expression converted to a string
 */
#define stringify_helper(arg) #arg

/**
 * This macro provides a simple API for run-time checking of the expected state of the applications
 * @param[in] condition The condition that must be verified as \c true for the current case
 * @param[in] fmt The formatting string to be included in the error report if
 *            the condition is \c false. This parameter may be followed by
 *            other parameters to be included in the error report, similar to \c printf arguments.
 * @note Terminates the application if the condition is \c false.
 */
#define check(condition, fmt, ...)                                      \
    if (!(condition)) {                                                 \
        fprintf(stderr,                                                 \
            "***** ERROR: %s:%d: %s: Assertion '%s' failed: " fmt "\n", \
            __FILE__,                                                   \
            __LINE__,                                                   \
            __ASSERT_FUNCTION,                                          \
            #condition,                                                 \
            ##__VA_ARGS__);                                             \
        exit(1);                                                        \
    }

#endif  // _TCC_PCHECK_H_
