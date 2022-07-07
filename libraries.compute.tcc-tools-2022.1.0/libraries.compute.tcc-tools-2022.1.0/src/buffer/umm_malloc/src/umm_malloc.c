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
 * umm_malloc.c - a memory allocator for embedded systems (microcontrollers)
 *
 * See LICENSE for copyright notice
 * See README.md for acknowledgements and description of internals
 * ----------------------------------------------------------------------------
 *
 * R.Hempel 2007-09-22 - Original
 * R.Hempel 2008-12-11 - Added MIT License biolerplate
 *                     - realloc() now looks to see if previous block is free
 *                     - made common operations functions
 * R.Hempel 2009-03-02 - Added macros to disable tasking
 *                     - Added function to dump heap and check for valid free
 *                        pointer
 * R.Hempel 2009-03-09 - Changed name to umm_malloc to avoid conflicts with
 *                        the mm_malloc() library functions
 *                     - Added some test code to assimilate a free block
 *                        with the very block if possible. Complicated and
 *                        not worth the grief.
 * D.Frank 2014-04-02  - Fixed heap configuration when UMM_TEST_MAIN is NOT set,
 *                        added user-dependent configuration file umm_malloc_cfg.h
 * R.Hempel 2016-12-04 - Add support for Unity test framework
 *                     - Reorganize source files to avoid redundant content
 *                     - Move integrity and poison checking to separate file
 * R.Hempel 2017-12-29 - Fix bug in realloc when requesting a new block that
 *                        results in OOM error - see Issue 11
 * ----------------------------------------------------------------------------
 */

#include "umm_malloc.h"
#include "tcc_log.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcc/err_code.h>


/* ------------------------------------------------------------------------- */

#define UMM_TYPE_SIZE_BITS (sizeof(bsize_t) * 8)
#define UMM_FREELIST_MASK ((bsize_t)(1U << (UMM_TYPE_SIZE_BITS - 1)))
#define UMM_BLOCKNO_MASK ((bsize_t)(~UMM_FREELIST_MASK))
#define UMM_BLOCK_MAX_SIZE UMM_BLOCKNO_MASK
#define UMM_MIN_BLOCKS 3
#define UMM_MAX_BLOCKS (UMM_FREELIST_MASK)
#define UMM_MAX_HEAP_SIZE (sizeof(umm_block) * UMM_MAX_BLOCKS)

#define UMM_CRITICAL_ENTRY()
#define UMM_CRITICAL_EXIT()

/* ------------------------------------------------------------------------ */

#define UMM_NUMBLOCKS(allocator) ((allocator)->umm_numblocks)
#define UMM_BLOCK(b, allocator) ((allocator)->umm_heap[b])
#define UMM_NBLOCK(b, allocator) (UMM_BLOCK(b, allocator).header.used.next)
#define UMM_PBLOCK(b, allocator) (UMM_BLOCK(b, allocator).header.used.prev)
#define UMM_NFREE(b, allocator) (UMM_BLOCK(b, allocator).body.free.next)
#define UMM_PFREE(b, allocator) (UMM_BLOCK(b, allocator).body.free.prev)
#define UMM_DATA(b, allocator) (UMM_BLOCK(b, allocator).body.data)

/* -------------------------------------------------------------------------
 * There are additional files that may be included here. Normally it's
 * not a good idea to include .c files but in this case it keeps the
 * main umm_malloc file clear and prevents issues with exposing internal
 * data structures to other programs.
 * -------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------ */

typedef enum umm_block_status
{
    UMM_BLOCK_STATUS_BAD_BLOCK,
    UMM_BLOCK_STATUS_ALLOCATED,
    UMM_BLOCK_STATUS_FREE
} umm_block_status_t;

static bsize_t umm_get_number_of_blocks(size_t size)
{
    /*
     * The calculation of the block size is not too difficult, but there are
     * a few things that we need to be mindful of.
     *
     * When a block is removed from the free list, the space used by the free
     * pointers is available for data. That's what the first calculation
     * of size is doing.
     */
    const size_t umm_body_size = sizeof(((umm_block*)0)->body);
    if (size <= umm_body_size) {
        return (1);
    }

    /*
     * If it's for more than that, then we need to figure out the number of
     * additional whole blocks the size of an umm_block requires.
     */
    size_t rest_size = size - umm_body_size;                       // Rest bytes (not stored in body of the first block)
    size_t rest_blocks = (rest_size - 1) / sizeof(umm_block) + 1;  // integer round-up division
    return 1 + rest_blocks;  // first block (data uses only body) + rest whole blocks
}

/* ------------------------------------------------------------------------ */
/*
 * Split the block `c` into two blocks: `c` and `c + blocks`.
 *
 * - `new_freemask` should be `0` if `c + blocks` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 *
 * Note that free pointers are NOT modified by this function.
 */
static void umm_split_block(bsize_t c, bsize_t blocks, bsize_t new_freemask, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%p", c, blocks, new_freemask, allocator);

    UMM_NBLOCK(c + blocks, allocator) = (UMM_NBLOCK(c, allocator) & UMM_BLOCKNO_MASK) | new_freemask;
    UMM_PBLOCK(c + blocks, allocator) = c;

    UMM_PBLOCK(UMM_NBLOCK(c, allocator) & UMM_BLOCKNO_MASK, allocator) = (c + blocks);
    UMM_NBLOCK(c, allocator) = (c + blocks);
}

/* ------------------------------------------------------------------------ */

static void umm_disconnect_from_free_list(bsize_t c, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%" PRIu32 ",%p", c, allocator);

    /* Disconnect this block from the FREE list */

    UMM_NFREE(UMM_PFREE(c, allocator), allocator) = UMM_NFREE(c, allocator);
    UMM_PFREE(UMM_NFREE(c, allocator), allocator) = UMM_PFREE(c, allocator);

    /* And clear the free block indicator */

    UMM_NBLOCK(c, allocator) &= (~UMM_FREELIST_MASK);
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_up() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

static void umm_assimilate_up(bsize_t c, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%" PRIu32 ",%p", c, allocator);

    if (UMM_NBLOCK(UMM_NBLOCK(c, allocator), allocator) & UMM_FREELIST_MASK) {
        /*
         * The next block is a free block so assimilate up and remove it from
         * the free list
         */

        TCC_LOG_DEBUG("Assimilate up to next block, which is FREE\n");

        /* Disconnect the next block from the FREE list */

        umm_disconnect_from_free_list(UMM_NBLOCK(c, allocator), allocator);

        /* Assimilate the next block with this one */

        UMM_PBLOCK(UMM_NBLOCK(UMM_NBLOCK(c, allocator), allocator) & UMM_BLOCKNO_MASK, allocator) = c;
        UMM_NBLOCK(c, allocator) = UMM_NBLOCK(UMM_NBLOCK(c, allocator), allocator) & UMM_BLOCKNO_MASK;
    }
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_down() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

static bsize_t umm_assimilate_down(bsize_t c, bsize_t freemask, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%" PRIu32 ",%" PRIu32 ",%p", c, freemask, allocator);

    UMM_NBLOCK(UMM_PBLOCK(c, allocator), allocator) = UMM_NBLOCK(c, allocator) | freemask;
    UMM_PBLOCK(UMM_NBLOCK(c, allocator), allocator) = UMM_PBLOCK(c, allocator);

    return (UMM_PBLOCK(c, allocator));
}

// ------------------------------------------------------------------------

// Get block index for given pointer
// NOTE: this function assumes that this pointer within umm_malloc.heap
// Do checks before
static bsize_t umm_get_block_by_ptr(void* ptr, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%p %p", ptr, allocator);
    return ((uint8_t*)ptr - (uint8_t*)(allocator->umm_heap)) / sizeof(umm_block);
}

// Checks that given pointer is in heap
static int umm_check_ptr_in_heap(void* ptr, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%p %p", ptr, allocator);
    int ok = (uint8_t*)ptr >= (uint8_t*)allocator->umm_heap &&
             (uint8_t*)ptr < (uint8_t*)allocator->umm_heap + allocator->umm_numblocks * sizeof(umm_block);
    if (!ok) {
        TCC_LOG_ERROR("Ptr %p is of bounds", ptr);
    }
    return ok;
}

// Checks that given pointer is aligned with block
static int umm_check_ptr_is_block(void* ptr, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%p %p", ptr, allocator);
    size_t header_size = sizeof(((umm_block*)0)->header);
    int ok = ((uint8_t*)ptr - (uint8_t*)(allocator->umm_heap) - header_size) % sizeof(umm_block) == 0;
    if (!ok) {
        TCC_LOG_ERROR("Ptr %p is not aligned with umm_block size", ptr);
    }
    return ok;
}

// Checks that given block is allocated
static umm_block_status_t umm_check_allocated(bsize_t block, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%d %p", block, allocator);
    bsize_t cur_block_index = UMM_NBLOCK(0, allocator) & UMM_BLOCKNO_MASK;
    do {
        TCC_LOG_TRACE("Checking block %d", cur_block_index);
        if (cur_block_index == block) {
            int is_free = UMM_NBLOCK(cur_block_index, allocator) & UMM_FREELIST_MASK;
            if (is_free) {
                TCC_LOG_DEBUG("Block %d is free", block);
                return UMM_BLOCK_STATUS_FREE;
            }
            TCC_LOG_DEBUG("Block %d is allocated", block);
            return UMM_BLOCK_STATUS_ALLOCATED;
        }
        cur_block_index = UMM_NBLOCK(cur_block_index, allocator) & UMM_BLOCKNO_MASK;
    } while (cur_block_index != 0);

    return UMM_BLOCK_STATUS_BAD_BLOCK;
}

// Checks that given pointer is valid
// (Allocated by umm_malloc() or umm_realloc())
static int umm_check_pointer(void* ptr, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%p %p", ptr, allocator);
    int ok = umm_check_ptr_in_heap(ptr, allocator) && umm_check_ptr_is_block(ptr, allocator) &&
             umm_check_allocated(umm_get_block_by_ptr(ptr, allocator), allocator) == UMM_BLOCK_STATUS_ALLOCATED;
    if (!ok) {
        TCC_LOG_ERROR("Ptr %p is invalid", ptr);
    }
    return ok;
}

int umm_init(umm_malloc_t* allocator, void* memory, size_t size)
{
    TCC_LOG_TRACE("allocator:%p memory:%p size:%zu", allocator, memory, size);

    if (memory == NULL || allocator == NULL) {
        TCC_LOG_ERROR("One or the several parameters are NULL: memory=%p, allocator=%p", memory, allocator);
        return -TCC_E_BAD_PARAM;
    }

    TCC_LOG_DEBUG("umm_malloc initialization");
    TCC_LOG_DEBUG("Buffer size:      %lu", size);
    TCC_LOG_DEBUG("Pointer size:     %lu", sizeof(bsize_t));
    TCC_LOG_DEBUG("Max blocks count: %u", UMM_MAX_BLOCKS);
    TCC_LOG_DEBUG("Max heap size:    %lu", UMM_MAX_HEAP_SIZE);

    if (size > UMM_MAX_HEAP_SIZE) {
        TCC_LOG_ERROR("Required size (%lu) is bigger than maximum available with this pointer size", size);
        return -TCC_E_BAD_PARAM;
    }
    size_t umm_numblocks = (size / sizeof(umm_block));

    if (umm_numblocks < UMM_MIN_BLOCKS) {
        TCC_LOG_ERROR(
            "Minimum buffer size is %d blocks (%lu bytes)", UMM_MIN_BLOCKS, UMM_MIN_BLOCKS * sizeof(umm_block));
        return -TCC_E_BAD_PARAM;
    }

    allocator->umm_numblocks = umm_numblocks;
    allocator->umm_heap = memory;

    /* setup initial blank heap structure */
    {
        /* index of the 0th `umm_block` */
        const bsize_t block_0th = 0;
        /* index of the 1st `umm_block` */
        const bsize_t block_1st = 1;
        /* index of the latest `umm_block` */
        const bsize_t block_last = UMM_NUMBLOCKS(allocator) - 1;
        /* setup the 0th `umm_block`, which just points to the 1st */
        UMM_NBLOCK(block_0th, allocator) = block_1st;
        UMM_NFREE(block_0th, allocator) = block_1st;
        UMM_PFREE(block_0th, allocator) = block_1st;
        /*
         * Now, we need to set the whole heap space as a huge free block. We should
         * not touch the 0th `umm_block`, since it's special: the 0th `umm_block`
         * is the head of the free block list. It's a part of the heap invariant.
         *
         * See the detailed explanation at the beginning of the file.
         */

        /*
         * 1st `umm_block` has pointers:
         *
         * - next `umm_block`: the latest one
         * - prev `umm_block`: the 0th
         *
         * Plus, it's a free `umm_block`, so we need to apply `UMM_FREELIST_MASK`
         *
         * And it's the last free block, so the next free block is 0.
         */
        UMM_NBLOCK(block_1st, allocator) = block_last | UMM_FREELIST_MASK;
        UMM_NFREE(block_1st, allocator) = 0;
        UMM_PBLOCK(block_1st, allocator) = block_0th;
        UMM_PFREE(block_1st, allocator) = block_0th;

        /*
         * latest `umm_block` has pointers:
         *
         * - next `umm_block`: 0 (meaning, there are no more `umm_blocks`)
         * - prev `umm_block`: the 1st
         *
         * It's not a free block, so we don't touch NFREE / PFREE at all.
         */
        UMM_NBLOCK(block_last, allocator) = 0;
        UMM_PBLOCK(block_last, allocator) = block_1st;
    }

    return TCC_E_SUCCESS;
}

/* ------------------------------------------------------------------------ */

void umm_free(void* ptr, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%p,%p", ptr, allocator);

    if (allocator == NULL) {
        TCC_LOG_ERROR("Parameter is NULL: allocator=%p", allocator);
        return;
    }

    if ((void*)0 == ptr) {
        TCC_LOG_DEBUG("Free a null pointer -> do nothing\n");
        return;
    }

    /* Protect the critical section... */
    UMM_CRITICAL_ENTRY();

    if (!umm_check_pointer(ptr, allocator)) {
        TCC_LOG_ERROR("Ptr %p is invalid", ptr);
        return;
    }

    /* Figure out which block we're in */
    bsize_t c = umm_get_block_by_ptr(ptr, allocator);
    TCC_LOG_DEBUG("Freeing block %6i\n", c);

    /* Now let's assimilate this block with the next one if possible. */
    umm_assimilate_up(c, allocator);

    /* Then assimilate with the previous block if possible */
    if (UMM_NBLOCK(UMM_PBLOCK(c, allocator), allocator) & UMM_FREELIST_MASK) {
        TCC_LOG_DEBUG("Assimilate down to next block, which is FREE\n");

        c = umm_assimilate_down(c, UMM_FREELIST_MASK, allocator);
    } else {
        /*
         * The previous block is not a free block, so add this one to the head
         * of the free list
         */

        TCC_LOG_DEBUG("Just add to head of free list\n");

        UMM_PFREE(UMM_NFREE(0, allocator), allocator) = c;
        UMM_NFREE(c, allocator) = UMM_NFREE(0, allocator);
        UMM_PFREE(c, allocator) = 0;
        UMM_NFREE(0, allocator) = c;

        UMM_NBLOCK(c, allocator) |= UMM_FREELIST_MASK;
    }

    /* Release the critical section... */
    UMM_CRITICAL_EXIT();
}

/* ------------------------------------------------------------------------ */

void* umm_malloc(size_t size, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%lu,%p", size, allocator);

    bsize_t blocks;
    bsize_t blockSize = 0;

    bsize_t bestSize;
    bsize_t bestBlock;

    bsize_t cf;

    if (allocator == NULL) {
        TCC_LOG_ERROR("Parameter is NULL: allocator=%p", allocator);
        return ((void*)NULL);
    }
    /*
     * the very first thing we do is figure out if we're being asked to allocate
     * a size of 0 - and if we are we'll simply return a null pointer. If not,
     * reduce the size by 1 byte so that the subsequent calculations on
     * the number of blocks to allocate are easier.
     */

    if (0 == size) {
        TCC_LOG_DEBUG("malloc a block of 0 bytes -> do nothing");
        return ((void*)NULL);
    }

    /* Protect the critical section */
    UMM_CRITICAL_ENTRY();

    blocks = umm_get_number_of_blocks(size);
    TCC_LOG_DEBUG("Allocating %lu bytes", size);
    TCC_LOG_DEBUG("%d blocks required for this allocation", blocks);

    /*
     * Now we can scan through the free list until we find a space that's big
     * enough to hold the number of blocks we need.
     *
     * This part may be customized to be a best-fit, worst-fit, or first-fit
     * algorithm
     */

    cf = UMM_NFREE(0, allocator);

    bestBlock = UMM_NFREE(0, allocator);
    bestSize = UMM_BLOCK_MAX_SIZE;

    while (cf) {
        blockSize = (UMM_NBLOCK(cf, allocator) & UMM_BLOCKNO_MASK) - cf;

        TCC_LOG_TRACE("Looking at block %u size %u", cf, blockSize);

        if ((blockSize >= blocks) && (blockSize < bestSize)) {
            bestBlock = cf;
            bestSize = blockSize;
        }

        cf = UMM_NFREE(cf, allocator);
    }

    if (UMM_BLOCK_MAX_SIZE != bestSize) {
        cf = bestBlock;
        blockSize = bestSize;
    }

    if (UMM_NBLOCK(cf, allocator) & UMM_BLOCKNO_MASK && blockSize >= blocks) {
        /*
         * This is an existing block in the memory heap, we just need to: 
         * 1. Split off what we need 
         * 2. Unlink it from the free list and mark it as in use
         * 3. Link the rest of the block back into the freelist 
         * as if it was a new block on the free list.
         */

        if (blockSize == blocks) {
            /* It's an exact fit and we don't neet to split off a block. */
            TCC_LOG_DEBUG("Allocating %6i blocks starting at %i - exact", blocks, cf);

            /* Disconnect this block from the FREE list */

            umm_disconnect_from_free_list(cf, allocator);
        } else {
            /* It's not an exact fit and we need to split off a block. */
            TCC_LOG_DEBUG("Allocating %6i blocks starting at %i - existing", blocks, cf);

            /*
             * Split current free block `cf` into two blocks. The first one will be
             * returned to user, so it's not free, and the second one will be freed.
             */
            umm_split_block(cf, blocks, UMM_FREELIST_MASK, allocator /*new block is free*/);

            /*
             * `umm_split_block()` does not update the free pointers (it affects
             * only free flags), but effectively we've just moved beginning of the
             * free block from `cf` to `cf + blocks`. So we have to adjust pointers
             * to and from adjacent free blocks.
             */

            /* previous free block */
            UMM_NFREE(UMM_PFREE(cf, allocator), allocator) = cf + blocks;
            UMM_PFREE(cf + blocks, allocator) = UMM_PFREE(cf, allocator);

            /* next free block */
            UMM_PFREE(UMM_NFREE(cf, allocator), allocator) = cf + blocks;
            UMM_NFREE(cf + blocks, allocator) = UMM_NFREE(cf, allocator);
        }
    } else {
        /* Out of memory */
        TCC_LOG_DEBUG("Can't allocate %i blocks", blocks);

        /* Release the critical section... */
        UMM_CRITICAL_EXIT();

        return ((void*)NULL);
    }

    /* Release the critical section... */
    UMM_CRITICAL_EXIT();

    return ((void*)&UMM_DATA(cf, allocator));
}

/* ------------------------------------------------------------------------ */

void* umm_realloc(void* ptr, size_t size, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%p,%lu,%p", ptr, size, allocator);

    bsize_t blocks;
    bsize_t blockSize;
    bsize_t prevBlockSize = 0;
    bsize_t nextBlockSize = 0;

    size_t curSize;

    if (allocator == NULL) {
        TCC_LOG_ERROR("Allocator is NULL");
        return NULL;
    }

    /*
     * This code detects the case of a NULL value for ptr. The ANSI C
     * standard says that if ptr is NULL and size is non-zero, then it should
     * work just like malloc(). If size is also 0, then our version
     * of malloc() returns a NULL pointer, which is OK as far as the ANSI C
     * standard is concerned.
     */

    if (((void*)NULL == ptr)) {
        TCC_LOG_DEBUG("realloc the NULL pointer - call malloc()\n");
        return (umm_malloc(size, allocator));
    }

    /*
     * Now we're sure that we have a non_NULL ptr, but we're not sure what
     * we should do with it. If the size is 0, then the ANSI C standard says that
     * we should operate the same as free.
     */

    if (0 == size) {
        TCC_LOG_DEBUG("realloc to 0 size, just free the block\n");
        umm_free(ptr, allocator);
        return ((void*)NULL);
    }

    if (!umm_check_pointer(ptr, allocator)) {
        TCC_LOG_ERROR("Ptr %p is invalid", ptr);
        return NULL;
    }

    /* Figure out which block we're in */
    bsize_t c = umm_get_block_by_ptr(ptr, allocator);
    TCC_LOG_DEBUG("Freeing block %6i\n", c);

    /*
     * Otherwise we need to actually do a reallocation. A naive approach
     * would be to malloc() a new block of the correct size, copy the old data
     * to the new block, and then free the old block.
     *
     * While this will work, we end up doing a lot of possibly unnecessary
     * copying. So first, let's figure out how many blocks we'll need.
     */

    blocks = umm_get_number_of_blocks(size);

    /* Figure out how big this block is. The free bit is not set. */

    blockSize = (UMM_NBLOCK(c, allocator) - c);

    /* Figure out how many bytes are in this block */

    curSize = (blockSize * sizeof(umm_block)) - (sizeof(((umm_block*)0)->header));

    /* Protect the critical section... */
    UMM_CRITICAL_ENTRY();

    /* Now figure out if the previous and/or next blocks are free as well as
     * their sizes - this will help us to minimize special code later when we
     * decide if it's possible to use the adjacent blocks.
     *
     * We set prevBlockSize and nextBlockSize to non-zero values ONLY if they
     * are free!
     */

    if ((UMM_NBLOCK(UMM_NBLOCK(c, allocator), allocator) & UMM_FREELIST_MASK)) {
        nextBlockSize = (UMM_NBLOCK(UMM_NBLOCK(c, allocator), allocator) & UMM_BLOCKNO_MASK) - UMM_NBLOCK(c, allocator);
    }

    if ((UMM_NBLOCK(UMM_PBLOCK(c, allocator), allocator) & UMM_FREELIST_MASK)) {
        prevBlockSize = (c - UMM_PBLOCK(c, allocator));
    }

    TCC_LOG_DEBUG("realloc blocks %i blockSize %i nextBlockSize %i prevBlockSize %i\n",
        blocks,
        blockSize,
        nextBlockSize,
        prevBlockSize);

    /*
     * Ok, now that we're here we know how many blocks we want and the current
     * blockSize. The prevBlockSize and nextBlockSize are set and we can figure
     * out the best strategy for the new allocation as follows:
     *
     * 1. If the new block is of the same size or smaller than the current block, do
     *    nothing.
     * 2. If the next block is free and adding it to the current block gives us
     *    enough memory, assimilate the next block.
     * 3. If the prev block is free and adding it to the current block gives us
     *    enough memory, remove the previous block from the free list, assimilate
     *    it, copy to the new block.
     * 4. If the prev and next blocks are free and adding them to the current
     *    block gives us enough memory, assimilate the next block, remove the
     *    previous block from the free list, assimilate it, copy to the new block.
     * 5. Otherwise try to allocate an entirely new block of memory. If the
     *    allocation works free the old block and return the new pointer. If
     *    the allocation fails, return NULL and leave the old block intact.
     *
     * All that's left to do is decide if the fit was exact or not. If the fit
     * was not exact, then split the memory block so that we use only the requested
     * number of blocks and add what's left to the free list.
     */

    if (blockSize >= blocks) {
        TCC_LOG_DEBUG("realloc the same or smaller size block - %i, do nothing\n", blocks);
        /* This space intentionally left blank */
    } else if ((blockSize + nextBlockSize) >= blocks) {
        TCC_LOG_DEBUG("realloc using next block - %i\n", blocks);
        umm_assimilate_up(c, allocator);
        blockSize += nextBlockSize;
    } else if ((prevBlockSize + blockSize) >= blocks) {
        TCC_LOG_DEBUG("realloc using prev block - %i\n", blocks);
        umm_disconnect_from_free_list(UMM_PBLOCK(c, allocator), allocator);
        c = umm_assimilate_down(c, 0, allocator);
        memmove((void*)&UMM_DATA(c, allocator), ptr, curSize);
        ptr = (void*)&UMM_DATA(c, allocator);
        blockSize += prevBlockSize;
    } else if ((prevBlockSize + blockSize + nextBlockSize) >= blocks) {
        TCC_LOG_DEBUG("realloc using prev and next block - %i\n", blocks);
        umm_assimilate_up(c, allocator);
        umm_disconnect_from_free_list(UMM_PBLOCK(c, allocator), allocator);
        c = umm_assimilate_down(c, 0, allocator);
        memmove((void*)&UMM_DATA(c, allocator), ptr, curSize);
        ptr = (void*)&UMM_DATA(c, allocator);
        blockSize += (prevBlockSize + nextBlockSize);
    } else {
        TCC_LOG_DEBUG("realloc a completely new block %i\n", blocks);
        void* oldptr = ptr;
        if ((ptr = umm_malloc(size, allocator))) {
            TCC_LOG_DEBUG("realloc %i to a bigger block %i, copy, and free the old\n", blockSize, blocks);
            memcpy(ptr, oldptr, curSize);
            umm_free(oldptr, allocator);
        } else {
            TCC_LOG_DEBUG(
                "realloc %i to a bigger block %i failed - return NULL and leave the old block!\n", blockSize, blocks);
            /* This space intentionally left blnk */
        }
        blockSize = blocks;
    }

    /* Now all we need to do is figure out if the block fit exactly or if we
     * need to split and free ...
     */

    if (blockSize > blocks) {
        TCC_LOG_DEBUG("split and free %i blocks from %i\n", blocks, blockSize);
        umm_split_block(c, blocks, 0, allocator);
        umm_free((void*)&UMM_DATA(c + blocks, allocator), allocator);
    }

    /* Release the critical section... */
    UMM_CRITICAL_EXIT();

    return (ptr);
}

/* ------------------------------------------------------------------------ */

void* umm_calloc(size_t num, size_t size, umm_malloc_t* allocator)
{
    TCC_LOG_TRACE("%lu,%lu,%p", num, size, allocator);

    void* ret;

    if (allocator == NULL) {
        TCC_LOG_ERROR("Parameter is NULL: allocator=%p", allocator);
        return NULL;
    }

    size_t actual_size;
    if (__builtin_umull_overflow(num, size, &actual_size)) {
        TCC_LOG_ERROR("Integer overflow %lu * %lu", num, size);
        return NULL;
    }
    ret = umm_malloc(actual_size, allocator);

    if (ret == NULL) {
        TCC_LOG_ERROR("umm_malloc cannot allocate memory.");
        return ret;
    }

    memset(ret, 0x00, actual_size);

    return ret;
}
