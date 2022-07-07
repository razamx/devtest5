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
#include <gmock/gmock.h>

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <sys/mman.h>
#include "tcc_log.h"
#include "TccDriverFixture.hpp"
#include "tcc_buffer_test_helpers.hpp"
//#ifndef TCC_BUFFER_DRIVER_TEST_TARGET
//#endif //TCC_BUFFER_DRIVER_TEST_TARGET

#define DRIVER_PATH  "/dev/tcc/tcc_buffer"
#define SHM_PREFIX   "/dev/tcc/tcc_buffer"

namespace
{
using namespace ::testing;
using ::testing::Mock;

#ifdef TCC_BUFFER_DRIVER_TEST_TARGET
class Testing : public Test{
protected:
	const int DEFAULT_CACHE_REGION_ID = 0;
  	const size_t DEFAULT_CACHE_REGION_SIZE = 1024*128;
};
#else
using namespace ::tcc_buffer_test_utils;
class TestingBase: public
	TccDriverFixture<
	LibcMockFixture<
	Test
>>{};

class Testing : public TestingBase
{
protected:
	void SetUp() override{
		TestingBase::SetUp();
	}
	const int DEFAULT_CACHE_REGION_ID = INDEX_L2;
  	const size_t DEFAULT_CACHE_REGION_SIZE = SIZE_L2;
};
#endif // TCC_BUFFER_DRIVER_TEST_TARGET

class test_tcc_driver_host_base: public Testing
{
protected:
	const std::string get_shm_name(unsigned int nodeid)
	{
		return SHM_PREFIX + std::to_string(nodeid);
	}
};

class test_tcc_driver_open_close : public test_tcc_driver_host_base
{
protected:

	int driver_fd = -1;

	void SetUp() override
	{
		test_tcc_driver_host_base::SetUp();
		driver_fd = open(DRIVER_PATH, O_RDWR);
		ASSERT_GT(driver_fd, 0);
		TCC_LOG_INFO("---- TEST BEGIN ----");
	}

	void TearDown() override
	{
		TCC_LOG_INFO("---- TEST END ----");
		int ret = close(driver_fd);
		ASSERT_EQ(0, ret);
		test_tcc_driver_host_base::TearDown();
	}
};

class test_tcc_driver_check_count : public test_tcc_driver_host_base
{
protected:

	int driver_fd = -1;
	int region_count = -1;

	void SetUp() override
	{
		test_tcc_driver_host_base::SetUp();
		driver_fd = open(DRIVER_PATH, O_RDWR);
		ASSERT_GT(driver_fd, 0);

		int ret = ioctl(driver_fd, TCC_GET_REGION_COUNT, &region_count);
		ASSERT_EQ(0, ret);
		ASSERT_GT(region_count, 0);

		TCC_LOG_INFO("---- TEST BEGIN ----");
	}

	void TearDown() override
	{
		TCC_LOG_INFO("---- TEST END ----");
		int ret = close(driver_fd);
		ASSERT_EQ(0, ret);
		test_tcc_driver_host_base::TearDown();
	}
};

////////////////////////////////////////////////////////////////////////////////
/// Basic open-close tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_driver_open_close, open_and_close_positive)
{
	int fd = open(DRIVER_PATH, O_RDWR);
	ASSERT_GT(fd, 0);
	int ret = close(fd);
	ASSERT_EQ(0, ret);
}

////////////////////////////////////////////////////////////////////////////////
/// TCC_GET_REGION_COUNT tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_driver_open_close, get_number_of_regions_positive)
{
	int region_count = -1;
	int ret = ioctl(driver_fd, TCC_GET_REGION_COUNT, &region_count);
	ASSERT_EQ(0, ret);
	EXPECT_GT(region_count, 0);
}

TEST_F(test_tcc_driver_open_close, get_number_of_regions_arg_is_nullptr_negative)
{
	int ret = ioctl(driver_fd, TCC_GET_REGION_COUNT, nullptr);
	EXPECT_LT(ret, 0);
}

////////////////////////////////////////////////////////////////////////////////
/// TCC_GET_MEMORY_CONFIG tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_driver_open_close, get_memory_config_arg_is_nullptr_negative)
{
	int ret = ioctl(driver_fd, TCC_GET_MEMORY_CONFIG, nullptr);
	EXPECT_LT(ret, 0);
}

TEST_F(test_tcc_driver_check_count, get_memory_config_positive)
{
	for (unsigned int region_id = 0; region_id < region_count; region_id++) {
		tcc_buf_mem_config_s region_config;
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		region_config.cpu_mask_p=&cpu_set;
		region_config.id = region_id;
		int ret = ioctl(driver_fd, TCC_GET_MEMORY_CONFIG, &region_config);
		TCC_LOG_TRACE("ioctl return: %d", ret);
		TCC_LOG_TRACE("latency: %u, size: %zu, type: %d", region_config.latency, region_config.size, region_config.type);

		EXPECT_EQ(0, ret)<< region_id;

		EXPECT_GT(region_config.latency, 0) << region_id;;
		EXPECT_GT(region_config.size, 0) << region_id;;
		EXPECT_GE(region_config.type, RGN_L2) << region_id;;

		TCC_LOG_TRACE("cpu_mask %p", region_config.cpu_mask_p) ;

		int bit_cnt = CPU_COUNT((cpu_set_t*)region_config.cpu_mask_p);
		EXPECT_GT(bit_cnt, 0);

		if(region_config.type != RGN_MALLOC) {
			TCC_LOG_TRACE("%u", region_config.ways);
			EXPECT_GT(region_config.ways, 0)<< region_id;
		}
	}
}

TEST_F(test_tcc_driver_check_count, get_memory_config_bitmask_is_nullptr_negative)
{
	tcc_buf_mem_config_s region_config;
	region_config.cpu_mask_p=nullptr;
	region_config.id = 0;

	int ret = ioctl(driver_fd, TCC_GET_MEMORY_CONFIG, &region_config);
	EXPECT_LT(ret, 0);
}

TEST_F(test_tcc_driver_check_count, get_memory_config_index_is_out_of_range_negative)
{
	tcc_buf_mem_config_s region_config;
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	region_config.cpu_mask_p=&cpu_set;
	region_config.id = region_count;

	int ret = ioctl(driver_fd, TCC_GET_MEMORY_CONFIG, &region_config);
	EXPECT_LT(ret, 0);
	EXPECT_GT(errno, 0);
}

////////////////////////////////////////////////////////////////////////////////
/// TCC_REQ_BUFFER tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_driver_check_count, req_buffer_positive)
{
	struct tcc_buf_mem_req_s data;
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = DEFAULT_CACHE_REGION_ID;
	data.size = 4096;

	int ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	ASSERT_EQ(0, ret);
	ASSERT_GE(data.devnode, 0);

	// clean-up
	TCC_LOG_TRACE("Clean-up");
	auto shm_path = get_shm_name(data.devnode);
	ret = unlink(shm_path.c_str());
	EXPECT_EQ(0, ret);
}


TEST_F(test_tcc_driver_check_count, req_buffer_index_is_out_of_range_negative)
{
	struct tcc_buf_mem_req_s data;
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = region_count+1;
	data.size = 4096;

	int ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	EXPECT_LT(ret, 0);
}
#ifdef TCC_BUFFER_DRIVER_TEST_TARGET
// This test is need only for integration. They not test any internal behaviour
TEST_F(test_tcc_driver_check_count, req_buffer_not_enough_memory_negative)
{
	int ret;

	// first request, request entire L2 region
	struct tcc_buf_mem_req_s data1;
	memset(&data1, 0, sizeof(data1));
	data1.devnode = -1;
	data1.id = DEFAULT_CACHE_REGION_ID;
	data1.size = DEFAULT_CACHE_REGION_SIZE;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data1);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data1.devnode, 0);

	// second request, request entire L2 region
	// we can't request so much memory
	struct tcc_buf_mem_req_s data2;
	memset(&data2, 0, sizeof(data2));
	data2.devnode = -1;
	data2.id = DEFAULT_CACHE_REGION_ID;
	data2.size = DEFAULT_CACHE_REGION_SIZE;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data2);
	ASSERT_LT(ret, 0);

	// clean-up
	TCC_LOG_TRACE("Clean-up");
	auto shm_path = get_shm_name(data1.devnode);
	ret = unlink(shm_path.c_str());
	EXPECT_EQ(0, ret);
}
#endif //TCC_BUFFER_DRIVER_TEST_TARGET
////////////////////////////////////////////////////////////////////////////////
/// Open and mmap buffer tests
////////////////////////////////////////////////////////////////////////////////

TEST_F(test_tcc_driver_check_count, open_shm_device_positive)
{
	size_t size = 4096;
	int ret;

	struct tcc_buf_mem_req_s data;
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = DEFAULT_CACHE_REGION_ID;
	data.size = size;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data.devnode, 0);
	auto shm_path = get_shm_name(data.devnode);

	int shm_fd = open(shm_path.c_str(), O_RDWR);
	ASSERT_GE(shm_fd, 0);

	// clean-up
	TCC_LOG_TRACE("Clean-up");
	ret = unlink(shm_path.c_str());
	EXPECT_EQ(0, ret);
	ret = close(shm_fd);
	EXPECT_EQ(0, ret);
}

TEST_F(test_tcc_driver_check_count, mmap_shm_device_positive)
{
	size_t map_size = 4096;

	struct tcc_buf_mem_req_s data;
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = DEFAULT_CACHE_REGION_ID;
	data.size = map_size;

	int ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data.devnode, 0);
	auto shm_path = get_shm_name(data.devnode);

	int shm_fd = open(shm_path.c_str(), O_RDWR);
	ASSERT_GE(shm_fd, 0);

	void* mem = mmap(NULL, map_size,  PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, shm_fd, 0);
	ASSERT_NE(MAP_FAILED, mem);

	// clean-up
	TCC_LOG_TRACE("Clean-up");
	ret = unlink(shm_path.c_str());
	EXPECT_EQ(0, ret);
	ret = close(shm_fd);
	EXPECT_EQ(0, ret);
}

TEST_F(test_tcc_driver_check_count, request_and_mmap_shm_device_multiple_times_positive)
{
	size_t map_size = 4096;
	int ret;

	// Request #1
	struct tcc_buf_mem_req_s data1;
	memset(&data1, 0, sizeof(data1));
	data1.devnode = -1;
	data1.id = DEFAULT_CACHE_REGION_ID;
	data1.size = map_size;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data1);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data1.devnode, 0);
	auto shm1_path = get_shm_name(data1.devnode);

	int shm1_fd = open(shm1_path.c_str(), O_RDWR);
	ASSERT_GE(shm1_fd, 0);

	void* mem = mmap(NULL, map_size,  PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, shm1_fd, 0);
	ASSERT_NE(MAP_FAILED, mem);

	// Request #2
	struct tcc_buf_mem_req_s data2;
	memset(&data2, 0, sizeof(data2));
	data2.devnode = -1;
	data2.id = DEFAULT_CACHE_REGION_ID;
	data2.size = map_size;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data2);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data2.devnode, 0);
	auto shm2_path = get_shm_name(data2.devnode);

	int shm2_fd = open(shm2_path.c_str(), O_RDWR);
	ASSERT_GE(shm2_fd, 0);

	mem = mmap(NULL, map_size,  PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, shm2_fd, 0);
	ASSERT_NE(MAP_FAILED, mem);

	// Clean-up
	TCC_LOG_TRACE("Clean-up");
	ret = unlink(shm1_path.c_str());
	EXPECT_EQ(0, ret);
	ret = close(shm1_fd);
	EXPECT_EQ(0, ret);

	ret = unlink(shm2_path.c_str());
	EXPECT_EQ(0, ret);
	ret = close(shm2_fd);
	EXPECT_EQ(0, ret);
}

TEST_F(test_tcc_driver_check_count, destroy_shm_device_positive)
{
	size_t map_size = 4096;
	int ret;

	struct tcc_buf_mem_req_s data;
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = DEFAULT_CACHE_REGION_ID;
	data.size = map_size;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data.devnode, 0);
	auto shm_path = get_shm_name(data.devnode);

	int shm_fd = open(shm_path.c_str(), O_RDWR);
	ASSERT_GE(shm_fd, 0);

	void* mem = mmap(NULL, map_size,  PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, shm_fd, 0);
	ASSERT_NE(MAP_FAILED, mem);

	// Clean-up
	TCC_LOG_TRACE("Clean-up");
	EXPECT_EQ(0, unlink(shm_path.c_str()));
	EXPECT_EQ(0, close(shm_fd));
}

TEST_F(test_tcc_driver_check_count, request_memory_after_free_positive)
{
	size_t map_size = 4096;
	int ret;

	struct tcc_buf_mem_req_s data;

	// Request entire L2 region
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = DEFAULT_CACHE_REGION_ID;
	data.size = map_size;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data.devnode, 0);
	auto shm1_path = get_shm_name(data.devnode);

	int fd = open(shm1_path.c_str(), O_RDWR);
	ASSERT_GE(fd, 0);

	void* mem = mmap(NULL, map_size,  PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);
	ASSERT_NE(MAP_FAILED, mem);

	EXPECT_EQ(0, unlink(shm1_path.c_str()));
	EXPECT_EQ(0, close(fd));

	// Request entire L2 region again
	// If the destroy failed, we can't request again
	memset(&data, 0, sizeof(data));
	data.devnode = -1;
	data.id = DEFAULT_CACHE_REGION_ID;
	data.size = map_size;

	ret = ioctl(driver_fd, TCC_REQ_BUFFER, &data);
	ASSERT_EQ(ret, 0);
	ASSERT_GE(data.devnode, 0);
	auto shm2_path = get_shm_name(data.devnode);

	fd = open(shm2_path.c_str(), O_RDWR);
	ASSERT_GE(fd, 0);

	mem = mmap(NULL, map_size,  PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fd, 0);
	ASSERT_NE(MAP_FAILED, mem);

	// Clean-up
	TCC_LOG_TRACE("Clean-up");
	EXPECT_EQ(0, unlink(shm2_path.c_str()));
	EXPECT_EQ(0, close(fd));
}

}
