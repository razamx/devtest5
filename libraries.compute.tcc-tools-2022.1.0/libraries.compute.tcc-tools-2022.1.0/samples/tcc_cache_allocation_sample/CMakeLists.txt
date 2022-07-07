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

set(SAMPLE_NAME tcc_cache_allocation_sample)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/samples/common/include)
add_executable(${SAMPLE_NAME} src/main.c)
target_link_libraries(${SAMPLE_NAME} tcc ittnotify tcc_static pthread pointer_chasing_workload)
install (TARGETS ${SAMPLE_NAME} DESTINATION ${BINARIES_INSTALL_PREFIX})

install_sample_sources(${SAMPLE_NAME})
install (FILES latencies.png DESTINATION ${SAMPLES_INSTALL_PREFIX}/tcc_cache_allocation_sample)

#-------------------------------------------------------------------------------
# Test suites
#-------------------------------------------------------------------------------

install_dev(PROGRAMS tests/test_tcc_cache_allocation_sample_host.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_cache_allocation_sample_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_output_tcc_cache_allocation_sample_host.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_output_tcc_cache_allocation_sample_target.robot DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_cache_allocation_sample_perf_regr_target.robot  DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_cache_allocation_sample_latency_target.robot  DESTINATION tests)
install_dev(PROGRAMS tests/test_tcc_cache_allocation_sample_cpu_core_target.robot  DESTINATION tests)

include(CMakeLists.cmake)
