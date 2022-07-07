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

#ifndef PDATASOURCE_MOCK_H
#define PDATASOURCE_MOCK_H

#include "mock_common.hpp"
#include <gmock/gmock.h>

struct pdatasource_mock {
    MOCK_METHOD2(prepare_datasource, void (uint64_t tcc_latency, int affinity));
    MOCK_METHOD0(destroy_datasource, void (void));
    MOCK_METHOD1(put_value_in_datasource, void (void *data));
    MOCK_METHOD0(get_value_from_datasource, void *(void));
    MOCK_METHOD0(print_datasource_statistic, void (void));

    COMMON_MOCK_INTERFACE_DEFINITIONS(pdatasource_mock);
};

#endif // PDATASOURCE_MOCK_H
