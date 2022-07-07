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

add_library(bench_measurement STATIC
        src/bench_measurement.c
        src/configure_pmc.c
)

target_link_libraries(bench_measurement
    PRIVATE
        tcc_log
    PUBLIC
        tcc
        tcc_msr
)

target_include_directories(bench_measurement
    PUBLIC
        include
        ${PROJECT_SOURCE_DIR}/src/measurement/src
)

set_target_properties(bench_measurement
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
)

# -----------------------------------------------------------------------------

add_executable(cache_lock_benchmark_v2
        src/cache_lock_benchmark_v2.c
        src/buffer_allocator.c
        src/workload.c
        src/params.c
)

target_link_libraries(cache_lock_benchmark_v2 PUBLIC
        bench_measurement
        region_manager
        pointer_chasing_workload
        tcc_log
)

target_include_directories(cache_lock_benchmark_v2
    PUBLIC
        include
        ${PROJECT_SOURCE_DIR}/src/measurement/src
)

set_target_properties(cache_lock_benchmark_v2
    PROPERTIES COMPILE_FLAGS  "-ggdb3"
)

install_dev(TARGETS cache_lock_benchmark_v2 DESTINATION ${TESTS_BIN_INSTALL_PREFIX})
include(CMakeLists.cmake)