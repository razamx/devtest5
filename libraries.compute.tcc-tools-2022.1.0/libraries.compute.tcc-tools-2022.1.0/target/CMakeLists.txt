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

install(PROGRAMS tcc_setup.py DESTINATION ${TARGET_INSTALL_PREFIX})
install(FILES tcc_setup.md DESTINATION ${TARGET_INSTALL_PREFIX})
install(DIRECTORY configs DESTINATION ${TARGET_INSTALL_PREFIX})

install_dev(PROGRAMS tests/test_tcc_setup_validation_host.robot DESTINATION tests)
