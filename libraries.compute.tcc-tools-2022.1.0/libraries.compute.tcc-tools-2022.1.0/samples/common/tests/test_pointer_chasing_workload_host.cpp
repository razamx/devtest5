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
#include <cstdint>
#include "pointer_chasing.h"


namespace{

using namespace ::testing;

const unsigned LINE_SIZE = 64;
const int FAKE_RANDOM_SEED=42;
const size_t MINIMAL_BUFFER_SIZE = (LINE_SIZE*6);
const unsigned long long INAPPLICABLE_NUMBER = 0xBAD0DEADBAD0DEAD;

class pointer_chasing_bad_param_tests: public Test
{
public:
    void* INAPPLICABLE_BUFFER_PTR = reinterpret_cast<void*>(INAPPLICABLE_NUMBER);
    size_t INAPPLICABLE_SIZE = INAPPLICABLE_NUMBER;
    pointer_chase_random_generator_t INAPPLICABLE_RANDOM_GENERATOR =
        reinterpret_cast<pointer_chase_random_generator_t>(INAPPLICABLE_NUMBER);
    virtual void SetUp() override
    {
        errno = 0;
    }
    virtual void TearDown() override
    {
        EXPECT_EQ(errno, EINVAL);
        errno = 0;
    }
};

TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_linear_buffer_return_nullptr_if_buffer_is_nullptr_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_linear(nullptr, INAPPLICABLE_SIZE));
}

TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_linear_return_nullptr_if_size_is_zero_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_linear(INAPPLICABLE_BUFFER_PTR, 0));
}

TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_linear_return_nullptr_if_size_less_than_minimal_size_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_linear(INAPPLICABLE_BUFFER_PTR, MINIMAL_BUFFER_SIZE-1));
}

TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_random_buffer_return_nullptr_if_buffer_is_nullptr_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_random(nullptr, INAPPLICABLE_SIZE, INAPPLICABLE_RANDOM_GENERATOR));
}

TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_random_return_nullptr_if_size_is_zero_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_random(INAPPLICABLE_BUFFER_PTR, 0, INAPPLICABLE_RANDOM_GENERATOR));
}

TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_random_return_nullptr_if_size_less_than_minimal_size_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_random(INAPPLICABLE_BUFFER_PTR, MINIMAL_BUFFER_SIZE-1, INAPPLICABLE_RANDOM_GENERATOR));
}
TEST_F(pointer_chasing_bad_param_tests, pointer_chasing_create_random_return_nullptr_if_generator_is_nullptr_negative)
{
    EXPECT_EQ(nullptr, pointer_chase_create_random(INAPPLICABLE_BUFFER_PTR, INAPPLICABLE_SIZE,  nullptr));
}

class pointer_chasing_base
{
public:
    pointer_chasing_base()
    {
    }
    virtual ~pointer_chasing_base()
    {
    }
};

class pointer_chasing_fixture:
    public pointer_chasing_base,
    public TestWithParam<std::pair<cache_line_node_t*(*)(void*, size_t), std::string>>
{
protected:
    uint8_t m_buffer[777];
    size_t m_EXPECT_NODES_NUMBER = sizeof(m_buffer )/ LINE_SIZE;
    cache_line_node_t* m_pointer_chase = nullptr;
    cache_line_node_t* first_it = nullptr;
    
    virtual void SetUp() override
    {
        errno = 0;
        ASSERT_TRUE(sizeof(m_buffer) > MINIMAL_BUFFER_SIZE);
        srandom(FAKE_RANDOM_SEED);
        m_pointer_chase = GetParam().first(m_buffer, sizeof(m_buffer));
        ASSERT_NE(nullptr, m_pointer_chase);
        first_it = m_pointer_chase;
        ASSERT_NE(nullptr, first_it);
    }    
    virtual void TearDown() override
    {
        errno = 0;
    }

};

INSTANTIATE_TEST_CASE_P(
        pointer_chasing_test,
        pointer_chasing_fixture,
        ::testing::Values(
            std::pair([](void* buffer, size_t size)
            {
               return pointer_chase_create_linear(buffer, size);
            },"pointer_chase_create_linear"),
            std::pair([](void* buffer, size_t size)
            {
                return pointer_chase_create_random(buffer, size, random);
            }, "pointer_chase_create_random"),
            std::pair([](void* buffer, size_t size)
            {
                return pointer_chase_create_random(buffer, size, []()->long
                {
                    // Here we send random values that tests different branches of node swapping
                    static long value = -1;
                    value++;
                    switch (value)
                    {
                    case 0: return 0; //try swap same nodes
                    case 1: return 1; //try swap same nodes
                    case 2: return 1; //try swap near node than line2 on left
                    case 3: return 4; //try swap near node than line2 on right
                    case 4: return 4; //try swap same nodes
                    case 5: return 5; //try swap same nodes
                    case 6: return 4; //try swap far node than line2 on left
                    case 7: return 9; //try swap far node than line2 on right
                    default: return value*2;
                    }

                });
           }, "pointer_chase_create_random_random_generation")
        ),[](auto& info)
        {
            return info.param.second;
        });

TEST_P(pointer_chasing_fixture, pointer_chasing_create_positive){}

TEST_P(pointer_chasing_fixture, pointer_chase_run_read_workload_cyclic_positive)
{
    EXPECT_EQ(first_it, pointer_chase_run_read_workload(m_pointer_chase, m_EXPECT_NODES_NUMBER));
}

TEST_P(pointer_chasing_fixture, pointer_chase_run_read_write_workload_cyclic_positive)
{
    EXPECT_NE(nullptr, pointer_chase_run_read_write_workload(m_pointer_chase, m_EXPECT_NODES_NUMBER));
}

TEST_P(pointer_chasing_fixture, pointer_chase_run_workload_read_cyclic_positive)
{
    EXPECT_EQ(first_it, pointer_chase_run_workload_read_cyclic(m_pointer_chase, 3));
}

TEST_P(pointer_chasing_fixture, pointer_chase_run_read_workload_not_have_additional_cycles_for_all_nodes_positive)
{
    auto* current = m_pointer_chase;
    for (size_t i = 0; i < m_EXPECT_NODES_NUMBER; i++) {
        current = pointer_chase_run_read_workload(current, 1);
        ASSERT_NE(nullptr, current) << i;
        if(i != m_EXPECT_NODES_NUMBER -1) {
            ASSERT_NE(m_pointer_chase, current) << i;
        } else {
            ASSERT_EQ(m_pointer_chase, current) << i;
        }
    }
}

TEST_P(pointer_chasing_fixture, cycle_check_all_uniq_iterators_uniq_positive)
{
    std::set<cache_line_node_t*> was{};
    auto* current = m_pointer_chase;
    for (size_t i = 0; i < m_EXPECT_NODES_NUMBER; i++) {
        current =  pointer_chase_run_read_workload(current, 1);
        ASSERT_NE(nullptr, current) << i;  
        ASSERT_TRUE(was.find(current) == was.end()) << i;
        was.insert(current);
    }
}

}

