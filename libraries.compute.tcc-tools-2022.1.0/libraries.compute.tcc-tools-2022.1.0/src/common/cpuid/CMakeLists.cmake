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

add_library(tcc_cpuid_mock STATIC mock/src/tcc_cpuid_mock.cpp)
target_include_directories(tcc_cpuid_mock PUBLIC mock/include)
target_link_libraries(tcc_cpuid_mock
  PUBLIC
    tcc_common_mock
    tcc_cpuid
)

add_executable(read_cpuid src/read_cpuid.c)
target_link_libraries(read_cpuid PRIVATE tcc_cpuid)
install_dev(TARGETS read_cpuid DESTINATION ${TESTS_BIN_INSTALL_PREFIX})
