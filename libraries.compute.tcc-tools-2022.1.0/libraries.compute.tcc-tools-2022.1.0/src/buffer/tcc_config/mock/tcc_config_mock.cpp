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
#include "tcc_config_mock.hpp"

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(tcc_config_mock)
void tcc_config_mock::init(){}

size_t tcc_config_count(const tcc_config_t* config)
{
	return ALLOW_REGULAR_MOCK(tcc_config_mock, tcc_config_count)(config);
}

const tcc_config_item_t* tcc_config_get(const tcc_config_t* config, size_t index)
{
	return ALLOW_REGULAR_MOCK(tcc_config_mock, tcc_config_get)(config, index);
}

tcc_config_t* tcc_config_read(const char* filename)
{
	return ALLOW_REGULAR_MOCK(tcc_config_mock, tcc_config_read)(filename);
}

void tcc_config_destroy(tcc_config_t* config)
{
	ALLOW_REGULAR_MOCK(tcc_config_mock, tcc_config_destroy)(config);
}
