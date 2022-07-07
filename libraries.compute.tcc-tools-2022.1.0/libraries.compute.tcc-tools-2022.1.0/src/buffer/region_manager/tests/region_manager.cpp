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
#include "tcc_config_finder.h"
#include "tcc_log.h"
#include <stdexcept>
#include <thread>
#include <sstream>
#include <sched.h>


RegionManager::RegionManager()
    : m_config {tcc_config_read(get_available_config_file())}, m_region_manager {region_manager_create(m_config)}
{
    if (m_region_manager == nullptr) {
        throw std::runtime_error("Can't initialyze region manager");
    }
    ssize_t size = region_manager_count(m_region_manager);
    if (size < 0) {
        throw std::runtime_error("Can't get region_number");
    }
    m_size = static_cast<size_t>(size);
    for (size_t i = 0; i < m_size; i++) {
        const memory_properties_t* prop = region_manager_get(m_region_manager, i);
        if (prop == nullptr) {
            throw std::runtime_error("Can't get props for region " + std::to_string(i));
        }
        m_props.push_back(*prop);
    }
}
RegionManager* RegionManager::s_region_manager = nullptr;
RegionManager& RegionManager::instance()
{
    if (s_region_manager == nullptr) {
        s_region_manager = new RegionManager();
    }
    return *s_region_manager;
}

RegionManager::~RegionManager()
{
    region_manager_destroy(m_region_manager);
    tcc_config_destroy(m_config);
}

size_t RegionManager::size() const
{
    return m_size;
}

const memory_properties_t& RegionManager::prop(size_t index) const
{
    if (index > m_props.size()) {
        throw std::out_of_range("Index " + std::to_string(index));
    }
    return m_props[index];
}
void* RegionManager::mmap(size_t index, size_t size)
{
    return region_manager_mmap(m_region_manager, index, size);
}

std::string RegionManager::get_test_name(size_t index) const
{
    std::stringstream ss;
    int processor_count = static_cast<int>(std::thread::hardware_concurrency());
    auto prop_value = prop(index);
    ss << prop_value.id << "_";
    ss << tcc_memory_level_to_str(prop_value.type) << "_";
    ss << prop_value.size << "_";
    for (int i = 0; i < processor_count; i++) {
        if (CPU_ISSET(i, &(prop_value.mask))) {
            ss << "1";
        } else {
            ss << "0";
        }
    }
    return ss.str();
}

std::vector<uint8_t> RegionManager::get_applicable_cores(size_t index){
    int processor_count = static_cast<int>(std::thread::hardware_concurrency());
    std::vector<uint8_t> cores;
    for (uint8_t i = 0; i < static_cast<uint8_t>(processor_count); i++) {
        if (CPU_ISSET(i, &(prop(index).mask))) {
            cores.push_back(i);
        }
    }
    return cores;
}

