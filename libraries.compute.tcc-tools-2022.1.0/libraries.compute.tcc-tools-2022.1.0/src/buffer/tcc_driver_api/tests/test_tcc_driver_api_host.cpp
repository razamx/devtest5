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
#include <libc_mock.hpp>
#include <alloc_mock.hpp>
#include <cstdarg>
#include "errno.h"
#include <linux/tcc_buffer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "gtest_helpers.hpp"
#include "memory_properties.h"
#include "SubprocessTest.hpp"
#include "tcc_driver_api.h"
#include "tcc_time_fake.hpp"

#define PAGE_SIZE 4096
#define FAKE_ERRNO_CODE EFBIG

namespace
{

using namespace testing;

class tcc_driver_api_simple : public SubprocessTest<Test>
{
protected:
	libc_mock m_libc_mock;
	void SetUp() override
	{
		errno = 0;
	}
	void TearDown() override
	{
		errno = 0;
	}
};

TEST_F(tcc_driver_api_simple, tcc_driver_get_memory_config_out_nullptr_negative)
{
	EXPECT_EQ(-1, tcc_driver_get_memory_config(0, nullptr));
	EXPECT_EQ(EINVAL, errno);
}

TEST_F(tcc_driver_api_simple, tcc_driver_req_buffer_size_0_ngative)
{
	EXPECT_EQ(-1, tcc_driver_req_buffer(0, 0));
	EXPECT_EQ(EINVAL, errno);
}
TEST_F(tcc_driver_api_simple, tcc_driver_read_rtct_rtct_null_size_null_failure)
{
	EXPECT_EQ(-1, tcc_driver_read_rtct(nullptr, nullptr));
	EXPECT_EQ(EINVAL, errno);
}

TEST_F(tcc_driver_api_simple, tcc_driver_read_rtct_rtct_fake_size_null_failure)
{
	void** fake_header  = reinterpret_cast<void**>(0xBEDABEDA);
	EXPECT_EQ(-1, tcc_driver_read_rtct(fake_header, nullptr));
	EXPECT_EQ(EINVAL, errno);
}

TEST_F(tcc_driver_api_simple, tcc_driver_read_rtct_rtct_null_size_fake_failure)
{
	size_t* fake_size  = reinterpret_cast<size_t*>(0xBEDABEDA);
	EXPECT_EQ(-1, tcc_driver_read_rtct(nullptr, fake_size));
	EXPECT_EQ(EINVAL, errno);
}


class tcc_driver_api_fd_negative : public tcc_driver_api_simple
{
protected:
	void SetUp() override
	{
		EXPECT_CALL(m_libc_mock, open(StrEq(TCC_BUFFER_NAME), O_RDWR, _))
			.WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
	}
	void TearDown() override
	{
		EXPECT_EQ(FAKE_ERRNO_CODE, errno);
	}

};

SUBPROCESS_TEST_F(tcc_driver_api_fd_negative, tcc_driver_get_region_count_negative)
{
	EXPECT_EQ(-1, tcc_driver_get_region_count());
}

SUBPROCESS_TEST_F(tcc_driver_api_fd_negative, tcc_driver_get_memory_config_out_nullptr_negative)
{
	memory_properties_t properties;
	memset(&properties, 0, sizeof(properties));
	EXPECT_EQ(-1, tcc_driver_get_memory_config(0, &properties));
}

SUBPROCESS_TEST_F(tcc_driver_api_fd_negative, tcc_driver_req_buffer_negative)
{
	EXPECT_EQ(-1, tcc_driver_req_buffer(0, PAGE_SIZE));
}

SUBPROCESS_TEST_F(tcc_driver_api_fd_negative, tcc_driver_read_rtct_negative)
{
	size_t fake_size = 0;
	void* fake_header  = nullptr;
	EXPECT_EQ(-1, tcc_driver_read_rtct(&fake_header, &fake_size));
}

class tcc_driver_api_base: public tcc_driver_api_simple
{
protected:
	StrictMock<tcc_cpuid_mock> m_tcc_cpuid_mock; //requered for tcc_time_fake
};

class tcc_driver_api : public tcc_time_fake<tcc_driver_api_base>
{
protected:
	static const int FAKE_FD;
	static const int FAKE_REG_COUNT;
	static const unsigned int FAKE_REGION_ID;
	static const unsigned int FAKE_SIZE;
	static const unsigned int FAKE_BUFFER_SIZE;
	static const int FAKE_DEVNODE;
	static const int FAKE_RTCT_SIZE;

	NiceMock<alloc_mock> m_alloc_mock;

	void SetUp() override
	{
		tcc_driver_api_simple::SetUp();
    	EXPECT_CALL(m_libc_mock, open(StrEq(TCC_BUFFER_NAME), O_RDWR, _)).WillOnce(Return(FAKE_FD));
	}
	void TearDown() override
	{
		tcc_driver_api_simple::TearDown();
	}

};
const int tcc_driver_api::FAKE_FD = 42;
const int tcc_driver_api::FAKE_REG_COUNT = 37;
const unsigned int tcc_driver_api::FAKE_REGION_ID = 7;
const unsigned int tcc_driver_api::FAKE_SIZE = 1234;
const unsigned int tcc_driver_api::FAKE_BUFFER_SIZE = 12345;
const int tcc_driver_api::FAKE_DEVNODE = 57;
const int tcc_driver_api::FAKE_RTCT_SIZE = 128;

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_get_region_count_P1_positive)
{
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_GET_REGION_COUNT, _))
			.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		unsigned int* region_count = va_arg(list, unsigned int*);
		EXPECT_NE(nullptr, region_count);
		if(region_count) *region_count=FAKE_REG_COUNT;
		return 0;
	}));
	ASSERT_EQ(FAKE_REG_COUNT, tcc_driver_get_region_count());
}

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_get_memory_config_P1_positive)
{
	memory_properties_t FAKE_PROPERTIES;
	memset(&FAKE_PROPERTIES, 0, sizeof(FAKE_PROPERTIES));
	CPU_SET(3, &(FAKE_PROPERTIES.mask));
	CPU_SET(16, &(FAKE_PROPERTIES.mask));
	FAKE_PROPERTIES.latency=777000000;
	FAKE_PROPERTIES.size=12345;
	FAKE_PROPERTIES.type=TCC_MEM_EDRAM;
	FAKE_PROPERTIES.id = FAKE_REGION_ID;

	memory_properties_t properties;
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_GET_MEMORY_CONFIG, _))
			.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		struct tcc_buf_mem_config_s* props = va_arg(list, struct tcc_buf_mem_config_s*);
		EXPECT_NE(nullptr, props);
		EXPECT_NE(nullptr, props->cpu_mask_p);
		EXPECT_EQ(FAKE_REGION_ID, props->id);
		*static_cast<cpu_set_t*>(props->cpu_mask_p) = FAKE_PROPERTIES.mask;
		props->latency=FAKE_PROPERTIES.latency;
		props->size=FAKE_PROPERTIES.size;
		props->ways=0;
		props->type=RGN_EDRAM;
		return 0;
	}));
	ASSERT_EQ(0, tcc_driver_get_memory_config(FAKE_REGION_ID, &properties));
	EXPECT_EQ(FAKE_REGION_ID, properties.id);
	EXPECT_EQ(FAKE_PROPERTIES.type, properties.type);
	EXPECT_EQ(memcmp(&FAKE_PROPERTIES.mask, &properties.mask, sizeof(cpu_set_t)), 0);
	EXPECT_EQ(FAKE_PROPERTIES.latency, properties.latency);
	EXPECT_EQ((FAKE_PROPERTIES.size/PAGE_SIZE)*PAGE_SIZE, properties.size);
}

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_req_buffer_P1_positive)
{
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_REQ_BUFFER, _))
			.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		struct tcc_buf_mem_req_s* requared = va_arg(list, struct tcc_buf_mem_req_s*);
		EXPECT_NE(nullptr, requared);
		EXPECT_EQ(requared->id, FAKE_REGION_ID);
		EXPECT_EQ(requared->size, FAKE_SIZE);
		requared->size = FAKE_BUFFER_SIZE;
		requared->devnode = FAKE_DEVNODE;
		return 0;
	}));
	ASSERT_EQ(FAKE_DEVNODE, tcc_driver_req_buffer(FAKE_REGION_ID, FAKE_SIZE));
}

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_read_rtct_cant_read_ptrct_size_negative)
{
	size_t fake_size = 0;
	void* fake_header  = nullptr;
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_QUERY_RTCT_SIZE, _))
		.WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
	EXPECT_EQ(-1, tcc_driver_read_rtct(&fake_header, &fake_size));
	EXPECT_EQ(fake_header, nullptr);
	EXPECT_EQ(errno, FAKE_ERRNO_CODE);
}

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_read_rtct_cant_acclocate_memory_for_rtct_negative)
{
	size_t fake_size = 0;
	void* fake_header  = nullptr;
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_QUERY_RTCT_SIZE, _))
		.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		size_t* size = va_arg(list, size_t*);
		*size = FAKE_RTCT_SIZE;
		return 0;
	}));
	EXPECT_CALL(m_alloc_mock, calloc(1, FAKE_RTCT_SIZE)).WillOnce(ReturnNull());
	EXPECT_EQ(-1, tcc_driver_read_rtct(&fake_header, &fake_size));
	EXPECT_EQ(fake_header, nullptr);
}

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_read_rtct_cant_read_rtct_negative)
{
	size_t fake_size = 0;
	void* fake_header  = nullptr;
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_QUERY_RTCT_SIZE, _))
		.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		size_t* size = va_arg(list, size_t*);
		*size = FAKE_RTCT_SIZE;
		return 0;
	}));
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_GET_RTCT, _))
		.WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
	EXPECT_EQ(-1, tcc_driver_read_rtct(&fake_header, &fake_size));
	EXPECT_EQ(fake_header, nullptr);
	EXPECT_EQ(errno, FAKE_ERRNO_CODE);
}

SUBPROCESS_TEST_F(tcc_driver_api, tcc_driver_read_rtct_success)
{
	size_t fake_size = 0;
	void* fake_header  = nullptr;
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_QUERY_RTCT_SIZE, _))
		.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		size_t* size = va_arg(list, size_t*);
		*size = FAKE_RTCT_SIZE;
		return 0;
	}));
	EXPECT_CALL(m_libc_mock, ioctl(FAKE_FD, TCC_GET_RTCT, _))
		.WillOnce(Invoke([&, this](int, unsigned long, va_list list){
		void** rtct = va_arg(list, void**);
		EXPECT_NE(rtct, nullptr);
		return 0;
	}));
	EXPECT_EQ(0, tcc_driver_read_rtct(&fake_header, &fake_size));
	EXPECT_NE(fake_header, nullptr);
	EXPECT_EQ(fake_size, static_cast<size_t>(FAKE_RTCT_SIZE));
	EXPECT_EQ(errno, 0);
	free(fake_header);
}

class tcc_driver_api_ioctl_failure : public tcc_driver_api
{
protected:

	void SetUp() override
	{
		tcc_driver_api::SetUp();
		EXPECT_CALL(m_libc_mock, ioctl(_, _, _)).WillOnce(RETURN_NEGATIVE_AND_SET_ERRNO(FAKE_ERRNO_CODE));
	}
	void TearDown() override
	{
		EXPECT_EQ(FAKE_ERRNO_CODE, errno);
		tcc_driver_api::TearDown();
	}

};

SUBPROCESS_TEST_F(tcc_driver_api_ioctl_failure, tcc_driver_get_region_count_positive)
{
	EXPECT_EQ(-1, tcc_driver_get_region_count());
}

SUBPROCESS_TEST_F(tcc_driver_api_ioctl_failure, tcc_driver_get_memory_config_positive)
{
	memory_properties_t properties;
	EXPECT_EQ(-1, tcc_driver_get_memory_config(FAKE_REGION_ID, &properties));
}

SUBPROCESS_TEST_F(tcc_driver_api_ioctl_failure, tcc_driver_req_buffer_positive)
{
	EXPECT_EQ(-1, tcc_driver_req_buffer(FAKE_REGION_ID, FAKE_SIZE));
}

//*/
}
