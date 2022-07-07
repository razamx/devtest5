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


################################################################################
# MOCK
################################################################################

add_library(umm_malloc_mock
  STATIC
    mock/umm_malloc_mock.cpp
)

target_include_directories(umm_malloc_mock PUBLIC mock include)
target_link_libraries(umm_malloc_mock
  PUBLIC
    tcc_common_mock
)
set_property(TARGET umm_malloc_mock PROPERTY CXX_STANDARD 11)

################################################################################
# TESTS
################################################################################

add_host_test(test_umm_malloc_host
    tests/test_umm_malloc_host.cpp
    src/umm_malloc.c
)
target_include_directories(test_umm_malloc_host
  PUBLIC
    include
)
target_link_libraries(test_umm_malloc_host
  PRIVATE
    gtest_depend
    tccsdk_mocks
    tcc_log
)
