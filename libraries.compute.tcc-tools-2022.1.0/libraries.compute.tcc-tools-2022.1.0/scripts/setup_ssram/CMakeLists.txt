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

get_filename_component(SCRIPT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(SCRIPT_DIR "${SCRIPTS_INSTALL_PREFIX}/${SCRIPT_NAME}")

install(DIRECTORY config
        DESTINATION ${SCRIPT_DIR})
install(PROGRAMS tcc_setup_ssram.sh DESTINATION ${SCRIPT_DIR})
install(PROGRAMS control_rtcm_uefi.sh DESTINATION ${SCRIPT_DIR})
install(PROGRAMS control_rtcm_sbl.sh DESTINATION ${SCRIPT_DIR})
install(PROGRAMS control_tcc_driver.sh DESTINATION ${SCRIPT_DIR})
install(PROGRAMS cache_configuration_comparator.py DESTINATION ${SCRIPT_DIR})
install(PROGRAMS select_config.py DESTINATION ${SCRIPT_DIR})
install(PROGRAMS apply_capsule_uefi.sh DESTINATION ${SCRIPT_DIR})
install(FILES README.md DESTINATION ${SCRIPT_DIR})
install(FILES import_helper.py DESTINATION ${SCRIPT_DIR})

# install expected to test package to avoid duplication
install_dev(FILES config/tgl_u_i7/enable/expected_rtct.json DESTINATION ${TESTS_INSTALL_PREFIX}/robot/data/cc_testing/host_target/tgl-u/UEFI)
install_dev(FILES config/tgl_h_11865/enable/expected_rtct.json DESTINATION ${TESTS_INSTALL_PREFIX}/robot/data/cc_testing/host_target/tgl-h/UEFI)
