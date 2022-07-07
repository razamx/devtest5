/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#define ERR_MSG(format, ...)                     \
    fprintf(stderr, format "\n", ##__VA_ARGS__); \
    TCC_LOG_ERROR(format, ##__VA_ARGS__);

#define STR_TO_U(X, str, var_name)                                  \
    ({                                                              \
        unsigned long value___ = strtoul(str, NULL, 0);             \
        if (value___ > UINT##X##_MAX) {                             \
            errno = ERANGE;                                         \
            ERR_MSG("Can't parse " #var_name " form '%s'", str);    \
            goto error;                                             \
        }                                                           \
        (var_name) = value___;                                      \
        TCC_LOG_TRACE("Parse value from '%s' for " #var_name, str); \
    })
