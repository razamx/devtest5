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

#include "region_manager.hpp"
#include "bench_measurement.h"
#include <gtest/gtest.h>
#include <functional>
#include <stdlib.h>
#include <map>
#include <vector>
#include "pointer_chasing.h"
#include <sys/mman.h>
#include "tcc_time.h"
#include "tcc_log.h"
#include "payload_tcc_region_manager_common.hpp"
#define PAYLOAD_MEMORY_SIZE 2*1024*1024

// WARNING: this test don't check that PSRAM or DRAM regions exist.

namespace
{
using namespace ::testing;

struct test_data
{
    size_t region_id;
    uint8_t core_number;
};

std::ostream& operator<<(std::ostream& s, const test_data& data){
    s << "{\"region_id\":" << data.region_id << ",";
    s << "\"core_number\":" << static_cast<unsigned>(data.core_number)<< "}";
    return s;
}

class mesurement_results
{
public:
    mesurement_results() : m_size {bench_measurement_size()}
    {
        for (auto type : m_types) {
            std::vector<int64_t> data{};
            for (size_t i = 0; i < m_size; i++) {
                auto value = bench_measurement_get(type, i);
                data.push_back(value);
            }
            m_data.insert({type, data});
        }
    }

    uint64_t get_sum(bench_measurement_value_t type)
    {
        uint64_t sum = 0;
        for (auto value : m_data[type]) {
            sum += value;
        }
        return sum;
    }

    size_t size()
    {
        return m_size;
    }

    double get_average(bench_measurement_value_t type)
    {
        return static_cast<double>(get_sum(type)) / m_data[type].size();
    }

private:
    std::vector<bench_measurement_value_t> m_types
    {
        bench_measurement_clk,
        bench_measurement_l2_hit,
        bench_measurement_l2_miss,
        bench_measurement_l3_hit,
        bench_measurement_l3_miss
    };
    size_t m_size;
    std::map<bench_measurement_value_t, std::vector<int64_t>> m_data;
};

class RegionManagerPerformanceBaseFixture : public TestWithParam<test_data>
{
public:
    test_data m_test_data = GetParam();
    uint8_t m_cpu_number = m_test_data.core_number;
    size_t m_region_id = m_test_data.region_id;
    int m_affinity_set_status = setafinity_for_core(m_cpu_number);
    RegionManager& m_manager = RegionManager::instance();
    const memory_properties_t& m_properties = m_manager.prop(m_test_data.region_id);
    size_t m_memory_size = (m_properties.type == TCC_MEM_DRAM) ? PAYLOAD_MEMORY_SIZE : m_properties.size;
    uint64_t m_memory_latency = m_properties.latency;
    void* m_memory_raw = nullptr;
    size_t m_iteration_number{100};
    size_t m_accesses{128};
    double m_performace_accuracy;
    double max_performancee_degradation_factor = 1.2;

    static int setafinity_for_core(uint8_t core_number)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(static_cast<int>(core_number), &cpuset);
        int status = sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset);
        if (status != 0) {
            TCC_LOG_FATAL("Can't set afinity for core %u", core_number);
        } else {
            TCC_LOG_INFO("Set afinity for core %u", core_number);
        }
        return status;
    }

    void SetUp() override
    {
        ASSERT_EQ(0, setenv("TCC_MEASUREMENTS_BUFFERS", MEASUREMENT_NAME ":1000", 1));
        ASSERT_NE(-1, setafinity_for_core(m_cpu_number));

        ASSERT_EQ(bench_measurement_init(MEASUREMENT_NAME, m_cpu_number, true), 0);
        m_memory_raw = (m_properties.type == TCC_MEM_DRAM)?allocate_stress_buffer(m_memory_size, true): m_manager.mmap(m_region_id, m_memory_size);
        ASSERT_NE(nullptr, m_memory_raw);
    }

    static void* allocate_stress_buffer(size_t size, bool use_huge_pages)
    {
        void* memory = nullptr;
        if (use_huge_pages) {
            // Huge pages used to avoid TLB impact for performance.
            if (system(
                "set -e; "
                "echo 20 > /proc/sys/vm/nr_hugepages;"
                "mkdir -p /mnt/huge;"
                "mount -t hugetlbfs nodev /mnt/huge;"
            ) < 0) {
                fprintf(stderr, "Can't mount huge pages\n");
                return nullptr;
            }
            int memory_fd = memfd_create("any", MFD_HUGETLB);
            if (memory_fd == -1) {
                TCC_LOG_FATAL("Can't create descriptor for stress memory");
                return nullptr;
            }
            if (ftruncate(memory_fd, size) != 0) {
                TCC_LOG_FATAL("Can't populate stress memory file");
                close(memory_fd);
                return nullptr;
            }
            memory = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_HUGETLB | MAP_LOCKED, memory_fd, 0);
            if (memory == MAP_FAILED) {
                TCC_LOG_FATAL("Can't mmap memory for stress for size: %zu: error:%s(%i)", size, strerror(errno), errno);
                memory = nullptr;
            }
            close(memory_fd);
        } else {
            memory = aligned_alloc(PAGE_SIZE, size);
            if (memory == NULL) {
                TCC_LOG_FATAL("Can't allocate alligned memory");
            }
        }
        return memory;
    }

    static void run_stress()
    {
        static cache_line_node_t* stress_chasing = nullptr;
        if (stress_chasing == nullptr) {
            stress_chasing = static_cast<cache_line_node_t*>(allocate_stress_buffer(STRESS_BUFFER_SIZE, true));
            if (stress_chasing == nullptr) {
                TCC_LOG_FATAL("Can't allocate stress memory");
                exit(1);
            }
            stress_chasing = pointer_chase_create_random(stress_chasing, STRESS_BUFFER_SIZE, random);
        }
        stress_chasing = pointer_chase_run_read_write_workload(stress_chasing, STRESS_BUFFER_SIZE /CACHE_LINE_SIZE/6 +1);
    }

    void test_region_memory_performance(size_t accesses, size_t iterations)
    {
        cache_line_node_t* line = pointer_chase_create_random(m_memory_raw, m_memory_size, random);
        while (iterations--) {
            run_stress();
            bench_measurement_begin();
            line = pointer_chase_run_read_workload(line, accesses);
            bench_measurement_end();
        }
    }

    void TearDown() override
    {
        if (m_memory_raw != nullptr) {
            munmap(m_memory_raw, m_memory_size);
        }
    }
};

TEST_P(RegionManagerPerformanceBaseFixture, all_memory_performance_test)
{
    test_region_memory_performance(m_accesses, m_iteration_number);
    mesurement_results results{};
    EXPECT_EQ(m_iteration_number, results.size());
    uint64_t sum = results.get_sum(bench_measurement_clk);
    double latency = static_cast<double>(tcc_clk2ns(sum))/(results.size()*m_accesses);
    double min_perf_setting = m_memory_latency*max_performancee_degradation_factor;
    min_perf_setting = (min_perf_setting - m_memory_latency < MIN_PERF_DEVIATION)?m_memory_latency+MIN_PERF_DEVIATION:min_perf_setting;
    EXPECT_LT(latency, min_perf_setting);
    std::cout << " actual:" << latency << " requared:" << m_memory_latency << " min_setting:" << min_perf_setting <<  std::endl;
}

std::vector<test_data> get_test_cases()
{
    auto& manager = RegionManager::instance();
    static std::vector<test_data> data{};
    for (size_t region_id = 0; region_id < manager.size(); region_id++) {
        for (auto core_num : manager.get_applicable_cores(region_id)) {
            data.push_back({region_id, core_num});
        }
    }
    return data;
}

static std::vector<test_data> test_cases = get_test_cases();


INSTANTIATE_TEST_CASE_P(
    RegionManagerAllocationTests,
    RegionManagerPerformanceBaseFixture,
    ::testing::ValuesIn(test_cases.begin(), test_cases.end()),
    [](auto& param_val) {
        std::stringstream ss;
        ss << RegionManager::instance().get_test_name(param_val.param.region_id) << "_core" <<
            std::to_string(static_cast<unsigned>(param_val.param.core_number));
        return ss.str();
    }
);

}  // namespace
