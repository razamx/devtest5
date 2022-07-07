/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_ARGS_PARSE_H_
#define _TCC_ARGS_PARSE_H_


#ifdef __cplusplus
extern "C" {
#endif

// This header defines an interface for parsing parameters passed by user

#define DECIMAL_NUM_SYS 10

/**
 * @brief Return values for arguments parsing functuon
 */
typedef enum
{
    TCC_PARSE_ERROR = -1, /**< Error return code. Arguments weren't parsed successfully */
    TCC_PARSE_SUCCESS,    /**< Default return code. Arguments were parsed successfully */
    TCC_PRINT_HELP        /**< Help request. Print help and exit with 0 return code */
} TCC_ARGS_PARSE_STATUS;

typedef struct settings_t settings_t;

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t*);
void print_usage(void);

#ifdef __cplusplus
}  // extern "C"
#endif


#endif  // _TCC_ARGS_PARSE_H_
