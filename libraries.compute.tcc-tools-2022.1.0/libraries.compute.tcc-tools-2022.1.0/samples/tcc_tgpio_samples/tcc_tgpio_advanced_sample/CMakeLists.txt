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

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/samples/common/include)

add_executable(tcc_tgpio_advanced_sample src/main.c)
target_link_libraries(tcc_tgpio_advanced_sample PUBLIC swgpio tgpio time_convert)
install (TARGETS tcc_tgpio_advanced_sample DESTINATION ${BINARIES_INSTALL_PREFIX})

install_sample_sources (tcc_tgpio_samples/tcc_tgpio_advanced_sample)
install(DIRECTORY cfg DESTINATION ${SAMPLES_INSTALL_PREFIX}/tcc_tgpio_samples/tcc_tgpio_advanced_sample)

set(SAMPLE_DIR ${PROJECT_SOURCE_DIR}/samples/tcc_tgpio_samples/tcc_tgpio_advanced_sample)

################################################################################
# TESTS
################################################################################

# Tests for entire sample
add_host_test(test_tcc_tgpio_advanced_sample_host
    tests/test_tcc_tgpio_advanced_sample_host.cpp
    ${SAMPLE_DIR}/src/main.c
)
target_compile_definitions(test_tcc_tgpio_advanced_sample_host PRIVATE -Dmain=fake_main)
target_include_directories(test_tcc_tgpio_advanced_sample_host
  PUBLIC
    ${SAMPLE_DIR}/src
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/samples/common/include
)
target_link_libraries(test_tcc_tgpio_advanced_sample_host
  PRIVATE
    gtest_depend
    tccsdk_mocks
    tgpio_mock
    swgpio_mock
    gtest_helpers
    time_convert
)
install_dev(PROGRAMS tests/test_tcc_tgpio_advanced_sample_target.robot DESTINATION tests)
