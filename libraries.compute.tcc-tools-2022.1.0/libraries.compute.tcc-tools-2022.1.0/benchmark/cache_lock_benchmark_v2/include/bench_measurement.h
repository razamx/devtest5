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

#ifndef PMC_MEASUREMENT_H
#define PMC_MEASUREMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef enum bench_measurement_value_t
{
    bench_measurement_clk,
    bench_measurement_l2_hit,
    bench_measurement_l2_miss,
    bench_measurement_l3_hit,
    bench_measurement_l3_miss,
    bench_measurement_counters_number
} bench_measurement_value_t;

int bench_measurement_init(const char* name, uint8_t cpuid, bool collect_pmc);
void bench_measurement_begin();
void bench_measurement_end();
int64_t bench_measurement_get(bench_measurement_value_t value_type, size_t id) __attribute__((pure));
size_t bench_measurement_size();
int bench_measurement_store_data_to_files(const char* file_base_name);


#ifdef __cplusplus
}
#endif

#endif  // PMC_MEASUREMENT_H
