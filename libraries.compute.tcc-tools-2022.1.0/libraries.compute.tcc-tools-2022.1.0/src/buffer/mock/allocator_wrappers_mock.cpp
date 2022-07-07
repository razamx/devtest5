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

#include "allocator_wrappers_mock.hpp"

#define UNUSED(x)    ((void)x)

allocator_interface_t* make_allocator(void*(*pmalloc)(allocator_interface_t*, size_t),
                                      void*(*pcalloc)(allocator_interface_t*, size_t, size_t),
                                      void*(*prealloc)(allocator_interface_t*, void*, size_t),
                                      void(*pfree)(allocator_interface_t*, void*),
                                      void(*pdestroy)(allocator_interface_t*))
{
	return new allocator_interface_t {.malloc=pmalloc, .calloc=pcalloc,
	                                  .realloc=prealloc, .free=pfree,
                                          .destroy=pdestroy};
}

#define MAKE_COMMON_ALLOCATOR_FUNCTIONS(name)\
	void* name##_allocator_malloc(allocator_interface_t* self, size_t size)                 \
	{                                                                                       \
	        return name##_allocator_interface::get().malloc(self, size);                    \
	}                                                                                       \
	void* name##_allocator_calloc(allocator_interface_t* self, size_t num, size_t size)     \
	{                                                                                       \
	        return name##_allocator_interface::get().calloc(self, num, size);               \
	}                                                                                       \
	void* name##_allocator_realloc(allocator_interface_t* self, void* ptr, size_t size)     \
	{                                                                                       \
	        return name##_allocator_interface::get().realloc(self, ptr, size);              \
	}                                                                                       \
	void name##_allocator_free(allocator_interface_t* self, void* ptr)                      \
	{                                                                                       \
	        name##_allocator_interface::get().free(self, ptr);                              \
	}                                                                                       \
	void name##_allocator_destroy(allocator_interface_t* self)                              \
	{                                                                                       \
	        name##_allocator_interface::get().destroy(self);                                \
	}                                                                                       \
// -----------------------------------------------------------------------------

COMMON_MOCK_INTERFACE_IMPLEMENTATION(dram_allocator_interface);

allocator_interface_t* create_real_dram_allocator_ptr()
{
	return make_allocator(dram_allocator_malloc, dram_allocator_calloc, dram_allocator_realloc,
			      dram_allocator_free, dram_allocator_destroy);
}

dram_allocator_mock::dram_allocator_mock()
{
	ON_CALL(*this, dram_allocator_create())
		.WillByDefault(Invoke([](){
			TCC_LOG_TRACE("dram_allocator create");
			return create_real_dram_allocator_ptr();
		}));
	ON_CALL(*this, destroy(_))
		.WillByDefault(Invoke([](allocator_interface_t* self){
			TCC_LOG_TRACE("dram_allocator destroy %p", self);
			if(self != NULL) delete self;
		}));
}

allocator_interface_t* dram_allocator_create()
{
	return dram_allocator_interface::get().dram_allocator_create();
}

MAKE_COMMON_ALLOCATOR_FUNCTIONS(dram)

// -----------------------------------------------------------------------------

COMMON_MOCK_INTERFACE_IMPLEMENTATION(region_allocator_interface);

allocator_interface_t* create_real_region_allocator_ptr(void* memory_ptr, size_t size)
{
	UNUSED(memory_ptr);
	UNUSED(size);
	return make_allocator(region_allocator_malloc, region_allocator_calloc, region_allocator_realloc,
			      region_allocator_free, region_allocator_destroy);
}

region_allocator_mock::region_allocator_mock()
{
	ON_CALL(*this, region_allocator_create(_, _))
		.WillByDefault(Invoke([](void* ptr, size_t size){
			TCC_LOG_TRACE("region_allocator create");
			return create_real_region_allocator_ptr(ptr, size);
		}));
	ON_CALL(*this, destroy(_))
		.WillByDefault(Invoke([](allocator_interface_t* self){
			TCC_LOG_TRACE("region_allocator destroy %p", self);
			if(self != NULL) delete self;
		}));
}

allocator_interface_t* region_allocator_create(void* memory, size_t size)
{
	return region_allocator_interface::get().region_allocator_create(memory, size);
}

MAKE_COMMON_ALLOCATOR_FUNCTIONS(region)
