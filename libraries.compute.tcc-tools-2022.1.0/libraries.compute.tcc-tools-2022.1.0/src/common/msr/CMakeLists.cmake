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

add_library(tcc_msr_mock
    mock/tcc_msr_mock.cpp
)
target_link_libraries(tcc_msr_mock PUBLIC
    tcc_common_mock
    tcc_log
)
target_include_directories(tcc_msr_mock PUBLIC
    include
    mock
)

add_host_test(test_tcc_msr_host
  tests/test_tcc_msr_host.cpp
)
target_link_libraries(test_tcc_msr_host PUBLIC
    gtest_depend
    tccsdk_mocks
    tcc_msr
)

target_include_directories(test_tcc_msr_host PUBLIC include)


add_executable(tcc_wrmsr
    tests/rwmsr/tcc_wrmsr.c
)
target_link_libraries(tcc_wrmsr
    tcc_msr
    tcc_log
)
install_dev(TARGETS tcc_wrmsr DESTINATION ${TESTS_BIN_INSTALL_PREFIX})

add_executable(tcc_rdmsr
    tests/rwmsr/tcc_rdmsr.c
)
target_link_libraries(tcc_rdmsr
    tcc_msr
    tcc_log
)
install_dev(TARGETS tcc_rdmsr DESTINATION ${TESTS_BIN_INSTALL_PREFIX})

