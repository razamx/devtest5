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

if(BUILD_TESTS)
    add_subdirectory(test_manuals)
endif()

install_dev(PROGRAMS setenv.sh  DESTINATION ${TESTS_INSTALL_PREFIX})
install_dev(PROGRAMS oneapi/run.sh  DESTINATION ${TESTS_INSTALL_PREFIX}/oneapi)

################################################################################
# TESTS
################################################################################
#-------------------------------------------------------------------------------
# Common files
#-------------------------------------------------------------------------------
install_dev(DIRECTORY robot  DESTINATION tests)
install_dev(DIRECTORY util DESTINATION ${TESTS_INSTALL_PREFIX} USE_SOURCE_PERMISSIONS)

# install expected to test package to avoid duplication
install_dev(FILES util/setup_ssram_sbl/config/tgl_u_i7/enable/expected_rtct.json DESTINATION ${TESTS_INSTALL_PREFIX}/robot/data/cc_testing/host_target/tgl-u/SBL)
install_dev(FILES util/setup_ssram_sbl/config/tgl_h_11865/enable/expected_rtct.json DESTINATION ${TESTS_INSTALL_PREFIX}/robot/data/cc_testing/host_target/tgl-h/SBL)

#-------------------------------------------------------------------------------
# Test suites
#-------------------------------------------------------------------------------
install_dev(PROGRAMS test_build_sources_host.robot DESTINATION tests)

#-------------------------------------------------------------------------------
# Argument files
#-------------------------------------------------------------------------------

if(BUILD_TESTS)
    foreach(PLATFORM ${PLATFORMS})
        string(TOLOWER ${PLATFORM} LOWER_PLATFORM_NAME)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/argumentfile_bkc.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/${LOWER_PLATFORM_NAME}/argumentfile_bkc.txt)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/argumentfile_compatible.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/${LOWER_PLATFORM_NAME}/argumentfile_compatible.txt)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/argumentfile_daily.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/${LOWER_PLATFORM_NAME}/argumentfile_daily.txt)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/argumentfile_host_target.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/${LOWER_PLATFORM_NAME}/argumentfile_host_target.txt)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/argumentfile_pre_commit.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/${LOWER_PLATFORM_NAME}/argumentfile_pre_commit.txt)
    endforeach(PLATFORM)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/host/argumentfile_daily.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/host/argumentfile_daily.txt)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/host/argumentfile_compatible.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/host/argumentfile_compatible.txt)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/host/argumentfile_pre_commit.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/host/argumentfile_pre_commit.txt)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/argument_files/host/argumentfile_pv.txt ${CMAKE_INSTALL_PREFIX}/tests/argument_files/host/argumentfile_pv.txt)
endif()

################################################################################
# INSTALL ROBOTFRAMEWORK AND ROBOT TEST LISTS
################################################################################

foreach(PLATFORM ${PLATFORMS})
    install_dev(CODE "execute_process(
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/tests
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/install_testing_utilities.sh ${ALL_ROBOT_TESTS_LIST} ${PLATFORM}
        RESULT_VARIABLE INSTALL_EXTERNALS_STATUS
    )
    if(NOT \\\${INSTALL_EXTERNALS_STATUS} EQUAL 0)
        message(FATAL_ERROR \"FATAL_ERROR: ${CMAKE_CURRENT_SOURCE_DIR}/install_testing_utilities.sh ${ALL_ROBOT_TESTS_LIST} ${PLATFORM} \
        error \\\${INSTALL_EXTERNALS_STATUS}\")
    endif(NOT \\\${INSTALL_EXTERNALS_STATUS} EQUAL 0)")
endforeach(PLATFORM)

install_dev(CODE "execute_process(
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/tests
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/install_testing_utilities.sh ${ALL_ROBOT_TESTS_LIST} HOST
    RESULT_VARIABLE INSTALL_EXTERNALS_STATUS
)
if(NOT \\\${INSTALL_EXTERNALS_STATUS} EQUAL 0)
    message(FATAL_ERROR \"FATAL_ERROR: ${CMAKE_CURRENT_SOURCE_DIR}/install_testing_utilities.sh ${ALL_ROBOT_TESTS_LIST} HOST \
    error \\\${INSTALL_EXTERNALS_STATUS}\")
endif(NOT \\\${INSTALL_EXTERNALS_STATUS} EQUAL 0)")


add_custom_target(bats-core)
add_dependencies(host_tests bats-core)
configure_file(install_bats-core.sh ${CMAKE_BINARY_DIR}/install_bats-core.sh COPYONLY)
add_custom_command(
    TARGET bats-core
    COMMAND bash -x ./install_bats-core.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(bash_shell_mock)
add_dependencies(host_tests bash_shell_mock)
configure_file(install_bash_shell_mock.sh ${CMAKE_BINARY_DIR}/install_bash_shell_mock.sh COPYONLY)
add_custom_command(
    TARGET bash_shell_mock
    COMMAND bash -x ./install_bash_shell_mock.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

macro(add_host_test_bats test_file configurations)

  get_filename_component(add_host_test_bats_test_name ${test_file} NAME)
  get_filename_component(add_host_test_bats_test_directory ${test_file} DIRECTORY)

  add_test(
    NAME ${add_host_test_bats_test_name}
    COMMAND env TCC_LOG_LEVEL=TRACE ${CMAKE_SOURCE_DIR}/tests/run_bats_test.sh ${CMAKE_CURRENT_SOURCE_DIR}/${test_file}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    CONFIGURATIONS host_test host_test_bats ${configurations}
  )

endmacro()
