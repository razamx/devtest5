/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#define _GNU_SOURCE
#include "pointer_chasing_workload.h"

#include "pointer_chasing.h"
#include "macros.h"

#include <errno.h>
#include <sched.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <tcc/cache.h>

#define PAGE_SIZE (4096)
#define INTERNAL_STRESS_PRIO (0)
#define WORKLOAD_RANDOM_SEED (42)
#define INTERNAL_STRESS_AFFINITY (1)
#define INTERNAL_STRESS_NODES (12 * 1024 * 1024 / sizeof(cache_line_node_t))

#define WORKLOAD_BUFFER_SIZE (127UL * 1024)

static pthread_t gStressThread;
static bool gBackgroundStressReq = false;
static cache_line_node_t gStressBuffer[INTERNAL_STRESS_NODES] __attribute__((aligned(PAGE_SIZE)));
static cache_line_node_t* gWorkloadBuffer = NULL;

static void* background_stress_thread(void* args);

/* Public functions */
int pc_init(uint64_t latency, int affinity)
{
    int res;

    /* Check if the pointer chasing support already initialized */
    if (gWorkloadBuffer) {
        pr_err("Error: the Poitner Chasing support already initialized\n");
        return -EBUSY;
    }

    /* Initialize the workload buffer for the requested CPU core */
    res = tcc_cache_init(affinity);
    PRINT_ERROR(res);
    if (res < 0) {
        tcc_cache_finish();
        return res;
    }

    /* Allocate the workload buffer */
    gWorkloadBuffer = (cache_line_node_t*)tcc_cache_malloc(WORKLOAD_BUFFER_SIZE, latency);
    if (!gWorkloadBuffer) {
        pr_err("Error: cannot allocate buffer for the pointer chasing workload\n");
        return -ENOMEM;
    }

    /* Initialize the Random Pointer Chasing nodes in the workload buffer */
    srandom(WORKLOAD_RANDOM_SEED);
    pointer_chase_create_random(gWorkloadBuffer, WORKLOAD_BUFFER_SIZE, random);

    /* Create the background stress thread */
    gBackgroundStressReq = true;
    res = pthread_create(&gStressThread, NULL, background_stress_thread, NULL);
    if (res) {
        /* Reset the background stress request */
        gBackgroundStressReq = false;
        /* Release the previously allocated workload buffer */
        tcc_cache_free(gWorkloadBuffer);
        gWorkloadBuffer = NULL;
        pr_err("Error: cannot create the background stress thread: res: %d, errno: %d\n", res, errno);
        return res;
    }

    return 0;
}

void pc_fini(void)
{
    int res = 0;

    /* Release the workload buffer first */
    if (gWorkloadBuffer) {
        tcc_cache_free(gWorkloadBuffer);
        gWorkloadBuffer = NULL;
    }

    /* Finish the background stress thread */
    if (gBackgroundStressReq) {
        gBackgroundStressReq = false;
        res = pthread_join(gStressThread, NULL);
        if (res) {
            pr_err("Warning: cannot join the stress thread: res: %d, errno: %d\n", res, errno);
        }
    }
}

cache_line_node_t* pc_get_workload_buffer(size_t* size)
{
    if (size) {
        *size = (gWorkloadBuffer ? WORKLOAD_BUFFER_SIZE : 0);
    }

    return gWorkloadBuffer;
}

#pragma GCC push_options
#pragma GCC optimize("O0")
static void* background_stress_thread(void* args)
{
    int res;
    int sched_policy;
    cpu_set_t cpu_set;
    struct sched_param sp = {0};

    /* Set the CPU affinity for the stress thread */
    CPU_ZERO(&cpu_set);
    CPU_SET(INTERNAL_STRESS_AFFINITY, &cpu_set);
    res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
    if (res) {
        pr_err("Warning: stress thread: cannot set CPU affinity: res: %d, errno: %d\n", res, errno);
    }

    /* Update the stress thread scheduling policy */
    res = pthread_getschedparam(pthread_self(), &sched_policy, &sp);
    if (!res) {
        sp.sched_priority = INTERNAL_STRESS_PRIO;
        res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &sp);
        if (res) {
            pr_err("Warning: stress thread: cannot set thread params: res: %d, errno: %d\n", res, errno);
        }
    } else {
        pr_err("Warning: stress thread: cannot get thread params: res: %d, errno: %d\n", res, errno);
    }

    pointer_chase_create_linear(gStressBuffer, sizeof(gStressBuffer));
    while (gBackgroundStressReq) {
        /* Additionally store the returned pointer in 'args',
           this is done just to make it more difficult for the compiler to compile
           this block of code out, it is not used in other resprcts */
        args = pointer_chase_run_read_workload(gStressBuffer, INTERNAL_STRESS_NODES);
    }

    return args;
}
#pragma GCC pop_options
