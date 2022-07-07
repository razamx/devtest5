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
  add_library(tcc_config_mock STATIC mock/tcc_config_mock.cpp)
  target_include_directories(tcc_config_mock PUBLIC mock include)
  target_link_libraries(tcc_config_mock
    PUBLIC
      tcc_common_mock
      tcc_log
      tcc_driver_api
  )

  add_host_test(test_tcc_config_host
    tests/test_tcc_config.cpp
  )
  target_include_directories(test_tcc_config_host PRIVATE include)
  set_target_properties(test_tcc_config_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(test_tcc_config_host
    PRIVATE
      tcc_config
      gtest_depend
      tccsdk_mocks
      tcc_log
      gtest_helpers
      tcc_driver_api
  )

  #-------------------------------------------------------------------------------

  add_host_test(test_tcc_config_finder_host
    tests/test_tcc_config_finder.cpp
  )
  target_include_directories(test_tcc_config_finder_host PRIVATE include)
  set_target_properties(test_tcc_config_finder_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(test_tcc_config_finder_host
    PRIVATE
      tcc_config
      gtest_depend
      tccsdk_mocks
      tcc_log
  )
endif()