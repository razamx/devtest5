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

add_library(tcc_measurement_mock
  STATIC
    mock/tcc_measurement_mock.cpp
)
target_link_libraries(tcc_measurement_mock
  PUBLIC
    tcc_common_mock
)
target_include_directories(tcc_measurement_mock PUBLIC mock include)
set_property(TARGET tcc_measurement_mock PROPERTY CXX_STANDARD 11)

################################################################################
# TESTS
################################################################################

set(TEST_DIR ${CMAKE_INSTALL_PREFIX}/tests)

add_host_test(test_tcc_measurement_itt_host
    tests/test_tcc_measurement_itt_host.cpp
    src/tcc_measurement_itt.c
)
set_target_properties(test_tcc_measurement_itt_host
    PROPERTIES COMPILE_FLAGS  "-DUNIT_TESTS"
)
target_include_directories(test_tcc_measurement_itt_host
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(test_tcc_measurement_itt_host
PUBLIC
   gtest_depend
   tcc_measurement_mock
   tccsdk_mocks
   tcc_log
)
add_target_test(payload_measurement_to_dump_target
    tests/measurement_to_dump.c
)
target_link_libraries(payload_measurement_to_dump_target
PRIVATE
   ittnotify
   tcc_static
   tcc
)
target_include_directories(payload_measurement_to_dump_target
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
install_dev(PROGRAMS tests/test_measurement_to_dump_file_target.robot DESTINATION tests)

#===============================================================================
