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
#include "mock_common.hpp"
#include "tcc_region_manager_internal.h"
#include "tcc_buffer_test_helpers.hpp"
#include "linux/tcc_buffer.h"

#define PAGE_SIZE 4096

namespace
{
    using namespace ::testing;
    using namespace ::tcc_buffer_test_utils;

    class buffer_autodetection_nullptr_obj : public Test
    {
    };

    TEST_F(buffer_autodetection_nullptr_obj, region_manager_destroy)
    {
        region_manager_destroy(nullptr);
    }

    TEST_F(buffer_autodetection_nullptr_obj, region_manager_cout)
    {
        EXPECT_EQ(-1, region_manager_count(nullptr));
    }

    TEST_F(buffer_autodetection_nullptr_obj, region_manager_get)
    {
        EXPECT_EQ(nullptr, region_manager_get(nullptr, 0));
    }

    TEST_F(buffer_autodetection_nullptr_obj, region_manager_mmap)
    {
        EXPECT_EQ(nullptr, region_manager_mmap(nullptr, 0, PAGE_SIZE));
    }

    struct region_manager_driver_mock
    {
        MOCK_METHOD1(region_manager_create, region_manager_t *(const tcc_config_t *config));
        MOCK_METHOD1(region_manager_destroy, void(region_manager_t *self));
        MOCK_METHOD1(region_manager_count, ssize_t(region_manager_t *self));
        MOCK_METHOD2(region_manager_get, const memory_properties_t *(region_manager_t *self, size_t index));
        MOCK_METHOD3(region_manager_mmap, void *(region_manager_t *self, size_t index, size_t size));
        COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(region_manager_driver_mock);
    };

    COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(region_manager_driver_mock)
    void region_manager_driver_mock::init() {}

    class buffer_autodetection_base : public CheckSetUpTearDownSeqFixture<Test>
    {
    };

    class tcc_region_manager_autodetect_create : public buffer_autodetection_base
    {
    protected:
        NiceMock<alloc_mock> m_alloc_mock;
        StrictMock<libc_mock> m_libc_mock;
        const tcc_config_t *FAKE_CONFIG_PTR = reinterpret_cast<const tcc_config_t *>(0xDEADDEAD);
        const int FAKE_ERRNO = 611;
        region_manager_t *FAKE_REGION_MANAGER = reinterpret_cast<region_manager_t *>(0xBEAFBEAF);

        region_manager_t *m_manager = nullptr;

        void SetUp() override
        {
            buffer_autodetection_base::SetUp();
            errno = 0;
        }

        void TearDown() override
        {
            errno = 0;
            region_manager_destroy(m_manager);
            buffer_autodetection_base::TearDown();
        }
    };

    TEST_F(tcc_region_manager_autodetect_create, calloc_failure_negative)
    {
        EXPECT_CALL(m_alloc_mock, calloc(_, _)).WillOnce(MOCK_RETURN_VAL_SET_ERRNO(nullptr, ENOMEM));
        EXPECT_EQ(nullptr, region_manager_create(FAKE_CONFIG_PTR));
        EXPECT_EQ(errno, ENOMEM);
    }

    TEST_F(tcc_region_manager_autodetect_create, calloc_failure_and_nullptr_negative)
    {
        EXPECT_CALL(m_alloc_mock, calloc(_, _))
            .WillOnce(MOCK_RETURN_VAL_SET_ERRNO(nullptr, ENOMEM));
        EXPECT_EQ(nullptr, region_manager_create(nullptr));
        EXPECT_EQ(errno, ENOMEM);
    }

    // ////////////////////////////////////////////////////////////////////////////////////////////
    /// TCC_DRIVER
    // ////////////////////////////////////////////////////////////////////////////////////////////

    class tcc_region_manager_autodetect_driver_create : public tcc_region_manager_autodetect_create
    {
    protected:
        StrictMock<region_manager_driver_mock> m_region_manager_mock;
        void SetUp() override
        {
            tcc_region_manager_autodetect_create::SetUp();
            EXPECT_CALL(m_libc_mock, stat(StrEq(TCC_BUFFER_NAME), NotNull()))
                .WillOnce(Return(0));
        }
    };

    TEST_F(tcc_region_manager_autodetect_driver_create, region_manager_create_negative)
    {
        EXPECT_CALL(m_region_manager_mock, region_manager_create(FAKE_CONFIG_PTR))
            .WillOnce(MOCK_RETURN_VAL_SET_ERRNO(nullptr, FAKE_ERRNO));

        m_manager = region_manager_create(FAKE_CONFIG_PTR);
        EXPECT_EQ(nullptr, m_manager);
        EXPECT_EQ(errno, FAKE_ERRNO);
    }

    TEST_F(tcc_region_manager_autodetect_driver_create, region_manager_create_positive)
    {
        EXPECT_CALL(m_region_manager_mock, region_manager_create(FAKE_CONFIG_PTR))
            .WillOnce(MOCK_RETURN_VAL_SET_ERRNO(FAKE_REGION_MANAGER, 0));

        m_manager = region_manager_create(FAKE_CONFIG_PTR);
        ASSERT_NE(nullptr, m_manager);
        EXPECT_CALL(m_region_manager_mock, region_manager_destroy(FAKE_REGION_MANAGER))
            .WillOnce(Return());

        EXPECT_EQ(errno, 0);
    }

    class tcc_region_manager_autodetect_driver_methods : public tcc_region_manager_autodetect_driver_create
    {
    protected:
        void SetUp() override
        {
            tcc_region_manager_autodetect_driver_create::SetUp();
            EXPECT_CALL(m_region_manager_mock, region_manager_create(FAKE_CONFIG_PTR))
                .WillOnce(MOCK_RETURN_VAL_SET_ERRNO(FAKE_REGION_MANAGER, 0));
            m_manager = region_manager_create(FAKE_CONFIG_PTR);
            ASSERT_NE(nullptr, m_manager);
            EXPECT_CALL(m_region_manager_mock, region_manager_destroy(FAKE_REGION_MANAGER))
                .WillOnce(Return());
        }
    };

    TEST_F(tcc_region_manager_autodetect_driver_methods, region_manager_count)
    {
        InSequence s;
        const ssize_t FAKE_COUT_ID = 42;
        EXPECT_CALL(m_region_manager_mock, region_manager_count(FAKE_REGION_MANAGER))
            .WillOnce(Return(FAKE_COUT_ID));
        EXPECT_CALL(m_region_manager_mock, region_manager_count(FAKE_REGION_MANAGER))
            .WillOnce(Return(-1));
        EXPECT_EQ(FAKE_COUT_ID, region_manager_count(m_manager));
        EXPECT_EQ(-1, region_manager_count(m_manager));
    }

    TEST_F(tcc_region_manager_autodetect_driver_methods, region_manager_get)
    {
        InSequence s;

        const memory_properties_t *FAKE_PROPERTY = reinterpret_cast<const memory_properties_t *>(0xDEDADEDA);
        const size_t ID1 = 0;
        const size_t ID2 = 42;
        EXPECT_CALL(m_region_manager_mock, region_manager_get(FAKE_REGION_MANAGER, ID1))
            .WillOnce(Return(FAKE_PROPERTY));
        EXPECT_CALL(m_region_manager_mock, region_manager_get(FAKE_REGION_MANAGER, ID2))
            .WillOnce(Return(nullptr));
        EXPECT_EQ(FAKE_PROPERTY, region_manager_get(m_manager, ID1));
        EXPECT_EQ(nullptr, region_manager_get(m_manager, ID2));
    }

    TEST_F(tcc_region_manager_autodetect_driver_methods, region_manager_mmap)
    {
        InSequence s;

        void *FAKE_PTR = reinterpret_cast<void *>(0xDADADEDA);
        const size_t ID1 = 3;
        const size_t SIZE1 = 4096;
        const size_t ID2 = 12;
        const size_t SIZE2 = 4096 * 8;
        EXPECT_CALL(m_region_manager_mock, region_manager_mmap(FAKE_REGION_MANAGER, ID1, SIZE1))
            .WillOnce(Return(FAKE_PTR));
        EXPECT_CALL(m_region_manager_mock, region_manager_mmap(FAKE_REGION_MANAGER, ID2, SIZE2))
            .WillOnce(Return(reinterpret_cast<void *>(-1)));
        EXPECT_EQ(FAKE_PTR, region_manager_mmap(m_manager, ID1, SIZE1));
        EXPECT_EQ(reinterpret_cast<void *>(-1), region_manager_mmap(m_manager, ID2, SIZE2));
    }

} // namespace

///Driver region manager methods
region_manager_t *region_manager_driver_create(const tcc_config_t *config)
{
    return ALLOW_REGULAR_MOCK(region_manager_driver_mock, region_manager_create)(config);
}

void region_manager_driver_destroy(region_manager_t *self)
{
    return ALLOW_REGULAR_MOCK(region_manager_driver_mock, region_manager_destroy)(self);
}

ssize_t region_manager_driver_count(region_manager_t *self)
{
    return ALLOW_REGULAR_MOCK(region_manager_driver_mock, region_manager_count)(self);
}

const memory_properties_t *region_manager_driver_get(region_manager_t *self, size_t index)
{
    return ALLOW_REGULAR_MOCK(region_manager_driver_mock, region_manager_get)(self, index);
}

void *region_manager_driver_mmap(region_manager_t *self, size_t index, size_t size)
{
    return ALLOW_REGULAR_MOCK(region_manager_driver_mock, region_manager_mmap)(self, index, size);
}

