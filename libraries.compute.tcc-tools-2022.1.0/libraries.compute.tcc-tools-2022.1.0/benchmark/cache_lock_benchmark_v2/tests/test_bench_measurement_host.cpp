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
#include "bench_measurement.h"
#include "alloc_mock.hpp"
#include "tcc_measurement_mock.hpp"
#include "tcc_msr_mock.hpp"
#include "libc_mock.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <memory>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include <gtest_file_helpers.hpp>
#include <map>
#include <algorithm>


extern "C" {
#define static
#define inline
#include "bench_measurement.c"
#undef static
#undef inline
}

///////////////////////////////////////////////////////////////////////////////
/// PERFORMANCE COUNTER MOCK //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define MEASUREMENT_NAME s_measurement_name

class performance_counters_mock
{
public:
    MOCK_METHOD1(rdpmc, uint64_t(uint32_t counter));
    MOCK_METHOD0(rdtscp, uint64_t(void));

    COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(performance_counters_mock)
};
COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(performance_counters_mock)
void performance_counters_mock::init()
{
}

extern "C" {
uint64_t __wrap_rdpmc(uint32_t counter)
{
    TCC_TRACE_FUNC("called");
    return ALLOW_REGULAR_MOCK(performance_counters_mock, rdpmc)(counter);
}
uint64_t __wrap_rdtscp(void)
{
    TCC_TRACE_FUNC("called");
    return ALLOW_REGULAR_MOCK(performance_counters_mock, rdtscp)();
}
}

namespace
{

const std::string s_measurement_name = "measurement_name";
const std::string s_measurement_dump_file_name = "dump_file.txt";
const std::string s_rdrtcm_file = "/sys/bus/event_source/devices/cpu/rdpmc";
const std::string s_clk_file_name = s_measurement_dump_file_name;
const std::string s_l2_hit_file_name = s_measurement_dump_file_name + ".l2_hit.txt";
const std::string s_l2_miss_file_name = s_measurement_dump_file_name + ".l2_miss.txt";
const std::string s_l3_hit_file_name = s_measurement_dump_file_name + ".l3_hit.txt";
const std::string s_l3_miss_file_name = s_measurement_dump_file_name + ".l3_miss.txt";
const std::vector<std::string> s_files = {
        s_clk_file_name,
        s_l2_hit_file_name,
        s_l2_miss_file_name,
        s_l3_hit_file_name,
        s_l3_miss_file_name
    };
const size_t s_fake_buffer_size = 10;
const uint8_t s_fake_cpuid = 42;


using namespace ::testing;
using namespace ::fake_file;

template<typename TestingType>
class fixture_base: public TestingType {};

#define UNI_FIXTURE(name, parent) \
template<typename T> class name##_base; \
typedef name ## _base<Test> name; \
template<typename T> class name##_base: public parent##_base<T>

#define BASE(parent) \
typedef parent##_base<T> base;

UNI_FIXTURE(bench_measurement_not_initialysed_test, fixture)
{
    NiceMock<performance_counters_mock> m_performance_counters_mock;
};

TEST_F(bench_measurement_not_initialysed_test, bench_measurement_begin_failure)
{
    bench_measurement_begin();
}
TEST_F(bench_measurement_not_initialysed_test, bench_measurement_end_failure)
{
    bench_measurement_end();
}
TEST_F(bench_measurement_not_initialysed_test, bench_measurement_get_failure)
{
    EXPECT_EQ(-1, bench_measurement_get(bench_measurement_l3_miss, 0));
}
TEST_F(bench_measurement_not_initialysed_test, bench_measurement_size_failure)
{
    EXPECT_EQ(0, bench_measurement_size());
}
TEST_F(bench_measurement_not_initialysed_test, bench_measurement_store_data_to_files_failure)
{
    EXPECT_EQ(-1, bench_measurement_store_data_to_files(nullptr));
}

UNI_FIXTURE(bench_measurement_test, fixture)
{
protected:
    std::shared_ptr<performance_counters_mock> m_performance_counters_mock =
    std::make_shared<NiceMock<performance_counters_mock>>();
    int m_fake_fd = 33;

    NiceMock<alloc_mock> m_alloc_mock;
    StrictMock<measurement_mock> m_tcc_measurement_mock;
    StrictMock<tcc_msr_mock> m_tcc_msr_mock;
    StrictMock<libc_mock> m_libc_mock;
    void TearDown() override
    {
        //Deinit call with error.
        EXPECT_CALL(m_tcc_measurement_mock, tcc_measurement_get_dump_file_from_env())
            .Times(AnyNumber())
            .WillRepeatedly(ReturnNull());
        bench_measurement_deinit();
    }
};

TEST_F(bench_measurement_test, bench_measurement_init_return_error_if_name_is_nullptr_negative)
{
    EXPECT_EQ(-1, bench_measurement_init(nullptr, s_fake_cpuid, false));
}
TEST_F(bench_measurement_test, benchd_measurement_init_return_if_error_can_t_allocate_data_for_internal_state_negative)
{
    EXPECT_CALL(m_alloc_mock, calloc(_, _)).WillOnce(ReturnNull());
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, false));
}
TEST_F(bench_measurement_test, bench_measurement_init_return_error_if_measuremetn_buffer_size_equal_0_negative)
{
    EXPECT_CALL(m_tcc_measurement_mock, tcc_measurement_get_buffer_size_from_env(StrEq(s_measurement_name)))
        .WillOnce(Return(0));
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, false));
}
TEST_F(bench_measurement_test,
    benchd_measurement_init_return_if_error_can_t_allocate_data_for_measurement_buffer_negative)
{
    InSequence s;
    EXPECT_CALL(m_tcc_measurement_mock, tcc_measurement_get_buffer_size_from_env(StrEq(s_measurement_name)))
        .WillOnce(Return(s_fake_buffer_size));
    EXPECT_CALL(m_alloc_mock, malloc(_)).WillOnce(ReturnNull());
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, false));
}

TEST_F(bench_measurement_test,
    benchd_measurement_init_return_error_if_can_t_allocate_data_for_measurement_name_negative)
{
    InSequence s;
    EXPECT_CALL(m_tcc_measurement_mock, tcc_measurement_get_buffer_size_from_env(StrEq(s_measurement_name)))
        .WillOnce(Return(s_fake_buffer_size));
    EXPECT_CALL(m_alloc_mock, strdup(StrEq(s_measurement_name))).WillOnce(ReturnNull());
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, false));
}

TEST_F(bench_measurement_test, benchd_measurement_init_return_error_if_can_open_file_to_enable_pmu_negative)
{
    InSequence s;
    EXPECT_CALL(m_libc_mock, open(StrEq(s_rdrtcm_file), O_WRONLY, _)).WillOnce(Return(-1));
    EXPECT_CALL(m_libc_mock, close(-1)).WillOnce(Return(0));
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, true));
}

TEST_F(bench_measurement_test, benchd_measurement_init_return_error_if_can_t_write_to_file_to_enable_pmu_negative)
{
    InSequence s;
    EXPECT_CALL(m_libc_mock, open(StrEq(s_rdrtcm_file), O_WRONLY, _)).WillOnce(Return(m_fake_fd));
    EXPECT_CALL(m_libc_mock, dprintf(m_fake_fd, NotNull(), _)).WillOnce(Return(-1));
    EXPECT_CALL(m_libc_mock, close(m_fake_fd)).WillOnce(Return(m_fake_fd));
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, true));
}

TEST_F(bench_measurement_test, benchd_measurement_init_return_error_if_can_t_write_msr_enable_pmu_negative)
{
    InSequence s;
    EXPECT_CALL(m_libc_mock, open(StrEq(s_rdrtcm_file), O_WRONLY, _)).WillOnce(Return(m_fake_fd));
    EXPECT_CALL(m_libc_mock, dprintf(m_fake_fd, NotNull(), _))
        .WillOnce(Invoke([](int, const char* format, va_list args) {
            char* str = nullptr;
            int answer = vasprintf(&str, format, args);
            EXPECT_GT(answer, 0);
            if (answer < 0) {
                return -1;
            }
            EXPECT_NE(nullptr, str);
            EXPECT_STREQ("2", str);
            free(str);
            return answer;
        }));
    EXPECT_CALL(m_libc_mock, close(m_fake_fd)).WillOnce(Return(m_fake_fd));

    EXPECT_CALL(m_tcc_msr_mock, wrmsr(_, _, _)).WillOnce(Return(-1));
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, true));
}

TEST_F(bench_measurement_test, benchd_measurement_init_return_error_cant_allocate_memory_for_control_structure_negative)
{
    InSequence s;
    EXPECT_CALL(m_libc_mock, open(StrEq(s_rdrtcm_file), O_WRONLY, _)).WillOnce(Return(m_fake_fd));
    EXPECT_CALL(m_libc_mock, dprintf(m_fake_fd, NotNull(), _))
        .WillOnce(Invoke([](int, const char* format, va_list args) {
            char* str = nullptr;
            int answer = vasprintf(&str, format, args);
            EXPECT_GT(answer, 0);
            if (answer < 0) {
                return -1;
            }
            EXPECT_NE(nullptr, str);
            EXPECT_STREQ("2", str);
            free(str);
            return answer;
        }));
    EXPECT_CALL(m_libc_mock, close(m_fake_fd)).WillOnce(Return(m_fake_fd));

    EXPECT_CALL(m_tcc_msr_mock, wrmsr(_, _, _)).Times(AnyNumber()).WillRepeatedly(Return(0));
    EXPECT_CALL(m_alloc_mock, calloc(_, _)).WillOnce(ReturnNull());
    EXPECT_EQ(-1, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, true));
}


TEST_F(bench_measurement_test, bench_measurement_init_0_positive)
{
    EXPECT_CALL(m_tcc_measurement_mock, tcc_measurement_get_buffer_size_from_env(StrEq(s_measurement_name)))
        .WillOnce(Return(s_fake_buffer_size));
    EXPECT_EQ(0, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, false));
}

UNI_FIXTURE(bench_measurement_func_tsc_only_tests, bench_measurement_test)
{
protected:
    BASE(bench_measurement_test);

    bool m_pmc_enabled = false;
    void SetUp() override
    {
        EXPECT_CALL(base::m_tcc_measurement_mock, tcc_measurement_get_buffer_size_from_env(StrEq(s_measurement_name)))
            .WillOnce(Return(s_fake_buffer_size));

        EXPECT_EQ(0, bench_measurement_init(s_measurement_name.c_str(), s_fake_cpuid, m_pmc_enabled));

        ASSERT_TRUE(base::m_libc_mock.verify_and_clear());

        EXPECT_CALL(base::m_libc_mock, fprintf(_, _, _)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(base::m_libc_mock, fread(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_fread));
        EXPECT_CALL(base::m_libc_mock, fwrite(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_fwrite));
    }
    void TearDown() override
    {
        EXPECT_CALL(base::m_tcc_measurement_mock, tcc_measurement_get_dump_file_from_env())
            .Times(AnyNumber())
            .WillRepeatedly(Return(s_measurement_dump_file_name.c_str()));
        bench_measurement_deinit();
    }
};

TEST_F(bench_measurement_func_tsc_only_tests, failed_if_cant_create_name_for_clk_file_negative)
{
    EXPECT_CALL(m_alloc_mock, asprintf(NotNull(), StrEq("%s"), _)).WillOnce(Return(-1));
}

UNI_FIXTURE(bench_measurement_func_all_counters_tests, bench_measurement_func_tsc_only_tests)
{
protected:
    BASE(bench_measurement_func_tsc_only_tests);

    std::map<std::string, std::string>  m_file_data = [](){
        std::map<std::string, std::string> ret;
        for (auto& file_name: s_files) {
            ret.insert({file_name, s_measurement_name + ": "});
        }
        return ret;
    }();
    std::vector<std::pair<struct bench_counters, struct bench_counters>> m_counters;
    void SetUp() override
    {
        base::m_pmc_enabled = true;
        EXPECT_CALL(base::m_libc_mock, open(StrEq(s_rdrtcm_file), O_WRONLY, _)).WillOnce(Return(base::m_fake_fd));
        EXPECT_CALL(base::m_libc_mock, dprintf(base::m_fake_fd, NotNull(), _))
            .WillOnce(Invoke([](int, const char* format, va_list args) {
                char* str = nullptr;
                int answer = vasprintf(&str, format, args);
                EXPECT_GT(answer, 0);
                if (answer < 0) {
                    return -1;
                }
                EXPECT_NE(nullptr, str);
                EXPECT_STREQ("2", str);
                free(str);
                return answer;
            }));
        EXPECT_CALL(base::m_libc_mock, close(base::m_fake_fd)).WillOnce(Return(base::m_fake_fd));
        EXPECT_CALL(base::m_tcc_msr_mock, wrmsr(_, _, _)).Times(AnyNumber()).WillRepeatedly(Return(0));
        base::SetUp();
    }
    void TearDown() override
    {
        {
            InSequence s;
            for (auto& measured_data : m_counters) {
                auto& counters_mock = *base::m_performance_counters_mock;
                auto& begin = measured_data.first;
                auto& end = measured_data.second;
                for (size_t i = 0; i < sizeof(begin.pmc) / sizeof(begin.pmc[0]); i++) {
                    EXPECT_CALL(counters_mock, rdpmc(i)).WillOnce(Return(begin.pmc[i]));
                }
                EXPECT_CALL(counters_mock, rdtscp()).WillOnce(Return(begin.tsc));
                EXPECT_CALL(counters_mock, rdtscp()).WillOnce(Return(end.tsc));
                for (size_t i = 0; i < sizeof(begin.pmc) / sizeof(begin.pmc[0]); i++) {
                    EXPECT_CALL(counters_mock, rdpmc(i)).WillOnce(Return(end.pmc[i]));
                }
            }
            for(size_t i = 0; i < m_counters.size(); i++) {
                bench_measurement_begin();
                bench_measurement_end();
            }
        }
        base::TearDown();

            /*size_t total_counters = sizeof(begin.counters)/sizeof(begin.counters[0]);
            for(size_t j = 0; j < total_counters; j++) {
                m_file_data[s_files[j]].append(std::to_string(end.counters[j]-begin.counters[j]));
                if(j !=)
            }*/
    }

};


TEST_F(bench_measurement_func_tsc_only_tests, failed_if_cant_open_tsc_file_negative)
{
    EXPECT_CALL(m_libc_mock, fopen(StrEq(s_measurement_dump_file_name), "w")).WillOnce(ReturnNull());
}

TEST_F(bench_measurement_func_all_counters_tests, failed_if_cant_open_tsc_file_negative)
{
    EXPECT_CALL(m_libc_mock, fopen(StrEq(s_measurement_dump_file_name), "w")).WillOnce(ReturnNull());
}


std::vector<uint8_t> fget_content(FILE* file)
{
    long size = ftell(file);
    if (size < 0) {
        ADD_FAILURE() << "Can't get size of file " << reinterpret_cast<long>(file);
        return {};
    }
    EXPECT_NE(-1, fseek(file, 0, SEEK_SET)) << "file:" << reinterpret_cast<long>(file);
    std::vector<uint8_t> data(size, 0);
    if (fread(data.data(), sizeof(uint8_t), data.size(), file) == static_cast<size_t>(-1)) {
        ADD_FAILURE() << "Can't read data from file " << reinterpret_cast<long>(file);
        return {};
    }
    return data;
}

std::string fgets_content(FILE* file)
{
    auto data = fget_content(file);
    data.push_back('\0');
    return std::string(reinterpret_cast<const char*>(data.data()));
}

FILE* expect_content_on_close(std::string file_name, std::string mode, const char* content)
{
    FILE* file = create_file();
    std::string content_data = {content?content:""};

    EXPECT_CALL(libc_mock::get(), fopen(StrEq(file_name), StrEq(mode)))
        .Times(AtMost(1))
        .WillRepeatedly(
            Invoke([=](...) -> FILE*
            {
                EXPECT_NE(file, nullptr) << file_name;
                if (file == nullptr) {
                    return nullptr;
                }
                EXPECT_CALL(libc_mock::get(), fclose(file)).WillOnce(Invoke([=](FILE* file4close)
                {
                    if(content) {
                        EXPECT_STREQ(content_data.c_str(), fgets_content(file4close).c_str()) << file_name;
                    }
                    EXPECT_EQ(0, __real_fclose(file4close)) << file_name;
                    return 0;
                }));
                return file;
            }));
    return file;
}

TEST_F(bench_measurement_func_all_counters_tests, failed_if_cant_create_name_for_pmc_file_negative)
{
    EXPECT_CALL(m_alloc_mock, asprintf(NotNull(), StrEq("%s"), _)).WillOnce(DoDefault());
    EXPECT_CALL(m_alloc_mock, asprintf(NotNull(), StrEq("%s.%s.txt"), _)).WillOnce(Return(-1));
    for (auto& file_name: s_files) {
        expect_content_on_close(file_name, "w", m_file_data[file_name].c_str());
    }
}

auto s_file_name_test_printer = [](auto& val){
        std::string ret = val.param;
        std::replace(ret.begin(), ret.end(), '.', '_');
        return "file_" + ret;
    };

class bench_measurement_func_file_write_issues_test: public bench_measurement_func_all_counters_tests_base<TestWithParam<std::string>>
{
protected:
    typedef bench_measurement_func_all_counters_tests_base<TestWithParam<std::string>> base;
    std::string m_failure_for_file = GetParam();
};

INSTANTIATE_TEST_CASE_P(all_counters,
    bench_measurement_func_file_write_issues_test,
    ValuesIn(s_files.begin(), s_files.end()),
    s_file_name_test_printer);
TEST_P(bench_measurement_func_file_write_issues_test, failed_if_cant_open_file_negative)
{
    for (auto& file_name : s_files) {
        if (file_name == m_failure_for_file) {
            EXPECT_CALL(m_libc_mock, fopen(StrEq(file_name), StrEq("w"))).WillOnce(ReturnNull());
        } else {
            expect_content_on_close(file_name, "w", m_file_data[file_name].c_str());
        }
    }
}

TEST_P(bench_measurement_func_file_write_issues_test, failed_if_cant_write_measurement_name_negative)
{
    for (auto& file_name : s_files) {
        if (file_name == m_failure_for_file) {
            FILE* file = expect_content_on_close(file_name, "w", "");
            EXPECT_CALL(m_libc_mock, fprintf(file, StrEq("%s: ") ,_)).WillOnce(Return(-1));
        } else {
            expect_content_on_close(file_name, "w", m_file_data[file_name].c_str());
        }
    }
}

TEST_F(bench_measurement_func_all_counters_tests, empty_measurement_positive)
{
    for (auto& file_name : s_files) {
            expect_content_on_close(file_name, "w", m_file_data[file_name].c_str());
    }
}

//TODO prepare fflush for libcmock
//TEST_P(bench_measurement_func_file_write_issues_test, flush_failure_negative)
//{
//   for (auto& file_name : s_files) {
//        if (file_name == m_failure_for_file) {
//            FILE* file = expect_content_on_close(file_name, "w", "");
//            EXPECT_CALL(m_libc_mock, fflush(file)).WillOnce(Return(-1));
//        } else {
//            expect_content_on_close(file_name, "w", m_file_data[file_name]);
//        }
//    }
//}

TEST_F(bench_measurement_func_all_counters_tests, some_results_positive)
{
    for (auto& file_name : s_files) {
        expect_content_on_close(file_name, "w", (m_file_data[file_name] + "1, 2, 3").c_str());
    }
    for(size_t i = 0; i<3; i++){
        std::vector<uint64_t> begin;
        std::vector<uint64_t> end;
        for(size_t j = 0; j<bench_measurement_counters_number; j++){
            begin.push_back(i);
            end.push_back(i*2+1);
        }
        m_counters.push_back({
            *reinterpret_cast<struct bench_counters*>(begin.data()),
            *reinterpret_cast<struct bench_counters*>(end.data())
        });
    }

}

}  // namespace
