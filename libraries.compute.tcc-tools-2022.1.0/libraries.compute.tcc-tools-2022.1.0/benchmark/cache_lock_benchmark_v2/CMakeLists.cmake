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

add_host_test(test_bench_measurement_host
    src/configure_pmc.c
    tests/test_bench_measurement_host.cpp
)
target_link_options(test_bench_measurement_host
PRIVATE
    -Wl,--defsym=rdpmc=__wrap_rdpmc
    -Wl,--defsym=rdtscp=__wrap_rdtscp
)
target_compile_options(test_bench_measurement_host PUBLIC -ggdb3 -Werror)

target_include_directories(test_bench_measurement_host PRIVATE
    include
    src
    ${PROJECT_SOURCE_DIR}/src/measurement/src
)
target_link_libraries(test_bench_measurement_host PRIVATE
    tcc_msr_mock
    region_manager
    pointer_chasing_workload
    tcc_log
    tcc
    tcc_msr
    gtest_depend
    tccsdk_mocks
    tcc_measurement_mock
    tccsdk_allock_mock
    gtest_helpers
)
#------------------------------------------------------------------------------
add_host_test(test_cache_lock_benchmark_workload_host
    src/workload.c
    src/params.c
    tests/test_cache_lock_benchmark_workload_host.cpp
    tests/test_cache_lock_benchmark_params_host.cpp
)

target_compile_options(test_cache_lock_benchmark_workload_host PUBLIC -ggdb3 -Werror)

target_include_directories(test_cache_lock_benchmark_workload_host PRIVATE
    include
    src
)
target_link_libraries(test_cache_lock_benchmark_workload_host PRIVATE
    bench_measurement
    region_manager
    pointer_chasing_workload
    tcc_log
    gtest_helpers
    gtest_depend
)

