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

#include "region_manager_mock.hpp"

using namespace ::testing;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(region_manager_mock);

void region_manager_mock::init()
{
	ON_CALL(*this, region_manager_create(_)).WillByDefault(ReturnNull());
	ON_CALL(*this, region_manager_count(_)).WillByDefault(Return(0));
	ON_CALL(*this, region_manager_get(_, _)).WillByDefault(ReturnNull());
	ON_CALL(*this, region_manager_mmap(_, _, _)).WillByDefault(ReturnNull());
	ON_CALL(*this, is_tcc_buffer_exists(_)).WillByDefault(Return(1));
}

region_manager_t* region_manager_create(const tcc_config_t* config)
{
	return region_manager_mock::get().region_manager_create(config);
}

void region_manager_destroy(region_manager_t* self)
{
	region_manager_mock::get().region_manager_destroy(self);
}

ssize_t region_manager_count(region_manager_t* self)
{
	return region_manager_mock::get().region_manager_count(self);
}

const memory_properties_t* region_manager_get(region_manager_t* self, size_t index)
{
	return region_manager_mock::get().region_manager_get(self, index);
}

void* region_manager_mmap(region_manager_t* self, size_t index, size_t size)
{
	return region_manager_mock::get().region_manager_mmap(self, index, size);
}

int is_tcc_buffer_exists(region_manager_t* self)
{
    return region_manager_mock::get().is_tcc_buffer_exists(self);
}