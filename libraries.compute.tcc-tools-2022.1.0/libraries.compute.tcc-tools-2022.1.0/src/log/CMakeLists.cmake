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

if(BUILD_TESTS)
    add_host_test(test_tcc_log_host tests/test_tcc_log_host.cpp)

    target_include_directories(test_tcc_log_host PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_tcc_log_host PRIVATE tcc_log tccsdk_mocks gtest_depend gtest_helpers)

    set_target_properties(test_tcc_log_host PROPERTIES COMPILE_FLAGS  "-ggdb3")

endif()
