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
#include "rtct_format.h"
#include "tcc_driver_api.h"
#include <stddef.h>
#include <stdio.h>


static void print_buffer(const void* start, const void* end)
{
    size_t size = (size_t)end - (size_t)start;
    for (size_t byte_num = 0; byte_num < size; byte_num++) {
        if (byte_num % 16 == 0) {
            printf("\n%04lx  ", byte_num / 16);
        }
        printf("%02x ", ((const uint8_t*)start)[byte_num]);
    }
    printf("\n\n");
}

#define case_enum2str(x) \
    case x:              \
        return #x

__attribute__((const)) static const char* rtct_type2str(unsigned type)
{
    switch (type) {
        case_enum2str(RTCD_Limits);
        case_enum2str(RTCM_Binary);
        case_enum2str(WRC_L3WayMasks);
        case_enum2str(GT_L3WayMasks);
        case_enum2str(PseudoSRAM);
        case_enum2str(StreamDataPath);
        case_enum2str(TimeAwareSubSystems);
        case_enum2str(RealTimeIOMMU);
        case_enum2str(MemoryHierarchyLatency);
    default:
        return "unknown";
    }
}
__attribute__((const)) static const char* rtct_memory_hierarhy2str(unsigned hierarhy)
{
    switch (hierarhy) {
        case_enum2str(RTCT_MHL_L2);
        case_enum2str(RTCT_MHL_L3);
        case_enum2str(RTCT_MHL_DRAM);
    default:
        return "unknown";
    }
}


static void print_entries(const rtct_table_entries_t* entries)
{
    const rtct_entry_header_t* entry = entries->begin;
    for (unsigned int entry_id = 0; entry != entries->end; entry_id++) {
        printf("Entry:%02u\n", entry_id);
        printf("size:%u\n", entry->size);
        printf("format:%u\n", entry->format);
        printf("type:%s(%u)\n", rtct_type2str(entry->type), entry->type);
        if (entry->type == MemoryHierarchyLatency) {
            const memory_hierarchy_t* hierarhy = (const memory_hierarchy_t*)entry;
            printf("   hierarhy:%s(%u)\n", rtct_memory_hierarhy2str(hierarhy->hierarhy), hierarhy->hierarhy);
            printf("   latency:%u\n", hierarhy->clock_cycles_latency);
        }
        const rtct_entry_header_t* new_entry = rtct_next_entry(entries, entry);
        print_buffer(entry, new_entry);
        entry = new_entry;
    }
}

int main()
{
    void* rtct = NULL;
    size_t size = 0;
    if (tcc_driver_read_rtct(&rtct, &size) < 0) {
        fprintf(stderr, "\nCan't read RTCT form driver\n");
        return 1;
    }
    rtct_table_entries_t entries;
    if (rtct_get_entries(rtct, size, &entries) < 0) {
        fprintf(stderr, "\nCan't entries from RTCT table\n");
        return -1;
    }
    printf("\nACPI Table Header:\n");
    print_buffer(rtct, entries.begin);
    printf("\nACPI RTCT Table entries:\n");
    print_entries(&entries);
    return 0;
}
