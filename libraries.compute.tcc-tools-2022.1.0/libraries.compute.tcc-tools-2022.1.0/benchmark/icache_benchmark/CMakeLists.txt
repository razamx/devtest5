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

set(SAMPLE_NAME icache_benchmark)
set(CMAKE_BUILD_TYPE Debug)

add_executable(
    ${SAMPLE_NAME}
        main.c)

target_link_libraries(
    ${SAMPLE_NAME}
    PRIVATE
        tcc
        measurement_internal
        tcc_log
    )

target_include_directories(
    ${SAMPLE_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src) # for private measurement functions

include_directories(${PROJECT_SOURCE_DIR}/samples/common/include)

install_dev(TARGETS ${SAMPLE_NAME} DESTINATION ${TESTS_BIN_INSTALL_PREFIX})

