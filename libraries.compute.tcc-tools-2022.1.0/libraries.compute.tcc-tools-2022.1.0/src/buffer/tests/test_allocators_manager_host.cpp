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

#include "gmock/gmock-actions.h"
#include "gmock/gmock-more-actions.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "region_manager_mock.hpp"
#include "allocators_manager.h"
#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include "umm_malloc_mock.hpp"
#include "allocator_wrappers_mock.hpp"
#include "memory_properties_test_utils.hpp"
#include "tcc_log.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

namespace
{
using namespace testing;
using ::testing::Mock;

/**
 * @brief Helper to simplify memory management for tcc_config
 */
class tcc_config_holder
{
public:
	struct tcc_config {
		size_t count;
		tcc_config_item_t* items;
	};

	tcc_config_holder(const std::vector<memory_properties_t>& config_items)
	{
		config.items = new tcc_config_item[config_items.size()];
		config.count = config_items.size();
		for(size_t i = 0; i < config_items.size(); i++) {
			config.items[i].size = config_items[i].size;
			config.items[i].level = config_items[i].type;
		}
	}

	~tcc_config_holder()
	{
		if(config.items) {
			delete[] config.items;
			config.items = nullptr;
			config.count = 0;
		}
	}

	const tcc_config_t* operator*()
	{
		return (tcc_config_t*)&config;
	}

private:
	tcc_config config;
};

class test_allocators_manager_base : public Test
{
protected:

	NiceMock<alloc_mock> m_alloc_mock;
	StrictMock<region_manager_mock> m_rgn_manager_mock;
	StrictMock<dram_allocator_mock> m_dram_alloc_mock;
	StrictMock<region_allocator_mock> m_region_alloc_mock;

	region_manager_t* region_mgr = (region_manager_t*)0xDEAFBEAF;
	allocators_manager_t* allocator_mgr = nullptr;

	const unsigned int LATENCY_DRAM = 100;
	const unsigned int LATENCY_L3 = 50;
	const unsigned int LATENCY_L2 = 10;
	const size_t ANY_SIZE = 12345;

	// Setup expectations for Region Manager to return region information
	void expect_regions(const std::vector<memory_properties_t>& region_properties)
	{
		auto prop_ptr = std::make_shared<std::vector<memory_properties_t>>(region_properties);

		EXPECT_CALL(m_rgn_manager_mock, region_manager_count(region_mgr))
			.WillRepeatedly(Invoke([prop_ptr](region_manager_t*) {
				return prop_ptr->size();
			}));

		for(size_t i = 0; i < region_properties.size(); i++) {
			EXPECT_CALL(m_rgn_manager_mock, region_manager_get(region_mgr, i))
				.WillRepeatedly(Invoke([prop_ptr, i](region_manager_t*, size_t) {
					return &prop_ptr->at(i);
				}));
		}
	}
};

class test_allocators_manager_advanced : public test_allocators_manager_base
{
protected:

	StrictMock<umm_malloc_mock> m_umm_malloc_mock;
	allocator_interface_t* fake_dram_allocator_ptr = create_real_dram_allocator_ptr();

	// We can't create allocators_manager in SetUp, because it's request number
	// of region in contstructor, but exact list of regions cant be specified
	// only in test body
	void init_region_manager(const std::vector<memory_properties_t>& region_properties)
	{
		expect_regions(region_properties);
		EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
			.WillOnce(Return(fake_dram_allocator_ptr));

		allocator_mgr = allocators_manager_create(region_mgr, nullptr);
		ASSERT_NE(nullptr, allocator_mgr);
		TCC_LOG_INFO("---- TEST BEGIN ----");

	}

	void TearDown() override
	{
		TCC_LOG_INFO("----- TEST END -----");

		EXPECT_CALL(m_region_alloc_mock, destroy(_))
			.WillRepeatedly(DoDefault());
		EXPECT_CALL(m_dram_alloc_mock, destroy(fake_dram_allocator_ptr))
			.WillOnce(DoDefault());

		allocators_manager_destroy(allocator_mgr);
		allocator_mgr = nullptr;
	}

};

class test_allocators_manager_with_regions : public test_allocators_manager_advanced
{
protected:

	const size_t index_dram = 0;
	const size_t index_l3 = 1;
	const size_t index_l2 = 2;

	void SetUp() override
	{
		init_region_manager({
			prop(.latency=LATENCY_DRAM, .mask=0b1111, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM),
			prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3),
			prop(.latency=LATENCY_L2, .mask=0b1100, .size=100,  .type=TCC_MEM_L2)
		});
	}
};

////////////////////////////////////////////////////////////////////////////////
// Initialization tests
////////////////////////////////////////////////////////////////////////////////

class test_allocators_manager_create : public test_allocators_manager_base
{
};

TEST_F(test_allocators_manager_create, region_manager_is_null_negative)
{
	ASSERT_EQ(nullptr, allocators_manager_create(nullptr, nullptr));
}

TEST_F(test_allocators_manager_create, instance_calloc_fails_negative)
{
	EXPECT_CALL(m_alloc_mock, calloc(1, _))
		.WillOnce(ReturnNull());

	ASSERT_EQ(nullptr, allocators_manager_create(region_mgr, nullptr));
}

TEST_F(test_allocators_manager_create, get_regions_count_fails_negative)
{
	EXPECT_CALL(m_rgn_manager_mock, region_manager_count(region_mgr))
		.WillOnce(Return(-1));

	ASSERT_EQ(nullptr, allocators_manager_create(region_mgr, nullptr));
}

TEST_F(test_allocators_manager_create, allocators_array_malloc_fails_negative)
{
	ssize_t fake_num_of_regions = 3;
	EXPECT_CALL(m_rgn_manager_mock, region_manager_count(region_mgr))
		.WillOnce(Return(fake_num_of_regions));
	EXPECT_CALL(m_alloc_mock, calloc(1, _))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_alloc_mock, calloc(fake_num_of_regions, _))
		.WillOnce(ReturnNull());

	ASSERT_EQ(nullptr, allocators_manager_create(region_mgr, nullptr));
}

TEST_F(test_allocators_manager_create, size_array_malloc_fails_negative)
{
	ssize_t fake_num_of_regions = 3;
	EXPECT_CALL(m_rgn_manager_mock, region_manager_count(region_mgr))
		.WillOnce(Return(fake_num_of_regions));
	EXPECT_CALL(m_alloc_mock, calloc(1, _))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_alloc_mock, calloc(fake_num_of_regions, _))
		.WillOnce(DoDefault())
		.WillOnce(ReturnNull());

	ASSERT_EQ(nullptr, allocators_manager_create(region_mgr, nullptr));
}

TEST_F(test_allocators_manager_create, unable_to_create_dram_allocator_negative)
{
	expect_regions({ prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3) });
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.WillOnce(ReturnNull());
	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_EQ(nullptr, manager);
}

TEST_F(test_allocators_manager_create, create_positive)
{
	expect_regions({ prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3) });
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.WillOnce(DoDefault());
	EXPECT_CALL(m_dram_alloc_mock, destroy(_))
		.WillOnce(DoDefault());

	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);
	allocators_manager_destroy(manager);
}

TEST_F(test_allocators_manager_create, double_create_negative)
{
	expect_regions({ prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3) });
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.WillOnce(DoDefault());
	EXPECT_CALL(m_dram_alloc_mock, destroy(_))
		.WillOnce(DoDefault());

	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);
	ASSERT_EQ(nullptr, allocators_manager_create(region_mgr, nullptr));
	allocators_manager_destroy(manager);
}


TEST_F(test_allocators_manager_create, create_after_destroy)
{
	expect_regions({ prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3) });
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.Times(2)
		.WillRepeatedly(DoDefault());
	EXPECT_CALL(m_dram_alloc_mock, destroy(_))
		.Times(2)
		.WillRepeatedly(DoDefault());

	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);
	allocators_manager_destroy(manager);
	manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);
	allocators_manager_destroy(manager);
}

////////////////////////////////////////////////////////////////////////////////
// Get next tests
// Config is nullptr
////////////////////////////////////////////////////////////////////////////////

// This fixture used when regions list is custom
class test_allocators_manager_get_next : public test_allocators_manager_advanced
{
protected:
};

TEST_F(test_allocators_manager_get_next, self_is_null_negative)
{
	auto requirements = prop(.latency=10, .mask=0b11, .size=DEFAULT_SIZE, .type=DEFAULT_TYPE);
	init_region_manager({
		prop(.latency=LATENCY_DRAM, .mask=0xFF, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM)
	});

	EXPECT_EQ(nullptr, allocators_manager_get_next(nullptr, &requirements, nullptr));
}

TEST_F(test_allocators_manager_get_next, requirements_is_null_negative)
{
	init_region_manager({
		prop(.latency=LATENCY_DRAM, .mask=0xFF, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM)
	});

	EXPECT_EQ(nullptr, allocators_manager_get_next(allocator_mgr, nullptr, nullptr));
}

TEST_F(test_allocators_manager_get_next, create_new_dram_allocator_when_it_doesnt_exist_positive)
{
	auto requirements = prop(.latency=LATENCY_DRAM, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	init_region_manager({
		prop(.latency=LATENCY_DRAM, .mask=0xFF, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM)
	});

	// Get new allocator
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it);

	// Check that correct allocator returned
	EXPECT_EQ(fake_dram_allocator_ptr, get_allocator_from_iterator(it));
}

TEST_F(test_allocators_manager_get_next, create_new_region_allocator_when_it_doesnt_exist_unable_to_get_region_negative)
{
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	init_region_manager({
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3)
	});

	// Unable to get region
	EXPECT_CALL(m_rgn_manager_mock, region_manager_get(region_mgr, 0))
		.WillOnce(ReturnNull());

	// Get new allocator
	ASSERT_EQ(nullptr, allocators_manager_get_next(allocator_mgr, &requirements, nullptr));
}

TEST_F(test_allocators_manager_get_next, create_new_region_allocator_when_it_doesnt_exist_region_allocator_create_fails_negative)
{
	void* fake_region_ptr = (void*)0xdead;
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	init_region_manager({
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3)
	});

	// request memory from L3 and create allocator
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 0, _))
		.WillOnce(Return(fake_region_ptr));
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_region_ptr, _))
		.WillOnce(ReturnNull());

	// Get new allocator
	ASSERT_EQ(nullptr, allocators_manager_get_next(allocator_mgr, &requirements, nullptr));

}

TEST_F(test_allocators_manager_get_next, create_new_region_allocator_when_it_doesnt_exist_positive)
{
	void* fake_l3_region_ptr = (void*)0xdead;
	allocator_interface_t* fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	init_region_manager({
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3)
	});

	// request memory from L3 and create allocator
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 0, _))
		.WillOnce(Return(fake_l3_region_ptr));
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(Return(fake_l3_allocator_ptr));

	// Get new allocator
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it);

	// Check that correct allocator returned
	EXPECT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it));
}

//Disable tests wile next not fixed:
//DE9044: Real-time Configuration Driver return UNKNOWN(0) type of memory for TCC_GET_MEMORY_CONFIG ioctl
TEST_F(test_allocators_manager_get_next, DISABLED_create_new_allocator_when_it_doesnt_exist_region_has_bad_type_negative)
{
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	init_region_manager({
		// Invalid region in the regions table;
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_UNKNOWN)
	});

	// Region satisfy our request but has invalid type
	EXPECT_EQ(nullptr, allocators_manager_get_next(allocator_mgr, &requirements, nullptr));
}

////////////////////////////////////////////////////////////////////////////////

// Fixture with common list of regions
class test_allocators_manager_get_next_with_table : public test_allocators_manager_with_regions {};


TEST_F(test_allocators_manager_get_next_with_table, create_new_allocator_failed_then_try_next_positive)
{
	void* fake_l3_region_ptr = (void*)0xdead;
	allocator_interface_t* fake_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// request memory from L3 failed
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(ReturnNull());
	// request memory from L2 OK
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l2, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// Create allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(Return(fake_allocator_ptr));

	// Get new allocator
	// We request L3, but L2 is also suitable
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it);

	// Check that correct allocator returned
	EXPECT_EQ(fake_allocator_ptr, get_allocator_from_iterator(it));
}

TEST_F(test_allocators_manager_get_next_with_table, request_allocator_again_positive)
{
	void* fake_l3_region_ptr = (void*)0xdead;
	allocator_interface_t* fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// request memory from L3
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// Create allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(Return(fake_l3_allocator_ptr));

	// Get new allocator
	auto it1 = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it1);
	ASSERT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it1));

	// Get allocator again, same allocator should be used
	auto it2 = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it2);
	ASSERT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it2));
}

TEST_F(test_allocators_manager_get_next_with_table, request_for_another_allocator_positive)
{
	void* fake_l3_region_ptr = (void*)0xdead1;
	void* fake_l2_region_ptr = (void*)0xdead2;
	allocator_interface_t* fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	allocator_interface_t* fake_l2_allocator_ptr = create_real_region_allocator_ptr(fake_l2_region_ptr, ANY_SIZE);
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// request memory from L3 OK
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// Create allocator for L3
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(Return(fake_l3_allocator_ptr));
	// request memory from L2 OK
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l2, _))
		.WillOnce(Return(fake_l2_region_ptr));
	// Create allocator for L2
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l2_region_ptr, _))
		.WillOnce(Return(fake_l2_allocator_ptr));

	// Get new allocator
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it);

	// Don't want this allocator, request for another one
	it = allocators_manager_get_next(allocator_mgr, &requirements, it);
	ASSERT_NE(nullptr, it);

	// Check that correct allocator returned
	EXPECT_EQ(fake_l2_allocator_ptr, get_allocator_from_iterator(it));
}

TEST_F(test_allocators_manager_get_next_with_table, request_for_another_allocator_unable_to_get_memory_negative)
{
	void* fake_l3_region_ptr = (void*)0xdead1;
	allocator_interface_t* fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// request memory from L3 OK
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// Create allocator for L3
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr,_))
		.WillOnce(Return(fake_l3_allocator_ptr));
	// request memory from L2 failed
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l2, _))
		.WillOnce(ReturnNull());

	// Get new allocator
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_NE(nullptr, it);

	// Don't want this allocator, request for another one
	it = allocators_manager_get_next(allocator_mgr, &requirements, it);
	ASSERT_EQ(nullptr, it);
}

TEST_F(test_allocators_manager_get_next_with_table, cant_find_region_which_satisfies_latency_requirements_negative)
{
	auto requirements = prop(1, .mask=0b1000, .size=100, .type=DEFAULT_TYPE); // latency too small

	// Get new allocator
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_EQ(nullptr, it);
}

TEST_F(test_allocators_manager_get_next_with_table, cant_find_region_which_satisfies_mask_requirements_negative)
{
	auto requirements = prop(.latency=LATENCY_L2, .mask=0b0001, .size=100, .type=DEFAULT_TYPE); // no such mask

	// Get new allocator
	// We need L2 latency, but L2 region has bad mask. L3 region has good mask,
	// but it has too big latency
	auto it = allocators_manager_get_next(allocator_mgr, &requirements, nullptr);
	ASSERT_EQ(nullptr, it);
}

TEST_F(test_allocators_manager_get_next_with_table, get_allocator_from_iterator_fails_when_iterator_is_null_negative)
{
	EXPECT_EQ(nullptr, get_allocator_from_iterator(nullptr));
}

////////////////////////////////////////////////////////////////////////////////
// ALLOCATORS_FOREACH tests
// Full-flow scenario
////////////////////////////////////////////////////////////////////////////////

class test_allocators_manager_foreach : public test_allocators_manager_with_regions {};

TEST_F(test_allocators_manager_foreach, foreach_macro_first_is_good_positive)
{
	// We request L3 memory and it's successful

	size_t fake_size = 100;
	void* fake_l3_region_ptr = (void*)0xdead1;
	void* fake_l3_allocated_ptr = (void*)0xbeaf1;
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// L3 memory
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// L3 allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_region_alloc_mock, malloc(_,_))
		.WillOnce(Return(fake_l3_allocated_ptr));

	// Full scenario - try every allocator that satisfies requirements until
	// we can successfully allocate memory
	const allocator_it_t* it = NULL;
	void* ptr = nullptr;
	ALLOCATORS_FOREACH(allocator_mgr, &requirements, it)
	{
		auto allocator = get_allocator_from_iterator(it);
		ptr = allocator->malloc(allocator, fake_size);
		if (ptr) break; // we found what we need
	}

	ASSERT_EQ(fake_l3_allocated_ptr, ptr);
}

TEST_F(test_allocators_manager_foreach, foreach_macro_second_is_good_positive)
{
	// We request L3 memory, but L3 allocator can't allocate memory, so
	// we go next to the L2

	size_t fake_size = 100;
	void* fake_l3_region_ptr = (void*)0xdead1;
	void* fake_l2_region_ptr = (void*)0xdead2;
	void* fake_l2_allocated_ptr = (void*)0xbeaf2;
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// L3 memory
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// L2 memory
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l2, _))
		.WillOnce(Return(fake_l2_region_ptr));
	// L3 & L2 allocators
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l2_region_ptr, _))
		.WillOnce(DoDefault());
	// L3 allocator can't allocate memory
	EXPECT_CALL(m_region_alloc_mock, malloc(_,_))
		.WillOnce(Return(nullptr))
		.WillOnce(Return(fake_l2_allocated_ptr));


	const allocator_it_t* it = NULL;
	void* ptr = nullptr;
	ALLOCATORS_FOREACH(allocator_mgr, &requirements, it)
	{
		auto allocator = get_allocator_from_iterator(it);
		ptr = allocator->malloc(allocator, fake_size);
		if (ptr) break; // we found what we need
	}

	ASSERT_EQ(fake_l2_allocated_ptr, ptr);
}

TEST_F(test_allocators_manager_foreach, foreach_macro_nothing_good_negative)
{
	// We request L3 memory, but L3 allocator can't allocate memory, so
	// we go next to the L2, but no one can allocate memory

	size_t fake_size = 100;
	void* fake_l3_region_ptr = (void*)0xdead1;
	void* fake_l2_region_ptr = (void*)0xdead2;
	auto requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	// L3 memory
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l3, _))
		.WillOnce(Return(fake_l3_region_ptr));
	// L2 memory
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, index_l2, _))
		.WillOnce(Return(fake_l2_region_ptr));
	// L3 & L2 allocators
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l2_region_ptr, _))
		.WillOnce(DoDefault());
	// Both allocators can't allocate memory
	EXPECT_CALL(m_region_alloc_mock, malloc(_,_))
		.WillOnce(Return(nullptr))
		.WillOnce(Return(nullptr));


	const allocator_it_t* it = NULL;
	void* ptr = nullptr;
	ALLOCATORS_FOREACH(allocator_mgr, &requirements, it)
	{
		auto allocator = get_allocator_from_iterator(it);
		ptr = allocator->malloc(allocator, fake_size);
		if (ptr) break; // we found what we need
	}

	// Can't allocate memory at all
	ASSERT_EQ(nullptr, ptr);
}

////////////////////////////////////////////////////////////////////////////////
// Config tests
// Test that used correct buffer size:
//  - use from config when it exists
//  - use entire region size when no confi entry found
////////////////////////////////////////////////////////////////////////////////

class test_allocators_manager_config : public test_allocators_manager_advanced
{
protected:

	size_t fake_l3_region_size = 1000;
	size_t fake_l2_region_size = 100;
	void* fake_l3_region_ptr = (void*)0xdead1;
	void* fake_l2_region_ptr = (void*)0xdead2;

	memory_properties_t l3_requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	memory_properties_t l2_requirements = prop(.latency=LATENCY_L2, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	std::vector<memory_properties_t> regions = {
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=fake_l3_region_size, .type=TCC_MEM_L3),
		prop(.latency=LATENCY_L2, .mask=0b1100, .size=fake_l2_region_size, .type=TCC_MEM_L2)
	};

	void SetUp() override
	{
		EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
			.WillOnce(Return(fake_dram_allocator_ptr));
	}
};

TEST_F(test_allocators_manager_config, use_full_region_when_config_is_nullptr)
{
	expect_regions(regions);
	allocator_mgr = allocators_manager_create(region_mgr, nullptr);

	auto fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	auto fake_l2_allocator_ptr = create_real_region_allocator_ptr(fake_l2_region_ptr, ANY_SIZE);

	// L3 memory - use entire region size
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 0, fake_l3_region_size))
		.WillOnce(Return(fake_l3_region_ptr));
	// L2 memory - use entire region size
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 1, fake_l2_region_size))
		.WillOnce(Return(fake_l2_region_ptr));
	// L3 allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, fake_l3_region_size))
		.WillOnce(Return(fake_l3_allocator_ptr));
	// L2 allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l2_region_ptr, fake_l2_region_size))
		.WillOnce(Return(fake_l2_allocator_ptr));

	TCC_LOG_INFO("--- TEST BEGIN ---");

	auto it = allocators_manager_get_next(allocator_mgr, &l3_requirements, nullptr);
	ASSERT_NE(nullptr, it);
	EXPECT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it));

	it = allocators_manager_get_next(allocator_mgr, &l2_requirements, nullptr);
	ASSERT_NE(nullptr, it);
	EXPECT_EQ(fake_l2_allocator_ptr, get_allocator_from_iterator(it));
}

TEST_F(test_allocators_manager_config, use_full_region_when_config_has_no_entry_for_region)
{
	size_t fake_l2_config_size = 50;
	expect_regions(regions);
	tcc_config_holder config ({
		prop(.latency=LATENCY_L2, .mask=0b1100, .size=fake_l2_config_size, .type=TCC_MEM_L2),
	});
	allocator_mgr = allocators_manager_create(region_mgr, *config);

	auto fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);

	// L3 memory - it's not in config, so use entire region size
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 0, fake_l3_region_size))
		.WillOnce(Return(fake_l3_region_ptr));
	// L3 allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, fake_l3_region_size))
		.WillOnce(Return(fake_l3_allocator_ptr));

	TCC_LOG_INFO("--- TEST BEGIN ---");

	auto it = allocators_manager_get_next(allocator_mgr, &l3_requirements, nullptr);
	ASSERT_NE(nullptr, it);
	EXPECT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it));

}

TEST_F(test_allocators_manager_config, use_size_from_config)
{
	size_t fake_l3_config_size = 500;
	expect_regions(regions);
	tcc_config_holder config ({
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=fake_l3_config_size, .type=TCC_MEM_L3),
	});
	allocator_mgr = allocators_manager_create(region_mgr, *config);

	auto fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);

	// L3 memory - use size from config
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 0, fake_l3_config_size))
		.WillOnce(Return(fake_l3_region_ptr));
	// L3 allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, fake_l3_config_size))
		.WillOnce(Return(fake_l3_allocator_ptr));

	TCC_LOG_INFO("--- TEST BEGIN ---");

	auto it = allocators_manager_get_next(allocator_mgr, &l3_requirements, nullptr);
	ASSERT_NE(nullptr, it);
	EXPECT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it));
}

TEST_F(test_allocators_manager_config, unable_to_get_region_when_create_size_map_negative)
{
	size_t fake_l3_config_size = 500;
	expect_regions(regions);
	tcc_config_holder config ({
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=fake_l3_config_size, .type=TCC_MEM_L3),
	});
	allocator_mgr = allocators_manager_create(region_mgr, *config);

	EXPECT_CALL(m_rgn_manager_mock, region_manager_get(region_mgr, 0))
		.WillOnce(ReturnNull());

	TCC_LOG_INFO("--- TEST BEGIN ---");

	EXPECT_EQ(nullptr, allocators_manager_get_next(allocator_mgr, &l3_requirements, nullptr));
}

TEST_F(test_allocators_manager_config, DISABLED_unable_to_get_config_item_when_create_size_map_negative)
{
}

TEST_F(test_allocators_manager_config, use_full_region_when_config_has_zero_size)
{
	size_t local_l3_config_size = 0;
	expect_regions(regions);
	tcc_config_holder config ({
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=local_l3_config_size, .type=TCC_MEM_L3),
	});
	allocator_mgr = allocators_manager_create(region_mgr, *config);

	auto fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);

	// L3 memory - use size from config
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 0, fake_l3_region_size))
		.WillOnce(Return(fake_l3_region_ptr));
	// L3 allocator
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, fake_l3_region_size))
		.WillOnce(Return(fake_l3_allocator_ptr));

	TCC_LOG_INFO("--- TEST BEGIN ---");

	auto it = allocators_manager_get_next(allocator_mgr, &l3_requirements, nullptr);
	ASSERT_NE(nullptr, it);
	EXPECT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it));
}

////////////////////////////////////////////////////////////////////////////////
// Get allocator by ptr tests
////////////////////////////////////////////////////////////////////////////////

class test_allocators_manager_get_allocator_by_ptr : public test_allocators_manager_base
{
protected:
	std::vector<memory_properties_t> regions;

	// addresses of the regions
	void* fake_l3_region_ptr = (void*)1000;
	void* fake_l2_region_ptr = (void*)5000;
	// size of regions
	size_t fake_l3_region_size = 1000;
	size_t fake_l2_region_size = 100;

	// Fake allocators
	allocator_interface_t* fake_dram_allocator_ptr = create_real_dram_allocator_ptr();
	allocator_interface_t* fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_region_ptr, ANY_SIZE);
	allocator_interface_t* fake_l2_allocator_ptr = create_real_region_allocator_ptr(fake_l2_region_ptr, ANY_SIZE);

	// Requirements
	memory_properties_t dram_requirements = prop(.latency=LATENCY_DRAM, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	memory_properties_t l3_requirements = prop(.latency=LATENCY_L3, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);
	memory_properties_t l2_requirements = prop(.latency=LATENCY_L2, .mask=0b1000, .size=100, .type=DEFAULT_TYPE);

	void SetUp() override
	{
		regions = {
			prop(.latency=LATENCY_DRAM, .mask=0b1111, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM),
			prop(.latency=LATENCY_L3, .mask=0b1111, .size=fake_l3_region_size, .type=TCC_MEM_L3),
			prop(.latency=LATENCY_L2, .mask=0b1100, .size=fake_l2_region_size,  .type=TCC_MEM_L2)
		};
		expect_regions(regions);
		EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
			.WillOnce(Return(fake_dram_allocator_ptr));

		allocator_mgr = allocators_manager_create(region_mgr, nullptr);
		ASSERT_NE(nullptr, allocator_mgr);

		// Expect for memory mmap
		EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 1, _))
			.WillOnce(Return(fake_l3_region_ptr));
		EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 2, _))
			.WillOnce(Return(fake_l2_region_ptr));
		// Expect for allocators creation
		EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_region_ptr, _))
			.WillOnce(Return(fake_l3_allocator_ptr));
		EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l2_region_ptr, _))
			.WillOnce(Return(fake_l2_allocator_ptr));

		// Get suitable allocators
		auto it_l3 = allocators_manager_get_next(allocator_mgr, &l3_requirements, nullptr);
		ASSERT_EQ(fake_l3_allocator_ptr, get_allocator_from_iterator(it_l3));
		auto it_l2 = allocators_manager_get_next(allocator_mgr, &l2_requirements, nullptr);
		ASSERT_EQ(fake_l2_allocator_ptr, get_allocator_from_iterator(it_l2));

		TCC_LOG_INFO("---- TEST BEGIN ----");
	}

	void TearDown() override
	{
		TCC_LOG_INFO("---- TEST END ----");
		EXPECT_CALL(m_region_alloc_mock, destroy(_))
			.WillRepeatedly(DoDefault());
		EXPECT_CALL(m_dram_alloc_mock, destroy(_))
			.WillOnce(DoDefault());
		EXPECT_NO_THROW(allocators_manager_destroy(allocator_mgr));
	}
};


TEST_F(test_allocators_manager_get_allocator_by_ptr, self_is_null_negative)
{
	EXPECT_EQ(nullptr, allocators_manager_get_alloc_by_ptr(nullptr, (void*)0x123));
}

TEST_F(test_allocators_manager_get_allocator_by_ptr, pointer_is_null_negative)
{
	EXPECT_EQ(nullptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, nullptr));
}

TEST_F(test_allocators_manager_get_allocator_by_ptr, if_pointer_at_the_start_of_the_region_then_return_region_positive)
{
	void* fake_l3_ptr = fake_l3_region_ptr;
	void* fake_l2_ptr = fake_l2_region_ptr;
	EXPECT_EQ(fake_l3_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_l3_ptr));
	EXPECT_EQ(fake_l2_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_l2_ptr));
}

TEST_F(test_allocators_manager_get_allocator_by_ptr, if_pointer_in_the_middle_of_the_region_then_return_region_positive)
{
	void* fake_l3_ptr = (uint8_t*)fake_l3_region_ptr + fake_l3_region_size / 2;
	void* fake_l2_ptr = (uint8_t*)fake_l2_region_ptr + fake_l2_region_size / 2;
	EXPECT_EQ(fake_l3_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_l3_ptr));
	EXPECT_EQ(fake_l2_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_l2_ptr));
}

TEST_F(test_allocators_manager_get_allocator_by_ptr, if_pointer_at_the_end_of_the_region_then_return_region_positive)
{
	void* fake_l3_ptr = (uint8_t*)fake_l3_region_ptr + fake_l3_region_size - 1;
	void* fake_l2_ptr = (uint8_t*)fake_l2_region_ptr + fake_l2_region_size - 1;
	EXPECT_EQ(fake_l3_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_l3_ptr));
	EXPECT_EQ(fake_l2_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_l2_ptr));
}

TEST_F(test_allocators_manager_get_allocator_by_ptr, if_pointer_before_the_start_of_the_region_then_return_dram_positive)
{
	void* fake_bad_l3_ptr = (uint8_t*)fake_l3_region_ptr - 1;
	void* fake_bad_l2_ptr = (uint8_t*)fake_l2_region_ptr - 1;
	EXPECT_EQ(fake_dram_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_bad_l3_ptr));
	EXPECT_EQ(fake_dram_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_bad_l2_ptr));
}

TEST_F(test_allocators_manager_get_allocator_by_ptr, if_pointer_after_the_start_of_the_region_then_return_dram_positive)
{
	void* fake_bad_l3_ptr = (uint8_t*)fake_l3_region_ptr + fake_l3_region_size;
	void* fake_bad_l2_ptr = (uint8_t*)fake_l2_region_ptr + fake_l2_region_size;
	EXPECT_EQ(fake_dram_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_bad_l3_ptr));
	EXPECT_EQ(fake_dram_allocator_ptr, allocators_manager_get_alloc_by_ptr(allocator_mgr, fake_bad_l2_ptr));
}

////////////////////////////////////////////////////////////////////////////////
// Destroy tests
////////////////////////////////////////////////////////////////////////////////

class test_allocators_manager_destroy : public test_allocators_manager_base
{
};

TEST_F(test_allocators_manager_destroy, destroy_self_is_null_positive)
{
	ASSERT_NO_THROW(allocators_manager_destroy(nullptr));
}

TEST_F(test_allocators_manager_destroy, self_is_not_fully_created_positive)
{
	EXPECT_CALL(m_rgn_manager_mock, region_manager_count(region_mgr))
		.WillOnce(Return(0));
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.WillOnce(DoDefault());
	EXPECT_CALL(m_dram_alloc_mock, destroy(_))
		.WillOnce(DoDefault());
	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);
	ASSERT_NO_THROW(allocators_manager_destroy(manager));
}

TEST_F(test_allocators_manager_destroy, destroy_allocators_positive)
{
	expect_regions({
		prop(.latency=LATENCY_DRAM, .mask=0b1111, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM),
		prop(.latency=LATENCY_L3, .mask=0b1111, .size=1000, .type=TCC_MEM_L3),
		prop(.latency=LATENCY_L2, .mask=0b1100, .size=100, .type=TCC_MEM_L2)
	});

	void* fake_l3_ptr = (void*)0xabc1;
	void* fake_l2_ptr = (void*)0xabc2;

	auto fake_dram_allocator_ptr = create_real_dram_allocator_ptr();
	auto fake_l3_allocator_ptr = create_real_region_allocator_ptr(fake_l3_ptr, ANY_SIZE);
	auto fake_l2_allocator_ptr = create_real_region_allocator_ptr(fake_l2_ptr, ANY_SIZE);

	auto reqs_dram = prop(.latency=LATENCY_DRAM, .mask=0b1000, .size=10, .type=DEFAULT_TYPE);
	auto reqs_l3 = prop(.latency=LATENCY_L3, .mask=0b1000, .size=10, .type=DEFAULT_TYPE);
	auto reqs_l2 = prop(.latency=LATENCY_L2, .mask=0b1000, .size=10, .type=DEFAULT_TYPE);

	// Expect for memory mmap
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 1, _))
		.WillOnce(Return(fake_l3_ptr));
	EXPECT_CALL(m_rgn_manager_mock, region_manager_mmap(region_mgr, 2, _))
		.WillOnce(Return(fake_l2_ptr));

	// Expect for allocators creation
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.WillOnce(Return(fake_dram_allocator_ptr));
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l3_ptr, _))
		.WillOnce(Return(fake_l3_allocator_ptr));
	EXPECT_CALL(m_region_alloc_mock, region_allocator_create(fake_l2_ptr, _))
		.WillOnce(Return(fake_l2_allocator_ptr));

	// Expect for allocators destory
	EXPECT_CALL(m_dram_alloc_mock, destroy(fake_dram_allocator_ptr))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_region_alloc_mock, destroy(fake_l3_allocator_ptr))
		.WillOnce(DoDefault());
	EXPECT_CALL(m_region_alloc_mock, destroy(fake_l2_allocator_ptr))
		.WillOnce(DoDefault());

	// Create
	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);

	TCC_LOG_INFO("---- BEGIN ----");

	// Request allocators
	auto it_dram = allocators_manager_get_next(manager, &reqs_dram, nullptr);
	ASSERT_NE(nullptr, it_dram);

	auto it_l3 = allocators_manager_get_next(manager, &reqs_l3, nullptr);
	ASSERT_NE(nullptr, it_l3);

	auto it_l2 = allocators_manager_get_next(manager, &reqs_l2, nullptr);
	ASSERT_NE(nullptr, it_l2);

	TCC_LOG_INFO("---- END ----");

	// Destroy
	ASSERT_NO_THROW(allocators_manager_destroy(manager));
}

TEST_F(test_allocators_manager_destroy, dram_allocator_destroyed_only_once)
{
	// DRAM allocator is created once in constructor and then the pointer
	// to the instance is stored in the "allocators" map. In theory,
	// multiple pointers to the same instance may be stored

	unsigned int bad_latency_1 = 100;
	unsigned int bad_latency_2 = 10;
	expect_regions({
		// Yes, multiple DRAM regions. the region_manager can has a multiple
		// DRAM regions by the issue, but the allocators_manager shouldn't crash
		prop(.latency=bad_latency_1, .mask=0b1111, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM),
		prop(.latency=bad_latency_2, .mask=0b1111, .size=DEFAULT_SIZE, .type=TCC_MEM_DRAM),
	});

	auto fake_dram_allocator_ptr = create_real_dram_allocator_ptr();
	auto reqs_dram1 = prop(.latency=bad_latency_1, .mask=0b1000, .size=10, .type=DEFAULT_TYPE);
	auto reqs_dram2 = prop(.latency=bad_latency_2, .mask=0b1000, .size=10, .type=DEFAULT_TYPE);

	// Expect for allocators creation
	EXPECT_CALL(m_dram_alloc_mock, dram_allocator_create())
		.WillOnce(Return(fake_dram_allocator_ptr));
	// Expect for allocators destory
	EXPECT_CALL(m_dram_alloc_mock, destroy(fake_dram_allocator_ptr))
		.WillOnce(DoDefault());

	// Create
	auto manager = allocators_manager_create(region_mgr, nullptr);
	ASSERT_NE(nullptr, manager);

	TCC_LOG_INFO("---- BEGIN ----");

	// Request allocators
	auto it_dram1 = allocators_manager_get_next(manager, &reqs_dram1, nullptr);
	ASSERT_NE(nullptr, it_dram1);

	auto it_dram2 = allocators_manager_get_next(manager, &reqs_dram2, nullptr);
	ASSERT_NE(nullptr, it_dram2);

	TCC_LOG_INFO("---- END ----");

	// Destroy
	ASSERT_NO_THROW(allocators_manager_destroy(manager));
}

}
