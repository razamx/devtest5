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
  add_library(memory_properties_test_utils INTERFACE)
  target_include_directories(memory_properties_test_utils INTERFACE tests)
  target_link_libraries(memory_properties_test_utils INTERFACE memory_properties)
  #-------------------------------------------------------------------------------
  add_host_test(test_memory_properties_host
    tests/test_memory_properties_host.cpp
  )
  set_target_properties(test_memory_properties_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(test_memory_properties_host
    PRIVATE
      gtest_depend
      tcc_log
      memory_properties
  )
endif()