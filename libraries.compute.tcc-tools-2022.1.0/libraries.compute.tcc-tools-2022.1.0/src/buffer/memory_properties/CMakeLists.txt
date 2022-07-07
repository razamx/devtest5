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

add_library(memory_properties STATIC
    src/memory_properties.c
)

target_link_libraries(memory_properties
  PRIVATE
    tcc_log
    tcc_time
)
set_property(TARGET memory_properties PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(memory_properties PUBLIC include)

include(CMakeLists.cmake)
