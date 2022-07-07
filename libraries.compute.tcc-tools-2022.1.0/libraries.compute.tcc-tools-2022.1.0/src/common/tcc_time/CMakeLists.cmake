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

if(NOT BUILD_TESTS)
    return()
endif()

add_library(tcc_time_fake INTERFACE)
target_include_directories(tcc_time_fake INTERFACE mock)
target_link_libraries(tcc_time_fake
  INTERFACE
    tcc_time
    tcc_cpuid_mock
)


set_target_properties(tcc_time PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_host_test(test_tcc_time_host tests/test_tcc_time_host.cpp)
target_link_libraries(test_tcc_time_host
  PRIVATE
    tcc_time
    tcc_cpuid_mock
    gtest_helpers
    gtest_depend
)

