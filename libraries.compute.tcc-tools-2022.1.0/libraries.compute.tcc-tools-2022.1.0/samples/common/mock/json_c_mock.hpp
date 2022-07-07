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

#ifndef JSON_C_MOCK_H
#define JSON_C_MOCK_H

#include "mock_common.hpp"
#include "gmock/gmock.h"

#include <json-c/json.h>

struct json_c_mock {
    MOCK_METHOD1(json_object_from_file, struct json_object *(const char *filename));

    COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(json_c_mock);
};

#endif // JSON_C_MOCK_H
