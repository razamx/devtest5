/* This sample is a simple self-contained example program demonstrating how to get a report of
available Time-Aware GPIO (TGPIO) capabilities for a Precision Time Protocol (PTP) device.*/

/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.
*******************************************************************************/
#pragma GCC diagnostic ignored "-Wunused-result"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "tcc/err_code.h"
#include "macros.h"
#include "tgpio.h"
#include "args_parse.h"

#define TGPIO_DEVICE "/dev/ptp0"

/* Short option equivalents */
#define COLUMNS_NUM 4
#define NAME_LEN 26
#define INDEX_LEN 12
#define FUNC_LEN 16
#define CHAN_LEN 12

#define YELLOW_COLOR "\033[33m"
#define DEFAULT_COLOR "\033[39m"

struct settings_t
{
    const char* device;
};

void print_table_header(const char* col_1, const char* col_2, const char* col_3, const char* col_4);
void print_table_row(const char* col_1, const unsigned col_2_value, const char* col_3, const unsigned col_4_value);

static const char* functions[] = {"None", "External timestamp", "Periodic signal"};


int main(int argc, char** argv)
{
    settings_t sample_setting = {
        .device = TGPIO_DEVICE,
    };

    struct ptp_clock_caps caps = {0};
    struct ptp_pin_desc desc = {0};
    int n_pins = 0;

    TCC_ARGS_PARSE_STATUS tcc_parse_status = parse_args(argc, argv, &sample_setting);
    if (tcc_parse_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    } else if (tcc_parse_status == TCC_PARSE_ERROR) {
        pr_err("Cannot recognize input option\n");
        print_usage();
        return -TCC_E_BAD_PARAM;
    }

    int fd = open(sample_setting.device, O_RDWR);
    if (fd < 0) {
        pr_err("Unable to open device %s: %s\n", sample_setting.device, strerror(errno));
        return -TCC_E_ERROR;
    }

    ioctl(fd, PTP_CLOCK_GETCAPS2, &caps); /* Get capabilities */

    printf("Available capabilities:\n"
           "  Pins: %d\n"
           "  Input function  - External timestamp channels: %d\n"
           "  Output function - Programmable periodic signals: %d\n"
           "  Precise system-device cross-timestamps: %s\n",
        caps.n_pins,
        caps.n_ext_ts,
        caps.n_per_out,
        caps.cross_timestamping ? "Supported" : "Not supported");

    n_pins = caps.n_pins;
    if (n_pins > 0) {
        printf("=======================================================\n");
        printf("Available TGPIO pins:\n");
        print_table_header("Pin Name", "Index", "Used Function", "Channel");
        for (int i = 0; i < n_pins; i++) {
            desc.index = (unsigned)i;
            ioctl(fd, PTP_PIN_GETFUNC2, &desc); /* Get current settings of pin i */
            print_table_row(desc.name, desc.index, functions[desc.func], desc.chan);
        }
    }
    close(fd);
    return TCC_E_SUCCESS;
}

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* setting)
{
    static const struct option opts[] = {
        {"help", no_argument, NULL, 'h'}, {"device", required_argument, NULL, 'd'}, {NULL, 0, NULL, 0}};

    int index = 0;
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "h", opts, &index);
        switch (opt) {
        case 'd':
            setting->device = optarg;
            break;
        case 'h':
            return TCC_PRINT_HELP;
        case -1:
            break;
        default:
            return TCC_PARSE_ERROR;
        }
    }
    return TCC_PARSE_SUCCESS;
}

void print_usage()
{
    printf("Usage: tcc_tgpio_info [--device <device_name>]\n"
           "Options:\n"
           "    --device   Specify the TGPIO device. Default: \"/dev/ptp0\".\n");
}

static void print_table_separator_section(const unsigned num)
{
    for (unsigned i = 0; i < num; i++) {
        printf("-");
    }
    printf("+");
}

static void print_table_separator(const char* color)
{
    printf("%s", color);
    printf("+");
    print_table_separator_section(NAME_LEN);
    print_table_separator_section(INDEX_LEN);
    print_table_separator_section(FUNC_LEN);
    print_table_separator_section(CHAN_LEN);
    printf("\n");
    printf(DEFAULT_COLOR);
}

void print_table_row_impl(const char* col_1, const char* col_2, const char* col_3, const char* col_4, const char* color)
{
    printf("%s", color);
    printf("|%*s%*s|%*s%*s|%*s%*s|%*s%*s|\n",
        NAME_LEN / 2 + (int)(strlen(col_1) / 2),
        col_1,
        NAME_LEN / 2 - (int)(strlen(col_1) / 2),
        "",

        INDEX_LEN / 2 + (int)(strlen(col_2) / 2),
        col_2,
        INDEX_LEN / 2 - (int)(strlen(col_2) / 2),
        "",

        FUNC_LEN / 2 + (int)(strlen(col_3) / 2),
        col_3,
        FUNC_LEN / 2 - (int)(strlen(col_3) / 2),
        "",

        CHAN_LEN / 2 + (int)(strlen(col_4) / 2),
        col_4,
        CHAN_LEN / 2 - (int)(strlen(col_4) / 2),
        "");
    printf(DEFAULT_COLOR);
}

void print_table_header(const char* col_1, const char* col_2, const char* col_3, const char* col_4)
{
    print_table_separator(YELLOW_COLOR);
    print_table_row_impl(col_1, col_2, col_3, col_4, YELLOW_COLOR);
    print_table_separator(YELLOW_COLOR);
}

void print_table_row(const char* col_1, const unsigned col_2_value, const char* col_3, const unsigned col_4_value)
{
    char col_2[snprintf(NULL, 0, "%u", col_2_value) + 1];
    char col_4[snprintf(NULL, 0, "%u", col_4_value) + 1];

    snprintf(col_2, sizeof(col_2), "%u", col_2_value);
    snprintf(col_4, sizeof(col_4), "%u", col_4_value);

    print_table_row_impl(col_1, col_2, col_3, col_4, DEFAULT_COLOR);
    print_table_separator(DEFAULT_COLOR);
}
