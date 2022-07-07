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

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(log)
add_subdirectory(buffer)

### tcc ###
add_library(tcc SHARED empty_source.c)
set_target_properties(
    tcc
    PROPERTIES
    SOVERSION ${MAJOR_VERSION}
    VERSION ${PROJ_VERSION}
)
target_link_libraries(tcc
PUBLIC
    tcc_cache
PRIVATE
    tcc_log
    -Wl,--whole-archive $<TARGET_FILE:measurement> -Wl,--no-whole-archive
)
add_dependencies(tcc measurement)

target_include_directories(tcc
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
install(TARGETS tcc DESTINATION ${CMAKE_INSTALL_LIBDIR})

install (DIRECTORY ${PROJECT_SOURCE_DIR}/include/tcc DESTINATION include)

add_library(
    tcc_interface
    INTERFACE)

target_include_directories(
    tcc_interface
    INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(measurement)
add_subdirectory(common)

include(CMakeLists.cmake)
