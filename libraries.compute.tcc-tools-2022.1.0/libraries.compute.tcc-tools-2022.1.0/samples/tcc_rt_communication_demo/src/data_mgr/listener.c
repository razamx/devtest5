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
 * @file listener.c
 * @brief Implements simulated actuator collecting and analyzing the input data read from the datasource
 *
 * The listener implements a periodic task that reads the test data in the incoming
 * memory buffer and collects statistics about updates. The test data layout is defined in
 * \c GenericPayload and includes the following values:
 * \arg sequence_num This incrementing value represents GenericPayload::sequence_num item;
 * \arg type This value defines \c GenericPayload::type item, \c TALKER or \c COMPUTE values
 *           are supported;
 * \arg timestamp This value defines GenericPayload::talker::sec/nsec or
 *                GenericPayload::compute::prev_sec/prev_nsec items and represents
 *                the time when the value is published to the memory buffer on the talker side.
 *
 * The listener should be configured before use, see \c PConfig structure for input parameters description
 */

#include "pcheck.h"
#include "pconfig.h"
#include "ptimer.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_SAMPLES_COUNT (2000)

static uint32_t TheSeq[MAX_SAMPLES_COUNT];
static uint64_t TheStatsOrig[MAX_SAMPLES_COUNT];
static uint64_t TheStatsNow[MAX_SAMPLES_COUNT];
static uint64_t TheStatsMiddle[MAX_SAMPLES_COUNT];
static uint32_t TheStatsCount = 0;

static uint32_t TheLastSequence = 0;

static bool gStarted = false;
static bool gInitSeqLatched = false;
static uint32_t gPrevSeq = -1;
static PConfig* gConfig = NULL;

static void show_stats(void);

static void timer_handler(uint64_t timestamp)
{
    const GenericPayload* const pData = (const GenericPayload*)gConfig->incomingDataSource;

    uint32_t sequence = pData->sequence_num;
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

    if ((gPrevSeq > 0) && (sequence == 0)) {
        if (gConfig->verbose) {
            fprintf(stderr, "Unexpected incoming sequence number %d\n", sequence);
        }
    }

    if (sequence == gPrevSeq) {
        /* No received packet */
        if (!gConfig->iterations) {
            /* No more iterations, stop the timer */
            ptimer_stop(gConfig);
        }
        return;
    }
    gPrevSeq = sequence;

    if (gConfig->packetType != pData->type) {
        if (gConfig->verbose) {
            fprintf(stderr, "Wrong packet type: 0x%x\n", pData->type);
        }
        if (!gConfig->iterations) {
            /* No more iterations, stop the timer */
            ptimer_stop(gConfig);
        }
        return;
    }

    TheLastSequence = sequence;
    TheSeq[TheStatsCount] = sequence;
    TheStatsNow[TheStatsCount] = timestamp;
    if (TALKER == gConfig->packetType) {
        TheStatsOrig[TheStatsCount] = UNIT2NANO_FACTOR * pData->load.talker.sec + pData->load.talker.nsec;
    } else {
        check(COMPUTE == gConfig->packetType, "Unknown packet type");
        TheStatsMiddle[TheStatsCount] = UNIT2NANO_FACTOR * pData->load.compute.out_sec + pData->load.compute.out_nsec;
        TheStatsOrig[TheStatsCount] = UNIT2NANO_FACTOR * pData->load.compute.prev_sec + pData->load.compute.prev_nsec;
    }
    ++TheStatsCount;
    check(TheStatsCount <= MAX_SAMPLES_COUNT, "Too many iterations");
    /* No more iterations, stop the timer */
    if (!gConfig->iterations) {
        ptimer_stop(gConfig);
    }
}

void create_consumer(PConfig* config)
{
    static bool stop_variable = false;
    config->pStopVariable = &stop_variable;
    ptimer_create(config, timer_handler, show_stats);
    gConfig = config;
}

static void show_stats(void)
{
    FILE* pFile = NULL;
    int64_t value;
    int64_t min, max, average;

    if (!TheStatsCount) {
        printf("\n*** Statistics for packets transmission ****************\n");
        printf("    No samples collected\n");
        printf("********************************************************\n\n");
        return;
    }

    if (gConfig->pLogFile) {
        pFile = fopen(gConfig->pLogFile, "w+");
        check(pFile, "Failed opening the log file");
    }

    average = 0;
    min = INT64_MAX;
    max = INT64_MIN;
    for (uint32_t i = 0; i < TheStatsCount; ++i) {
        value = TheStatsNow[i] - TheStatsOrig[i];

        if (pFile) {
            fprintf(pFile, "%" PRIu64 "\n", value);
        }

        average += value;
        if (min > value) {
            min = value;
        }
        if (max < value) {
            max = value;
        }
    }
    average /= TheStatsCount;

    if (pFile) {
        fclose(pFile);
    }

    printf("\n*** Statistics for packets transmission ****************\n");
    printf("    Received %" PRIu32 " packets, the last sequence number: %" PRIu32 "\n", TheStatsCount, TheLastSequence);
    printf("    Minimum packet latency: %" PRId64 " us\n", min / USEC2NANOSEC_FACTOR);
    printf("    Maximum packet latency: %" PRId64 " us\n", max / USEC2NANOSEC_FACTOR);
    printf("    Average packet latency: %" PRId64 " us\n", average / USEC2NANOSEC_FACTOR);
    printf("********************************************************\n\n");
}
