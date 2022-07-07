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


add_library(tgpio STATIC src/tgpio.c)
target_include_directories(tgpio PUBLIC include ${PROJECT_SOURCE_DIR}/include)

add_library(swgpio STATIC src/swgpio.c)
target_include_directories(swgpio PUBLIC include ${PROJECT_SOURCE_DIR}/include)

add_library(time_convert STATIC src/time_convert.c)
target_include_directories(time_convert PUBLIC include ${PROJECT_SOURCE_DIR}/include)

add_library(ui STATIC src/ui.c)
target_include_directories(ui PUBLIC include ${PROJECT_SOURCE_DIR}/include)

add_library(pointer_chasing_workload STATIC src/pointer_chasing.c)
target_include_directories(pointer_chasing_workload PUBLIC include ${PROJECT_SOURCE_DIR}/include)

install (DIRECTORY src DESTINATION ${SAMPLES_INSTALL_PREFIX}/common)
install (DIRECTORY include DESTINATION ${SAMPLES_INSTALL_PREFIX}/common)
install (FILES Makefile DESTINATION ${SAMPLES_INSTALL_PREFIX}/common)
install (FILES README.md DESTINATION ${SAMPLES_INSTALL_PREFIX}/common)

if(NOT BUILD_TESTS)
  return()
endif()

################################################################################
# MOCK
################################################################################
# tgpio helpers mock
add_library(tgpio_mock
  STATIC
    mock/tgpio_mock.cpp
)
target_include_directories(tgpio_mock
  PUBLIC
    mock
    include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(tgpio_mock
  PUBLIC
    tcc_common_mock
)
set_property(TARGET tgpio_mock PROPERTY CXX_STANDARD 11)

# swgpio helpers mock
add_library(swgpio_mock
  STATIC
    mock/swgpio_mock.cpp
)
target_include_directories(swgpio_mock
  PUBLIC
    mock
    include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(swgpio_mock
  PUBLIC
    tcc_common_mock
)
set_property(TARGET swgpio_mock PROPERTY CXX_STANDARD 11)

# open62541 helpers mock
find_package(PkgConfig REQUIRED)
pkg_check_modules(OPEN62541 REQUIRED "open62541-iotg")

add_library(open62541_mock
  STATIC
    mock/open62541_mock.cpp
)
target_include_directories(open62541_mock
  PUBLIC
    mock
    include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_include_directories(open62541_mock
  SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)
target_link_libraries(open62541_mock
  PUBLIC
    tcc_common_mock
)
set_property(TARGET open62541_mock PROPERTY CXX_STANDARD 11)

# pthread helper mock
add_library(pthread_mock
  STATIC
    mock/libc_pthread_mock.cpp
)
target_include_directories(pthread_mock
  PUBLIC
    mock
    include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(pthread_mock
  PUBLIC
    tcc_common_mock
)

# pdatasource helper mock
add_library(pdatasource_mock
  STATIC
    mock/pdatasource_mock.cpp
)
target_include_directories(pdatasource_mock
  PUBLIC
    mock
    include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(pdatasource_mock
  PUBLIC
    tcc_common_mock
)

# json-c helpers mock
add_library(json_c_mock
  STATIC
    mock/json_c_mock.cpp
)
target_include_directories(json_c_mock
  PUBLIC
    mock
    include
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(json_c_mock
  PUBLIC
    tcc_common_mock
)
set_property(TARGET json_c_mock PROPERTY CXX_STANDARD 11)

################################################################################
# TESTS
################################################################################
#------------------------------------------------------------------------------
# Tests for tgpio.c helper
#------------------------------------------------------------------------------
add_host_test(test_tcc_tgpio_helpers_host
    tests/test_tcc_tgpio_helpers_host.cpp
    src/tgpio.c
)
target_include_directories(test_tcc_tgpio_helpers_host
  PRIVATE
    mock
    include
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(test_tcc_tgpio_helpers_host
  PRIVATE
    gtest_depend
    tccsdk_mocks
    gtest_helpers
)

#------------------------------------------------------------------------------
# Tests for swgpio.c helpers
#------------------------------------------------------------------------------
add_host_test(test_tcc_swgpio_helpers_host
    tests/test_tcc_swgpio_helpers_host.cpp
    src/swgpio.c
)
target_compile_definitions(test_tcc_swgpio_helpers_host PRIVATE -Dmain=fake_main)
target_include_directories(test_tcc_swgpio_helpers_host
  PRIVATE
    mock
    include
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(test_tcc_swgpio_helpers_host
  PRIVATE
    gtest_depend
    tccsdk_mocks
    gtest_helpers
)

#------------------------------------------------------------------------------
# Tests for swgpio.c helpers
#------------------------------------------------------------------------------
add_host_test(test_samples_common_host
    tests/test_samples_common_host.cpp
    src/time_convert.c
)
target_include_directories(test_samples_common_host
  PRIVATE
    include
)
target_link_libraries(test_samples_common_host
  PRIVATE
    gtest_depend
    gtest_helpers
)

#------------------------------------------------------------------------------
# Tests for pointer_chasing_workload
#------------------------------------------------------------------------------

add_host_test(test_pointer_chasing_workload_host
    tests/test_pointer_chasing_workload_host.cpp
)
target_include_directories(test_pointer_chasing_workload_host
  PRIVATE
    include
)
target_link_libraries(test_pointer_chasing_workload_host
  PRIVATE
    gtest_depend
    pointer_chasing_workload
)
