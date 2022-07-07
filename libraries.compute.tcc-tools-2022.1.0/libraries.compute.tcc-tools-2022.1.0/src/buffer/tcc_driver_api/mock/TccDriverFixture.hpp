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

#ifndef TCC_BUFFER_DRIVER_FIXTURE_HPP
#define TCC_BUFFER_DRIVER_FIXTURE_HPP

#include <linux/tcc_buffer.h>
#include <memory_properties_test_utils.hpp>
#include <gmock/gmock.h>
#include <cstdarg>

namespace tcc_buffer_test_utils
{
    using namespace ::testing;
    template <typename T>
    class TccDriverFixture : public T
    {
    public:
        const int TCC_BUFFER_DRIVER_FD = 777;
        const int FAKE_ERRNO_CODE = 611;
        const unsigned LATENCY_DRAM = 100;
        const unsigned LATENCY_L3 = 50;
        const unsigned LATENCY_L2 = 10;
        const unsigned LATENCY_DEFAULT_CACHE = LATENCY_L2;

        const size_t SIZE_DRAM = (size_t)-1;
        const size_t SIZE_L3 = 12 * 1024 * 1024;
        const size_t SIZE_L2 = 1280 * 1024;

        const unsigned INDEX_DRAM = 0;
        const unsigned INDEX_L2 = 1;
        const unsigned INDEX_L3 = 2;

        const cpu_set_t MASK_DRAM = cpu_set_from_list({0, 1, 2, 3});
        const cpu_set_t MASK_L3 = cpu_set_from_list({0, 1, 2, 3});
        const cpu_set_t MASK_L2_0 = cpu_set_from_list({0, 1});
        const cpu_set_t MASK_L2_1 = cpu_set_from_list({2, 3});

        const std::vector<uint8_t> RTCT_TABLE{ //Dump of real rtct table
            0x50, 0x54, 0x43, 0x54, 0x64, 0x01, 0x00, 0x00, 0x01, 0xee, 0x49, 0x4e, 0x54, 0x45, 0x4c, 0x20,
            0x45, 0x44, 0x4b, 0x32, 0x20, 0x20, 0x20, 0x20, 0x05, 0x00, 0x00, 0x00, 0x49, 0x4e, 0x54, 0x4c,
            0x0d, 0x00, 0x00, 0x01, 0x2c, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
            0x09, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
            0x18, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x04, 0x00,
            0x06, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
            0x30, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
            0x30, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x8e, 0x01, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
            0x30, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x72, 0x30, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
            0x14, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x40, 0x00, 0x00, 0x00, 0x00,
            0xa8, 0x33, 0x00, 0x00
        };

        const std::vector<tcc_buf_mem_config_s> m_tcc_buffer_config{
            {.id = INDEX_DRAM,
             .latency = LATENCY_L3,
             .size = SIZE_L3,
             .type = RGN_L3,
             .ways = 20,
             .cpu_mask_p = (void *)&MASK_L3},
            {.id = INDEX_L2,
             .latency = LATENCY_L2,
             .size = SIZE_L2,
             .type = RGN_L2,
             .ways = 20,
             .cpu_mask_p = (void *)&MASK_L2_0},
            {.id = INDEX_L3,
             .latency = LATENCY_L2,
             .size = SIZE_L2,
             .type = RGN_L2,
             .ways = 20,
             .cpu_mask_p = (void *)&MASK_L2_1}};

        const size_t ALLOC_SIZE = 1000;
        const size_t PAGE_SIZE = 1 << 12; //4096
        const unsigned int LATENCY_TOO_SMALL = 1;

        int memory_fd_start{0};
        std::map<int, std::pair<size_t, void *>> m_allocated_tcc_driver_memory;

        void SetUp() override
        {
            T::SetUp();

            EXPECT_CALL(T::m_libc_mock, stat(StrEq(TCC_BUFFER_NAME), NotNull()))
                .WillRepeatedly(Return(0));
            EXPECT_CALL(T::m_libc_mock, open(StrEq(TCC_BUFFER_NAME), _, _))
                .WillRepeatedly(Return(TCC_BUFFER_DRIVER_FD));
            EXPECT_CALL(T::m_libc_mock, close(TCC_BUFFER_DRIVER_FD))
                .WillRepeatedly(Return(0));
            EXPECT_CALL(T::m_libc_mock, open(ContainsRegex("^" TCC_BUFFER_NAME "[0-9]+$"), _, _))
                .WillRepeatedly(Return(memory_fd_start++));
            EXPECT_CALL(T::m_libc_mock, unlink(ContainsRegex("^" TCC_BUFFER_NAME "[0-9]+$")))
                .WillRepeatedly(Return(0));
            EXPECT_CALL(T::m_libc_mock, mmap(nullptr, Ne(0), _, _, Ge(0), 0))
                .WillRepeatedly(Invoke([&](void *, size_t length, int, int,
                                           int fd, off_t) {
                    auto it = m_allocated_tcc_driver_memory.find(fd);
                    EXPECT_FALSE(it == m_allocated_tcc_driver_memory.end());
                    if (T::HasFailure())
                    {
                        return reinterpret_cast<void *>(-1);
                    }
                    EXPECT_EQ(length, m_allocated_tcc_driver_memory[fd].first);
                    if (T::HasFailure())
                    {
                        return reinterpret_cast<void *>(-1);
                    }
                    return m_allocated_tcc_driver_memory[fd].second;
                }));
            EXPECT_CALL(T::m_libc_mock, ioctl(TCC_BUFFER_DRIVER_FD, TCC_GET_REGION_COUNT, _))
                .Times(AtMost(1))
                .WillRepeatedly(Invoke([&](int, unsigned long, va_list list) {
                    //TODO Create matcher for va_list
                    unsigned int *region_count = va_arg(list, unsigned int *);
                    if (region_count == nullptr)
                    {
                        TCC_LOG_ERROR("Negative test for nullptr");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    *region_count = m_tcc_buffer_config.size();
                    return 0;
                }));
            EXPECT_CALL(T::m_libc_mock, ioctl(TCC_BUFFER_DRIVER_FD, TCC_GET_MEMORY_CONFIG, _))
                .Times(AtMost(m_tcc_buffer_config.size()))
                .WillRepeatedly(Invoke([&](int, unsigned long, va_list list) {
                    //TODO Create matcher for va_list
                    struct tcc_buf_mem_config_s *properties = va_arg(list, struct tcc_buf_mem_config_s *);
                    if (properties == nullptr)
                    {
                        TCC_LOG_ERROR("Negative test for nullptr");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    if (properties->cpu_mask_p == nullptr)
                    {
                        TCC_LOG_ERROR("Negative test for nullptr mask");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    if (properties->id >= m_tcc_buffer_config.size())
                    {
                        TCC_LOG_ERROR("Negative test for region id out of range");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    EXPECT_EQ(properties->id, m_tcc_buffer_config[properties->id].id) << "Internal test error";
                    ;
                    if (T::HasFailure())
                    {
                        return -1;
                    }
                    properties->latency = m_tcc_buffer_config[properties->id].latency;
                    properties->size = m_tcc_buffer_config[properties->id].size;
                    properties->ways = m_tcc_buffer_config[properties->id].ways;
                    properties->type = m_tcc_buffer_config[properties->id].type;
                    *((cpu_set_t *)properties->cpu_mask_p) = *((const cpu_set_t *)(m_tcc_buffer_config[properties->id].cpu_mask_p));
                    return 0;
                }));
            EXPECT_CALL(T::m_libc_mock, ioctl(TCC_BUFFER_DRIVER_FD, TCC_REQ_BUFFER, _))
                .WillRepeatedly(Invoke([&](int, unsigned long, va_list list) {
                    //TODO Create matcher for va_list
                    struct tcc_buf_mem_req_s *requared = va_arg(list, struct tcc_buf_mem_req_s *);
                    if (requared == nullptr)
                    {
                        TCC_LOG_ERROR("Negativfe test for nullptr");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    if (requared->id >= m_tcc_buffer_config.size())
                    {
                        TCC_LOG_ERROR("Negative test for region id out of range");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    if (requared->size % PAGE_SIZE != 0)
                    {
                        TCC_LOG_ERROR("Negative test for not aligned allocation size");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    requared->devnode = m_allocated_tcc_driver_memory.size();
                    m_allocated_tcc_driver_memory[requared->devnode] = {requared->size, aligned_alloc(PAGE_SIZE, requared->size)};
                    EXPECT_NE(nullptr, m_allocated_tcc_driver_memory[requared->devnode].second) << "Internal test error";
                    if (T::HasFailure())
                    {
                        return -1;
                    }
                    return 0;
                }));

            EXPECT_CALL(T::m_libc_mock, close(0))
                .WillRepeatedly(Return(0));
            EXPECT_CALL(T::m_libc_mock, ioctl(TCC_BUFFER_DRIVER_FD, TCC_QUERY_RTCT_SIZE, _))
                .WillRepeatedly(Invoke([&](int, unsigned long, va_list list) {
                    unsigned int *size = va_arg(list, unsigned int*);
                    if (size == nullptr) {
                        TCC_LOG_ERROR("Negative test for nullptr");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    *size = RTCT_TABLE.size();
                    TCC_LOG_TRACE("Size  is %u", *size);
                    return 0;
                }));
            EXPECT_CALL(T::m_libc_mock, ioctl(TCC_BUFFER_DRIVER_FD, TCC_GET_RTCT, _))
                .WillRepeatedly(Invoke([&](int, unsigned long, va_list list) {
                    unsigned int *rtct = va_arg(list,  unsigned int*);
                    if (rtct == nullptr) {
                        TCC_LOG_ERROR("Negative test for nullptr");
                        errno = FAKE_ERRNO_CODE;
                        return -1;
                    }
                    memcpy(rtct, RTCT_TABLE.data(), RTCT_TABLE.size());
                    return 0;
                }));
        }
        void TearDown() override
        {
            for (auto &mem : m_allocated_tcc_driver_memory)
            {
                free(mem.second.second);
            }
            errno = 0;
            T::TearDown();
        }
    };
} // namespace tcc_buffer_test_utils

#endif //TCC_BUFFER_DRIVER_FIXTURE_HPP
