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

add_executable(rtct_dump
  tests/rtct_dump/rtct_dump.c
)
target_link_libraries(rtct_dump
  PRIVATE
    rtct
    tcc_driver_api
)
target_include_directories(rtct
  PUBLIC
    src
)
install_dev(TARGETS rtct_dump DESTINATION ${TESTS_BIN_INSTALL_PREFIX})

if(BUILD_TESTS)

  add_host_test(test_rtct_host
    tests/test_rtct_host.cpp
  )

  target_link_libraries(test_rtct_host
    PRIVATE
      rtct
      gtest_depend
      gtest_helpers
  )
  set_target_properties(test_rtct_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )

endif()
