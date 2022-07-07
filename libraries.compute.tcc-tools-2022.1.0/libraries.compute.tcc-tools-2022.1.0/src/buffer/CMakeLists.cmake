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


add_executable(lspsram
        tests/lspsram/lspsram.c
)
target_link_libraries(lspsram
  PRIVATE
    region_manager
    tcc_log
)

install_dev(TARGETS lspsram DESTINATION ${TESTS_BIN_INSTALL_PREFIX})


if(BUILD_TESTS)
  ################################################################################
  # MOCK
  ################################################################################

  #-------------------------------------------------------------------------------

  add_library(allocator_wrappers_mock STATIC mock/allocator_wrappers_mock.cpp)
  target_include_directories(allocator_wrappers_mock PUBLIC mock include)
  target_link_libraries(allocator_wrappers_mock PUBLIC tcc_common_mock tcc_log umm_malloc)

  add_library(tcc_cache_mock STATIC mock/tcc_cache_mock.cpp)
  target_include_directories(tcc_cache_mock PUBLIC mock include)
  target_link_libraries(tcc_cache_mock PUBLIC tcc_common_mock tcc_log)

  ################################################################################
  # TESTS
  ################################################################################
  #-------------------------------------------------------------------------------
  # tcc_buffer_test_helpers
  #-------------------------------------------------------------------------------

  add_library(tcc_buffer_test_helpers INTERFACE)
  target_include_directories(tcc_buffer_test_helpers
    INTERFACE
      src
      include
      tests
  )
   target_link_libraries(tcc_buffer_test_helpers
    INTERFACE
      tcc_log
      tcc_cache
      umm_malloc
      gtest_depend
      gtest_helpers
      tccsdk_mocks
      memory_properties_test_utils
      tcc_driver_api_mock
   )
  #-------------------------------------------------------------------------------
  # TARGET TESTS
  #-------------------------------------------------------------------------------

  install_dev(PROGRAMS tests/test_buffer_api_target.robot DESTINATION tests)

  add_target_test(payload_test_buffer_api_target
    tests/payload_test_buffer_api_target.cpp
  )
  target_include_directories(payload_test_buffer_api_target
    PUBLIC
      ${PROJECT_SOURCE_DIR}/include
  )
  target_link_libraries(payload_test_buffer_api_target
    PRIVATE
      gtest_depend
      gtest_helpers
      tcc
  )

  #-------------------------------------------------------------------------------
  # HOST TESTS
  #-------------------------------------------------------------------------------

  add_host_test(test_buffer_host
    src/cache.c
    src/allocators_manager.c
    src/dram_allocator.c
    src/region_allocator.c
    tests/test_buffer_host.cpp
  )
  target_compile_definitions(test_buffer_host PRIVATE )
  target_include_directories(test_buffer_host PRIVATE include)
  target_link_libraries(test_buffer_host
    PRIVATE
      tcc_log
      umm_malloc
      gtest_depend
      tccsdk_mocks
      region_manager_mock
      gtest_helpers
      memory_properties
      tcc_config
  )
      
  set_target_properties(test_buffer_host
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
  )
  #-------------------------------------------------------------------------------

  add_host_test(test_allocators_manager_host
    src/allocators_manager.c
    tests/test_allocators_manager_host.cpp
  )
  target_include_directories(test_allocators_manager_host PRIVATE include)
  target_link_libraries(test_allocators_manager_host
    PRIVATE
      tcc_config
      tcc_log
      gtest_depend
      tccsdk_mocks
      region_manager_mock
      umm_malloc_mock
      allocator_wrappers_mock
      memory_properties_test_utils
  )

  #-------------------------------------------------------------------------------

  add_host_test(test_tcc_driver_host
    tests/test_tcc_driver.cpp
  )
  target_include_directories(test_tcc_driver_host PRIVATE tests include)
  target_link_libraries(test_tcc_driver_host
    PRIVATE
      tcc_buffer_test_helpers
      tcc_driver_api_mock
  )
  set_target_properties(test_tcc_driver_host PROPERTIES COMPILE_FLAGS  "-ggdb3 -std=c++17")

  #-------------------------------------------------------------------------------

  add_target_test(test_tcc_driver_target
    tests/test_tcc_driver.cpp
  )
  target_compile_definitions(test_tcc_driver_target
    PRIVATE -DTCC_BUFFER_DRIVER_TEST_TARGET
  )
  target_include_directories(test_tcc_driver_target PRIVATE tests include)
  target_link_libraries(test_tcc_driver_target
    PRIVATE
      tcc_log
      tcc_cache
      tcc_buffer_test_helpers
      tcc_driver_api_mock
  )

  set_target_properties(test_tcc_driver_target PROPERTIES COMPILE_FLAGS  "-ggdb3 -std=c++17")

  #-------------------------------------------------------------------------------
  #Get the sources list from target tcc_driver_api
  get_target_property(TCC_DRIVER_API_SOURCE_DIR tcc_driver_api SOURCE_DIR)
  get_target_property(TCC_DRIVER_API_SOURCES tcc_driver_api SOURCES)
  set(TCC_DRIVER_API_SOURCES_PATH "")
  foreach(SOURCE_FILE ${TCC_DRIVER_API_SOURCES})
    string(APPEND TCC_DRIVER_API_SOURCES_PATH ${TCC_DRIVER_API_SOURCE_DIR}/${SOURCE_FILE})
  endforeach()

  add_host_test(test_tcc_buffer_e2e_host
    tests/test_tcc_buffer_e2e_host.cpp
    src/cache.c
    src/dram_allocator.c
    src/region_allocator.c
    src/allocators_manager.c
    ${TCC_DRIVER_API_SOURCES_PATH} # need to provide wrapped libc functions to TCC driver API
  )
  target_include_directories(test_tcc_buffer_e2e_host PRIVATE tests include)
  target_compile_definitions(test_tcc_buffer_e2e_host PRIVATE USE_MOCKS -DSTATIC=)
  # FIXME: workaround because our cmake version doesn't support CXX_STANDART 17
  target_compile_options(test_tcc_buffer_e2e_host PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
  target_link_libraries(test_tcc_buffer_e2e_host
    PRIVATE
      region_manager
      tcc_buffer_test_helpers
      tcc_config
      tcc_time_fake
  )
endif()
