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

function(deleteinplace IN_FILE pattern)
    file(READ ${IN_FILE} CONTENTS)
    string(REGEX REPLACE ${pattern} "" STRIPPED ${CONTENTS})
    file(WRITE ${IN_FILE} "${STRIPPED}")
endfunction()

file(GLOB_RECURSE file_list  "${CMAKE_INSTALL_PREFIX}/*/CMakeLists.txt")
foreach(txtfile ${file_list})
  message(STATUS "file for remove CMakeLists.cmake = '${txtfile}'")
  deleteinplace(${txtfile} "include\\(CMakeLists.cmake\\)")
endforeach()
