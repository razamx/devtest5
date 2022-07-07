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

### tcc_cpuid ###
add_library(tcc_cpuid SHARED src/tcc_cpuid.c)
set_target_properties(
  tcc_cpuid
  PROPERTIES
  SOVERSION ${MAJOR_VERSION}
  VERSION ${PROJ_VERSION}
)
target_include_directories(tcc_cpuid PUBLIC include)
install(TARGETS tcc_cpuid DESTINATION ${CMAKE_INSTALL_LIBDIR})

include(CMakeLists.cmake)
