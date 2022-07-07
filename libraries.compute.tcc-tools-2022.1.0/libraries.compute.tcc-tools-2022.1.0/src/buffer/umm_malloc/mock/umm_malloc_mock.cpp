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

#include "umm_malloc_mock.hpp"
#include <tcc/err_code.h>

COMMON_MOCK_INTERFACE_IMPLEMENTATION(umm_malloc_interface)

int umm_init(umm_malloc_t *allocator, void* memory, size_t size) {
	return umm_malloc_interface::get().umm_init(allocator, memory, size);
}

void* umm_malloc(size_t size, umm_malloc_t* allocator) {
	return umm_malloc_interface::get().umm_malloc(size, allocator);
}

void* umm_calloc(size_t num, size_t size, umm_malloc_t* allocator) {
	return  umm_malloc_interface::get().umm_calloc(num, size, allocator);
}

void* umm_realloc(void *ptr, size_t size, umm_malloc_t *allocator) {
	return umm_malloc_interface::get().umm_realloc(ptr, size, allocator);
}

void umm_free(void *ptr, umm_malloc_t* allocator) {
	umm_malloc_interface::get().umm_free(ptr, allocator);
}

int umm_clean_instance(umm_malloc_t* allocator) {
	return umm_malloc_interface::get().umm_clean_instance(allocator);
}

umm_malloc_mock::umm_malloc_mock()
{
	ON_CALL(*this, umm_init(_, _, _)).WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
	ON_CALL(*this, umm_malloc(_, _)).WillByDefault(Return(nullptr));
	ON_CALL(*this, umm_calloc(_, _, _)).WillByDefault(Return(nullptr));
	ON_CALL(*this, umm_realloc(_, _, _)).WillByDefault(Return(nullptr));
	ON_CALL(*this, umm_free(_, _)).WillByDefault(Return());
	ON_CALL(*this, umm_clean_instance(_)).WillByDefault(Return(-TCC_E_NOT_AVAILABLE));
}
