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


if(BUILD_TESTS)

    add_host_test(test_tcc_cache_allocation_sample_host
        tests/test_tcc_cache_allocation_sample_host.cpp
    )

    target_link_libraries(test_tcc_cache_allocation_sample_host
        PUBLIC
            itt_notify_mock
    )

    target_include_directories(test_tcc_cache_allocation_sample_host
        PUBLIC
            src
            include
    )

    target_link_libraries(test_tcc_cache_allocation_sample_host
        PRIVATE
            pthread
            pointer_chasing_workload
            gtest_depend
            gtest_helpers
            tcc
            itt_notify_mock
            tcc_static
            dl
    )

endif()
