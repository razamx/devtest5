# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

add_subdirectory(umm_malloc)
add_subdirectory(memory_properties)
add_subdirectory(tcc_driver_api)
add_subdirectory(tcc_config)
add_subdirectory(region_manager)
add_subdirectory(rtct)

#-------------------------------------------------------------------------------

add_library(tcc_cache SHARED
  src/cache.c
  src/dram_allocator.c
  src/region_allocator.c
  src/allocators_manager.c
)
set_target_properties(
   tcc_cache
   PROPERTIES
   SOVERSION ${MAJOR_VERSION}
   VERSION ${PROJ_VERSION}
)
target_include_directories(tcc_cache
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  PRIVATE
    include
)
target_link_libraries(tcc_cache
  PUBLIC
    umm_malloc
    region_manager
    tcc_config
)
install(TARGETS tcc_cache DESTINATION ${CMAKE_INSTALL_LIBDIR})

include(CMakeLists.cmake)
