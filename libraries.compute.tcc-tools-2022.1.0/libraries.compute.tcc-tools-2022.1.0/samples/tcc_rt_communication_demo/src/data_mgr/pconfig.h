/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_PCONFIG_H_
#define _TCC_PCONFIG_H_

/**
 * @file pconfig.h
 * @brief Defines the API for the sample's configuration and state
 *
 * This file defines the API and data structures for creating producers and consumers.
 */
#include "opcua_shared.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * The application configuration/state structure
 *
 * @brief This structure defines the state and configuration for the data manager component
 */
typedef struct _PConfig
{
    /** Affinity for the timer thread associated with talker/listener/compute components */
    int affinity;
    /** Flag defining whether the verbose console output is enabled */
    bool verbose;
    /** The expected packet type for the Time-Sensitive Networking (TSN) listener */
    uint32_t packetType;
    /** Defines the start time of operations bound to the TAI timeline
        with respect to the start of each period */
    int32_t startTime;
    /** Defines the operations period on the TAI timeline. Each period starts at time,
        such that 'TAI-time % period == 0' */
    uint32_t period;
    /** The requested number of operations */
    uint32_t iterations;
    /** This state field defines the current sequence number used in the current operation */
    uint32_t sequenceNumber;
    /** The allocated file name for storing the output data/statistics */
    char* pLogFile;
    /** Pointer to the incoming data for the listener and compute parts */
    void* incomingDataSource;
    /** Pointer to the stop variable checked by timer functionality */
    bool* pStopVariable;
    /** Requested base time, or \c 0 if not set */
    int64_t baseTime;
} PConfig;

struct GenericPayload;
typedef struct GenericPayload GenericPayload;

/**
 * Create a data producer based on the incoming configuration. The data producer uses
 * the cache allocation library to show the benefits of cache locking.
 * @param[in] config The pointer to the configuration for the data producer.
 * @note Starts a new background detached thread.
 */
void create_producer(PConfig* config);
/**
 * Create a data consumer based on the incoming configuration. The data consumer measures
 * the transmission latency of each packet.
 * @param[in] config The pointer to the configuration for the data consumer.
 * @note Starts a new background detached thread.
 */
void create_consumer(PConfig* config);
/**
 * Create the data compute part based on the incoming configuration. The data compute part uses
 * the cache allocation library to show the benefits of cache locking.
 * @param[in] config  The pointer to the configuration for the data compute part.
 * @note Starts a new background detached thread.
 */
void create_compute(PConfig* config);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif  // _TCC_PCONFIG_H_
