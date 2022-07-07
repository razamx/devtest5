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

set(SAMPLE_NAME tcc_rt_communication_demo)
set(OPCUA_SERVER opcua_server)
set(DATA_MGR tcc_rtcdatamgr)
set(OPCUA_SRV_SRC_PATH src/opcua_pubsub_server)
set(DATA_MGR_SRC_PATH src/data_mgr)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPEN62541 REQUIRED "open62541-iotg")

# Build and install OPC UA* Server
add_executable(
        ${OPCUA_SERVER}
        ${OPCUA_SRV_SRC_PATH}/json_helper.c
        ${OPCUA_SRV_SRC_PATH}/multicallback_server.c
        ${OPCUA_SRV_SRC_PATH}/opcua_common.c
        ${OPCUA_SRV_SRC_PATH}/opcua_publish.c
        ${OPCUA_SRV_SRC_PATH}/opcua_subscribe.c
        ${OPCUA_SRV_SRC_PATH}/opcua_custom.c
        ${OPCUA_SRV_SRC_PATH}/opcua_datasource.c
)
target_include_directories(
        ${OPCUA_SERVER}
        PRIVATE
        ${OPCUA_SRV_SRC_PATH}
        ${DATA_MGR_SRC_PATH}
)
target_include_directories(${OPCUA_SERVER}
        SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

set_source_files_properties(
        ${OPCUA_SRV_SRC_PATH}/json_helper.c
        ${OPCUA_SRV_SRC_PATH}/multicallback_server.c
        ${OPCUA_SRV_SRC_PATH}/opcua_common.c
        ${OPCUA_SRV_SRC_PATH}/opcua_publish.c
        ${OPCUA_SRV_SRC_PATH}/opcua_subscribe.c
        ${OPCUA_SRV_SRC_PATH}/opcua_custom.c
        ${OPCUA_SRV_SRC_PATH}/opcua_datasource.c
        PROPERTIES COMPILE_FLAGS "-Wno-error=discarded-qualifiers -Wno-error=shadow"
)
set_source_files_properties(${OPCUA_SRV_SRC_PATH}/opcua_common.c PROPERTIES COMPILE_OPTIONS "-Wno-error=cast-qual")
set_source_files_properties(${OPCUA_SRV_SRC_PATH}/opcua_datasource.c PROPERTIES COMPILE_OPTIONS "-Wno-error=unused-parameter")

target_link_libraries(
        ${OPCUA_SERVER}
        tcc
        pthread
        ittnotify
        tcc_static
        dl
        rt
        json-c
        ${DATA_MGR}
        ${OPEN62541_LIBRARIES}
)

# open62541 clang-tidy warning -Wincompatible-pointer-types-discards-qualifiers suppress
if(CLANG_COMPATIBLE_FLAGS)
    set_target_properties(${OPCUA_SERVER}
        PROPERTIES COMPILE_FLAGS "-Wno-error=incompatible-pointer-types-discards-qualifiers"
    )
    set_source_files_properties(${DATA_MGR_SRC_PATH}/pointer_chasing_workload.c PROPERTIES COMPILE_OPTIONS "-Wno-error=self-assign")
endif()

# Build and install data manager shared library
add_library(
        ${DATA_MGR}
        SHARED
        ${DATA_MGR_SRC_PATH}/ptimer.c
        ${DATA_MGR_SRC_PATH}/pdatasource.c
        ${DATA_MGR_SRC_PATH}/talker.c
        ${DATA_MGR_SRC_PATH}/listener.c
        ${DATA_MGR_SRC_PATH}/compute.c
        ${DATA_MGR_SRC_PATH}/pointer_chasing_workload.c
)
set_target_properties(
        ${DATA_MGR}
        PROPERTIES
        SOVERSION ${MAJOR_VERSION}
        VERSION ${PROJ_VERSION}
)
target_include_directories(
        ${DATA_MGR}
        PRIVATE
        ${OPCUA_SRV_SRC_PATH}
        ${PROJECT_SOURCE_DIR}/samples/common/include
)
target_compile_definitions(
        ${DATA_MGR} PRIVATE DATA_MGR_TIMER_RT_THREAD
)
target_link_libraries(
        ${DATA_MGR}
        pthread
        rt
        tcc
        ittnotify
        tcc_static
        pointer_chasing_workload
)
install(
        TARGETS
        ${DATA_MGR}
        DESTINATION
        ${CMAKE_INSTALL_LIBDIR}
)
install (
        TARGETS ${OPCUA_SERVER}
        DESTINATION ${BINARIES_INSTALL_PREFIX}
)

# Install sample sources in share
install_sample_sources(${SAMPLE_NAME})
install(
        PROGRAMS
        ${PROJECT_SOURCE_DIR}/samples/${SAMPLE_NAME}/src/scripts/tcc_rt_communication_demo.py
        DESTINATION
        ${SAMPLES_INSTALL_PREFIX}/${SAMPLE_NAME}/src/scripts
)
install_dev(
        PROGRAMS
        tests/test_tcc_rt_communication_demo_target.robot
        DESTINATION
        tests
)
install_dev(
        PROGRAMS
        tests/test_tcc_rt_communication_demo_two_targets.robot
        DESTINATION
        tests
)
install_dev(
        PROGRAMS
        tests/test_output_tcc_rt_communication_demo_target.robot
        DESTINATION
        tests
)

install_symlink(
    "../${SAMPLES_INSTALL_PREFIX}/${SAMPLE_NAME}/src/scripts/${SAMPLE_NAME}.py"
    "${CMAKE_INSTALL_PREFIX}/${BINARIES_INSTALL_PREFIX}/${SAMPLE_NAME}"
)

################################################################################
# TESTS
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_json_helper_c_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_json_helper_c_host.cpp
    ${OPCUA_SRV_SRC_PATH}/json_helper.c
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_json_helper_c_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
)

target_link_libraries(
    test_tcc_rt_communication_demo_opcua_server_json_helper_c_host PRIVATE
    gtest_depend
    json-c
)

################################################################################
# RT Sample: Unit Tests for the json_helper.c with mocks
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_json_helper_mocked_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_json_helper_mocked_host.cpp
    ${OPCUA_SRV_SRC_PATH}/json_helper.c
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_json_helper_mocked_host
    PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_json_helper_mocked_host
    SYSTEM PRIVATE
    ${OPEN62541_INCLUDE_DIRS}
    ${OPCUA_SRV_SRC_PATH}
)

set_target_properties(test_tcc_rt_communication_demo_opcua_server_json_helper_mocked_host
    PROPERTIES COMPILE_FLAGS "-ggdb3"
)

target_link_libraries(test_tcc_rt_communication_demo_opcua_server_json_helper_mocked_host PRIVATE
    gtest_depend
#    pthread_mock
#    open62541_mock
    json-c
    tccsdk_allock_mock
#    ${DATA_MGR}
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server, publisher
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_publish_c_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_publish_c_host.cpp
    ${OPCUA_SRV_SRC_PATH}/opcua_publish.c
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_publish_c_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_publish_c_host
  SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

target_link_libraries(test_tcc_rt_communication_demo_opcua_server_publish_c_host PRIVATE
    gtest_depend
    open62541_mock
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server, subscriber
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_subscribe_c_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_subscribe_c_host.cpp
    ${OPCUA_SRV_SRC_PATH}/opcua_subscribe.c
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_subscribe_c_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_subscribe_c_host
  SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

set_target_properties(test_tcc_rt_communication_demo_opcua_server_subscribe_c_host
    PROPERTIES COMPILE_FLAGS "-ggdb3"
)

target_link_libraries(
    test_tcc_rt_communication_demo_opcua_server_subscribe_c_host PRIVATE
    gtest_depend
    open62541_mock
    tccsdk_mocks
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server, opcua-custom.c utilities
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_custom_c_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_custom_c_host.cpp
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_custom_c_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(test_tcc_rt_communication_demo_opcua_server_custom_c_host
  SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

set_target_properties(test_tcc_rt_communication_demo_opcua_server_custom_c_host
    PROPERTIES COMPILE_FLAGS "-ggdb3"
)

target_link_libraries(test_tcc_rt_communication_demo_opcua_server_custom_c_host PRIVATE
    gtest_depend
    pthread_mock
    open62541_mock
    tccsdk_allock_mock
    ${DATA_MGR}
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server, common
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_common_c_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_common_c_host.cpp
    ${OPCUA_SRV_SRC_PATH}/json_helper.c
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_common_c_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_common_c_host
    SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

target_compile_options(test_tcc_rt_communication_demo_opcua_server_common_c_host PUBLIC
    -O2 #remove extra branches from coverage
)

target_link_libraries(
    test_tcc_rt_communication_demo_opcua_server_common_c_host PRIVATE
    gtest_depend
    open62541_mock
    json-c
    json_c_mock
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server, common, with libcmocks
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_common_ex_c_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_common_ex_c_host.cpp
    ${OPCUA_SRV_SRC_PATH}/json_helper.c
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_common_ex_c_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_common_ex_c_host
    SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

target_compile_options(test_tcc_rt_communication_demo_opcua_server_common_ex_c_host PUBLIC
    -O2 #remove extra branches from coverage
)

target_link_libraries(
    test_tcc_rt_communication_demo_opcua_server_common_ex_c_host PRIVATE
    gtest_depend
    open62541_mock
    json-c
    json_c_mock
    tccsdk_libc_mock
    tccsdk_allock_mock
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server multicallback
################################################################################
add_host_test(test_tcc_rt_communication_demo_opcua_server_multicallback_host
    tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_multicallback_host.cpp
    ${OPCUA_SRV_SRC_PATH}/json_helper.c
    ${OPCUA_SRV_SRC_PATH}/opcua_common.c
    ${OPCUA_SRV_SRC_PATH}/opcua_publish.c
    ${OPCUA_SRV_SRC_PATH}/opcua_subscribe.c
    ${OPCUA_SRV_SRC_PATH}/opcua_datasource.c
)

set_source_files_properties(tests/opcua_server/test_tcc_rt_communication_demo_opcua_server_multicallback_host.cpp PROPERTIES COMPILE_OPTIONS "-Wno-error=write-strings")

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_multicallback_host
    PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(
    test_tcc_rt_communication_demo_opcua_server_multicallback_host
    SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

target_compile_options(test_tcc_rt_communication_demo_opcua_server_multicallback_host PUBLIC
    -O2 #remove extra branches from coverage
)

target_link_libraries(
    test_tcc_rt_communication_demo_opcua_server_multicallback_host PRIVATE
    gtest_depend
    open62541_mock
    json-c
    tcc_static
    json_c_mock
    tccsdk_libc_mock
    tccsdk_allock_mock
)

################################################################################
# RT Sample: Unit Tests for the OPC UA* Server, datasource
################################################################################
add_host_test(test_tcc_rtc_opcua_server_datasource_host
    tests/opcua_server/test_tcc_rtc_opcua_server_datasource_host.cpp
)

target_include_directories(
    test_tcc_rtc_opcua_server_datasource_host PRIVATE
    ${OPCUA_SRV_SRC_PATH}
    ${DATA_MGR_SRC_PATH}
)

target_include_directories(
    test_tcc_rtc_opcua_server_datasource_host
    SYSTEM PRIVATE ${OPEN62541_INCLUDE_DIRS}
)

target_link_libraries(
    test_tcc_rtc_opcua_server_datasource_host PRIVATE
    gtest_depend
    open62541_mock
    pdatasource_mock
)

################################################################################
# TSN RT Timer tests
################################################################################
add_host_test(test_tcc_rt_communication_demo_ptimer_host
    tests/test_tcc_rt_communication_demo_ptimer_host.cpp
)

target_include_directories(
    test_tcc_rt_communication_demo_ptimer_host PRIVATE
    ${DATA_MGR_SRC_PATH}
    ${OPCUA_SRV_SRC_PATH}
)

target_compile_definitions(
    test_tcc_rt_communication_demo_ptimer_host PRIVATE DATA_MGR_TIMER_RT_THREAD
)

target_link_libraries(
    test_tcc_rt_communication_demo_ptimer_host PRIVATE
    gtest_depend
    pthread_mock
    tccsdk_allock_mock
)

################################################################################
# TSN Talker application tests
################################################################################
add_host_test(test_tcc_rt_communication_demo_talker_host
    tests/test_tcc_rt_communication_demo_talker_host.cpp
    ${DATA_MGR_SRC_PATH}/ptimer.c
)

target_include_directories(
    test_tcc_rt_communication_demo_talker_host PRIVATE
    ${DATA_MGR_SRC_PATH}
    ${OPCUA_SRV_SRC_PATH}
)

target_link_libraries(
    test_tcc_rt_communication_demo_talker_host PRIVATE
    gtest_depend
    pthread_mock
    pdatasource_mock
)

################################################################################
# TSN Listener application tests
################################################################################
add_host_test(test_tcc_rt_communication_demo_listener_host
    tests/test_tcc_rt_communication_demo_listener_host.cpp
    ${DATA_MGR_SRC_PATH}/ptimer.c
)

target_include_directories(
    test_tcc_rt_communication_demo_listener_host PRIVATE
    ${DATA_MGR_SRC_PATH}
    ${OPCUA_SRV_SRC_PATH}
)

target_link_libraries(
    test_tcc_rt_communication_demo_listener_host PRIVATE
    gtest_depend
    pthread_mock
)

################################################################################
# TSN Compute application tests (FIXME: update and enable UT section)
################################################################################

add_host_test(test_tcc_rt_communication_demo_compute_host
    tests/test_tcc_rt_communication_demo_compute_host.cpp
    ${DATA_MGR_SRC_PATH}/ptimer.c
)

target_include_directories(
    test_tcc_rt_communication_demo_compute_host PRIVATE
    ${DATA_MGR_SRC_PATH}
    ${OPCUA_SRV_SRC_PATH}
)

target_link_libraries(
    test_tcc_rt_communication_demo_compute_host PRIVATE
    gtest_depend
    pthread_mock
    pdatasource_mock
)

################################################################################
# TSN application tests for the pdatasource
################################################################################
add_host_test(test_tcc_rt_communication_demo_pdatasource_host
    tests/test_tcc_rt_communication_demo_pdatasource_host.cpp
    ${DATA_MGR_SRC_PATH}/pointer_chasing_workload.c
)
target_include_directories(
    test_tcc_rt_communication_demo_pdatasource_host PRIVATE
    ${DATA_MGR_SRC_PATH}
    ${OPCUA_SRV_SRC_PATH}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/samples/common/include
)
set_target_properties(test_tcc_rt_communication_demo_pdatasource_host
    PROPERTIES COMPILE_FLAGS "-ggdb3"
)
target_link_libraries(
    test_tcc_rt_communication_demo_pdatasource_host PRIVATE
    gtest_depend
    pthread_mock
    tcc_cache_mock
    itt_notify_mock
    tccsdk_libc_mock
    pdatasource_mock
    tcc_measurement_mock
    pointer_chasing_workload
    rt
)

################################################################################
# RTC Sample Pointer Chasing Workload Unit Tests
################################################################################
add_host_test(test_tcc_rt_communication_pointer_chasing_host
    tests/test_tcc_rt_communication_pointer_chasing_host.cpp
)
target_include_directories(
    test_tcc_rt_communication_pointer_chasing_host PRIVATE
    ${DATA_MGR_SRC_PATH}
    ${OPCUA_SRV_SRC_PATH}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/buffer/mock
)
target_link_libraries(
    test_tcc_rt_communication_pointer_chasing_host PRIVATE
    gtest_depend
    pthread_mock
    tcc_cache_mock
    pdatasource_mock
    pointer_chasing_workload
)
