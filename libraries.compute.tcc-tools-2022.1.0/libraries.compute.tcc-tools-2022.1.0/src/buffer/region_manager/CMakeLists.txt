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

add_library(region_manager STATIC
  src/tcc_region_manager.c
  src/tcc_region_manager_common.c
  src/tcc_region_manager_driver.c
  src/tcc_region_manager_default.c
)
target_include_directories(region_manager PUBLIC include)
set_property(TARGET region_manager PROPERTY POSITION_INDEPENDENT_CODE 1)

target_link_libraries(region_manager
  PUBLIC
    tcc_config
    memory_properties
  PRIVATE
    tcc_driver_api 
    tcc_log
    tcc_time
    rtct
)

include(CMakeLists.cmake)
