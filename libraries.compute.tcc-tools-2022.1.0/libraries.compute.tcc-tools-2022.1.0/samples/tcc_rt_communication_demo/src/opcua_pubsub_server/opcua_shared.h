/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_OPCUA_SHARED_H_
#define _TCC_OPCUA_SHARED_H_

#include <stdint.h>

/**
 * @file opcua_shared.h
 * @brief This file defines the structures and other elements shared between
 *        the OPC UA* server and worker applications
 */

/**
 * @brief The enumerator defining the supported packet types in network communications
 */
enum payload_t
{
    TALKER = 0xaabbccdd, /**< The \c TALKER packet type usually comes from \c tsn-talker */
    COMPUTE = 0xeeff0011 /**< The \c COMPUTE packet type usually comes from \c tsn-compute */
};

/**
 * @brief Structure representing the network payload specific to the \c TALKER packet type
 */
struct TalkerPayload
{
    int32_t sec;  /**< The seconds part of the instant when the packet is generated */
    int32_t nsec; /**< The nanoseconds part of the instant when the packet is generated */
};

/**
 * @brief Structure representing the network payload specific to the \c COMPUTE packet type
 */
struct ComputePayload
{
    int32_t lost_packets_num; /**< The number of lost packets */
    int32_t prev_sec;         /**< The original timestamp, seconds part */
    int32_t prev_nsec;        /**< The original timestamp, nanoseconds part */
    int32_t out_sec;          /**< The sending timestamp, seconds part */
    int32_t out_nsec;         /**< The sending timestamp, nanoseconds part*/
};

/**
 * @brief Structure representing both \c TALKER and \c COMPUTE packets while sent over the network
 */
struct GenericPayload
{
    int32_t sequence_num; /**< The sequence number sent in the packet */
    enum payload_t type;  /**< The payload type: \c TALKER or \c COMPUTE */
    union
    {
        struct TalkerPayload talker;   /**< The \c TALKER specific payload */
        struct ComputePayload compute; /**< The \c COMPUTE specific payload */
    } load;
};

#endif  // _TCC_OPCUA_SHARED_H_
