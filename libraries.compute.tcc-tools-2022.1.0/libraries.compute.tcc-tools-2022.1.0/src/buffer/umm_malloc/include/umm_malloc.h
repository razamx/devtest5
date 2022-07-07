/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

/* ----------------------------------------------------------------------------
 * umm_malloc.h - a memory allocator for embedded systems (microcontrollers)
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 */

#ifndef _TCC_UMM_MALLOC_H_
#define _TCC_UMM_MALLOC_H_

#include <stdint.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
extern "C" {
#endif

//#define UMM_PTR_2B
#define UMM_PTR_4B

#if defined(UMM_PTR_2B)
#define bsize_t uint16_t
#elif defined(UMM_PTR_4B)
#define bsize_t uint32_t
#else
#error umm_malloc pointer size is not defined
#endif


/* ------------------------------------------------------------------------ */

typedef struct umm_ptr_t
{
    bsize_t next;
    bsize_t prev;
} umm_ptr;

// block.header.used.next high bit is used as flag marking free block
typedef struct umm_block_t
{
    union
    {
        umm_ptr used;
    } header;
    union
    {
        umm_ptr free;
        unsigned char data[sizeof(umm_ptr)];
    } body;
} umm_block;

typedef struct
{
    umm_block* umm_heap;
    bsize_t umm_numblocks;
} umm_malloc_t;

/* ------------------------------------------------------------------------ */

int umm_init(umm_malloc_t* allocator, void* memory, size_t size);
int umm_explicit_mem_init(umm_malloc_t* allocator);
void* umm_malloc(size_t size, umm_malloc_t* allocator);
void* umm_calloc(size_t num, size_t size, umm_malloc_t* allocator);
void* umm_realloc(void* ptr, size_t size, umm_malloc_t* allocator);
void umm_free(void* ptr, umm_malloc_t* allocator);

int umm_clean_instance(umm_malloc_t* allocator);
/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_UMM_MALLOC_H_
