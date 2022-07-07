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
#include "rtct_format.h"
#include "test_log.hpp"
#include <tuple>
#include <errno.h>

namespace
{
using namespace testing;

template<int size, int type>
struct RtctEntry
{
    rtct_entry_header_t header;
    int data[size];
    RtctEntry():
        header{size * sizeof(int) + sizeof(rtct_entry_header_t), 1, type},
        data{0,}
    {
        TEST_LOG_TRACE("%p size: %zu, type %i", this, sizeof(*this), this->header.type);
    }
};

struct{
    uint8_t acpi_header[RTCT_TABLE_ENTRIES_OFFSET];
    RtctEntry<7,1> entry_0;
    RtctEntry<8,2> entry_1;
    RtctEntry<7,3> entry_2;
    RtctEntry<5,6> entry_3;
    RtctEntry<1,2> entry_4;
    RtctEntry<3,4> entry_5;
} m_rtct_table;

class test_rtct_get_entries : public Test
{
protected:
    void SetUp() override
    {
        EXPECT_EQ((size_t)&(m_rtct_table.entry_0), (size_t)&m_rtct_table+RTCT_TABLE_ENTRIES_OFFSET);
        errno = 0;
    }
    void TearDown() override
    {
        errno = 0;
    }
    // Tuple allocate the elements in another sequence
    //In array we get types 1 2 8 6 2 4
};

TEST_F(test_rtct_get_entries, rtct_table_nullptr_negative)
{
    rtct_table_entries_t entries = {0,};
    EXPECT_EQ(-1, rtct_get_entries(nullptr, sizeof(m_rtct_table), &entries));
    EXPECT_EQ(errno, EINVAL);
}

TEST_F(test_rtct_get_entries, size_0_negative)
{
    rtct_table_entries_t entries = {0,};
    EXPECT_EQ(-1, rtct_get_entries(&m_rtct_table, 0, &entries));
    EXPECT_EQ(errno, EINVAL);
}

TEST_F(test_rtct_get_entries, entries_nullptr_negative)
{
    EXPECT_EQ(-1, rtct_get_entries(&m_rtct_table, sizeof(m_rtct_table), nullptr));
    EXPECT_EQ(errno, EINVAL);
}

TEST_F(test_rtct_get_entries, wrong_rtct_table)
{
    rtct_table_entries_t entries = {0,};
    int8_t wrong_rtct_table[RTCT_TABLE_ENTRIES_OFFSET-1];
    EXPECT_EQ(-1, rtct_get_entries(&wrong_rtct_table, sizeof(wrong_rtct_table), &entries));
}

TEST_F(test_rtct_get_entries, rtct_table_empty_positive)
{
    rtct_table_entries_t entries = {0,};
    int8_t empty_rtct_table[RTCT_TABLE_ENTRIES_OFFSET];
    EXPECT_EQ(0, rtct_get_entries(&empty_rtct_table, sizeof(empty_rtct_table), &entries));
    EXPECT_EQ(entries.begin, entries.end);
}

TEST_F(test_rtct_get_entries, rtct_table_empty_read_positive)
{
    rtct_table_entries_t entries = {0,};
    int8_t empty_rtct_table[RTCT_TABLE_ENTRIES_OFFSET];
    EXPECT_EQ(0, rtct_get_entries(&empty_rtct_table, sizeof(empty_rtct_table), &entries));
    EXPECT_EQ(entries.end, rtct_next_entry(&entries, nullptr));
    EXPECT_EQ(entries.end, rtct_next_entry(&entries, entries.end));
}

TEST_F(test_rtct_get_entries, positive)
{
    rtct_table_entries_t entries = {0,};
    EXPECT_EQ(0, rtct_get_entries(&m_rtct_table, sizeof(m_rtct_table), &entries));
    EXPECT_EQ(entries.begin, reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_0)));
    EXPECT_EQ(entries.end, reinterpret_cast<rtct_entry_header_t*>(reinterpret_cast<size_t>(&m_rtct_table)+ sizeof(m_rtct_table)));
    EXPECT_EQ(errno, 0);
}

class test_rtct_get_next_entry : public test_rtct_get_entries
{
protected:
    rtct_table_entries_t m_entries = {0,};
    void SetUp() override
    {
        test_rtct_get_entries::SetUp();
        EXPECT_EQ(0, rtct_get_entries(&m_rtct_table, sizeof(m_rtct_table), &m_entries));

    }
};


TEST_F(test_rtct_get_next_entry, rtct_test_rtct_null_negative)
{
    EXPECT_EQ(nullptr, rtct_next_entry(nullptr, m_entries.begin));
    EXPECT_EQ(errno, EINVAL);
}

TEST_F(test_rtct_get_next_entry, rtct_test_start_null_positive)
{
    EXPECT_EQ(m_entries.begin, rtct_next_entry(&m_entries, nullptr));
    EXPECT_EQ(errno, 0);
}

TEST_F(test_rtct_get_next_entry, rtct_test_start_begin_positive)
{
    EXPECT_EQ(reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_0)), rtct_next_entry(&m_entries, nullptr));
    EXPECT_EQ(reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_1)), rtct_next_entry(&m_entries, reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_0))));
    EXPECT_EQ(reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_2)), rtct_next_entry(&m_entries, reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_1))));
    EXPECT_EQ(reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_3)), rtct_next_entry(&m_entries, reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_2))));
    EXPECT_EQ(reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_4)), rtct_next_entry(&m_entries, reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_3))));
    EXPECT_EQ(reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_5)), rtct_next_entry(&m_entries, reinterpret_cast<rtct_entry_header_t*>(&(m_rtct_table.entry_4))));
    EXPECT_EQ(m_entries.end, rtct_next_entry(&m_entries, reinterpret_cast<rtct_entry_header_t*>(&m_rtct_table.entry_5)));
    EXPECT_EQ(m_entries.end, rtct_next_entry(&m_entries, m_entries.end));
    EXPECT_EQ(errno, 0);
}

//*/
}
