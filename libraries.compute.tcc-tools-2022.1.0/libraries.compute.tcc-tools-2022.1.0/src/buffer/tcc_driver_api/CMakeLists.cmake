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

   add_executable(rdrtct
          tests/rdrtct/rdrtct.c
  )
  target_link_libraries(rdrtct
    PRIVATE
      tcc_driver_api
      tcc_log
  )

  install_dev(TARGETS rdrtct DESTINATION ${TESTS_BIN_INSTALL_PREFIX})

  #---------------------------------MOCKS----------------------------------------------
  add_library(tcc_driver_api_mock
    mock/tcc_driver_api_mock.cpp
  )
  target_link_libraries(tcc_driver_api_mock
    PUBLIC
      tcc_common_mock
      memory_properties
      memory_properties_test_utils
    PRIVATE
      tcc_log
  )
  target_include_directories(tcc_driver_api_mock
    PUBLIC
      include
      mock
  )
  set_target_properties(tcc_driver_api_mock
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )

  #---------------------------------TESTS----------------------------------------------
  add_host_test(test_tcc_driver_api_host
      tests/test_tcc_driver_api_host.cpp
  )
  target_compile_definitions(test_tcc_driver_api_host PRIVATE -DSTATIC= )
  target_include_directories(test_tcc_driver_api_host PRIVATE src include)
  target_link_libraries(test_tcc_driver_api_host
    PRIVATE
      gtest_depend
      tccsdk_mocks
      gtest_helpers
      tcc_driver_api
      tcc_time_fake
  )
  set_target_properties(test_tcc_driver_api_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
endif()
