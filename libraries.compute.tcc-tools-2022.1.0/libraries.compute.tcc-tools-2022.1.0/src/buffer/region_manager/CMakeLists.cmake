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

  add_library(region_manager_mock STATIC mock/region_manager_mock.cpp)
  target_include_directories(region_manager_mock PUBLIC mock include)
  target_link_libraries(region_manager_mock
    PUBLIC
      tcc_common_mock
      tcc_driver_api
      tcc_config
  )
  
  #-------------------------------------------------------------------------------

  add_host_test(test_tcc_region_manager_driver_host
    tests/test_tcc_region_manager_driver_host.cpp
    src/tcc_region_manager_common.c
    src/tcc_region_manager_driver.c
  )
  target_include_directories(test_tcc_region_manager_driver_host PRIVATE src include)
  target_link_libraries(test_tcc_region_manager_driver_host
    PRIVATE
      gtest_depend
      gtest_helpers
      tcc_driver_api_mock
      tcc_config
      tccsdk_mocks
      tcc
      rtct
      tcc_time_fake
  )
  set_target_properties(test_tcc_region_manager_driver_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )

  #-------------------------------------------------------------------------------

  add_host_test(test_tcc_region_manager_default_host
    src/tcc_region_manager_default.c
    src/tcc_region_manager_common.c
    tests/test_tcc_region_manager_default_host.cpp
  )
  target_include_directories(test_tcc_region_manager_default_host PRIVATE include src)
  set_target_properties(test_tcc_region_manager_default_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(test_tcc_region_manager_default_host
    PRIVATE
      gtest_depend
      gtest_helpers
      tccsdk_mocks
      tcc_config_mock
      memory_properties
      tcc_log
      tcc_time_fake
  )

  #-------------------------------------------------------------------------------

  add_host_test(test_tcc_buffer_lock_method_autodetect_host
    src/tcc_region_manager.c
    tests/test_tcc_buffer_lock_method_autodetect.cpp
  )
  set_target_properties(test_tcc_buffer_lock_method_autodetect_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )

  target_include_directories(test_tcc_buffer_lock_method_autodetect_host
    PRIVATE
      tests
      include
      src
  )
  target_link_libraries(test_tcc_buffer_lock_method_autodetect_host
    PRIVATE
        tcc_buffer_test_helpers
        tcc_driver_api_mock
        tcc_config
  )

  #-------------------------------------------------------------------------------
  # TARGET_TESTS
  #-------------------------------------------------------------------------------
  add_library(cls_region_manager STATIC
    tests/region_manager.cpp
  )
  target_include_directories(cls_region_manager
  PUBLIC
    tests
  )
  set_target_properties(cls_region_manager
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(cls_region_manager
  PUBLIC
      region_manager
  PRIVATE 
      tcc_log
  )

  #-------------------------------------------------------------------------------
  add_target_test(payload_tcc_region_manager_close_handler_target
    tests/payload_tcc_region_manager_close_handler_target.cpp
  )
  target_include_directories(payload_tcc_region_manager_close_handler_target
    PRIVATE
      include
      src
  )
  set_target_properties(payload_tcc_region_manager_close_handler_target
    PROPERTIES COMPILE_FLAGS  "-ggdb3 -std=c++17"
  )
  target_link_libraries(payload_tcc_region_manager_close_handler_target
    PRIVATE
        cls_region_manager
        bench_measurement
        pointer_chasing_workload
        tcc_log
        tcc_time
        gtest_depend
        gtest_helpers
  )

  #-------------------------------------------------------------------------------
  add_target_test(payload_tcc_region_manager_driver_allocator_target
    tests/payload_tcc_region_manager_driver_allocator_target.cpp
  )
  target_include_directories(payload_tcc_region_manager_driver_allocator_target
    PRIVATE
      include
      src
  )
  set_target_properties(payload_tcc_region_manager_driver_allocator_target
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(payload_tcc_region_manager_driver_allocator_target
    PRIVATE
        cls_region_manager
        pointer_chasing_workload
        tcc_log
        gtest_depend
  )

  #-------------------------------------------------------------------------------
  add_target_test(payload_tcc_region_manager_performance_target
    tests/payload_tcc_region_manager_performance_target.cpp
  )
  target_include_directories(payload_tcc_region_manager_performance_target
    PRIVATE
      include
      src
  )
  set_target_properties(payload_tcc_region_manager_performance_target
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  target_link_libraries(payload_tcc_region_manager_performance_target
    PRIVATE
        cls_region_manager
        bench_measurement
        pointer_chasing_workload
        tcc_log
        tcc_time
        gtest_depend
  )
  #-------------------------------------------------------------------------------
  
  install_dev(PROGRAMS tests/test_tcc_region_manager_target.robot DESTINATION tests)
  
endif()
