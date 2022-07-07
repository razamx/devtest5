/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TGPIO_SAMPLE_H_
#define _TCC_TGPIO_SAMPLE_H_

#include "tgpio_defs.h"
#include <stdint.h>
#include <stdlib.h>

#define CPUID 3
#define UNUSED(x) ((void)(x))
#define MAX_DEVICE_NAME_LEN 255
#define STRINGIFY(x) PRIMITIVE_STRINGIFY(x)
#define PRIMITIVE_STRINGIFY(x) #x

typedef enum
{
    MODE_UNKNOWN,
    MODE_SW_GPIO,
    MODE_TGPIO
} Mode;

typedef enum
{
    SCENARIO_UNKNOWN,
    SCENARIO_INPUT,
    SCENARIO_OUTPUT
} Scenario;

/* Helper struct for SW GPIO output scenario */
typedef struct
{
    int fd;
    uint64_t next_edge_time;
    uint8_t current_value;
} sw_gpio_output;

/* Helper struct for TGPIO output scenario */
typedef struct
{
    char* device;
    int fd;
    struct ptp_sys_offset_precise offset_prev;
    struct ptp_sys_offset_precise offset_cur;
} tgpio_output;

/* Pin configuration for the output period mode */
typedef struct
{
    uint32_t pin;
    uint64_t offset;
    uint64_t period;
    uint32_t channel;
    char device[MAX_DEVICE_NAME_LEN + 1];
} period_pin_config;

/* Command-line arguments */
struct settings_t
{
    char* signal_file;
    int scenario;
    int mode;
};

#endif  // _TCC_TGPIO_SAMPLE_H_
