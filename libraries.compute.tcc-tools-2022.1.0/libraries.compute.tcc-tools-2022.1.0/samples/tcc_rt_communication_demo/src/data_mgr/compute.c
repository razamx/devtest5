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
 * @file compute.c
 * @brief Implements a computing network component that handles the input data and produces the output data
 *
 * The computing network component implements a periodic task that reads the input memory buffer,
 * computes the output data based on the input data and publishes the output data to the output
 * memory block. Cache Allocation Library capabilities are used in order to allocate memory for data storage.
 * The input and output memory data format is defined in \c GenericPayload, it includes the
 * following values:
 * \arg sequence_num This represents GenericPayload::sequence_num item. The sequence numbers are
 *                   copied from the input buffer to the output without any update;
 * \arg type This value defines \c GenericPayload::type item, we receive TALKER packets and produce
 *           COMPUTE packets.
 * \arg timestamp This value defines GenericPayload::talker::sec/nsec items and represents
 *                the time when the value is published to the memory buffer, we copy this value
 *                from the input packets to the output packets, so the \c listener on the other
 *                side will be able to calculate the round trip time;
 *
 * The computing network component should be configured before use,
 * see \c PConfig structure for input parameters description
 */

#include "pconfig.h"
#include "pdatasource.h"
#include "ptimer.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define TIMESTAMPS_COUNT (100000)

static bool gStarted = false;
static bool gInitSeqLatched = false;
static uint32_t gPrevSeq = -1;

static PConfig* gConfig = NULL;
static size_t TheTimestampsCount = 0;
static uint32_t TheLatestSequenceNumber = 0;
static uint64_t TheSeqNumbers[TIMESTAMPS_COUNT] = {0};
static uint64_t TheRcvTimestmps[TIMESTAMPS_COUNT] = {0};
static uint64_t TheOrigTimestmps[TIMESTAMPS_COUNT] = {0};

static void show_stats(void);

static void timer_handler(uint64_t timestamp)
{
    static GenericPayload outgoingData = {};
    const GenericPayload* const pIncomingData = (const GenericPayload*)gConfig->incomingDataSource;

    uint32_t sequence = pIncomingData->sequence_num;
    if (!gInitSeqLatched) {
        /* Store the initial sequence */
        gPrevSeq = sequence;
        gInitSeqLatched = true;
        return;
    }
    if (!gStarted && gPrevSeq == sequence) {
        return;
    }

    if (!gStarted) {
        gStarted = true;
    }
    --gConfig->iterations;

    if (sequence == gPrevSeq) {
        /* No received packet */
        if (!gConfig->iterations) {
            /* No more iterations, stop the timer */
            ptimer_stop(gConfig);
        }
        return;
    }
    gPrevSeq = sequence;
    if (gConfig->packetType != pIncomingData->type) {
        fprintf(stderr, "Incoming data wrong type: 0x%x\n", pIncomingData->type);
        if (!gConfig->iterations) {
            /* No more iterations, stop the timer */
            ptimer_stop(gConfig);
        }
        return;
    }

    if (TheTimestampsCount >= TIMESTAMPS_COUNT) {
        fprintf(stderr, "No more statistics items\n");
        return;
    }

    TheLatestSequenceNumber = sequence;
    memset(&outgoingData, 0, sizeof(outgoingData));
    outgoingData.sequence_num = sequence;
    outgoingData.type = COMPUTE;
    outgoingData.load.compute.out_sec = timestamp / UNIT2NANO_FACTOR;
    outgoingData.load.compute.out_nsec = timestamp % UNIT2NANO_FACTOR;
    outgoingData.load.compute.prev_sec = pIncomingData->load.talker.sec;
    outgoingData.load.compute.prev_nsec = pIncomingData->load.talker.nsec;

    put_value_in_datasource(&outgoingData);
    TheSeqNumbers[TheTimestampsCount] = sequence;
    TheOrigTimestmps[TheTimestampsCount] =
        UNIT2NANO_FACTOR * pIncomingData->load.talker.sec + pIncomingData->load.talker.nsec;
    TheRcvTimestmps[TheTimestampsCount++] = timestamp;

    if (TheTimestampsCount >= TIMESTAMPS_COUNT) {
        // Ran out of statistics items, stop the timer
        ptimer_stop(gConfig);
        return;
    }

    if (!gConfig->iterations) {
        /* No more iterations, stop the timer */
        ptimer_stop(gConfig);
    }
}

void create_compute(PConfig* config)
{
    static bool stop_variable = false;
    config->pStopVariable = &stop_variable;
    ptimer_create(config, timer_handler, show_stats);
    gConfig = config;
}

static void show_stats(void)
{
    int64_t value;
    int64_t min, max, average;

    if (!TheTimestampsCount) {
        fprintf(stderr, "No samples collected\n");
        return;
    }

    if (gConfig->verbose) {
        /* Print the overview information */
        printf("Number of samples: %zd, latest sequence number: %u\n", TheTimestampsCount, TheLatestSequenceNumber);
        /* Show the phase information only in "verbose" mode */
        value = *TheRcvTimestmps % 500000;
        min = value;
        max = value;
        average = value;
        for (size_t i = 1; i < TheTimestampsCount; ++i) {
            value = (TheRcvTimestmps[i] % 500000);
            average += value;
            if (min > value) {
                min = value;
            }
            if (max < value) {
                max = value;
            }
        }
        value = average / TheTimestampsCount;
        printf("Phase info: average: %" PRId64 ", min: %" PRId64 ", max: %" PRId64 "\n", value, min, max);
    }

    value = *TheRcvTimestmps - *TheOrigTimestmps;
    average = value;
    min = value;
    max = value;
    for (size_t i = 1; i < TheTimestampsCount; ++i) {
        value = (TheRcvTimestmps[i] - TheOrigTimestmps[i]);
        average += value;
        if (min > value) {
            min = value;
        }
        if (max < value) {
            max = value;
        }
    }
    value = average / TheTimestampsCount;
    printf("Delay average: %" PRId64 ", min: %" PRId64 ", max: %" PRId64 "\n", value, min, max);
}
