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

set(UMM_MALLOC_SOURCES src/umm_malloc.c)
add_library(umm_malloc SHARED ${UMM_MALLOC_SOURCES})
set_target_properties(
   umm_malloc
   PROPERTIES
   SOVERSION ${MAJOR_VERSION}
   VERSION ${PROJ_VERSION}
)

target_include_directories(umm_malloc PUBLIC include)
target_link_libraries(umm_malloc
  PUBLIC
    tcc_log
)
install(TARGETS umm_malloc DESTINATION ${CMAKE_INSTALL_LIBDIR})

include(CMakeLists.cmake)
