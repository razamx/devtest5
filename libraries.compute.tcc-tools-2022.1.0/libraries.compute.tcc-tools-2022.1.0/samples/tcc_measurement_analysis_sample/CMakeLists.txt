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

set(SAMPLE_NAME tcc_measurement_analysis_sample)

add_target_test(
   payload_syntetic_multiple_measurement_instances_target
   tests/syntetic_multiple_measurement_instances_test.c)

target_link_libraries(
    payload_syntetic_multiple_measurement_instances_target
    PRIVATE
        tcc tcc_static ittnotify)

set_target_properties(
    payload_syntetic_multiple_measurement_instances_target
    PROPERTIES COMPILE_FLAGS  "-Wno-error")

target_include_directories(
    payload_syntetic_multiple_measurement_instances_target
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include)

install_sample_sources(tcc_measurement_analysis_sample)

# Separately install sample to add execytable rights for it.
install (PROGRAMS ${PROJECT_SOURCE_DIR}/samples/${SAMPLE_NAME}/src/${SAMPLE_NAME}.py
	DESTINATION ${SAMPLES_INSTALL_PREFIX}/${SAMPLE_NAME}/src)

install_symlink(
    "../${SAMPLES_INSTALL_PREFIX}/${SAMPLE_NAME}/src/${SAMPLE_NAME}.py"
    "${CMAKE_INSTALL_PREFIX}/${BINARIES_INSTALL_PREFIX}/${SAMPLE_NAME}"
)

install_dev(PROGRAMS tests/test_tcc_measurement_analysis_sample_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_measurement_analysis_sample_hist_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_measurement_analysis_sample_monitor_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_output_tcc_measurement_analysis_sample_host.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_output_tcc_measurement_analysis_sample_target.robot DESTINATION tests)
