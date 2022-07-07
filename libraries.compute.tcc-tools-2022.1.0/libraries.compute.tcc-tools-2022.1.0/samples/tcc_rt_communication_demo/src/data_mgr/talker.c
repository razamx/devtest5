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
 * @file talker.c
 * @brief Implements simulated sensor application periodically generating and publishing test data to
 *        datasource
 *
 * The emulated sensor application implements a periodic task that publishes test data to the memory
 * buffer. Cache allocation library capabilities are used to allocate memory for data storage.
 * The test data layout is defined in \c GenericPayload and includes the following values:
 * \arg sequence_num This incrementing value represents GenericPayload::sequence_num item;
 * \arg type This value defines \c GenericPayload::type item, only \c TALKER value is used;
 * \arg timestamp This value defines GenericPayload::talker::sec/nsec items and represents
 *                the time when the value is published to the memory buffer.
 *
 * The Talker should be configured before use, see \c PConfig structure for input parameters description.
 */

#include "opcua_shared.h"
#include "pcheck.h"
#include "pconfig.h"
#include "pdatasource.h"
#include "ptimer.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define TIMESTAMPS_COUNT (100000)

static int TheTimestampsCount = 0;
static uint64_t TheTimestamps[TIMESTAMPS_COUNT] = {0};
static PConfig* gConfig = NULL;

static void show_stats(void);

static void timer_handler(uint64_t timestamp)
{
    if (TheTimestampsCount < TIMESTAMPS_COUNT) {
        if (gConfig->iterations) {
            TheTimestamps[TheTimestampsCount++] = timestamp;

            static GenericPayload data = {};
            data.sequence_num = gConfig->sequenceNumber;
            data.type = TALKER;
            data.load.talker.sec = timestamp / UNIT2NANO_FACTOR;
            data.load.talker.nsec = timestamp % UNIT2NANO_FACTOR;

            put_value_in_datasource(&data);

            ++gConfig->sequenceNumber;
            --gConfig->iterations;
        } else {
            ptimer_stop(gConfig);
        }
    } else {
        ptimer_stop(gConfig);
    }
}

void create_producer(PConfig* config)
{
    static bool stop_variable = false;
    config->pStopVariable = &stop_variable;
    ptimer_create(config, timer_handler, show_stats);
    gConfig = config;
}

static void show_stats(void)
{
    if (gConfig->verbose) {
        int64_t value;
        int64_t min, max, average;

        if (!TheTimestampsCount) {
            fprintf(stderr, "No samples collected\n");
            return;
        }

        value = *TheTimestamps % 500000;
        min = value;
        max = value;
        average = value;
        for (int i = 1; i < TheTimestampsCount; ++i) {
            value = (TheTimestamps[i] % 500000);
            average += value;
            if (min > value) {
                min = value;
            }
            if (max < value) {
                max = value;
            }
        }

        value = average / TheTimestampsCount;
        printf("Phase info: Average: %" PRId64 ", min: %" PRId64 ", max: %" PRId64 "\n", value, min, max);
    }
}
