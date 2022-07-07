/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#include "rtct.h"
#include "rtct_format.h"
#include "tcc_log.h"
#include "tcc_time.h"
#include <stdlib.h>

int64_t rtct_get_dram_latency_clk(void* rtct, size_t size)
{
    TCC_TRACE_FUNC("rtct=%p size=%zu", rtct, size);

    rtct_table_entries_t entries;
    if (rtct_get_entries(rtct, size, &entries) < 0) {
        TCC_LOG_WARNING("Can't get RTCT entries for %p", rtct);
        goto error;
    }
    for (const rtct_entry_header_t* entry = entries.begin;;) {
        entry = rtct_next_entry(&entries, entry);
        if (entry == NULL) {
            TCC_LOG_ERROR("Can't get next entry");
            break;
        }
        if (entry == entries.end) {
            TCC_LOG_WARNING("Can't find DRAM latency in rtct table");
            break;
        }
        const memory_hierarchy_t* mem_hierarchy = (const memory_hierarchy_t*)entry;
        if (mem_hierarchy->header.format != MemoryHierarchyLatency && mem_hierarchy->hierarhy != RTCT_MHL_DRAM) {
            TCC_LOG_TRACE("Entry %p, not a entry for dram latency", entry);
            continue;
        }
        return mem_hierarchy->clock_cycles_latency;
    }
error:
    return -1;
}
