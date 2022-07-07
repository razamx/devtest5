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
#undef NO_TCC_MEASUREMENT //Not needed for measurement library

#include <ittnotify/ittnotify.h>
#include "tcc/err_code.h"
#include <gtest/gtest.h>
#include "alloc_mock.hpp"
#include "tcc_measurement_mock.hpp"
#include "libc_mock.hpp"
#include <iostream>
#include <cstdarg>

#define UNICODE_AGNOSTIC(name) name

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

extern "C"
{
__itt_string_handle* ITTAPI UNICODE_AGNOSTIC(string_handle_create)(const char* name);
__itt_domain* UNICODE_AGNOSTIC(domain_create)(const char* name);
int __tcc_measurement_get(__itt_domain* domain __attribute__ ((unused)), __itt_string_handle* handle, struct tcc_measurement**measurement);
extern void __tcc_measurement_itt_clean_state(void);
void task_begin(const __itt_domain *pDomain, __itt_id taskid, __itt_id parentid, __itt_string_handle *pName);
void tcc_itt_deinit();
}

/// ////////////////////////////////////////////////////////////////////////////
/// TCC measurement ITT test
/// ////////////////////////////////////////////////////////////////////////////

namespace {
using namespace testing;

struct tcc_measurement get_fake_measurement()
{
	struct tcc_measurement fake_tcc_measurement;
	for(size_t i = 0;i<sizeof(fake_tcc_measurement); i++)
	{
		((uint8_t*)&fake_tcc_measurement)[i] = (uint8_t)rand()%256;
	}
	return fake_tcc_measurement;
}

class test_tcc_measurement_itt_host: public Test {

protected:
	NiceMock<alloc_mock> m_alloc_mock;
	StrictMock<measurement_mock> m_measurement_mock;
	StrictMock<libc_mock> m_libc_mock;
	void SetUp() override {
		__tcc_measurement_itt_clean_state();
	}

	void TearDown() override {
		__tcc_measurement_itt_clean_state();
	}
};

class test_tcc_measurement_itt_deinit_host: public test_tcc_measurement_itt_host {

protected:
	void SetUp() override {
		test_tcc_measurement_itt_host::SetUp();
	}

	void TearDown() override {
		test_tcc_measurement_itt_host::TearDown();
	}

	void init_handle_expect(const char* fake_handle_name)
	{
		size_t fake_buffer_size=42;
		int fake_shared_memory_flag = 0;

		EXPECT_CALL(m_measurement_mock, tcc_measurement_shared_memory_flag_from_env())
			.WillOnce(Return(fake_shared_memory_flag));
		EXPECT_CALL(m_measurement_mock, tcc_measurement_get_buffer_size_from_env(StrEq(fake_handle_name)))
			.WillOnce(Return(fake_buffer_size));
		EXPECT_CALL(m_measurement_mock,
			    tcc_measurement_init_ex(
				    NotNull(),
				    fake_buffer_size,
				    0,
				    nullptr,
				    fake_shared_memory_flag,
				    StrEq(fake_handle_name),
				    _))
			.WillOnce(Return(TCC_E_SUCCESS));
	}

	void destroy_handle_expect()
	{
		EXPECT_CALL(m_measurement_mock, tcc_measurement_destroy(_))
			.WillOnce(Return(TCC_E_SUCCESS));
	}
};


TEST_F(test_tcc_measurement_itt_host, domain_create_nullptr_positive)
{
	EXPECT_NE((__itt_domain*)nullptr, domain_create(nullptr));
}

TEST_F(test_tcc_measurement_itt_host, domain_create_not_nullptr_positive)
{
	EXPECT_NE((__itt_domain*)nullptr, domain_create("fake_domain_name"));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_nullptr_negative)
{
	EXPECT_EQ((__itt_string_handle*)nullptr, string_handle_create(nullptr));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_calloc_negative)
{
	EXPECT_CALL(m_alloc_mock, calloc(1, _)).WillOnce(ReturnNull());
	EXPECT_EQ(nullptr, string_handle_create("fake_string_handle_name"));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_strdup_negative)
{
	EXPECT_CALL(m_alloc_mock, strdup(_)).WillOnce(ReturnNull());
	EXPECT_EQ(nullptr, string_handle_create("fake_string_handle_name"));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_calloc_2_negative)
{
	EXPECT_CALL(m_alloc_mock, calloc(1, _))
			.Times(1)
			.WillOnce(ReturnNull())
			.RetiresOnSaturation();
	EXPECT_EQ(nullptr, string_handle_create("fake_string_handle_name"));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_malloc_negative)
{
	EXPECT_CALL(m_alloc_mock, malloc(_)).WillOnce(ReturnNull());
	EXPECT_EQ(nullptr, string_handle_create("fake_string_handle_name"));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_tcc_measurement_init_ex_negative)
{

	size_t fake_buffer_size=42;
	int fake_shared_memory_flag = 1;
	const char* fake_string_handle_name = "fake_string_handle_name";
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name)))
			.WillOnce(Return(fake_buffer_size));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_shared_memory_flag_from_env())
			.WillOnce(Return(fake_shared_memory_flag));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name),
			    _)).Times(1);
	EXPECT_EQ(nullptr, string_handle_create(fake_string_handle_name));
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_positive)
{

	size_t fake_buffer_size=42;
	int fake_shared_memory_flag = 1;
	const char* fake_string_handle_name = "fake_string_handle_name";
	__itt_string_handle* handle = (__itt_string_handle*)__real_calloc(1, sizeof(__itt_string_handle));
	struct tcc_measurement fake_tcc_measurement = get_fake_measurement();
	struct tcc_measurement* measurement = (struct tcc_measurement*)__real_malloc(sizeof(struct tcc_measurement));

	EXPECT_CALL(m_alloc_mock, malloc(_))
			.WillOnce(Return(measurement));
	EXPECT_CALL(m_alloc_mock, calloc(_,_))
			.WillRepeatedly(Invoke(__real_calloc));
	EXPECT_CALL(m_alloc_mock, calloc(1, sizeof(__itt_string_handle)))
			.WillOnce(Return(handle));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name)))
			.WillOnce(Return(fake_buffer_size));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_shared_memory_flag_from_env())
			.WillOnce(Return(fake_shared_memory_flag));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name),
			    _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t , uint64_t ,
					 deadline_callback_t ,
					 bool , const char* , va_list ){
		*measurement = fake_tcc_measurement;
		return TCC_E_SUCCESS;
	}));
	EXPECT_CALL(m_measurement_mock,tcc_measurement_destroy(measurement))
			.Times(1);

	EXPECT_EQ(handle, string_handle_create(fake_string_handle_name));
	EXPECT_STREQ(fake_string_handle_name, handle->strA);
	EXPECT_EQ(nullptr, handle->strW);
	EXPECT_TRUE(memcmp(handle->extra2, &fake_tcc_measurement, sizeof(fake_tcc_measurement)) == 0);
	EXPECT_EQ(0, handle->extra1);
	EXPECT_EQ(nullptr, handle->next);
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_twice_positive)
{

	size_t fake_buffer_size=42;
	int fake_shared_memory_flag = 0;
	const char* fake_string_handle_name1 = "fake_string_handle_name1";
	const char* fake_string_handle_name2 = "fake_string_handle_name2";
	__itt_string_handle* handle1 =
			(__itt_string_handle*)__real_calloc(1, sizeof(__itt_string_handle));
	__itt_string_handle* handle2 =
			(__itt_string_handle*)__real_calloc(1, sizeof(__itt_string_handle));
	struct tcc_measurement fake_tcc_measurement1 = get_fake_measurement();
	struct tcc_measurement fake_tcc_measurement2 = get_fake_measurement();
	struct tcc_measurement* measurement1 = (struct tcc_measurement*)__real_malloc(sizeof(struct tcc_measurement));
	struct tcc_measurement* measurement2 = (struct tcc_measurement*)__real_malloc(sizeof(struct tcc_measurement));

	EXPECT_CALL(m_alloc_mock, malloc(_))
			.WillOnce(Return(measurement1))
			.WillOnce(Return(measurement2));

	EXPECT_CALL(m_alloc_mock, calloc(_,_))
			.WillRepeatedly(Invoke(__real_calloc));

	EXPECT_CALL(m_alloc_mock, calloc(1, sizeof(__itt_string_handle)))
			.WillOnce(Return(handle1))
			.WillOnce(Return(handle2));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name1)))
			.WillOnce(Return(fake_buffer_size));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name2)))
			.WillOnce(Return(fake_buffer_size));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_shared_memory_flag_from_env())
			.Times(2)
			.WillRepeatedly(Return(fake_shared_memory_flag));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name1),
			    _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t , uint64_t ,
					 deadline_callback_t ,
					 bool , const char* , va_list ){
		*measurement = fake_tcc_measurement1;
		return TCC_E_SUCCESS;
	}));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name2),
			    _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t , uint64_t ,
					 deadline_callback_t ,
					 bool , const char* , va_list ){
		*measurement = fake_tcc_measurement2;
		return TCC_E_SUCCESS;
	}));
	EXPECT_CALL(m_measurement_mock,tcc_measurement_destroy(measurement1))
			.Times(1);
	EXPECT_CALL(m_measurement_mock,tcc_measurement_destroy(measurement2))
			.Times(1);

	EXPECT_EQ(handle1, string_handle_create(fake_string_handle_name1));
	EXPECT_EQ(handle2, string_handle_create(fake_string_handle_name2));

	EXPECT_STREQ(fake_string_handle_name1, handle1->strA);
	EXPECT_EQ(nullptr, handle1->strW);
	EXPECT_TRUE(memcmp(handle1->extra2, &fake_tcc_measurement1, sizeof(fake_tcc_measurement1)) == 0);
	EXPECT_EQ(0, handle1->extra1);
	EXPECT_STREQ(fake_string_handle_name1, handle1->strA);
	EXPECT_EQ(nullptr, handle1->strW);
	EXPECT_TRUE(memcmp(handle1->extra2, &fake_tcc_measurement1, sizeof(fake_tcc_measurement1)) == 0);
	EXPECT_EQ(0, handle1->extra1);

	EXPECT_STREQ(fake_string_handle_name2, handle2->strA);
	EXPECT_EQ(nullptr, handle2->strW);
	EXPECT_TRUE(memcmp(handle2->extra2, &fake_tcc_measurement2, sizeof(fake_tcc_measurement2)) == 0);
	EXPECT_EQ(0, handle1->extra1);
	EXPECT_STREQ(fake_string_handle_name2, handle2->strA);
	EXPECT_EQ(nullptr, handle2->strW);
	EXPECT_TRUE(memcmp(handle2->extra2, &fake_tcc_measurement2, sizeof(fake_tcc_measurement2)) == 0);
	EXPECT_EQ(0, handle1->extra1);
	EXPECT_EQ(nullptr, handle2->next);
}

TEST_F(test_tcc_measurement_itt_host, string_handle_create_same_name_positive)
{
	size_t fake_buffer_size=42;
	int fake_shared_memory_flag = 0;
	const char* fake_string_handle_name1 = "fake_string_handle_name1";
	const char* fake_string_handle_name2 = "fake_string_handle_name2";
	__itt_string_handle* handle1 =
			(__itt_string_handle*)__real_calloc(1, sizeof(__itt_string_handle));
	__itt_string_handle* handle2 =
			(__itt_string_handle*)__real_calloc(1, sizeof(__itt_string_handle));
	struct tcc_measurement fake_tcc_measurement1 = get_fake_measurement();
	struct tcc_measurement fake_tcc_measurement2 = get_fake_measurement();
	struct tcc_measurement* measurement1 = (struct tcc_measurement*)__real_malloc(sizeof(struct tcc_measurement));
	struct tcc_measurement* measurement2 = (struct tcc_measurement*)__real_malloc(sizeof(struct tcc_measurement));

	EXPECT_CALL(m_alloc_mock, malloc(_))
			.WillOnce(Return(measurement1))
			.WillOnce(Return(measurement2));

	EXPECT_CALL(m_alloc_mock, calloc(_,_))
			.WillRepeatedly(Invoke(__real_calloc));

	EXPECT_CALL(m_alloc_mock, calloc(1, sizeof(__itt_string_handle)))
			.WillOnce(Return(handle1))
			.WillOnce(Return(handle2));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name1)))
			.WillOnce(Return(fake_buffer_size));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name2)))
			.WillOnce(Return(fake_buffer_size));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_shared_memory_flag_from_env())
			.Times(2)
			.WillRepeatedly(Return(fake_shared_memory_flag));

	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name1),
			    _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t , uint64_t ,
					 deadline_callback_t ,
					 bool , const char* , va_list ){
		*measurement = fake_tcc_measurement1;
		return TCC_E_SUCCESS;
	}));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name2),
			    _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t , uint64_t ,
					 deadline_callback_t ,
					 bool , const char* , va_list ){
		*measurement = fake_tcc_measurement2;
		return TCC_E_SUCCESS;
	}));
	EXPECT_CALL(m_measurement_mock,tcc_measurement_destroy(measurement1))
			.Times(1);
	EXPECT_CALL(m_measurement_mock,tcc_measurement_destroy(measurement2))
			.Times(1);

	EXPECT_EQ(handle1, string_handle_create(fake_string_handle_name1));
	EXPECT_EQ(handle2, string_handle_create(fake_string_handle_name2));
	EXPECT_EQ(handle1, string_handle_create(fake_string_handle_name1));
	EXPECT_EQ(handle2, string_handle_create(fake_string_handle_name2));
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_domain_nullptr_negative)
{
	struct tcc_measurement* measurement = (struct tcc_measurement*)0xDEADBEAF;
	__itt_string_handle* handle =(__itt_string_handle*)0xDEADBEAF;
	EXPECT_EQ( -TCC_E_BAD_PARAM, __tcc_measurement_get(nullptr, handle, &measurement));
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_handle_nullptr_negative)
{
	__itt_domain* domain =(__itt_domain*)0xDEADBEAF;
	struct tcc_measurement* measurement = (struct tcc_measurement*)0xDEADBEAF;
	EXPECT_EQ( -TCC_E_BAD_PARAM, __tcc_measurement_get(domain, nullptr, &measurement));
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_measurement_nullptr_negative)
{
	__itt_domain* domain = (__itt_domain*)0xDEADBEAF;
	__itt_string_handle* handle =(__itt_string_handle*)0xDEADBEAF;
	EXPECT_EQ( -TCC_E_BAD_PARAM, __tcc_measurement_get(domain, handle, nullptr));
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_measurement_measurement_not_nullptr_positive)
{
	__itt_domain* domain = (__itt_domain*)0xDEADBEAF;
	__itt_string_handle handle = {
		"fake_handle_name",
		nullptr,
		0,
		(void*)0xBEAFBEAF,
		nullptr,
	};
	struct tcc_measurement* measurement = nullptr;
	EXPECT_EQ(TCC_E_SUCCESS, __tcc_measurement_get(domain, &handle, &measurement));
	EXPECT_EQ((void*)measurement, handle.extra2);
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_measurement_unregistred_measurement_calloc_negative)
{
	__itt_domain* domain = (__itt_domain*)0xDEADBEAF;
	__itt_string_handle handle = {
		"fake_handle_name",
		nullptr,
		0,
		nullptr,
		nullptr,
	};
	struct tcc_measurement* measurement = nullptr;
	EXPECT_CALL(m_alloc_mock, calloc(_,_)).WillOnce(ReturnNull());
	EXPECT_EQ(-TCC_E_MEMORY, __tcc_measurement_get(domain, &handle, &measurement));
	EXPECT_EQ((void*)measurement, handle.extra2);
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_measurement_unregistred_measurement_malloc_negative)
{
	__itt_domain* domain = (__itt_domain*)0xDEADBEAF;
	__itt_string_handle handle = {
		"fake_handle_name",
		nullptr,
		0,
		nullptr,
		nullptr,
	};
	struct tcc_measurement* measurement = nullptr;
	EXPECT_CALL(m_alloc_mock, malloc(_)).WillOnce(ReturnNull());
	EXPECT_EQ(-TCC_E_MEMORY, __tcc_measurement_get(domain, &handle, &measurement));
	EXPECT_EQ((void*)measurement, handle.extra2);
}

TEST_F(test_tcc_measurement_itt_host, __tcc_measurement_get_measurement_unregistred_measurement_tcc_measurement_init_ex_e_memory_negative)
{
	__itt_domain* domain = (__itt_domain*)0xDEADBEAF;
	__itt_string_handle handle = {
		"fake_handle_name",
		nullptr,
		0,
		nullptr,
		nullptr,
	};
	struct tcc_measurement* measurement = nullptr;
	EXPECT_CALL(m_measurement_mock, tcc_measurement_shared_memory_flag_from_env()).Times(AnyNumber());
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_buffer_size_from_env(_)).Times(AnyNumber());
	EXPECT_CALL(m_measurement_mock, tcc_measurement_init_ex(_, _, _, _, _, _, _)).WillOnce(Return(-TCC_E_MEMORY));
	EXPECT_EQ(-TCC_E_MEMORY, __tcc_measurement_get(domain, &handle, &measurement));
	EXPECT_EQ((void*)measurement, handle.extra2);
}

TEST_F(test_tcc_measurement_itt_host,
	   __tcc_measurement_get_measurement_unregistred_measurement_tcc_unregistred_measurement_positive)
{
	__itt_domain* domain = (__itt_domain*)0xDEADBEAF;
	__itt_string_handle* handle = (__itt_string_handle*)__real_calloc(1, sizeof(__itt_string_handle));
	ASSERT_NE(nullptr, handle);
	handle->strA = __real_strdup("fake_handle_name");
	ASSERT_NE(nullptr, handle->strA);
	struct tcc_measurement fake_tcc_measurement = get_fake_measurement();
	struct tcc_measurement* measurement = nullptr;
	struct tcc_measurement* measurement_malloced = (struct tcc_measurement*)
									__real_malloc(sizeof(struct tcc_measurement));

	EXPECT_CALL(m_alloc_mock, malloc(_))
			.WillOnce(Return(measurement_malloced));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_shared_memory_flag_from_env()).Times(AnyNumber());
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_buffer_size_from_env(_)).Times(AnyNumber());
	EXPECT_CALL(m_measurement_mock, tcc_measurement_init_ex(_, _, _, _, _, _, _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t, uint64_t,
					 deadline_callback_t, bool, const char*, va_list){
									*measurement = fake_tcc_measurement;
									return TCC_E_SUCCESS;
								}));
	EXPECT_CALL(m_measurement_mock,tcc_measurement_destroy(measurement_malloced))
			.Times(1);

	EXPECT_EQ(TCC_E_SUCCESS, __tcc_measurement_get(domain, handle, &measurement));
	EXPECT_EQ((void*)measurement, handle->extra2);
}

TEST_F(test_tcc_measurement_itt_host, task_begin_succes)
{
	__itt_domain* fake_domain = (__itt_domain*)0xDEADBEAF;
	tcc_measurement measurement = get_fake_measurement();
	tcc_measurement expect_measurement = measurement;
	__itt_string_handle string_handle = {
		"fake_mesuare_name",
		NULL,
		0,
		(void*)&measurement,
		NULL,
	};
	task_begin(fake_domain, __itt_null, __itt_null, &string_handle);
	EXPECT_NE(measurement.clk_start, expect_measurement.clk_start);
	measurement.clk_start = expect_measurement.clk_start;
	EXPECT_EQ(0, memcmp(&measurement, &expect_measurement, sizeof(struct tcc_measurement)));
	EXPECT_NE((__itt_string_handle*)NULL, string_handle.next);
}

TEST_F(test_tcc_measurement_itt_host, task_begin_double_call_negative)
{
	__itt_domain* fake_domain = (__itt_domain*)0xDEADBEAF;
	tcc_measurement measurement = get_fake_measurement();
	__itt_string_handle string_handle = {
		"fake_mesuare_name",
		NULL,
		0,
		(void*)&measurement,
		NULL,
	};
	task_begin(fake_domain, __itt_null, __itt_null, &string_handle);
	__itt_string_handle expect_string_handle = string_handle;
	tcc_measurement expect_measurement = measurement;
	task_begin(fake_domain, __itt_null, __itt_null, &string_handle);
	ASSERT_EQ(0, memcmp(&expect_string_handle, &string_handle, sizeof(__itt_string_handle)));
	EXPECT_EQ(0, memcmp(&expect_measurement, string_handle.extra2, sizeof(struct tcc_measurement)));
}

TEST_F(test_tcc_measurement_itt_host, tcc_measurement_destroy_negative)
{

	size_t fake_buffer_size=42;
	int fake_shared_memory_flag = 1;
	const char* fake_string_handle_name = "fake_string_handle_name";
	struct tcc_measurement fake_tcc_measurement = get_fake_measurement();
	struct tcc_measurement* measurement = (struct tcc_measurement*)__real_malloc(sizeof(struct tcc_measurement));

	EXPECT_CALL(m_alloc_mock, malloc(_))
			.WillOnce(Return(measurement));
	EXPECT_CALL(m_alloc_mock, calloc(_,_))
			.WillRepeatedly(Invoke(__real_calloc));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_get_buffer_size_from_env(StrEq(fake_string_handle_name)))
			.WillOnce(Return(fake_buffer_size));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_shared_memory_flag_from_env())
			.WillOnce(Return(fake_shared_memory_flag));
	EXPECT_CALL(m_measurement_mock,
		    tcc_measurement_init_ex(
			    NotNull(),
			    fake_buffer_size,
			    0,
			    nullptr,
			    fake_shared_memory_flag,
			    StrEq(fake_string_handle_name),
			    _))
			.WillOnce(Invoke([&](struct tcc_measurement* measurement, size_t , uint64_t ,
					 deadline_callback_t ,
					 bool , const char* , va_list ){
		*measurement = fake_tcc_measurement;
		return TCC_E_SUCCESS;
	}));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_destroy(measurement))
			.WillOnce(Return(-TCC_E_MEMORY));

	EXPECT_NE(nullptr, string_handle_create(fake_string_handle_name));
}

// TCC_MEASUREMENTS_BUFFERS env variable not specified, no dumping
TEST_F(test_tcc_measurement_itt_deinit_host, tcc_measurement_deinit_filename_null_positive)
{
	const char* fake_handle_name_1 = "fake_handle_1";

	init_handle_expect(fake_handle_name_1);
	destroy_handle_expect();

	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_dump_file_from_env())
		.WillOnce(Return(nullptr));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_time_unit_from_env())
		.WillOnce(Return(TCC_TU_UNKNOWN));

	__itt_string_handle *handle1 = string_handle_create(fake_handle_name_1);
	ASSERT_NE(nullptr, handle1);
	tcc_itt_deinit();
}

// TCC_MEASUREMENTS_BUFFERS env variable specified, but dump file open fails
TEST_F(test_tcc_measurement_itt_deinit_host, tcc_measurement_deinit_fopen_negative)
{
	const char* fake_handle_name_1 = "fake_handle_1";
	const char* fake_dump_file = "dump.txt";

	init_handle_expect(fake_handle_name_1);
	destroy_handle_expect();

	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_dump_file_from_env())
		.WillOnce(Return(fake_dump_file));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_time_unit_from_env())
		.WillOnce(Return(TCC_TU_UNKNOWN));

	EXPECT_CALL(m_libc_mock, fopen(StrEq(fake_dump_file), StrEq("w")))
		.WillOnce(Return(nullptr));
	// Handle fprintf call with error message
	// TODO: Maybe will be better add fprintf to stderr with any number of
	//       calls allowed to the StartUp()?

	__itt_string_handle *handle1 = string_handle_create(fake_handle_name_1);
	ASSERT_NE(nullptr, handle1);
	tcc_itt_deinit();
}

// TCC_MEASUREMENTS_TIME_UNIT not specified, default value used, dump success
TEST_F(test_tcc_measurement_itt_deinit_host, tcc_measurement_deinit_timeunit_default_positive)
{
	const char* fake_handle_name_1 = "fake_handle_1";
	const char* fake_dump_file = "dump.txt";
	// We use stderr as valid FILE* ptr, because it will be passed to the
	// fflush() in tcc_itt_deinit()
	// Using fflush() mock with GTest will cause freeze if any test fails.
	// Maybe deadlock somewhare in GTest
	FILE* fake_file = stderr;

	init_handle_expect(fake_handle_name_1);
	destroy_handle_expect();

	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_dump_file_from_env())
		.WillOnce(Return(fake_dump_file));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_time_unit_from_env())
		.WillOnce(Return(TCC_TU_UNKNOWN));

	EXPECT_CALL(m_libc_mock, fopen(StrEq(fake_dump_file), StrEq("w")))
		.WillOnce(Return(fake_file));

	__itt_string_handle *handle1 = string_handle_create(fake_handle_name_1);
	ASSERT_NE(nullptr, handle1);
	tcc_measurement* measurement = (tcc_measurement*)handle1->extra2;

	// When environment not set, clk is used by default
	EXPECT_CALL(m_measurement_mock, tcc_measurement_print_history(measurement, fake_file, TCC_TU_CLK))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_libc_mock, fclose(fake_file))
		.WillOnce(Return(0));

	tcc_itt_deinit();
}

// TCC_MEASUREMENTS_TIME_UNIT specified, dump success, dumped values converted
TEST_F(test_tcc_measurement_itt_deinit_host, tcc_measurement_deinit_timeunit_set_positive)
{
	const char* fake_handle_name_1 = "fake_handle_1";
	const char* fake_dump_file = "dump.txt";
	FILE* fake_file = stderr;
	TCC_TIME_UNIT unit = TCC_TU_NS;

	init_handle_expect(fake_handle_name_1);
	destroy_handle_expect();

	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_dump_file_from_env())
		.WillOnce(Return(fake_dump_file));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_time_unit_from_env())
		.WillOnce(Return(unit));

	EXPECT_CALL(m_libc_mock, fopen(StrEq(fake_dump_file), StrEq("w")))
		.WillOnce(Return(fake_file));

	__itt_string_handle *handle1 = string_handle_create(fake_handle_name_1);
	ASSERT_NE(nullptr, handle1);
	tcc_measurement* measurement = (tcc_measurement*)handle1->extra2;

	EXPECT_CALL(m_measurement_mock, tcc_measurement_print_history(measurement, fake_file, unit))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_libc_mock, fclose(fake_file))
		.WillOnce(Return(0));

	tcc_itt_deinit();
}

// Multiple string handles created, dump file contains multiple measurements
TEST_F(test_tcc_measurement_itt_deinit_host, tcc_measurement_deinit_multiple_measurements_dump_positive)
{
	const char* fake_handle_name_1 = "fake_handle_1";
	const char* fake_handle_name_2 = "fake_handle_2";
	const char* fake_dump_file = "dump.txt";
	FILE* fake_file = stderr;
	TCC_TIME_UNIT unit = TCC_TU_NS;
	size_t fake_buffer_size=42;
	int fake_shared_memory_flag = 0;

	{
		InSequence seq;
		init_handle_expect(fake_handle_name_1);
		init_handle_expect(fake_handle_name_2);
	}

	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_dump_file_from_env())
		.WillOnce(Return(fake_dump_file));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_get_time_unit_from_env())
		.WillOnce(Return(unit));

	EXPECT_CALL(m_libc_mock, fopen(StrEq(fake_dump_file), StrEq("w")))
		.WillOnce(Return(fake_file));

	__itt_string_handle *handle_1 = string_handle_create(fake_handle_name_1);
	ASSERT_NE(nullptr, handle_1);
	__itt_string_handle *handle_2 = string_handle_create(fake_handle_name_2);
	ASSERT_NE(nullptr, handle_2);
	tcc_measurement* measurement_1 = (tcc_measurement*)handle_1->extra2;
	tcc_measurement* measurement_2 = (tcc_measurement*)handle_2->extra2;

	EXPECT_CALL(m_measurement_mock, tcc_measurement_print_history(measurement_1, fake_file, unit))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_measurement_mock, tcc_measurement_print_history(measurement_2, fake_file, unit))
		.WillOnce(Return(TCC_E_SUCCESS));
	EXPECT_CALL(m_libc_mock, fclose(fake_file))
		.WillOnce(Return(0));

	{
		InSequence seq;
		destroy_handle_expect();
		destroy_handle_expect();
	}

	tcc_itt_deinit();
}

}
