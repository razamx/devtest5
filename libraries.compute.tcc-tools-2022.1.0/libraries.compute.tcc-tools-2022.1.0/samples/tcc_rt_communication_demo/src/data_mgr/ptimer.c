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
 * @file ptimer.c
 * @brief This file contains implementation of periodic real-time (RT) Timer API
 *
 * The API to the periodic timer provides the functionality to periodically invoke
 * the user callback function. The period is aligned with the system TAI clock.
 * The client can create the timer using timer callback(s), start time for the timer and
 * required period in the call \c ptimer_create. A thread with a Real-Time scheduling policy
 * is created for the timer, and the callback is invoked in the context of this thread.
 * Finalize callback is invoked before exit from timer thread.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* _GNU_SOURCE */
#include "ptimer.h"

#include "pcheck.h"

#include <inttypes.h>
#include <pthread.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLOCK_ID CLOCK_TAI

typedef struct _TimerItem
{
    int affinity;
    uint32_t period;
    int32_t startTime;
    ptimer_handler pTimerHandler;
    pfini_handler pFiniHandler;
    bool* pStopVariable;
    int64_t baseTime;
} TimerItem;

#ifdef DATA_MGR_TIMER_RT_THREAD
static void ptimer_setup_thread(int affinity);
#endif /* DATA_MGR_TIMER_RT_THREAD */
static void* ptimer_thread(void* argument);
static void ptimer_start(TimerItem* TimerConfig);

static uint32_t gTimerCount = 0;
static pthread_rwlock_t gTimerCountLock = PTHREAD_RWLOCK_INITIALIZER;

static uint32_t get_timers_count(void)
{
    uint32_t value = 0;
    pthread_rwlock_rdlock(&gTimerCountLock);
    value = gTimerCount;
    pthread_rwlock_unlock(&gTimerCountLock);

    return value;
}

static void dec_timers_count(void)
{
    pthread_rwlock_wrlock(&gTimerCountLock);
    gTimerCount--;
    pthread_rwlock_unlock(&gTimerCountLock);
}

static void inc_timers_count(void)
{
    pthread_rwlock_wrlock(&gTimerCountLock);
    gTimerCount++;
    pthread_rwlock_unlock(&gTimerCountLock);
}

void ptimer_create(PConfig* config, ptimer_handler pTimerHandler, pfini_handler pFiniHandler)
{
    TimerItem* TimerConfig = NULL;

    check(config, "Wrong 'config' argument");
    check(pTimerHandler, "Wrong 'pTimerHandler' argument");
    check(config->period > 0, "Wrong 'period' argument: %" PRIu32, config->period);
    check(config->pStopVariable, "Wrong 'pStopVariable' argument");

    TimerConfig = (TimerItem*)calloc(1, sizeof(TimerItem));
    check(TimerConfig != NULL, "Cannot allocate memory for timer configuration");
    TimerConfig->affinity = config->affinity;
    TimerConfig->period = config->period;
    TimerConfig->pTimerHandler = pTimerHandler;
    TimerConfig->pFiniHandler = pFiniHandler;
    TimerConfig->startTime = config->startTime;
    TimerConfig->pStopVariable = config->pStopVariable;
    TimerConfig->baseTime = config->baseTime;

    ptimer_start(TimerConfig);
}

void ptimer_stop(PConfig* config)
{
    *config->pStopVariable = true;
}

uint64_t ptimer_base_time(uint64_t baseTime, uint64_t timeNow)
{
    /* Check if the base time exists in the application config structure */
    if (baseTime) {
        int64_t timeDifference;

        /* If the base time is defined, evaluate its value */
        timeDifference = baseTime - timeNow;
        /* Check that the time difference is more than 1 second and less than 300 seconds */
        check(timeDifference >= 1 && timeDifference <= 300,
            "Provided wrong base time: %" PRIi64 ", time now: %" PRIi64 ", difference: %" PRIi64,
            baseTime,
            timeNow,
            timeDifference);
        return baseTime;
    } else {
        return ((timeNow + BASE_TIME_ALIGNMENT_DEFAULT - 1) / BASE_TIME_ALIGNMENT_DEFAULT) *
                   BASE_TIME_ALIGNMENT_DEFAULT +
               3;
    }
}

static void* ptimer_thread(void* argument)
{
    TimerItem* TimerConfig = NULL;
    int res;
    time_t local_next;
    uint64_t timestamp;
    struct tm local_now;
    struct tm local_time;
    uint64_t next_timestamp;
    struct timespec time_now;
    struct timespec next_time;

    check(argument, "Bad incoming 'argument'");
    TimerConfig = (TimerItem*)argument;

#ifdef DATA_MGR_TIMER_RT_THREAD
    /* Set up the affinity and priority of the timer thread */
    ptimer_setup_thread(TimerConfig->affinity);
#endif /* DATA_MGR_TIMER_RT_THREAD */

    /* Get the current time */
    res = clock_gettime(CLOCK_ID, &time_now);
    check(!res, "Failed getting the time");

    next_timestamp = ptimer_base_time(TimerConfig->baseTime, time_now.tv_sec);

    /* Show the next scheduled time and the current time */
    local_next = next_timestamp;
    localtime_r(&local_next, &local_time);
    localtime_r(&time_now.tv_sec, &local_now);
    printf("[INFO] Selected base time: %" PRIu64 " (%d:%d:%d), time now: %" PRIu64 " (%d:%d:%d)\n",
        next_timestamp,
        local_time.tm_hour,
        local_time.tm_min,
        local_time.tm_sec,
        (uint64_t)time_now.tv_sec,
        local_now.tm_hour,
        local_now.tm_min,
        local_now.tm_sec);

    next_timestamp *= UNIT2NANO_FACTOR;
    next_timestamp += TimerConfig->startTime;
    next_time.tv_sec = next_timestamp / UNIT2NANO_FACTOR;
    next_time.tv_nsec = next_timestamp % UNIT2NANO_FACTOR;
    check(next_time.tv_nsec >= 0 && next_time.tv_nsec < (suseconds_t)UNIT2NANO_FACTOR,
        "Wrong start time for the current timer");

    while (*(TimerConfig->pStopVariable) == false) {
        /* Sleep until the next time deadline */
        res = clock_nanosleep(CLOCK_ID, TIMER_ABSTIME, &next_time, NULL);
        check(!res, "Failed: clock_nanosleep");

        /* Get the system clock */
        res = clock_gettime(CLOCK_ID, &time_now);
        check(!res, "Failed getting the time");
        timestamp = UNIT2NANO_FACTOR * time_now.tv_sec + time_now.tv_nsec;

        /* Invoke the handler. Before invoking, we must minimize the CPU usage to
           pass accurate timestamp to the timer client */
        (*TimerConfig->pTimerHandler)(timestamp);

        /* Now, we are free to do a little bit more,
           but we should still  finish before the next deadline */
        /* Get the next deadline */
        next_time.tv_nsec += TimerConfig->period;

        /* Prepare the next expiration time */
        if (next_time.tv_nsec >= UNIT2NANO_FACTOR) {
            next_time.tv_nsec -= UNIT2NANO_FACTOR;
            ++next_time.tv_sec;
        }
    }

    if (TimerConfig->pFiniHandler) {
        (*TimerConfig->pFiniHandler)();
    }

    free(argument);

    dec_timers_count();
    if (get_timers_count() == 0) {
        kill(getpid(), SIGTERM);
    }

    return NULL;
}

#ifdef DATA_MGR_TIMER_RT_THREAD
static void ptimer_setup_thread(int affinity)
{
    int res;
    cpu_set_t cpuset;
    struct sched_param param;

    /* Run our code on CPU Core#3 (idx starts from 0)*/
    CPU_ZERO(&cpuset);
    CPU_SET(affinity, &cpuset);
    res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    check(!res, "Failed setting timer thread affinity");

    /* Set the priority for our thread */
    memset(&param, 0, sizeof(param));
    param.sched_priority = 90;
    res = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
    check(!res, "Failed setting timer thread priority");
}
#endif /* DATA_MGR_TIMER_RT_THREAD */

static void ptimer_start(TimerItem* TimerConfig)
{
    int res;
    pthread_t ThePTimerThreadId = 0;

    res = pthread_create(&ThePTimerThreadId, NULL, ptimer_thread, (void*)TimerConfig);
    check(!res, "Failed creating PTimer thread");
    check(ThePTimerThreadId, "No thread ID for the created timer thread");
    res = pthread_detach(ThePTimerThreadId);
    check(!res, "Failed to detach PTimer thread");
    inc_timers_count();
}
