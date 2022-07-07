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

# Install install_host_dependencies.sh script
if(HOST_STRUCTURE)
  if(PACKAGE_TYPE STREQUAL "PUBLIC")
    install(PROGRAMS install_host_dependencies.sh DESTINATION ${SCRIPTS_INSTALL_PREFIX})
  else(PACKAGE_TYPE STREQUAL "PUBLIC")
    install(PROGRAMS install_host_dependencies_nda.sh DESTINATION ${SCRIPTS_INSTALL_PREFIX} RENAME install_host_dependencies.sh)
  endif(PACKAGE_TYPE STREQUAL "PUBLIC")
else(HOST_STRUCTURE)
  add_subdirectory(setup_ssram)
#  add_subdirectory(setup_ssram_v1)
endif(HOST_STRUCTURE)

# NOTE: Script and configs is required on host for Cache Configuator
# host target tests. Workaround to install it.
if (BUILD_TESTS)
  install_dev(DIRECTORY setup_ssram DESTINATION
    ${TESTS_INSTALL_PREFIX}/scripts)
#  install_dev(DIRECTORY setup_ssram_v1 DESTINATION
#    ${TESTS_INSTALL_PREFIX}/scripts)
endif()
