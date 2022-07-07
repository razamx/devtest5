/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include <errno.h>
#include <getopt.h>
#include <ittnotify/ittnotify.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include "args_parse.h"
#include "macros.h"
#include "tcc/measurement.h"
#include "tcc/measurement_helpers.h"

#define TCC_NSEC_PER_USEC (1000U)

struct settings_t
{
    unsigned approximation;
    unsigned iterations;
    unsigned deadline;
    unsigned emulate_outliers;
    TCC_TIME_UNIT time_unit;
};

void print_params(settings_t sample_settings);
void run_measurement_cycle(__itt_domain* domain, __itt_string_handle* measurement, settings_t sample_settings);
void notify_deadline(struct tcc_measurement* measurement, uint64_t cycles);
double recursive_two_over_pi_approx(unsigned int n);
double iterative_two_over_pi_approx(unsigned int n);

static const char collector_library_var_name[] = "INTEL_LIBITTNOTIFY64";
static const char tcc_collector_name[] = "libtcc_collector.so";
static const char measurement_name[] = "Approximation";
static const char domain_name[] = "TCC";
static size_t deadline_counter = 0;
static size_t amount_of_outliers = 20;


int main(int argc, char** argv)
{
    /* Initialize the ITT task handle to collect performance data for the sample workload */
    __itt_domain* domain = __itt_domain_create(domain_name);
    __itt_string_handle* measurement = __itt_string_handle_create(measurement_name);
    if (!domain || !measurement) {
        printf("Unable to create ITT handles\n");
        return -TCC_E_NOT_AVAILABLE;
    }
    struct tcc_measurement* tcc_measurement_ptr = NULL;

    struct settings_t sample_settings = {
        .approximation = 0, .iterations = 1000, .deadline = 0, .emulate_outliers = 0, .time_unit = TCC_TU_NS};

    int tcc_status = parse_args(argc, argv, &sample_settings);
    if (tcc_status == TCC_PARSE_ERROR) {
        print_usage();
        return -TCC_E_BAD_PARAM;
    }
    if (tcc_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    }

    /* Set up ittnotify environment variable to allow latency measurement */
    setenv_if_not_set(collector_library_var_name, tcc_collector_name);

    /* Retrieves the measurement structure by ITT domain and task name. */
    if ((tcc_status = tcc_measurement_get(domain, measurement, &tcc_measurement_ptr)) != TCC_E_SUCCESS) {
        printf("Unable to get access to measurement structure\n");
        return -TCC_E_NOT_AVAILABLE;
    }
    /* Sets a deadline value and callback for the measurement structure. */
    unsigned deadline_in_clocks =
        tcc_measurement_convert_time_units_to_clock(sample_settings.deadline, sample_settings.time_unit);
    if ((tcc_status = tcc_measurement_set_deadline(tcc_measurement_ptr, deadline_in_clocks, notify_deadline)) !=
        TCC_E_SUCCESS) {
        printf("Unable to set up deadline: %s\n", get_error_description(tcc_status));
    }
    print_params(sample_settings);
    run_measurement_cycle(domain, measurement, sample_settings);
    tcc_measurement_print(tcc_measurement_ptr, sample_settings.time_unit);
    if (sample_settings.deadline) {
        printf("Number of exceeding deadlines: %zu of %u\n", deadline_counter, sample_settings.iterations);
    }

    /* Application exits too fast. We use sleep here for the shared memory reader to read measurement results */
    sleep(1);
    return 0;
}

/* Measure latency of calculating the Nth approximation of 2/pi. */
void run_measurement_cycle(__itt_domain* domain, __itt_string_handle* measurement, settings_t sample_settings)
{
    double res = 0;
    for (unsigned int i = 0; i < sample_settings.iterations; ++i) {
        /* Collects the measurement start time based on the CPU’s timestamp counter (TSC) */
        __itt_task_begin(domain, __itt_null, __itt_null, measurement);
        if ((sample_settings.emulate_outliers) && (i % (100 / amount_of_outliers) == 0)) {
            res = recursive_two_over_pi_approx(sample_settings.approximation);
        } else {
            res = iterative_two_over_pi_approx(sample_settings.approximation);
        }
        /* Collects the measurement end time based on the TSC and calculates the difference
           between the start and end times to get the latency measurement. */
        __itt_task_end(domain);
    }
    printf("Approximation #%u is:%lf\n", sample_settings.approximation, res);
}

double recursive_two_over_pi_approx(unsigned int n)
{
    if (n == 0) {
        return 1;
    }
    if (n == 1) {
        return sqrt(0.5);
    }
    return sqrt(0.5 + 0.5 * recursive_two_over_pi_approx(n - 1) / recursive_two_over_pi_approx(n - 2)) *
           recursive_two_over_pi_approx(n - 1);
}

double iterative_two_over_pi_approx(unsigned int n)
{
    double current = sqrt(0.5);
    double res = current;
    for (unsigned int i = 1; i < n; ++i) {
        current = sqrt(0.5 + 0.5 * current);
        res *= current;
    }
    return res;
}

/* Callback function for the deadline monitoring. Called when iteration latency exceeds the deadline. */
void notify_deadline(__attribute__((unused)) struct tcc_measurement* measurement, uint64_t cycles)
{
    long int nsec = tcc_measurement_convert_clock_to_timespec(cycles).tv_nsec;
    long int usec = nsec / TCC_NSEC_PER_USEC;
    printf("Latency exceeding deadline: %lu CPU cycles (%ld nsec) (%ld usec)\n", cycles, nsec, usec);
    deadline_counter++;
}

TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* sample_settings)
{
    static struct option long_options[] = {{"approximation", required_argument, NULL, 'a'},
        {"emulate-outliers", no_argument, NULL, 'o'},
        {"iterations", required_argument, NULL, 'i'},
        {"deadline", required_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "a:oi:d:h", long_options, &option_index);
        switch (opt) {
        case 'a': {
            if (optarg) {
                sample_settings->approximation = strtoul(optarg, NULL, DECIMAL_NUM_SYS);
                if ((strchr(optarg, '-') != NULL) || (sample_settings->approximation < 1) || (errno == ERANGE)) {
                    printf("Invalid approximation value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case 'o': {
            sample_settings->emulate_outliers = 1;
            break;
        }
        case 'i': {
            if (optarg) {
                if (((sample_settings->iterations = strtoul(optarg, NULL, DECIMAL_NUM_SYS)) == 0) ||
                    (strchr(optarg, '-') != NULL) || (errno == ERANGE)) {
                    printf("Invalid iterations value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case 'd': {
            if (optarg) {
                unsigned deadline_in_time_unit = strtoul(optarg, NULL, DECIMAL_NUM_SYS);
                char* units_str = strchr(optarg, ',');
                if (units_str) {
                    units_str++;
                }
                sample_settings->time_unit = tcc_measurement_get_time_unit_from_string(units_str);
                if (sample_settings->time_unit == TCC_TU_UNKNOWN) {
                    printf("Unknown time unit\n");
                    return TCC_PARSE_ERROR;
                }
                if (setenv("TCC_MEASUREMENTS_TIME_UNIT", units_str, 0) < 0) {
                    printf("Unable to set TCC_MEASUREMENTS_TIME_UNIT\n");
                    return TCC_PARSE_ERROR;
                }
                if (strchr(optarg, '-') != NULL) {
                    printf("Invalid deadline value\n");
                    return TCC_PARSE_ERROR;
                }
                sample_settings->deadline = deadline_in_time_unit;
            }
            break;
        }
        case 'h':
            return TCC_PRINT_HELP;
        case -1:
            break;
        default:
            return TCC_PARSE_ERROR;
        }
    }
    if (!sample_settings->approximation) {
        printf("The required option --approximation (-a) was not set\n");
        return TCC_PARSE_ERROR;
    }
    return TCC_PARSE_SUCCESS;
}

void print_params(settings_t sample_settings)
{
    const size_t len = 256;
    char deadline_str[len];

    switch (sample_settings.time_unit) {
    case TCC_TU_CLK:
        snprintf(deadline_str, len - 1, "%u clk", sample_settings.deadline);
        break;
    case TCC_TU_NS:
        snprintf(deadline_str, len - 1, "%u ns", sample_settings.deadline);
        break;
    case TCC_TU_US:
        snprintf(deadline_str, len - 1, "%u us", sample_settings.deadline);
        break;
    default:
        break;
    }

    printf("Running with arguments:\n"
           "    approximation = %u,\n"
           "    iterations = %u,\n"
           "    outliers = %s,\n"
           "    deadline = %s,\n",
        sample_settings.approximation,
        sample_settings.iterations,
        sample_settings.emulate_outliers ? "True" : "False",
        sample_settings.deadline != 0 ? deadline_str : "N/A");
}

void print_usage()
{
    printf("Usage: tcc_single_measurement_sample -a N [-i N] [-d N<,clk|,ns|,us>] [-o]\
\n\
Options:\n\
        -a | --approximation N          Required. Calculates the Nth approximation of 2/pi.\n\
        -i | --iterations N             Execute N iterations of the main loop to gather\n\
                                        more precise timing statistics. Default: 1,000.\n\
        -d | --deadline N<,clk|,ns|,us> Specify the maximum tolerable latency\n\
                                        for each iteration in N CPU clock cycles (clk),\n\
                                        nanoseconds (ns), or microseconds (us).\n\
                                        Default: 0 (no deadline).\n\
        -o | --emulate-outliers         Enable emulation of outliers (amount about 20 percent).\n\
        -h | --help                     Show this help message and exit.\n");
}
