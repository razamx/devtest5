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

install (FILES Makefile DESTINATION ${SAMPLES_INSTALL_PREFIX}/tcc_tgpio_samples)
install (FILES README.md DESTINATION ${SAMPLES_INSTALL_PREFIX}/tcc_tgpio_samples)

add_subdirectory(tcc_tgpio_advanced_sample)
add_subdirectory(tcc_tgpio_basic_samples)
add_subdirectory(tcc_tgpio_frequency_discipline_sample)
add_subdirectory(tests)
