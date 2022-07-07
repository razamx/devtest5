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
#include <linux/limits.h>

#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include <errno.h>

#include "tcc_config_finder.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

#define ENV_CONFIG_PATH "TCC_CONFIG_PATH"
#define ENV_HOME        "HOME"
#define CONFIG_SDK_PATH "/usr/share/tcc_tools/config"
#define CONFIG_FILE     ".tcc.config"
#define FAKE_ERRNO_CODE EFBIG
namespace
{
using namespace testing;
using ::testing::Mock;

class test_tcc_config_finder : public Test
{
protected:
	NiceMock<alloc_mock> m_alloc_mock;
	StrictMock<libc_mock> m_libc_mock;
	void SetUp() override
	{
		errno = 0;
	}
	void TearDown() override{
		errno = 0;
	}
};

/*
TEST_F(test_tcc_config_finder, path_null_negative)
{
	EXPECT_EQ(nullptr, get_available_config_file());
}
*/
TEST_F(test_tcc_config_finder, user_positive)
{
	std::string env_path = "/my/path";
	std::string full_path = env_path + "/" + CONFIG_FILE;
	
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(Return((char*)env_path.c_str()));
	EXPECT_CALL(m_libc_mock, access(StrEq(full_path.c_str()), R_OK))
		.WillOnce(Return(0));

	EXPECT_STREQ(full_path.c_str(), get_available_config_file());
}

TEST_F(test_tcc_config_finder, user_env_null_home_positive)
{
	std::string home_path = "/home/user";
	std::string full_path = home_path + "/" + CONFIG_FILE;

	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_HOME)))
		.WillOnce(Return((char*)home_path.c_str()));
	EXPECT_CALL(m_libc_mock, access(StrEq(full_path.c_str()), R_OK))
		.WillOnce(Return(0));

	EXPECT_STREQ(full_path.c_str(), get_available_config_file());
}

TEST_F(test_tcc_config_finder, user_no_access_home_positive)
{
	std::string env_path = "/my/path";
	std::string full_env_path = env_path + "/" + CONFIG_FILE;
	std::string home_path = "/home/user";
	std::string full_home_path = home_path + "/" + CONFIG_FILE;

	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(Return((char*)env_path.c_str()));
	EXPECT_CALL(m_libc_mock, access(StrEq(full_env_path.c_str()), R_OK))
		.WillOnce(Return(-1));
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_HOME)))
		.WillOnce(Return((char*)home_path.c_str()));
	EXPECT_CALL(m_libc_mock, access(StrEq(full_home_path.c_str()), R_OK))
		.WillOnce(Return(0));

	EXPECT_STREQ(full_home_path.c_str(), get_available_config_file());
}

TEST_F(test_tcc_config_finder, home_env_null_sdk_positive)
{
	auto full_path =  CONFIG_SDK_PATH "/" CONFIG_FILE;

	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_HOME)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, access(StrEq(full_path), R_OK))
		.WillOnce(Return(0));

	EXPECT_STREQ(full_path, get_available_config_file());
}

TEST_F(test_tcc_config_finder, home_no_access_sdk_positive)
{
	std::string home_path = "/home/user";
	std::string full_home_path = home_path + "/" + CONFIG_FILE;
	auto full_path =  CONFIG_SDK_PATH "/" CONFIG_FILE;

	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_HOME)))
		.WillOnce(Return((char*)home_path.c_str()));
	EXPECT_CALL(m_libc_mock, access(StrEq(full_home_path.c_str()), R_OK))
		.WillOnce(Return(-1));
	EXPECT_CALL(m_libc_mock, access(StrEq(full_path), R_OK))
		.WillOnce(Return(0));

	EXPECT_STREQ(full_path, get_available_config_file());
}

TEST_F(test_tcc_config_finder, sdk_no_access_negative)
{
	auto full_path =  CONFIG_SDK_PATH "/" CONFIG_FILE;
	char dest_path[PATH_MAX];

	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_HOME)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, access(StrEq(full_path), R_OK))
		.WillOnce(Return(-1));

	EXPECT_EQ(nullptr, get_available_config_file());
}

TEST_F(test_tcc_config_finder, sdk_asprintf_error_negative)
{
	auto full_path =  CONFIG_SDK_PATH "/" CONFIG_FILE;
	char dest_path[PATH_MAX];

	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_CONFIG_PATH)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_libc_mock, getenv(StrEq(ENV_HOME)))
		.WillOnce(ReturnNull());
	EXPECT_CALL(m_alloc_mock, asprintf(_, _, _)).WillOnce(MOCK_RETURN_VAL_SET_ERRNO(-1, FAKE_ERRNO_CODE));

	EXPECT_EQ(nullptr, get_available_config_file());
}

}
