/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_PTIMER_H_
#define _TCC_PTIMER_H_

/**
 * @file ptimer.h
 * @brief This header file defines the API for a periodic real-time (RT) timer
 *
 * The API to the timer defined in this file can periodically
 * invoke the user callback function. The period is aligned with the system TAI clock.
 * The client can create a timer using timer callbacks, the start time for the timer, and the
 * required period in the call \c ptimer_create. A thread with a Real-Time scheduling policy
 * is created for the timer, and the callback is invoked in the context of this thread.
 * A finalize callback is invoked before exiting from the timer thread.
 * The timer thread currently supports only 1 client.
 */

#include "pconfig.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Default alignment value for the base time */
#define BASE_TIME_ALIGNMENT_DEFAULT 5
/** Microunit-to-nanounit conversion factor */
#define USEC2NANOSEC_FACTOR UINT32_C(1000)
/** Unit-to-nanounit conversion factor */
#define UNIT2NANO_FACTOR INT64_C(1000000000)

/**
 * The periodic task handler prototype
 * @param[in] timestamp The current time in nanoseconds
 */
typedef void (*ptimer_handler)(uint64_t timestamp);
/**
 * The finalize task handler prototype
 */
typedef void (*pfini_handler)(void);

/**
 * Create and start the timer
 *
 * This call creates the timer thread and will start invoking the timer callback periodically
 * on the context of this new thread.
 * @param[in] config         Pointer to PConfig structure to be used for timer creation
 * @param[in] pTimerHandler  Pointer to function to be invoked on each timer tick
 * @param[in] pFiniHandler   Pointer to function to be invoked once before termination of timer's execution
 */
void ptimer_create(PConfig* config, ptimer_handler pTimerHandler, pfini_handler pFiniHandler);

/**
 ** Stop the timer
 *
 * This call stops the timer thread.
 * @param[in] config Pointer to PConfig structure to be used for termination of timer's task
 */
void ptimer_stop(PConfig* config);

/**
 * Get the base time
 *
 * @param[in] baseTime The configuration value for the base time or \c 0 if not defined
 * @param[in] timeNow The current time in UTC
 * @return The calculated base time in UTC
 */
uint64_t ptimer_base_time(uint64_t baseTime, uint64_t timeNow);

#ifdef __cplusplus
} /* extern "C" { */
#endif /* __cplusplus */

#endif  // _TCC_PTIMER_H_
