/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef _TCC_RTCT_FORMAT_H_
#define _TCC_RTCT_FORMAT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTCT_TABLE_ENTRIES_OFFSET 36

enum RTCT_ENTRY_TYPE
{
    RTCD_Limits = 0x00000001,
    RTCM_Binary = 0x00000002,
    WRC_L3WayMasks = 0x00000003,
    GT_L3WayMasks = 0x00000004,
    PseudoSRAM = 0x00000005,
    StreamDataPath = 0x00000006,
    TimeAwareSubSystems = 0x00000007,
    RealTimeIOMMU = 0x00000008,
    MemoryHierarchyLatency = 0x00000009,
};

enum RTCT_MHL_HIERARCHY
{
    RTCT_MHL_L2 = 0x00000002,
    RTCT_MHL_L3 = 0x00000003,
    RTCT_MHL_DRAM = 0x00000100,
};

typedef struct rtct_entry_header_t
{
    uint16_t size;
    uint16_t format;
    int type;
} rtct_entry_header_t;

typedef struct rtct_table_entries_t
{
    rtct_entry_header_t* begin;
    rtct_entry_header_t* end;
} rtct_table_entries_t;

typedef struct memory_hierarchy_t
{
    rtct_entry_header_t header;
    enum RTCT_MHL_HIERARCHY hierarhy;
    unsigned int clock_cycles_latency;
} memory_hierarchy_t;

/**
 * @brief Return next entry for rtct_table
 * @param rtct_table IN pointer to RTCT table
 * @param size IN size of RTCT table
 * @param entries OUTtype of entry that should be found
 * @return  0 on success
 *          -1 on failure
 */
int rtct_get_entries(const void* rtct_table, size_t size, rtct_table_entries_t* entries);
/**
 * @brief Return next entry for rtct_table
 * @param self pointer to RTCT table entries
 * @param start previsious entry
 * @return  pointer to found entry if found rec for type
 *          self->end no more entries in table
 *          NULL on error
 */
const rtct_entry_header_t* rtct_next_entry(const rtct_table_entries_t* self, const rtct_entry_header_t* start);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_RTCT_FORMAT_H_
