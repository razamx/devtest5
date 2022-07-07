/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#include "pointer_chasing.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define CACHE_LINE_SIZE 64     // Size of a cache line in bytes
#define MINIMAL_NODE_NUMBER 6  // Required for pointer_chase_run_read_write_workload

/**
 * @brief Pointer-chase list node. The size of a node is equal to the size of one cache line.
 *
 */
struct cache_line_node_t
{
    union
    {
        uint8_t buf[CACHE_LINE_SIZE];
        struct
        {
            cache_line_node_t* next;
            cache_line_node_t* prev;
        };
    };
};

static cache_line_node_t* pointer_chasing_init_linear(cache_line_node_t* nodes, size_t n_nodes)
{
    for (size_t i = 0; i < n_nodes; i++) {
        size_t next_id = (n_nodes + i + 1) % n_nodes;
        size_t prev_id = (n_nodes + i - 1) % n_nodes;
        nodes[i].prev = &(nodes[prev_id]);
        nodes[i].next = &(nodes[next_id]);
    }
    return nodes;
}

static inline void pointer_chasing_swap_lines_far(cache_line_node_t* node1, cache_line_node_t* node2)
{
    cache_line_node_t* node1_prev = node1->prev;
    cache_line_node_t* node1_next = node1->next;
    cache_line_node_t* node2_prev = node2->prev;
    cache_line_node_t* node2_next = node2->next;
    node1->prev->next = node2;
    node1->next->prev = node2;
    node2->next->prev = node1;
    node2->prev->next = node1;
    node1->prev = node2_prev;
    node1->next = node2_next;
    node2->next = node1_next;
    node2->prev = node1_prev;
}
static inline void pointer_chasing_swap_lines_near(cache_line_node_t* node1, cache_line_node_t* node2)
{
    cache_line_node_t* node1_prev = node1->prev;
    cache_line_node_t* node2_next = node2->next;
    node1->prev->next = node2;
    node2->next->prev = node1;
    node1->prev = node2;
    node1->next = node2_next;
    node2->next = node1;
    node2->prev = node1_prev;
}

static void pointer_chasing_swap_lines(cache_line_node_t* node1, cache_line_node_t* node2)
{
    if (node1->prev == node2) {
        pointer_chasing_swap_lines_near(node2, node1);
    } else if (node1->next == node2) {
        pointer_chasing_swap_lines_near(node1, node2);
    } else if (node1 != node2) {
        pointer_chasing_swap_lines_far(node1, node2);
    }
}

static inline cache_line_node_t* pointer_chase_run_read_workload_internal(cache_line_node_t* nodes, size_t n_nodes)
{
    while (n_nodes--) {
        nodes = nodes->next;
    }
    return nodes;
}

cache_line_node_t* pointer_chase_randomise(cache_line_node_t* nodes,
    size_t n_nodes,
    pointer_chase_random_generator_t generator)
{
    for (size_t i = 0; i < n_nodes; i++) {
        size_t rand = generator() % n_nodes;
        pointer_chasing_swap_lines(&nodes[i], &nodes[rand]);
    }
    return nodes;
}
cache_line_node_t* pointer_chase_create_linear(void* buffer, size_t size)
{
    size_t n_nodes = size / sizeof(cache_line_node_t);
    if (buffer == NULL || n_nodes < MINIMAL_NODE_NUMBER) {
        errno = EINVAL;
        return NULL;
    }
    return pointer_chasing_init_linear((cache_line_node_t*)buffer, n_nodes);
}

cache_line_node_t* pointer_chase_create_random(void* buffer, size_t size, pointer_chase_random_generator_t generator)
{
    if (generator == NULL) {
        errno = EINVAL;
        return NULL;
    }
    cache_line_node_t* self = pointer_chase_create_linear((cache_line_node_t*)buffer, size);
    if (self == NULL) {
        return NULL;
    }
    return pointer_chase_randomise(self, size / sizeof(cache_line_node_t), generator);
}

__attribute__((optimize("-O0"))) cache_line_node_t* pointer_chase_run_read_workload(cache_line_node_t* nodes,
    size_t n_nodes)
{
    return pointer_chase_run_read_workload_internal(nodes, n_nodes);
}

__attribute__((optimize("-O0"))) cache_line_node_t* pointer_chase_run_workload_read_cyclic(cache_line_node_t* nodes,
    size_t n_cycles)
{
    register cache_line_node_t* start = nodes;
    for (size_t cycle = 0; cycle < n_cycles; cycle++) {
        while (nodes != start) {
            nodes = nodes->next;
        }
    }
    return nodes;
}

cache_line_node_t* pointer_chase_run_read_write_workload(cache_line_node_t* nodes, size_t n_nodes)
{
    while (n_nodes--) {
        cache_line_node_t* node1 = nodes;
        cache_line_node_t* node2 = pointer_chase_run_read_workload_internal(node1, 3);
        nodes = pointer_chase_run_read_workload_internal(node2, 3);
        pointer_chasing_swap_lines_far(node1, node2);
    }
    return nodes;
}