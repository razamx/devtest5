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

#include <gtest/gtest.h>
#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include "umm_malloc.h"
#include "tcc/err_code.h"
#include <iostream>
#include <cstdarg>
#include <limits>
#include "tcc_log.h"

#define UNICODE_AGNOSTIC(name) name

#define T_UMM_TYPE_SIZE_BITS(type) (sizeof(type) * 8)
#define T_UMM_MAX_BLOCKS(type) ((type)(1 << (T_UMM_TYPE_SIZE_BITS(type) - 1)))
#define T_UMM_MAX_HEAP_SIZE(type) (sizeof(umm_block) * T_UMM_MAX_BLOCKS(type))

namespace
{
using namespace ::testing;


const size_t MINIMAL_MEMORY_SIZE = 3 * sizeof(umm_block);
const size_t MAX_MEMORY_SIZE = sizeof(umm_block) * T_UMM_MAX_BLOCKS(bsize_t);
const size_t MAX_MEMORY_SIZE_16 = sizeof(umm_block) * T_UMM_MAX_BLOCKS(uint16_t);
void* WRONG_MEMORY_POINTER = reinterpret_cast<void*>(0xDEADBEAF);
umm_malloc_t* WRONG_ALLOCATOR = reinterpret_cast<umm_malloc_t*>(WRONG_MEMORY_POINTER);

class tcc_allocator_test_wrong_values : public Test{};


TEST_F(tcc_allocator_test_wrong_values, DISABLED_umm_malloc_on_the_not_inited_instance_negative)
{
	umm_malloc_t allocator = {0};
	ASSERT_NE(nullptr, umm_malloc(100, &allocator));
}
TEST_F(tcc_allocator_test_wrong_values, umm_init_small_buffer_negative)
{
	ASSERT_EQ(-TCC_E_BAD_PARAM, umm_init(WRONG_ALLOCATOR, WRONG_MEMORY_POINTER, MINIMAL_MEMORY_SIZE - 1));
}

TEST_F(tcc_allocator_test_wrong_values, umm_init_zero_size_negative)
{
	ASSERT_EQ(-TCC_E_BAD_PARAM, umm_init(WRONG_ALLOCATOR, WRONG_MEMORY_POINTER, 0));
}
TEST_F(tcc_allocator_test_wrong_values, umm_init_null_memory_negative)
{
	ASSERT_EQ(-TCC_E_BAD_PARAM, umm_init(WRONG_ALLOCATOR, nullptr, MINIMAL_MEMORY_SIZE));
}
TEST_F(tcc_allocator_test_wrong_values, umm_init_too_big_negative)
{
	ASSERT_EQ(-TCC_E_BAD_PARAM, umm_init(WRONG_ALLOCATOR, WRONG_MEMORY_POINTER, MAX_MEMORY_SIZE + 1));
}
TEST_F(tcc_allocator_test_wrong_values, umm_realloc_null_allocator_negative)
{
	ASSERT_EQ(nullptr, umm_realloc(WRONG_MEMORY_POINTER, 100, nullptr));
}

class tcc_allocator_test_init : public Test
{

protected:
	NiceMock<alloc_mock> m_alloc_mock;
	umm_malloc_t m_allocator = {0};
	void* m_memory = nullptr;
	void TearDown() override
	{
		free(m_memory);
	}
};

/* Commentaries notations:
 * A - allocated block
 * F - freed block
 * ? - unknown/not matter
 */

/////////////////////////////
//    umm_init() tests     //
/////////////////////////////

// Minimal available buffer size
TEST_F(tcc_allocator_test_init, umm_init_positive)
{
	m_memory = malloc(MINIMAL_MEMORY_SIZE); ASSERT_NE(nullptr, m_memory);
	ASSERT_EQ(TCC_E_SUCCESS, umm_init(&m_allocator, m_memory, MINIMAL_MEMORY_SIZE));
}

// Maximum available buffer size
// ~32 Gb for 4 bytes index
#ifdef UMM_PTR_2B
TEST_F(tcc_allocator_test_init, umm_init_max_buffer_size_positive)
{
	m_memory = calloc(MAX_MEMORY_SIZE); ASSERT_NE(nullptr, m_memory);
	ASSERT_EQ(TCC_E_SUCCESS, umm_init(&m_allocator, m_memory, MAX_MEMORY_SIZE));
	
}
#endif

// Bigger then maximum available for original umm_malloc (2 bytes)
// This test will failed for 2 bytes umm_malloc
TEST_F(tcc_allocator_test_init, umm_init_bigger_than_2b_max_buffer_size_positive)
{
	m_memory = calloc(1, MAX_MEMORY_SIZE_16+1); ASSERT_NE(nullptr, m_memory);
	ASSERT_EQ(TCC_E_SUCCESS, umm_init(&m_allocator, m_memory, MAX_MEMORY_SIZE_16+1));
}

/////////////////////////////
//    umm_malloc() tests   //
/////////////////////////////
const size_t FAKE_MEMORY_REGION_SIZE=1024;
class tcc_allocator_test_1024: public tcc_allocator_test_init
{
protected:
	void SetUp() override
	{
		m_memory = malloc(FAKE_MEMORY_REGION_SIZE); ASSERT_NE(nullptr, m_memory);
		ASSERT_EQ(TCC_E_SUCCESS, umm_init(&m_allocator, m_memory, FAKE_MEMORY_REGION_SIZE));
	}
};
class tcc_allocator_test_4_umm_block: public tcc_allocator_test_init
{
protected:
	void SetUp() override
	{
		m_memory = malloc(4 * sizeof(umm_block)); ASSERT_NE(nullptr, m_memory);
		ASSERT_EQ(TCC_E_SUCCESS, umm_init(&m_allocator, m_memory, sizeof(umm_block)*4));
	}
};

class tcc_allocator_test_umm_max_block_16_plus_1: public tcc_allocator_test_init
{
protected:
	size_t m_blocks_size = T_UMM_MAX_BLOCKS(uint16_t) + 1;
	void SetUp() override
	{
		size_t blocks_size = T_UMM_MAX_BLOCKS(uint16_t) + 1;
		m_memory = malloc(sizeof(umm_block) * blocks_size); ASSERT_NE(nullptr, m_memory);
		ASSERT_EQ(TCC_E_SUCCESS, umm_init(&m_allocator, m_memory, sizeof(umm_block) * blocks_size));
	}
};

// generic success allocation
TEST_F(tcc_allocator_test_1024, umm_malloc_single_positive_size_positive)
{
	ASSERT_EQ(nullptr, umm_malloc(0, &m_allocator));
}

// allocation bigger than maxiumum size of the original umm_malloc
// This test will failed for 2 bytes umm_malloc
TEST_F(tcc_allocator_test_umm_max_block_16_plus_1, umm_malloc_single_bigger_than_2b_max_buffer_size_positive)
{
	ASSERT_NE(nullptr, umm_malloc((m_blocks_size - 2) * sizeof(uint16_t) * 4 + sizeof(uint16_t) * 2, &m_allocator));
}

// Allocate memory with exact size as available memory
TEST_F(tcc_allocator_test_4_umm_block, umm_malloc_single_exact_block_positive)
{
	ASSERT_NE(nullptr, umm_malloc(2 * sizeof(bsize_t) + sizeof(umm_block), &m_allocator));
}
TEST_F(tcc_allocator_test_4_umm_block, umm_malloc_single_too_big_negative)
{
	ASSERT_EQ(nullptr, umm_malloc(2 * sizeof(bsize_t) + sizeof(umm_block) + 1, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_malloc_multiple_small_positive)
{
	ASSERT_NE(nullptr, umm_malloc(10, &m_allocator));
	ASSERT_NE(nullptr, umm_malloc(20, &m_allocator));
	ASSERT_NE(nullptr, umm_malloc(100, &m_allocator));
	ASSERT_NE(nullptr, umm_malloc(500, &m_allocator));
}

// Allocate memory with exact size as first free block and it should be
// allocated in this  block
TEST_F(tcc_allocator_test_1024, umm_malloc_on_freed_block_positive)
{
	void *mem_1 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_1);
	void *mem_2 = umm_malloc(860, &m_allocator);
	ASSERT_NE(nullptr, mem_2);

	ASSERT_NO_FATAL_FAILURE(umm_free(mem_1, &m_allocator));
	ASSERT_EQ(mem_1, umm_malloc(100, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_malloc_null_allocator_negative)
{
	ASSERT_EQ(nullptr, umm_malloc(100,  nullptr));
}

// There are not enought free memory at all
TEST_F(tcc_allocator_test_1024, umm_malloc_not_enough_memory_negative)
{
	ASSERT_NE(nullptr, umm_malloc(100, &m_allocator));
	ASSERT_EQ(nullptr, umm_malloc(1000, &m_allocator));
}

// There are enough free memory (in sum), but there is no one continue block of
// required size
TEST_F(tcc_allocator_test_1024, umm_malloc_no_continue_block_with_required_size_negative)
{
	void *mem_1 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_1);
	void *mem_2 = umm_malloc(400, &m_allocator);
	ASSERT_NE(nullptr, mem_2);
	void *mem_3 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_3);
	void *mem_4 = umm_malloc(340, &m_allocator);
	ASSERT_NE(nullptr, mem_4);

	ASSERT_NO_FATAL_FAILURE(umm_free(mem_1, &m_allocator));
	ASSERT_NO_FATAL_FAILURE(umm_free(mem_3, &m_allocator));

	void *mem_5 = umm_malloc(200, &m_allocator);
	ASSERT_EQ(nullptr, mem_5);

	mem_5 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_5);
	mem_5 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_5);
}

// Now we have seg. fault in this case

/////////////////////////////
//    umm_free() tests     //
/////////////////////////////

TEST_F(tcc_allocator_test_1024, umm_free_nullptr_positive)
{

	ASSERT_NO_FATAL_FAILURE(umm_free(nullptr, &m_allocator));
}

TEST_F(tcc_allocator_test_init, umm_free_null_allocator_negative)
{
	// Will SEGFAULT without null-checking
	void* fake_ptr = (void*)0xdeadbeef;
	ASSERT_NO_FATAL_FAILURE(umm_free(fake_ptr, NULL));
}

// Simple free of allocated block
// 0F?... -> 0A?... -> 0F?...
TEST_F(tcc_allocator_test_1024, umm_free_positive)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	umm_free(ptr, &m_allocator);
	ASSERT_EQ(1, m_allocator.umm_heap[0].body.free.next);  // 1st free block is 1st block
							     // 1st block freed
}

// Invalid pointer passed: pointer outside the heap
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_free_ptr_before_head_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (uint8_t*)m_allocator.umm_heap - 1;
	umm_free(bad_ptr, &m_allocator);
	ASSERT_EQ(2, m_allocator.umm_heap[0].body.free.next);  // 1st block not freed
}

// Invalid pointer passed: pointer outside the heap
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_free_ptr_after_heap_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (uint8_t*)m_allocator.umm_heap + m_allocator.umm_numblocks * sizeof(umm_block);
	umm_free(bad_ptr, &m_allocator);
	ASSERT_EQ(2, m_allocator.umm_heap[0].body.free.next);  // 1st block not freed
}

// Invalid pointer passed: pointer doesn't point to the first byte of the block
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_free_ptr_not_aligned_block_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (uint8_t*)ptr + 1;
	umm_free(bad_ptr, &m_allocator);
	ASSERT_EQ(2, m_allocator.umm_heap[0].body.free.next);  // 1st block not freed
}

// Invalid pointer passed: pointer points to the wrong block
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_free_ptr_bad_block_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (void*)&m_allocator.umm_heap[5];
	umm_free(bad_ptr, &m_allocator);
	ASSERT_EQ(2, m_allocator.umm_heap[0].body.free.next);  // 1st block not freed
}

// Invalid pointer passed: pointer points to the wrong block
// Multiple blocks allocated at once, pointer must points to the first byte of
// this allocated memory
// 0F?... -> 0AAAA?... -> 0AAAA?...
TEST_F(tcc_allocator_test_1024, umm_free_ptr_not_first_block_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr) + sizeof(umm_block)*3, &m_allocator); // allocate 4 blocks
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (void*)&m_allocator.umm_heap[2];
	umm_free(bad_ptr, &m_allocator);
	ASSERT_EQ(5, m_allocator.umm_heap[0].body.free.next);  // 1st-4th blocks not freed
}

// 0FF?... -> 0AA?... -> 0FA?...
TEST_F(tcc_allocator_test_1024, umm_free_multiple_alloc_free_first_positive)
{
	void* ptr1 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr1);
	void* ptr2 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr2);
	umm_free(ptr1, &m_allocator);
	ASSERT_EQ(1, m_allocator.umm_heap[0].body.free.next);  // 1st block freed
	ASSERT_EQ(3, m_allocator.umm_heap[1].body.free.next);  // 1st points to the next free block
}

// 0FFF?... -> 0AAA?... -> 0AFA?...
TEST_F(tcc_allocator_test_1024, umm_free_multiple_alloc_free_middle_positive)
{
	void* ptr1 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr1);
	void* ptr2 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr2);
	void* ptr3 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr3);
	umm_free(ptr2, &m_allocator);
	ASSERT_EQ(2, m_allocator.umm_heap[0].body.free.next);  // 2nd block freed
	ASSERT_EQ(4, m_allocator.umm_heap[2].body.free.next);  // 2nd points to the next free block
}

// 0FFF?... -> 0AAA?... -> 0AAF?...
TEST_F(tcc_allocator_test_1024, umm_free_multiple_alloc_free_last_positive)
{
	void* ptr1 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr1);
	void* ptr2 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr2);
	void* ptr3 = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate 1 block
	ASSERT_NE(nullptr, ptr3);
	umm_free(ptr3, &m_allocator);
	ASSERT_EQ(3, m_allocator.umm_heap[0].body.free.next);  // 3rd block freed
}

/////////////////////////////
//   umm_realloc() tests   //
/////////////////////////////

TEST_F(tcc_allocator_test_1024, umm_realloc_positive_size_positive)
{
	void *mem = umm_malloc(256, &m_allocator);
	ASSERT_NE(nullptr, mem);
	ASSERT_NE(nullptr, umm_realloc(mem, 512, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_realloc_zero_size_positive)
{
	void *mem = umm_malloc(512, &m_allocator);
	ASSERT_NE(nullptr, mem);
	ASSERT_EQ(nullptr, umm_realloc(mem, 0, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_realloc_same_size_positive)
{
	void *mem = umm_malloc(512, &m_allocator);
	ASSERT_NE(nullptr, mem);
	ASSERT_NE(nullptr, umm_realloc(mem, 512, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_free_after_realloc_positive)
{
	void *mem = umm_malloc(512, &m_allocator);
	ASSERT_NE(nullptr, mem);
	ASSERT_NE(nullptr, umm_realloc(mem, 512, &m_allocator));
	ASSERT_NO_FATAL_FAILURE(umm_free(mem, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_realloc_on_freed_block_positive)
{
	void *mem_1 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_1);
	void *mem_2 = umm_malloc(860, &m_allocator);
	ASSERT_NE(nullptr, mem_2);

	ASSERT_NO_FATAL_FAILURE(umm_free(mem_2, &m_allocator));
	ASSERT_NE(nullptr, umm_realloc(mem_1, 200, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_realloc_after_free_mid_positive)
{
	void *mem_1 = umm_malloc(100, &m_allocator);
	ASSERT_NE(nullptr, mem_1);
	void *mem_2 = umm_malloc(150, &m_allocator);
	ASSERT_NE(nullptr, mem_2);
	void *mem_3 = umm_malloc(200, &m_allocator);
	ASSERT_NE(nullptr, mem_3);

	ASSERT_NO_FATAL_FAILURE(umm_free(mem_2, &m_allocator));

	ASSERT_NE(nullptr, umm_realloc(mem_1, 200, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_realloc_bad_ptr_negative)
{
	ASSERT_EQ(nullptr, umm_realloc(m_allocator.umm_heap+3000, 20, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_realloc_not_enough_memory_negative)
{
	void *mem = umm_malloc(512, &m_allocator);
	ASSERT_EQ(nullptr, umm_realloc(mem, 2000, &m_allocator));
}

// There are enough free memory (in sum), but there is no one continue block of
// required size
TEST_F(tcc_allocator_test_1024, umm_realloc_no_continue_block_with_required_size_negative)
{
	void *mem_1 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_1);
	void *mem_2 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_2);
	void *mem_3 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_3);
	void *mem_4 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_4);
	void *mem_5 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_5);
	void *mem_6 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_5);
	void *mem_7 = umm_malloc(128, &m_allocator);
	ASSERT_NE(nullptr, mem_5);

	ASSERT_NO_FATAL_FAILURE(umm_free(mem_2, &m_allocator));
	ASSERT_NO_FATAL_FAILURE(umm_free(mem_4, &m_allocator));

	// There is no continius 128*3b block
	ASSERT_EQ(nullptr, umm_realloc(mem_1, 128*3, &m_allocator));

	// But there are two blocks 128 + 256
	ASSERT_NE(nullptr, umm_realloc(mem_1, 256, &m_allocator));
	ASSERT_NE(nullptr, umm_malloc(128, &m_allocator));
}

// Invalid pointer passed: pointer outside the heap
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_realloc_ptr_before_head_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (uint8_t*)m_allocator.umm_heap - 1;
	ASSERT_EQ(nullptr, umm_realloc(bad_ptr, sizeof(umm_ptr) + sizeof(umm_block), &m_allocator)); // realloc to 2 blocks
}

// Invalid pointer passed: pointer outside the heap
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_realloc_ptr_after_heap_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (uint8_t*)m_allocator.umm_heap + m_allocator.umm_numblocks * sizeof(umm_block);
	ASSERT_EQ(nullptr, umm_realloc(bad_ptr, sizeof(umm_ptr) + sizeof(umm_block), &m_allocator)); // realloc to 2 blocks
}

// Invalid pointer passed: pointer doesn't point to the first byte of the block
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_realloc_ptr_not_aligned_block_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (uint8_t*)ptr + 1;
	ASSERT_EQ(nullptr, umm_realloc(bad_ptr, sizeof(umm_ptr) + sizeof(umm_block), &m_allocator)); // realloc to 2 blocks
}

// Invalid pointer passed: pointer points to the wrong block
// 0F?... -> 0A?... -> 0A?...
TEST_F(tcc_allocator_test_1024, umm_realloc_ptr_bad_block_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr), &m_allocator); // allocate single block
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (void*)&m_allocator.umm_heap[5];
	ASSERT_EQ(nullptr, umm_realloc(bad_ptr, sizeof(umm_ptr) + sizeof(umm_block), &m_allocator)); // realloc to 2 blocks
}

// Invalid pointer passed: pointer points to the wrong block
// Multiple blocks allocated at once, pointer must points to the first byte of
// this allocated memory
// 0F?... -> 0AAAA?... -> 0AAAA?...
TEST_F(tcc_allocator_test_1024, umm_realloc_ptr_not_first_block_negative)
{
	void* ptr = umm_malloc(sizeof(umm_ptr) + sizeof(umm_block)*3, &m_allocator); // allocate 4 blocks
	ASSERT_NE(nullptr, ptr);
	void* bad_ptr = (void*)&m_allocator.umm_heap[2];
	ASSERT_EQ(nullptr, umm_realloc(bad_ptr, sizeof(umm_ptr) + sizeof(umm_block), &m_allocator)); // realloc to 2 blocks
}

////////////////////////////////////////
//       umm_calloc() tests          //
///////////////////////////////////////
TEST_F(tcc_allocator_test_1024, umm_calloc_overflow_negative) {
	ASSERT_EQ(nullptr, umm_calloc(ULONG_MAX, ULONG_MAX, &m_allocator));
}

TEST_F(tcc_allocator_test_1024, umm_calloc_positive) {
	// Allocate memory and fill it
	size_t size = 10;
	int* buffer = (int*)umm_malloc(size*sizeof(int), &m_allocator);
	ASSERT_NE(nullptr, buffer);
	for(size_t i = 0; i < size; i++) {
		buffer[i] = i;
	}

	umm_free(buffer, &m_allocator);

	// Calloc this memory and check that it is filled with zeros
	buffer = (int*)umm_calloc(size, sizeof(int), &m_allocator);
	ASSERT_NE(nullptr, buffer);
	for(size_t i = 0; i < size; i++) {
		if(buffer[i] != 0) {
			FAIL() << "buffer[" << i << "]=" << buffer[i] << "\nBuffer must be filled with zeros";
		}
	}
}

} // namespace
