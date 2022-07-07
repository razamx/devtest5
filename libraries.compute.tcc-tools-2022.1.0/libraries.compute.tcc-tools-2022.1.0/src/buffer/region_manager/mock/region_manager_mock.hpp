/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef REGION_MANAGER_MOCK_HPP
#define REGION_MANAGER_MOCK_HPP

#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "tcc_region_manager.h"



struct region_manager_mock
{
	MOCK_METHOD1(region_manager_create, region_manager_t*(const tcc_config_t* config));
	MOCK_METHOD1(region_manager_destroy, void(region_manager_t*));
	MOCK_METHOD1(region_manager_count, ssize_t(region_manager_t*));
	MOCK_METHOD2(region_manager_get, const memory_properties_t*(region_manager_t*, size_t));
	MOCK_METHOD3(region_manager_mmap, void*(region_manager_t*, size_t, size_t));
	MOCK_METHOD1(is_tcc_buffer_exists, int(region_manager_t*));

	COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(region_manager_mock)
};

#endif // REGION_MANAGER_MOCK_HPP
