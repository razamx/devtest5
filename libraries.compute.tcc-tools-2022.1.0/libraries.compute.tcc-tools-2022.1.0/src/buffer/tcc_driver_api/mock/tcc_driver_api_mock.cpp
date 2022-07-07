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

#include "tcc_driver_api_mock.hpp"
#include "tcc_log.h"

using namespace ::testing;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(tcc_driver_api_mock)

void tcc_driver_api_mock::init()
{
        ON_CALL(*m_current, tcc_driver_get_region_count()).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, EPERM));
        ON_CALL(*m_current, tcc_driver_get_memory_config(_, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, EPERM));
        ON_CALL(*m_current, tcc_driver_req_buffer(_, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, EPERM));
        ON_CALL(*m_current, tcc_driver_read_rtct(_, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, EPERM));
}

int tcc_driver_get_region_count()
{
        TCC_TRACE_FUNC();
        return ALLOW_REGULAR_MOCK(tcc_driver_api_mock, tcc_driver_get_region_count)();
}

int tcc_driver_get_memory_config(unsigned int region_id,  memory_properties_t* out)
{
        TCC_TRACE_FUNC();
        return ALLOW_REGULAR_MOCK(tcc_driver_api_mock, tcc_driver_get_memory_config)(region_id, out);
}

int tcc_driver_req_buffer(unsigned int region_id, unsigned int size)
{
        TCC_TRACE_FUNC();
        return ALLOW_REGULAR_MOCK(tcc_driver_api_mock, tcc_driver_req_buffer)(region_id, size);
}

int tcc_driver_read_rtct(void** rtct, size_t* size)
{
        TCC_TRACE_FUNC();
        return ALLOW_REGULAR_MOCK(tcc_driver_api_mock, tcc_driver_read_rtct)(rtct, size);
}
