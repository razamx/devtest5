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

include_directories(${PROJECT_SOURCE_DIR}/samples/common/include)
set(SAMPLE_NAME tcc_ethernet_timestamps_sample)
add_executable(${SAMPLE_NAME} src/main.c)
target_link_libraries(${SAMPLE_NAME} PUBLIC tgpio time_convert ui)
install(TARGETS ${SAMPLE_NAME} DESTINATION ${BINARIES_INSTALL_PREFIX})

set(SAMPLE_DIR ${PROJECT_SOURCE_DIR}/samples/tcc_ethernet_timestamps_sample)

install_sample_sources(tcc_ethernet_timestamps_sample)
install(PROGRAMS ${SAMPLE_DIR}/start_synchronization.sh DESTINATION ${SAMPLES_INSTALL_PREFIX}/${SAMPLE_NAME})
install_symlink(
	"../${SAMPLES_INSTALL_PREFIX}/${SAMPLE_NAME}/start_synchronization.sh"
	"${CMAKE_INSTALL_PREFIX}/${BINARIES_INSTALL_PREFIX}/tcc_ethernet_sample_start_synchronization"
)

################################################################################
# TESTS
################################################################################

add_host_test(test_tcc_ethernet_timestamps_sample_host
    tests/test_tcc_ethernet_timestamps_sample_host.cpp
    ${SAMPLE_DIR}/src/main.c
)
target_compile_definitions(test_tcc_ethernet_timestamps_sample_host PRIVATE -Dmain=fake_main)
target_include_directories(test_tcc_ethernet_timestamps_sample_host
  PUBLIC
    ${SAMPLE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/samples/common/include
)
target_link_libraries(test_tcc_ethernet_timestamps_sample_host
  PRIVATE
    time_convert
    ui
    tgpio_mock
    gtest_depend
    tccsdk_mocks
    gtest_helpers
)
install_dev(PROGRAMS tests/test_tcc_ethernet_timestamps_sample_target.robot DESTINATION tests)
