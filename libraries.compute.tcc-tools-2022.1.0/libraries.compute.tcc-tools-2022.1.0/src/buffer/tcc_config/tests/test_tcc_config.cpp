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
#include <string>
#include <fcntl.h>
#include <unistd.h>

#include "alloc_mock.hpp"
#include "libc_mock.hpp"

#include "tcc_config.h"

#include "gtest_file_helpers.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

// Declare structure because it's private
struct tcc_config {
	size_t count;
	tcc_config_item_t* items;
};


namespace
{
using namespace testing;
using ::testing::Mock;

////////////////////////////////////////////////////////////////////////////////
/// Basic tests
////////////////////////////////////////////////////////////////////////////////

class test_tcc_config : public Test
{
protected:
	NiceMock<alloc_mock> m_alloc_mock;
	NiceMock<libc_mock> m_libc_mock;
};

TEST_F(test_tcc_config, read_config_null_ptr_negative)
{
	EXPECT_EQ(nullptr, tcc_config_read(nullptr));
}

TEST_F(test_tcc_config, read_config_cant_open_file_negative)
{
	auto filename = "config";
	EXPECT_CALL(m_libc_mock, open(StrEq(filename), O_RDONLY, _))
		.WillOnce(Return(-1));

	EXPECT_EQ(nullptr, tcc_config_read(filename));
}

TEST_F(test_tcc_config, get_count_null_ptr_negative)
{
	EXPECT_EQ(0, tcc_config_count(nullptr));
}

TEST_F(test_tcc_config, get_count_positive)
{
	size_t fake_size = 123;
	tcc_config_t config = { fake_size, nullptr };
	EXPECT_EQ(fake_size, tcc_config_count(&config));
}

TEST_F(test_tcc_config, at_null_ptr_negative)
{
	EXPECT_EQ(nullptr, tcc_config_get(nullptr, 0));
}

TEST_F(test_tcc_config, at_out_of_range_negative)
{
	tcc_config_item_t items[] = {
		{TCC_MEM_L1, 10},
		{TCC_MEM_L2, 20},
	};
	tcc_config_t config = { 2, items};

	EXPECT_EQ(nullptr, tcc_config_get(&config, 2));
}

TEST_F(test_tcc_config, at_positive)
{
	tcc_config_item_t items[] = {
		{TCC_MEM_L1, 10},
		{TCC_MEM_L2, 20},
	};
	tcc_config_t config = { 2, items};

	EXPECT_EQ(&items[0], tcc_config_get(&config, 0));
	EXPECT_EQ(&items[1], tcc_config_get(&config, 1));
}

TEST_F(test_tcc_config, destroy_nullptr_positive)
{
	// No crash
	tcc_config_destroy(nullptr);
}

TEST_F(test_tcc_config, destroy_positive)
{
	tcc_config_item_t items[] = {
		{TCC_MEM_L1, 10},
		{TCC_MEM_L2, 20},
	};
	tcc_config_t config = { 2, items};

	EXPECT_CALL(m_alloc_mock, free(_)).WillRepeatedly(Return());

	tcc_config_destroy(&config);
}

////////////////////////////////////////////////////////////////////////////////
/// Config parsing tests
////////////////////////////////////////////////////////////////////////////////


class test_tcc_config_parsing : public Test
{
protected:
	NiceMock<libc_mock> m_libc_mock;

	const std::string filename = "config";
	void expect_file(std::string file_content)
	{
		EXPECT_CALL(m_libc_mock, open(StrEq(filename.c_str()), O_RDONLY, _))
			.WillOnce(Return(fake_file::create_fd(file_content)));
	}

	void expect_memory_config(const std::vector<tcc_config_item_t> &items, const tcc_config_t* config)
	{

		ASSERT_NE(nullptr, config);
		ASSERT_NE(nullptr, config->items);
		ASSERT_EQ(items.size(), config->count);
		for(size_t i = 0; i < items.size(); i++) {
			EXPECT_EQ(items[i].level, config->items[i].level);
			EXPECT_EQ(items[i].size, config->items[i].size);
		}
	}
};

TEST_F(test_tcc_config_parsing, config_empty_negative)
{
	expect_file("");;
	EXPECT_EQ(nullptr, tcc_config_read(filename.c_str()));
}

TEST_F(test_tcc_config_parsing, config_another_json_file_negative)
{
	expect_file(R"({
  "section": {
	"name": value,
	"name2": value
  }
})");
	EXPECT_EQ(nullptr, tcc_config_read(filename.c_str()));
}

TEST_F(test_tcc_config_parsing, config_not_json_file_negative)
{
	expect_file("not json format");
	EXPECT_EQ(nullptr, tcc_config_read(filename.c_str()));
}

class test_tcc_config_parsing_positive : public test_tcc_config_parsing
{
protected:
	std::vector<tcc_config_item_t> m_expected;
	void TearDown() override
	{
		tcc_config_t* config = tcc_config_read(filename.c_str());
		expect_memory_config(m_expected, config);
		tcc_config_destroy(config);
	}
};
TEST_F(test_tcc_config_parsing_positive, single_section_L2_positive)
{
	expect_file(R"({
  "L2": {
	"buffer_size_bytes": 130000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_L2, .size = 130000, }
	};
}

TEST_F(test_tcc_config_parsing_positive, multiple_sections_L2_L2_positive)
{
	expect_file(R"({
  "L2": {
	"buffer_size_bytes": 130000  },
  "L2": {
	"buffer_size_bytes": 200000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_L2, .size = 200000, }
	};
}

TEST_F(test_tcc_config_parsing_positive, multiple_sections_L3_L3_positive)
{
	expect_file(R"({
  "L3": {
	"buffer_size_bytes": 130000
  },
  "L3": {
	"buffer_size_bytes": 200000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_L3, .size = 200000, }
	};
}

TEST_F(test_tcc_config_parsing_positive, multiple_sections_DRAM_DRAM_positive)
{
	expect_file(R"({
  "DRAM": {
	"buffer_size_bytes": 200000
	  },
  "DRAM": {
	"buffer_size_bytes": 10000000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_DRAM, .size = 10000000 }
	};
}

TEST_F(test_tcc_config_parsing_positive, multiple_sections_L2_L3_positive)
{
	expect_file(R"({
  "L2": {
	"buffer_size_bytes": 130000
  },
  "L3": {
	"buffer_size_bytes": 200000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_L2, .size = 130000 },
		{ .level = TCC_MEM_L3, .size = 200000 }
	};
}

TEST_F(test_tcc_config_parsing_positive, multiple_sections_L2_DRAM_positive)
{
	expect_file(R"({
  "L2": {
	"buffer_size_bytes": 130000
  },
  "DRAM": {
	"buffer_size_bytes": 10000000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_L2, .size = 130000 },
		{ .level = TCC_MEM_DRAM, .size = 10000000 }
	};
}

TEST_F(test_tcc_config_parsing_positive, multiple_sections_L3_DRAM_positive)
{
	expect_file(R"({
  "L3": {
	"buffer_size_bytes": 200000
  },
  "DRAM": {
	"buffer_size_bytes": 10000000
  }
})");
	m_expected = {
		{ .level = TCC_MEM_L3, .size = 200000 },
		{ .level = TCC_MEM_DRAM, .size = 10000000 }

	};
}


TEST_F(test_tcc_config_parsing, bad_line_invalid_buffer_size_negative)
{
	expect_file(R"({
  "L2": {
	"buffer_size_bytes": bad
  }
})");
	EXPECT_EQ(nullptr, tcc_config_read(filename.c_str()));
}

TEST_F(test_tcc_config_parsing, bad_line_invalid_cache_level_negative)
{
	expect_file(R"({
  "bad": {
	"buffer_size_bytes": 130000
  }
})");
	EXPECT_EQ(nullptr, tcc_config_read(filename.c_str()));
}

TEST_F(test_tcc_config_parsing, bad_line_no_buffer_size_negative)
{
	expect_file(R"({
  "L2": {
	"some_value": 130000
  }
})");
	EXPECT_EQ(nullptr, tcc_config_read(filename.c_str()));
}


}
