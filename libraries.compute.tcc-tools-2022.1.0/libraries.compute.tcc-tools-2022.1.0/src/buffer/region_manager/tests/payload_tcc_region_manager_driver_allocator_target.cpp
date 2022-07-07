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

#include "gtest/gtest.h"
#include "tcc_log.h"
#include <sys/mman.h>
#include "pointer_chasing.h"
#include <thread>
#include <random>
#include <stdlib.h>
#include "region_manager.hpp"
#include "tcc_log.h"
#include <sstream>

#define PAGE_SIZE 4096
#define LINE_SIZE 64
#define ITERATION_NUM_FOR_RANDOM_TESTS 100
#define DEFAULT_RANDOM_SEED 42
#define TEST_RANDOM_SEED_VAR "TEST_RANDOM_SEED_VAR"
#define FILE_AND_LINE std::string(__FILE__) + ":" + std::to_string(__LINE__) + ":"

#define CHECK_BUFFER(buf, size, message) ({\
        size_t expect_number_of_nodes = size / LINE_SIZE; \
        std::set<cache_line_node_t*> all_nodes {}; \
        EXPECT_EQ(0, memcmp(buf, m_zero_buffer, size)) << message; \
        cache_line_node_t* start_node = pointer_chase_create_linear(buf, size); \
        cache_line_node_t* node = start_node; \
        for (size_t i = 0; i < expect_number_of_nodes; i++) { \
            EXPECT_EQ(all_nodes.find(node), all_nodes.end()) << "id:" << i << " " << message; \
            all_nodes.insert(node); \
            node = pointer_chase_run_read_workload(node, 1); \
        } \
        EXPECT_EQ(node, start_node); \
    })

namespace
{
using namespace ::testing;

class RegionManagerLevelsFixture : public TestWithParam<size_t>
{
protected:
    RegionManager& m_region_manager = RegionManager::instance();
    size_t m_index = 0;
    size_t m_region_size = 0;
    void* m_zero_buffer = nullptr;

    void allocate_buffer_first_time()
    {
        std::set<size_t> idxs;
        if (idxs.find(m_index) ==  idxs.end()) {
            idxs.insert(m_index);
            void* buf = m_region_manager.mmap(m_index, m_region_size);
            std::stringstream message;
            message << "index:" << m_index <<  ",size:" << m_region_size;

            ASSERT_NE(buf, nullptr) << message.str();
            for(size_t i=0; i < m_region_size; i++)
            {
                (static_cast<uint8_t*>(buf))[i] = static_cast<uint8_t>(random());
            }
            EXPECT_EQ(0, munmap(buf, m_region_size)) << message.str();
        }
    }

    void SetUp() override
    {
        m_index = GetParam();
        ASSERT_EQ(0, sched_setaffinity(getpid(), CPU_SETSIZE, &(m_region_manager.prop(m_index).mask)));
        m_zero_buffer = calloc(1, m_region_manager.prop(m_index).size);
        ASSERT_NE(nullptr, m_zero_buffer);
        m_region_size = m_region_manager.prop(m_index).size;
        allocate_buffer_first_time();
    }
    void TearDown() override
    {
        free(m_zero_buffer);
    }

};

std::vector<size_t> test_data = std::function([]() {
    std::vector<size_t> data;
    auto& region_manager = RegionManager::instance();
    for (size_t i = 0; i < region_manager.size(); i++) {
        if (region_manager.prop(i).type != TCC_MEM_DRAM) {
            data.push_back(i);
        }
    }
    return data;
})();

INSTANTIATE_TEST_CASE_P(RegionManagerAllocationTests,
    RegionManagerLevelsFixture,
    ::testing::ValuesIn(test_data.begin(), test_data.end()),
    [](auto& param_val) {
        return RegionManager::instance().get_test_name(param_val.param);
    });


TEST_P(RegionManagerLevelsFixture, to_big_size_negative)
{
    EXPECT_EQ(nullptr, m_region_manager.mmap(m_index, m_region_size + PAGE_SIZE));
}

TEST_P(RegionManagerLevelsFixture, allocate_all_buffer_positive)
{
    void* buf = m_region_manager.mmap(m_index, m_region_size);
    ASSERT_NE(buf, nullptr);
    CHECK_BUFFER(buf, m_region_size, "");
    EXPECT_EQ(0, munmap(buf, m_region_size));
}
TEST_P(RegionManagerLevelsFixture, allocate_buffer_by_increase_size)
{
    for (size_t j = 1; j * PAGE_SIZE <= m_region_size; j++) {
        size_t size = j * PAGE_SIZE;
        void* buf = nullptr;
        auto message = "size:" + std::to_string(size);
        EXPECT_NE(buf = m_region_manager.mmap(m_index, size), nullptr) << message;
        CHECK_BUFFER(buf, size, message);
        EXPECT_EQ(0, munmap(buf, size)) << message;
    }
}

TEST_P(RegionManagerLevelsFixture, allocate_buffer_by_decrease_size)
{
    for (size_t j = 0; j * PAGE_SIZE < m_region_size; j++) {
        size_t size = m_region_size - j * PAGE_SIZE;
        void* buf = nullptr;
        auto message = "size:" + std::to_string(size);
        EXPECT_NE(buf = m_region_manager.mmap(m_index, size), nullptr) << message;
        CHECK_BUFFER(buf, size, message);
        EXPECT_EQ(0, munmap(buf, size)) << message;
    }
}

TEST_P(RegionManagerLevelsFixture, allocate_and_free_buffer_random_test)
{
    ASSERT_EQ(0, setenv(TEST_RANDOM_SEED_VAR, std::to_string(DEFAULT_RANDOM_SEED).c_str(), 0));
    const char* random_seed_str = getenv(TEST_RANDOM_SEED_VAR);
    ASSERT_NE(nullptr, random_seed_str);
    unsigned long int random_seed = std::strtoul(random_seed_str, nullptr, 10);
    std::cout << TEST_RANDOM_SEED_VAR << ":" << random_seed << std::endl;
    ASSERT_NE(ERANGE, errno);
    std::mt19937 random_seq(DEFAULT_RANDOM_SEED);
    size_t page_number = m_region_size / PAGE_SIZE;

    for (auto j = 0; j < ITERATION_NUM_FOR_RANDOM_TESTS; j++) {
        size_t size = (random_seq() % (page_number - 1) + 1) * PAGE_SIZE;
        void* buf = nullptr;
        auto message = "size:" + std::to_string(size);
        EXPECT_NE(buf = m_region_manager.mmap(m_index, size), nullptr) << message;
        CHECK_BUFFER(buf, size, message);
        EXPECT_EQ(0, munmap(buf, size)) << message;
    }
}
//*/
}  // namespace
