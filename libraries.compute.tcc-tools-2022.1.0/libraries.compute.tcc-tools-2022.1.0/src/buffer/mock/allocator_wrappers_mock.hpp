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

#ifndef REGION_ALLOCATOR_MOCK_HPP
#define REGION_ALLOCATOR_MOCK_HPP

#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "region_allocator.h"
#include "dram_allocator.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

struct base_allocator_interface
{
	virtual void* malloc(allocator_interface_t*, size_t) = 0;
	virtual void* calloc(allocator_interface_t*, size_t, size_t) = 0;
	virtual void* realloc(allocator_interface_t*, void*, size_t) = 0;
	virtual void free(allocator_interface_t*, void*) = 0;
	virtual void destroy(allocator_interface_t*) = 0;
};

// inheritance doesn't work somewhy
#define COMMON_MOCK_METHODS                                                    \
	MOCK_METHOD2(malloc, void*(allocator_interface_t*, size_t));           \
	MOCK_METHOD3(calloc, void*(allocator_interface_t*, size_t, size_t));   \
	MOCK_METHOD3(realloc, void*(allocator_interface_t*, void*, size_t));   \
	MOCK_METHOD2(free, void(allocator_interface_t*, void*));               \
	MOCK_METHOD1(destroy, void(allocator_interface_t*));                   \

// -----------------------------------------------------------------------------

struct dram_allocator_interface : public base_allocator_interface
{
	virtual allocator_interface_t* dram_allocator_create() = 0;
	// Other methods aren't needed
	COMMON_MOCK_INTERFACE_DEFINITIONS(dram_allocator_interface);
};

struct dram_allocator_mock : public dram_allocator_interface
{
	MOCK_METHOD0(dram_allocator_create, allocator_interface_t*());
	COMMON_MOCK_METHODS
	dram_allocator_mock();
};

// -----------------------------------------------------------------------------

struct region_allocator_interface : public base_allocator_interface
{
	virtual allocator_interface_t* region_allocator_create(void* memory, size_t size) = 0;
	COMMON_MOCK_INTERFACE_DEFINITIONS(region_allocator_interface);
};

struct region_allocator_mock : public region_allocator_interface
{
	MOCK_METHOD2(region_allocator_create, allocator_interface_t*(void*, size_t));
	COMMON_MOCK_METHODS
	region_allocator_mock();
};

// -----------------------------------------------------------------------------

// Helpers to create real allocator_interface_t structure, but with functions
// point to the mocked functions
allocator_interface_t* create_real_dram_allocator_ptr();
allocator_interface_t* create_real_region_allocator_ptr(void* memory_ptr, size_t size);

#endif // REGION_ALLOCATOR_MOCK_HPP
