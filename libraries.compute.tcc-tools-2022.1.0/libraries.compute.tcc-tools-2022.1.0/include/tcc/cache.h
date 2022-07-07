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
 * @file cache.h
 * @brief Intel(R) Time Coordinated Computing Tools (Intel® TCC Tools) cache allocation library
 *
 * Cache misses negatively affect the latency of real-time applications. With
 * Intel® TCC Tools, you can reduce cache misses by allocating buffers that are
 * less likely to be evicted from processor cache.
 *
 * The system uses the software SRAM process to create low-latency buffers.
 * Software SRAM is a software construct that uses hardware capabilities to
 * allocate a portion of the physical address space into the cache. The
 * addresses are less likely to be evicted by the same or other applications.
 *
 * First, you will need to use the cache configurator tool to reserve portions
 * of the cache for software SRAM buffers.
 *
 * After reserving the software SRAM buffers, you can use the cache allocation
 * library in your real-time application to use those buffers.
 *
 * The library is intended for an application that performs periodic
 * computations on the same data set and is sensitive to memory access latency.
 * You can replace malloc with the cache allocation library.
 *
 * The library allocates buffers based on the requested size and latency
 * requirements. The size of the buffer, specified in bytes, is the standard
 * input parameter to a malloc call. The latency requirement, specified in
 * nanoseconds, is the longest amount of time that can be tolerated
 * for accessing a single element in the buffer.
 *
 * For details, see the Developer Guide.
 */


#ifndef _TCC_CACHE_H_
#define _TCC_CACHE_H_

#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "tcc/err_code.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization
 *
 * Initializes internal structures and global settings for the memory buffer.
 *
 * Must be called before other tcc_cache* calls.
 *
 * Example:
 * @code
 * int cpuid = 3;
 * tcc_cache_init(cpuid);
 * @endcode
 *
 * @param[in] cpuid   Processor core where the real-time application should run (CPU affinity).
 *                    tcc_cache_init will attempt to set the CPU affinity to the specified
 *                    core and use cpuid for subsequent tcc_cache* calls. This parameter overrides
 *                    externally set CPU affinity. If the value doesn’t correspond to any core,
 *                    CPU affinity will not be changed.
 *
 * @retval TCC_E_SUCCESS Success
 * @retval TCC_BUFFER_NOT_FOUND Real-time configuration driver (tcc_buffer) is unavailable
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 * @retval -TCC_E_ERROR  Unable to set affinity or unable to initialize internal structures (::TCC_E_ERROR)
 */

int tcc_cache_init(int8_t cpuid);


/**
 * @brief Finalization
 *
 * Releases all resources and resets settings initialized by tcc_cache_init().
 *
 * Must be called before process exit. No further tcc_cache* calls can be
 * made after tcc_cache_finish() without an additional tcc_cache_init().
 *
 * @retval TCC_E_SUCCESS Success
 */
int tcc_cache_finish();

/**
 * @brief Sets the default latency requirement.
 *
 * Initializes the latency requirement for tcc_cache_malloc_default() and tcc_cache_calloc_default().
 *
 * Must be called before tcc_cache_calloc_default() and tcc_cache_malloc_default() calls.
 *
 * @param [in] latency Maximum tolerable latency requirement in nanoseconds
 *
 * @retval TCC_E_SUCCESS Success
 * @retval -TCC_E_NOT_AVAILABLE Resource is not available or insufficient (::TCC_E_NOT_AVAILABLE)
 */
int tcc_cache_set_default_latency(uint64_t latency);

/**
 * @brief Allocates memory according to the specified latency requirement. The memory will not be zero initialized.
 *
 * Allocates the specified size and returns a pointer to the allocated memory.
 *
 * Selects Level 2 (L2) cache, Level 3 (L3) cache, or DRAM depending on the specified latency value.
 *
 * @param [in] size    Size of the memory block in bytes
 * @param [in] latency Maximum tolerable latency requirement in nanoseconds
 *
 * @return A pointer to the allocated memory if the request was successful<br>
 *         NULL if memory allocation failed (for example, not enough memory, or size is 0) or the latency required cannot be
 *         satisfied
 */
void* tcc_cache_malloc(size_t size, uint64_t latency);

/**
 * @brief Allocates memory according to the default latency requirement set by tcc_cache_set_default_latency(). The memory will not be
 * zero initialized.
 *
 * Allocates the specified size and returns a pointer to the allocated memory.
 *
 * Selects Level 2 (L2) cache, Level 3 (L3) cache, or DRAM depending on the default latency set by tcc_cache_set_default_latency().
 *
 * @param [in] size  Size of the memory block in bytes
 *
 * @return A pointer to the allocated memory if the request was successful<br>
 *         NULL if memory allocation failed (for example, not enough memory, or size is 0) or latency required cannot be
 *         satisfied
 */
void* tcc_cache_malloc_default(size_t size);

/**
 * @brief Allocates memory according to the specified latency requirement. The memory will be zero initialized.
 *
 * Allocates the specified size and returns a pointer to the allocated memory.
 *
 * Selects Level 2 (L2) cache, Level 3 (L3) cache, or DRAM depending on the specified latency value.
 *
 * @param [in] num   Number of elements to be allocated
 * @param [in] size  Size of the elements in bytes
 * @param [in] latency Maximum tolerable latency requirement in nanoseconds
 *
 * @return A pointer to the allocated memory if the request was successful<br>
 *         NULL if memory allocation failed (for example, not enough memory, num is 0, or size is 0) or the latency required
 *         cannot be satisfied
 */
void* tcc_cache_calloc(size_t num, size_t size, uint64_t latency);

/**
 * @brief Allocates memory according to the default latency requirement set by tcc_cache_set_default_latency(). The memory will be zero
 * initialized.
 *
 * Allocates the specified size and returns a pointer to the allocated memory.
 *
 * Selects Level 2 (L2) cache, Level 3 (L3) cache, or DRAM depending on the default latency set by tcc_cache_set_default_latency().
 *
 * @param [in] num   Number of elements to be allocated
 * @param [in] size  Size of the elements in bytes
 *
 * @return A pointer to the allocated memory if the request was successful<br>
 *         NULL if memory allocation failed (for example, not enough memory, num is 0, or size is 0) or the latency required
 *         cannot be satisfied
 */
void* tcc_cache_calloc_default(size_t num, size_t size);

/**
 * @brief Changes the size of the specified memory block.
 *
 * Changes the size of the memory block specified by ptr
 * to the specified size. The contents remain unchanged in the range from
 * the start of the region up to the minimum of the old and new sizes. If
 * the new size is larger than the old size, the added memory will not be
 * initialized.
 *
 * Latency requirements remain unchanged.
 *
 * @param [in] ptr   Pointer to the memory block to be reallocated
 * @param [in] size  New size of the array in bytes
 *
 * @return A pointer to the newly allocated memory if the request was successful or NULL if the request failed<br>
 *         NULL (or a pointer suitable to be passed to tcc_free()) if the size was equal to 0
 * @note Input pointer is preserved if reallocation request cannot be completed.
 */
void* tcc_cache_realloc(void* ptr, size_t size);

/**
 * @brief Deallocates the memory previously allocated by tcc_cache_calloc(), tcc_cache_malloc(),
 * tcc_cache_calloc_default(), tcc_cache_malloc_default(), or tcc_cache_realloc().
 *
 * If a null pointer is passed as argument, no action occurs.
 *
 * @param [in] ptr  Pointer to the memory block
 */
void tcc_cache_free(void* ptr);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_CACHE_H_
