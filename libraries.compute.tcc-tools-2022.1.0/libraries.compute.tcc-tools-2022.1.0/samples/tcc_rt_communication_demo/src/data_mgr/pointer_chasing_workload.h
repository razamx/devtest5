/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/**
 * @file pointer_chasing_workload.h
 * @brief This file defines the API to the pointer chasing buffers used in the
 *        Real-Time Communication (RTC) Demo.
 *
 * The pointer chasing workload used in the demo includes the workload buffer with the specified
 * requirements for the maximum access time and the background workload stress thread. Together they
 * provide the workload buffer with initialized doubly linked list structure and access time depending
 * on the access time requirement specified during initialization.
 *
 * @sa pointer_chasing.h
 */

#ifndef _TCC_POINTER_CHASING_WORKLOAD_H_
#define _TCC_POINTER_CHASING_WORKLOAD_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CACHE_LINE_SIZE (64)

/**
 * This structure represents a doubly linked list with a value field and
 * aligned to the cache line size.
 */
struct cache_line_node_t
{
    union
    {
        /* The alignment buffer, aligns to the cache line size */
        uint8_t buf[CACHE_LINE_SIZE];
        struct
        {
            /** Pointer to the next list node */
            struct cache_line_node_t* next;
            /** Pointer to the previous list node */
            struct cache_line_node_t* prev;
            /** The data associated with the current list node */
            void* data;
        };
    };
};
typedef struct cache_line_node_t cache_line_node_t;

/**
 * Initialize the pointer chasing buffers
 * @param[in] latency The latency requirement for the workload buffer
 * @param[in] affinity The CPU affinity for the Pointer Chasing workload
 * @return The initialization success status
 */
int pc_init(uint64_t latency, int affinity);
/**
 * Deinitialize and release the pointer chasing buffers
 */
void pc_fini(void);
/**
 * Get the initialized pointer chasing buffer
 * @param[out] size The optional argument to store the workload buffer size
 * @return The pointer to the workload buffer or \c NULL,
 *         if the pointer chasing buffers are not initialized
 */
cache_line_node_t* pc_get_workload_buffer(size_t* size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // _TCC_POINTER_CHASING_WORKLOAD_H_
