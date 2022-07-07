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

cmake_minimum_required(VERSION 3.0)

add_library(measurement STATIC src/tcc_measurement.c)
target_include_directories(measurement
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
  PRIVATE
    include
)
set_target_properties(measurement PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(measurement
  PRIVATE
    tcc_log
)

add_library(measurement_internal INTERFACE)
target_include_directories(measurement_internal INTERFACE include)
target_link_libraries(measurement_internal
  INTERFACE
    tcc
)

### tcc_static ###
add_library(tcc_static STATIC src/tcc_measurement_static.c)
target_link_libraries(tcc_static PRIVATE dl)
set_property(TARGET tcc_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories( tcc_static
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include)
install(TARGETS tcc_static DESTINATION ${CMAKE_INSTALL_LIBDIR})

### tcc_collector ###
add_library(tcc_collector SHARED src/tcc_measurement_itt.c)
set_target_properties(
    tcc_collector
    PROPERTIES
    SOVERSION ${MAJOR_VERSION}
    VERSION ${PROJ_VERSION}
)
target_link_libraries(tcc_collector PRIVATE tcc tcc_log measurement_internal)
install(TARGETS tcc_collector DESTINATION ${CMAKE_INSTALL_LIBDIR})

include(CMakeLists.cmake)
