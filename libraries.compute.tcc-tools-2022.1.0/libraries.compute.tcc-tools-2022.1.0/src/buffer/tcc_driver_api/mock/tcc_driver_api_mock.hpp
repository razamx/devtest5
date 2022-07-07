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

#ifndef TCC_DRIVER_API_MOCK_HPP
#define TCC_DRIVER_API_MOCK_HPP
#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "tcc_driver_api.h"

struct tcc_driver_api_mock
{
	MOCK_METHOD0(tcc_driver_get_region_count, int(void));
	MOCK_METHOD2(tcc_driver_get_memory_config, int(unsigned int region_id,  memory_properties_t* out));
	MOCK_METHOD2(tcc_driver_req_buffer, int(unsigned int region_id, unsigned int size));
	MOCK_METHOD2(tcc_driver_read_rtct, int(void** rtct, size_t* size));

	COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(tcc_driver_api_mock)
};

#endif // TCC_DRIVER_API_MOCK_HPP
