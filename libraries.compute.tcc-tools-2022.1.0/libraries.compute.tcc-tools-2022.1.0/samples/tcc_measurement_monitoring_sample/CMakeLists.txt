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
set(SAMPLE_NAME tcc_measurement_monitoring_sample)
add_executable(${SAMPLE_NAME} src/main.c)
target_link_libraries(${SAMPLE_NAME} tcc)
install_sample_sources(tcc_measurement_monitoring_sample)
install (TARGETS ${SAMPLE_NAME} DESTINATION ${BINARIES_INSTALL_PREFIX})

################################################################################
# TESTS
################################################################################

install_dev (PROGRAMS tests/test_tcc_measurement_monitoring_sample_host.robot DESTINATION tests)
install_dev (PROGRAMS tests/test_tcc_measurement_monitoring_sample_target.robot DESTINATION tests)
install_dev (PROGRAMS tests/test_output_tcc_measurement_monitoring_sample_host.robot DESTINATION tests)
install_dev (PROGRAMS tests/test_output_tcc_measurement_monitoring_sample_target.robot DESTINATION tests)

set(SAMPLE_DIR ${PROJECT_SOURCE_DIR}/samples/tcc_measurement_monitoring_sample)
add_host_test(test_tcc_measurement_monitoring_sample_host
    tests/test_tcc_measurement_monitoring_sample_host.cpp
    ${SAMPLE_DIR}/src/main.c
)
target_compile_definitions(test_tcc_measurement_monitoring_sample_host PRIVATE -Dmain=fake_main)
target_include_directories(test_tcc_measurement_monitoring_sample_host
  PUBLIC
    ${SAMPLE_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/samples/common/include
)
target_link_libraries(test_tcc_measurement_monitoring_sample_host
  PRIVATE
    tcc_measurement_mock
    gtest_depend
    tccsdk_mocks
)

