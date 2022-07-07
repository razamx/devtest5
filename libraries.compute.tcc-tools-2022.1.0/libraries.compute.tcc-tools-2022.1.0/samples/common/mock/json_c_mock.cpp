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

#include "json_c_mock.hpp"

using ::testing::_;
using ::testing::Return;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(json_c_mock)

void json_c_mock::init() {
    ON_CALL(*this, json_object_from_file(_))
        .WillByDefault(Return(nullptr));
}

struct json_object *json_object_from_file(const char *filename) {
    return json_c_mock::get().json_object_from_file(filename);
}
