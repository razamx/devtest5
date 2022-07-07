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

#include <gmock/gmock.h>
#include "mock_common.hpp"
#include "tcc_config.h"

struct tcc_config_mock
{
	MOCK_METHOD1(tcc_config_count, size_t(const tcc_config_t* config));
	MOCK_METHOD2(tcc_config_get, const tcc_config_item_t*(const tcc_config_t* config, size_t index));
	MOCK_METHOD1(tcc_config_read, tcc_config_t*(const char* filename));
    MOCK_METHOD1(tcc_config_destroy, void(tcc_config_t* config));

	COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(tcc_config_mock)
};
