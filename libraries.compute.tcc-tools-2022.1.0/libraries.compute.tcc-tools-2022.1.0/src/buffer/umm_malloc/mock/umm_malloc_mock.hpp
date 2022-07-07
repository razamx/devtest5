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

#ifndef UMM_MALLOC_MOCK_H
#define UMM_MALLOC_MOCK_H

#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "umm_malloc.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

struct umm_malloc_interface {
	virtual int umm_init(umm_malloc_t *allocator, void* memory, size_t size) = 0;
	virtual void* umm_malloc(size_t size, umm_malloc_t *allocator) = 0;
	virtual void* umm_calloc(size_t num, size_t size, umm_malloc_t *allocator) = 0;
	virtual void* umm_realloc(void *ptr, size_t size, umm_malloc_t *allocator) = 0;
	virtual void umm_free(void *ptr, umm_malloc_t *allocator) = 0;
	virtual int umm_clean_instance(umm_malloc_t* allocator) = 0;

	COMMON_MOCK_INTERFACE_DEFINITIONS(umm_malloc_interface)
};

struct umm_malloc_mock: public umm_malloc_interface {
	MOCK_METHOD3(umm_init, int(umm_malloc_t *allocator, void* memory, size_t size));
	MOCK_METHOD2(umm_malloc, void*(size_t size, umm_malloc_t *allocator));
	MOCK_METHOD3(umm_calloc, void*(size_t num, size_t size, umm_malloc_t *allocator));
	MOCK_METHOD3(umm_realloc, void*(void *ptr, size_t size, umm_malloc_t *allocator));
	MOCK_METHOD2(umm_free, void(void *ptr, umm_malloc_t *allocator));
	MOCK_METHOD1(umm_clean_instance, int(umm_malloc_t* allocator));

	umm_malloc_mock();
};

#endif
