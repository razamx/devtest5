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

# Basic oneshot output sample
add_executable(tcc_tgpio_basic_oneshot_output src/tgpio_basic_oneshot_output.c)
target_link_libraries(tcc_tgpio_basic_oneshot_output tgpio)
install (TARGETS tcc_tgpio_basic_oneshot_output DESTINATION ${BINARIES_INSTALL_PREFIX})

# Basic periodic output sample
add_executable(tcc_tgpio_basic_periodic_output src/tgpio_basic_periodic_output.c)
install (TARGETS tcc_tgpio_basic_periodic_output DESTINATION ${BINARIES_INSTALL_PREFIX})

# Info sample
add_executable(tcc_tgpio_info src/tgpio_info.c)
install (TARGETS tcc_tgpio_info DESTINATION ${BINARIES_INSTALL_PREFIX})

# Input sample
add_executable(tcc_tgpio_basic_input src/tgpio_basic_input.c)
target_link_libraries(tcc_tgpio_basic_input PRIVATE tgpio)
install (TARGETS tcc_tgpio_basic_input DESTINATION ${BINARIES_INSTALL_PREFIX})

# Install whole sources
install_sample_sources(tcc_tgpio_samples/tcc_tgpio_basic_samples)


# Tests
if(NOT BUILD_TESTS)
  return()
endif()

install_dev(PROGRAMS tests/test_tcc_tgpio_basic_oneshot_output_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_tgpio_basic_periodic_output_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_tgpio_info_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_tgpio_basic_input_target.robot DESTINATION tests)

add_host_test(test_tcc_tgpio_basic_input_host
    tests/test_tcc_tgpio_basic_input_host.cpp
)
target_include_directories(test_tcc_tgpio_basic_input_host PUBLIC src)
target_link_libraries(test_tcc_tgpio_basic_input_host
  PRIVATE
    gtest_depend
    gtest_helpers
    tccsdk_mocks
)

add_host_test(test_tcc_tgpio_basic_oneshot_output_host
    tests/test_tcc_tgpio_basic_oneshot_output_host.cpp
)
target_include_directories(test_tcc_tgpio_basic_oneshot_output_host PUBLIC src)
target_link_libraries(test_tcc_tgpio_basic_oneshot_output_host
  PRIVATE
    gtest_depend
    gtest_helpers
    tccsdk_mocks
)

add_host_test(test_tcc_tgpio_basic_periodic_output_host
    tests/test_tcc_tgpio_basic_periodic_output_host.cpp
)
target_include_directories(test_tcc_tgpio_basic_periodic_output_host PUBLIC src)
target_link_libraries(test_tcc_tgpio_basic_periodic_output_host
  PRIVATE
    gtest_depend
    gtest_helpers
    tccsdk_mocks
)

add_host_test(test_tcc_tgpio_info_host
    tests/test_tcc_tgpio_info_host.cpp
)
target_include_directories(test_tcc_tgpio_info_host PUBLIC src)
target_link_libraries(test_tcc_tgpio_info_host
  PRIVATE
    gtest_depend
    gtest_helpers
    tccsdk_mocks
)
