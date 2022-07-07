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

set(LOG_IMPL_SOURCES tcc_log.c)
add_library(tcc_log SHARED ${LOG_IMPL_SOURCES})
set_target_properties(
   tcc_log
   PROPERTIES
   SOVERSION ${MAJOR_VERSION}
   VERSION ${PROJ_VERSION}
)
target_include_directories(tcc_log
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(tcc_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tcc_log PRIVATE m)
install(TARGETS tcc_log DESTINATION ${CMAKE_INSTALL_LIBDIR})

include(CMakeLists.cmake)
