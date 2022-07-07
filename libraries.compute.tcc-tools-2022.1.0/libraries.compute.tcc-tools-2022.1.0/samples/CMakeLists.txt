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


if(DEFINED NO_TCC_MEASUREMENT)
  add_definitions(-Wno-unused-variable -Wno-unused-const-variable)
endif()

add_subdirectory(common)
add_subdirectory(plot_scripts)

add_subdirectory(tcc_cache_allocation_sample)
add_subdirectory(tcc_ethernet_timestamps_sample)
add_subdirectory(tcc_measurement_monitoring_sample)
add_subdirectory(tcc_measurement_analysis_sample)
add_subdirectory(tcc_multiple_measurements_sample)
add_subdirectory(tcc_single_measurement_sample)
add_subdirectory(tcc_tgpio_samples)
add_subdirectory(tcc_rt_communication_demo)

add_subdirectory(tests)
