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

#include "pdatasource_mock.hpp"

extern "C" {
void prepare_datasource(uint64_t tcc_latency, int tcc_affinity);
void destroy_datasource(void);
void put_value_in_datasource(void *data);
void *get_value_from_datasource(void);
void print_datasource_statistic(void);
}

COMMON_MOCK_INTERFACE_IMPLEMENTATION(pdatasource_mock)

void prepare_datasource(uint64_t tcc_latency, int tcc_affinity)
{
    pdatasource_mock::get().prepare_datasource(tcc_latency, tcc_affinity);
}

void destroy_datasource(void)
{
    pdatasource_mock::get().destroy_datasource();
}

void put_value_in_datasource(void *data)
{
    pdatasource_mock::get().put_value_in_datasource(data);
}

void *get_value_from_datasource(void)
{
    return pdatasource_mock::get().get_value_from_datasource();
}

void print_datasource_statistic(void)
{
    pdatasource_mock::get().print_datasource_statistic();
}
