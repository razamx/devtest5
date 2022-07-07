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

#ifndef REGIONMANAGER_HPP
#define REGIONMANAGER_HPP

#include "tcc_region_manager.h"
#include <vector>
#include <string>

class RegionManager
{
public:
    static RegionManager& instance();

    virtual ~RegionManager();
    size_t size() const;
    const memory_properties_t& prop(size_t index) const;
    void* mmap(size_t index, size_t size);
    std::string get_test_name(size_t index) const;
    std::vector<uint8_t> get_applicable_cores(size_t index);
private:
    tcc_config_t* m_config = nullptr;
    region_manager_t* m_region_manager = nullptr;
    size_t m_size;
    std::vector<memory_properties_t> m_props;

    RegionManager();
    static RegionManager* s_region_manager;
    RegionManager(const RegionManager&) = delete;
    RegionManager& operator=(const RegionManager&) = delete;
};

#endif  // REGIONMANAGER_HPP
