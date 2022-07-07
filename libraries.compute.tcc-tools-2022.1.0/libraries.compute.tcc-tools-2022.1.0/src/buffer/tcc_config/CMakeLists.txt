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
add_library(tcc_config STATIC
  src/tcc_config.c
  src/tcc_config_finder.c
)

target_include_directories(tcc_config PUBLIC include ${PROJECT_SOURCE_DIR}/src/common)
set_property(TARGET tcc_config PROPERTY POSITION_INDEPENDENT_CODE 1)

target_link_libraries(tcc_config
  PUBLIC
    memory_properties
    json-c
  PRIVATE
    tcc_log
)

include(CMakeLists.cmake)
