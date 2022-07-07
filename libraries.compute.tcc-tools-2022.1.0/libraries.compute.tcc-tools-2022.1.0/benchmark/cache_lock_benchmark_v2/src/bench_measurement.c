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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "benchmark_common.h"
#include "tcc/measurement_helpers.h"
#include "tcc_log.h"
#include "tcc_msr.h"
#include "configure_pmc.h"
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bench_measurement.h"

#define BITS_32 32

#define PMC_NUMBER 4

#define MEM_LOAD_RETIRED_L2_HIT 0x4302d1
#define MEM_LOAD_RETIRED_L2_MISS 0x4310d1
#define MEM_LOAD_RETIRED_L3_HIT 0x4304d1
#define MEM_LOAD_RETIRED_L3_MISS 0x4320d1

#define RDPMC_INSTRUCTION_COUNTER (1 << 30)

void bench_measurement_deinit();

static const char* file_name_prefix[1 + PMC_NUMBER] = {"", "l2_hit", "l2_miss", "l3_hit", "l3_miss"};

static inline void lfence(void)
{
    asm volatile("lfence\n\t");
}

static inline uint64_t rdpmc(uint32_t counter)
{
    volatile uint32_t high;
    volatile uint32_t low;
    asm volatile("rdpmc\n\t" : "=a"(low), "=d"(high) : "c"(counter));
    return (((uint64_t)high << BITS_32) | low);
}

static inline uint64_t rdtscp(void)
{
    uint32_t cycles_high;
    uint32_t cycles_low;
    asm volatile("rdtscp\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 : "=r"(cycles_high), "=r"(cycles_low)
                 :
                 : "%rax", "%rdx");
    return (((uint64_t)cycles_high << BITS_32) | cycles_low);
}

struct bench_counters
{
    union
    {
        uint64_t counters[1 + PMC_NUMBER];
        struct
        {
            uint64_t tsc;
            union
            {
                uint64_t pmc[PMC_NUMBER];
                struct
                {
                    uint64_t l2_hit;
                    uint64_t l2_miss;
                    uint64_t l3_hit;
                    uint64_t l3_miss;
                };
            };
        };
    };
};

typedef struct bench_measurement
{
    struct bench_counters begin;
    bool read_pmc_reauered;
    size_t size;
    size_t offset;
    struct bench_counters* data;
    size_t iterations;
    char* name;
    bool full;
} bench_measurement_t;

bench_measurement_t* self = NULL;

#define CHECK_INITIALIZED()                           \
    if (self == NULL) {                               \
        TCC_LOG_ERROR("Measurement not initialysed"); \
        goto error;                                   \
    }
static struct bench_counters get_result(const struct bench_counters* begin, const struct bench_counters* end)
    __attribute__((pure));
static struct bench_counters get_result(const struct bench_counters* begin, const struct bench_counters* end)
{
    struct bench_counters result;
    for (size_t i = 0; i < sizeof(result.counters) / sizeof(result.counters[0]); i++) {
        result.counters[i] = end->counters[i] - begin->counters[i];
    }
    return result;
}

void bench_measurement_begin()
{
    CHECK_INITIALIZED();
    lfence();
    if (self->read_pmc_reauered) {
        for (size_t i = 0; i < PMC_NUMBER; i++) {
            self->begin.pmc[i] = rdpmc(i);
        }
    } else {
        memset(&(self->begin.pmc), 0, sizeof(self->begin.pmc));
    }
    self->begin.tsc = rdtscp();
error:
    return;
}

void bench_measurement_end()
{
    struct bench_counters end;
    end.tsc = rdtscp();
    CHECK_INITIALIZED();
    if (self->read_pmc_reauered) {
        for (size_t i = 0; i < PMC_NUMBER; i++) {
            end.pmc[i] = rdpmc(i);
        }
    }
    self->data[self->offset] = get_result(&(self->begin), &end);
    self->offset = (self->offset + 1) % self->size;
    if (self->offset == 0) {
        self->full = true;
    }
error:
    return;
}

static int print_to_file(const char* file, const char* data)
{
    int fd = -1;
    int status = -1;
    fd = open(file, O_WRONLY);
    if (fd < 0) {
        TCC_LOG_ERROR("Can't open file '%s':error:%s(%i)", file, strerror(errno), errno);
        goto end;
    }
    if (dprintf(fd, "%s", data) < 0) {
        TCC_LOG_ERROR("Can't write data to file '%s':error:%s(%i)", file, strerror(errno), errno);
        goto end;
    }
    status = 0;

end:
    close(fd);
    return status;
}

static int init_pmc_msrs(uint8_t cpuid)
{
    if (print_to_file("/sys/bus/event_source/devices/cpu/rdpmc", "2") != 0) {
        TCC_LOG_ERROR("Can't eanble pmc read");
        goto error;
    }
    struct IA32_PERFEVTSELx_msr value[PMC_NUMBER];
    value[0].data = MEM_LOAD_RETIRED_L2_HIT;
    value[1].data = MEM_LOAD_RETIRED_L2_MISS;
    value[2].data = MEM_LOAD_RETIRED_L3_HIT;
    value[3].data = MEM_LOAD_RETIRED_L3_MISS;
    for (size_t i = 0; i < sizeof(value) / sizeof(value[0]); i++) {
        if (IA32_PERFEVTSELx_msr_write(i, cpuid, value[i]) != 0) {
            TCC_LOG_ERROR("Can't set PMC counter msr for %s", file_name_prefix[i + 1]);
            goto error;
        }
    }
    return 0;
error:
    return -1;
}

int bench_measurement_init(const char* name, uint8_t cpuid, bool collect_pmc)
{
    //We can have single measurement per system and reauared some global file lock for work
    TCC_TRACE_FUNC("name: %s", name);
    bench_measurement_deinit();
    struct bench_counters* data = NULL;
    char* measurement_name = NULL;
    if (name == NULL) {
        TCC_LOG_ERROR("name is NULL");
        goto error;
    }

    if (collect_pmc) {
        if (init_pmc_msrs(cpuid) == -1) {
            TCC_LOG_ERROR("Can't init PMC MSRs");
            goto error;
        }
    } else {
        TCC_LOG_INFO("PMC collection disabled");
    }

    self = (bench_measurement_t*)calloc(1, sizeof(bench_measurement_t));
    if (self == NULL) {
        TCC_LOG_ERROR("calloc:%s(%d)", strerror(errno), errno);
        goto error;
    }

    self->size = tcc_measurement_get_buffer_size_from_env(name);
    if (self->size == 0) {
        TCC_LOG_ERROR("Can't get size from TCC_MEASUREMENTS_BUFFERS");
        goto error;
    }
    data = (struct bench_counters*)malloc(sizeof(struct bench_counters) * self->size);
    if (data == NULL) {
        TCC_LOG_ERROR("malloc:%s(%d)", strerror(errno), errno);
        goto error;
    }
    TCC_LOG_TRACE("pmu_measuremetn data:%p", data);
    self->data = data;
    measurement_name = strdup(name);
    if (measurement_name == NULL) {
        TCC_LOG_ERROR("strdup:%s(%d)", strerror(errno), errno);
        goto error;
    }
    self->name = measurement_name;
    self->read_pmc_reauered = collect_pmc;
    return 0;
error:
    free(self);
    self = NULL;
    free(data);
    data = NULL;
    free(measurement_name);
    measurement_name = NULL;
    return -1;
}

static char* data_file_name = NULL;
static const char* bench_measurement_get_file_name(const char* file_base_name, size_t counter_id)
{
    free(data_file_name);
    int status = -1;
    if (counter_id == 0) {
        status = asprintf(&data_file_name, "%s", file_base_name);
    } else {
        status = asprintf(&data_file_name, "%s.%s.txt", file_base_name, file_name_prefix[counter_id]);
    }
    if (status < 0) {
        data_file_name = NULL;
        TCC_LOG_ERROR("Can't create string for file name: asprintf():errno:%s(%i)", strerror(errno), errno);
    }
    return data_file_name;
}
static FILE* bench_measurement_get_file(const char* file_base_name, size_t counter_id)
{
    const char* file_name = bench_measurement_get_file_name(file_base_name, counter_id);
    FILE* file = NULL;
    if (file_name == NULL) {
        TCC_LOG_ERROR("can't get file name to store");
        goto error;
    }
    file = fopen(file_name, "w");
    if (file == NULL) {
        TCC_LOG_ERROR("can't open file %s for 'w':fopen():errno:%s(%i)", file_name, strerror(errno), errno);
        goto error;
    }
    return file;
error:
    return NULL;
}


static int bench_measurement_store_data(const char* file_base_name, size_t counter_id)
{
    int status = -1;
    FILE* file = bench_measurement_get_file(file_base_name, counter_id);
    if (file == NULL) {
        TCC_LOG_ERROR("can't get file to store");
        goto error;
    }
    if (fprintf(file, "%s: ", self->name) < 0) {
        TCC_LOG_ERROR("fprintf:error:%s(%i)", strerror(errno), errno);
        goto error;
    }
    {
        size_t offset = (self->full ? (self->offset + 1) % self->size : 0);
        size_t iterations = self->full ? self->size : self->offset;
        bool first = true;
        for (size_t j = 0; j < iterations; j++) {
            if (first) {
                first = false;
            } else {
                if (fprintf(file, ", ") < 0) {
                    TCC_LOG_ERROR("fprintf:error:%s(%i)", strerror(errno), errno);
                    goto error;
                }
            }
            if (fprintf(file, "%" PRIu64, self->data[offset].counters[counter_id]) < 0) {
                TCC_LOG_ERROR("fprintf:error:%s(%i)", strerror(errno), errno);
                goto error;
            }
            offset = (offset + 1) % self->size;
        }
    }
    if (fflush(file) < 0) {
        TCC_LOG_ERROR("fprintf:error:%s(%i)", strerror(errno), errno);
        goto error;
    }
    status = 0;
error:
    if (file) {
        fclose(file);
    }
    return status;
}

int bench_measurement_store_data_to_files(const char* file_base_name)
{
    TCC_TRACE_FUNC();
    size_t counter_index = 0;
    CHECK_INITIALIZED();
    if (bench_measurement_store_data(file_base_name, counter_index) < 0) {
        TCC_LOG_ERROR("Can't store data for tsc");
        goto error;
    }
    if (self->read_pmc_reauered) {
        for (size_t i = 0; i < PMC_NUMBER; i++) {
            counter_index++;
            if (bench_measurement_store_data(file_base_name, counter_index) < 0) {
                TCC_LOG_ERROR("Can't store data for pmc_%zu", i);
                goto error;
            }
        }
    }
    return 0;
error:
    return -1;
}

static int bench_measurement_store_data_to_env_files()
{
    TCC_TRACE_FUNC();
    const char* file_base_name = tcc_measurement_get_dump_file_from_env();
    CHECK_INITIALIZED();
    if (file_base_name == NULL) {
        TCC_LOG_WARNING("cant get filename to stroe data");
        goto error;
    }
    return bench_measurement_store_data_to_files(file_base_name);
error:
    return -1;
}

size_t bench_measurement_size()
{
    CHECK_INITIALIZED();
    return self->offset;
error:
    return 0;
}

int64_t bench_measurement_get(bench_measurement_value_t value_type, size_t id)
{
    size_t real_id = id;
    CHECK_INITIALIZED();
    if (self->full) {
        real_id = (self->offset + id) / self->size;
    } else if (id >= self->offset) {
        goto error;
    }
    return (int64_t)self->data[real_id].counters[value_type];
error:
    TCC_LOG_WARNING("%zu is out of range", id);
    return -1;
}


__attribute__((destructor)) void bench_measurement_deinit()
{
    TCC_TRACE_FUNC();
    CHECK_INITIALIZED();
    if (bench_measurement_store_data_to_env_files() < 0) {
        TCC_LOG_WARNING("Can't store measured data");
    }
    free(self->data);
    free(self->name);
    free(self);
    self = NULL;
error:
    return;
}
