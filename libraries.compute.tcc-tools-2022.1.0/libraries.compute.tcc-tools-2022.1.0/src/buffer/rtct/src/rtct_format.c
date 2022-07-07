/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#include "rtct_format.h"
#include "tcc_log.h"
#include <errno.h>

#define RTCT_TABLE_ENTRIES_OFFSET 36

int rtct_get_entries(const void* rtct_table, size_t size, rtct_table_entries_t* entries)
{
    TCC_TRACE_FUNC("rtct_table=%p size=%zu entries=%p", rtct_table, size, entries);

    if (rtct_table == NULL) {
        TCC_LOG_WARNING("Parameter \"rtct_table\" is NULL");
        errno = EINVAL;
        goto error;
    }
    if (size == 0) {
        TCC_LOG_WARNING("Parameter \"size\" is zero");
        errno = EINVAL;
        goto error;
    }
    if (entries == NULL) {
        TCC_LOG_WARNING("Entries pointer is NULL");
        errno = EINVAL;
        goto error;
    }
    entries->begin = (rtct_entry_header_t*)((size_t)rtct_table + RTCT_TABLE_ENTRIES_OFFSET);
    if (size == RTCT_TABLE_ENTRIES_OFFSET) {
        entries->end = entries->begin;
    } else if (size < RTCT_TABLE_ENTRIES_OFFSET + sizeof(rtct_entry_header_t)) {
        TCC_LOG_WARNING("Wrong RTCT table");
        errno = EINVAL;
        goto error;
    } else {
        entries->end = (rtct_entry_header_t*)((size_t)rtct_table + size);
    }
    TCC_LOG_TRACE("entries:%p{begin:%p, end:%p", entries, entries->begin, entries->end);
    return 0;
error:
    return -1;
}

const rtct_entry_header_t* rtct_next_entry(const rtct_table_entries_t* self, const rtct_entry_header_t* start)
{
    TCC_TRACE_FUNC("self=%p start=%p", self, start);

    if (self == NULL) {
        TCC_LOG_WARNING("Parameter \"self\" is NULL");
        errno = EINVAL;
        goto error;
    }
    if (start == NULL) {
        return self->begin;
    }
    if (start == self->end) {
        TCC_LOG_DEBUG("End of PTCP table found for start");
        return self->end;
    }
    rtct_entry_header_t* result = (rtct_entry_header_t*)((size_t)start + (size_t)start->size);
    if (result >= self->end) {
        TCC_LOG_DEBUG("End of PTCP table");
        return self->end;
    }
    TCC_LOG_TRACE("table:%p, find entry: %p", self->begin, result);
    return result;
error:
    return NULL;
}
