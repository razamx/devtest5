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
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/api/dox-api.cfg.in
    ${CMAKE_CURRENT_BINARY_DIR}/dox-api.cfg @ONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/api/header.html
    ${CMAKE_CURRENT_BINARY_DIR}/header.html @ONLY)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/api/patch_script.sh
    ${CMAKE_CURRENT_BINARY_DIR}/patch_script.sh @ONLY)
  add_custom_target(doc  ${ADD_ALL}
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/dox-api.cfg
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  add_custom_command(TARGET doc
    POST_BUILD
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/patch_script.sh
    )
  install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/ DESTINATION ${DOC_INSTALL_PREFIX} OPTIONAL)
endif(DOXYGEN_FOUND)
