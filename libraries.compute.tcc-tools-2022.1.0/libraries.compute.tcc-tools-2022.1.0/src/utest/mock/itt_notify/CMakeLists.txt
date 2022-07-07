# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/
add_library(itt_notify_mock
    itt_notify_mock.cpp
)
target_link_libraries(itt_notify_mock PUBLIC
    tcc_common_mock
    tcc_log
)
target_include_directories(itt_notify_mock PUBLIC
    include
    fake_include
    .
)