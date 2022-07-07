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

#ifndef TCC_BUFFER_TEST_HELPERS_HPP 
#define TCC_BUFFER_TEST_HELPERS_HPP

#include <gtest/gtest.h>
#include "libc_mock.hpp"
#include "alloc_mock.hpp"
#include "gtest_file_helpers.hpp"
#include "gtest_helpers.hpp"
#include "memory_properties_test_utils.hpp"
#include <stdarg.h>
#include "tcc/cache.h"
//#include <linux/tcc_buffer.h>
#include "tcc/err_code.h"
#include "tcc_log.h"
#include <errno.h>
#include <functional>

#define RESCTRL_NAME "/sys/fs/resctrl"

namespace tcc_buffer_test_utils{

    using namespace ::testing;

	/// This fixture help control than all chan of SetUp and TearDown method called.
	/// If SetUp or TearDonw for this clss not called that failed tests.
	template<typename T>
	class CheckSetUpTearDownSeqFixture : public T
	{
	public:
		///Check that SetUp and TearDonw chains called or fail the test. 
		virtual ~CheckSetUpTearDownSeqFixture()
		{
			if(m_is_need_check && m_is_need_check()){
				EXPECT_TRUE(m_is_set_up_seq_ok) << "SetUp sequence not called";
				EXPECT_TRUE(m_is_tear_down_seq_ok) << "TearDonw sequence not called";
			}
		}
	protected:
		void SetUp() override
		{
			T::SetUp();
			m_is_set_up_seq_ok = true;
		}
		void TearDown() override
		{
			m_is_tear_down_seq_ok=true;
			T::TearDown();
		}
		/// Need to custumise check for SetUp and TearDonw chains check.
		/// As example for Subprocess tests for main process we don't need it.
		std::function<bool()> m_is_need_check = [](){return true;}; //By default we always check that cahin called.
		private:
			bool m_is_tear_down_seq_ok = false;
			bool m_is_set_up_seq_ok = false;
	};

    template<typename T>
	class LibcMockFixture : public T
	{
	protected:
		StrictMock<libc_mock> m_libc_mock;
	};

	template<typename T>
	class AllocMockFixture : public T
	{
	protected:
		NiceMock<alloc_mock> m_alloc_mock;

	};

	template<typename T>
	class ConfigFixture : public T
	{
	protected:
		const std::string HOME_PATH_ENV_NAME = "HOME";
		const std::string TCC_CONFIG_CUSTOM_PATH_ENV_NAME = "TCC_CONFIG_PATH";
		const std::string TCC_CONFIG_FILENAME = ".tcc.config";
		const std::string TCC_CONFIG_CONTENT = "\
{\
  \"L2\": {\
    \"latency_ns\": 228,\
    \"buffer_size_bytes\": 262144\
  },\
  \"L3\": {\
    \"latency_ns\": 300,\
    \"buffer_size_bytes\": 524288\
   },\
   \"DRAM\": {\
     \"latency_ns\": 438,\
     \"buffer_size_bytes\": 10000000\
   }\
}";
		void SetUp() override
		{
			T::SetUp();
			EXPECT_CALL(T::m_libc_mock, getenv(StrEq(HOME_PATH_ENV_NAME)))
				.WillRepeatedly(ReturnNull());
			EXPECT_CALL(T::m_libc_mock, getenv(StrEq(TCC_CONFIG_CUSTOM_PATH_ENV_NAME)))
				.WillRepeatedly(ReturnNull());

			EXPECT_CALL(T::m_libc_mock, access(EndsWith(".tcc.config"), _))
				.WillRepeatedly(Return(0));
			EXPECT_CALL(T::m_libc_mock, open(EndsWith(".tcc.config"), O_RDONLY, _))
				.WillRepeatedly(Return(fake_file::create_fd(TCC_CONFIG_CONTENT)));
			EXPECT_CALL(T::m_libc_mock,  close(_))
				.WillRepeatedly(Invoke(__real_close));
		}
	};

	template<typename T>
	class ProcessorFixture : public T
	{
	protected:
		const unsigned NUM_PROCS = 4;
		const unsigned DEFAULT_CPUID = 3;
		const cpu_set_t DEFAULT_AFINITY = cpu_set_from_list({3});

		void SetUp() override
		{
			T::SetUp();

			EXPECT_CALL(T::m_libc_mock, sched_setaffinity(_, Eq(sizeof(cpu_set_t)), NotNull()))
				.Times(AnyNumber())
				.WillRepeatedly(Return(0));
			EXPECT_CALL(T::m_libc_mock, sched_getaffinity(_, Eq(sizeof(cpu_set_t)), NotNull()))
				.Times(AnyNumber())
				.WillRepeatedly(Invoke([&](__pid_t, size_t,
										   cpu_set_t *cpuset) {
					*cpuset = DEFAULT_AFINITY;
					return 0;
				}));
			EXPECT_CALL(T::m_libc_mock, get_nprocs())
				.Times(AnyNumber())
				.WillRepeatedly(Return(NUM_PROCS));
		}
	};
}

#endif // TCC_BUFFER_TEST_HELPERS_HPP
