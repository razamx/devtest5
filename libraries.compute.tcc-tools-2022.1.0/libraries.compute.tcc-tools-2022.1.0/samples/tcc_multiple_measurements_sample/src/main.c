/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include "args_parse.h"
#include "macros.h"
#include "tcc/measurement.h"
#include "tcc/measurement_helpers.h"

struct settings_t
{
    unsigned approximation;
    unsigned multiplication;
    unsigned iterations;
};

void print_params(settings_t sample_settings);
void run_workload_and_measure_cycles(settings_t sample_settings);
int* create_matrix(const unsigned matrix_size);
void matrix_multiplication(const int* A, const int* B, unsigned int n);
double two_over_pi_approx(unsigned int n);

static __itt_domain* domain = NULL;
static __itt_string_handle* measurement_multiplication_handler = NULL;
static __itt_string_handle* measurement_approximation_handler = NULL;
static __itt_string_handle* measurement_cycle_handler = NULL;


int main(int argc, char** argv)
{
    /* Parse params*/
    settings_t sample_setting = {.approximation = 0, .multiplication = 0, .iterations = 1000};

    int tcc_status = parse_args(argc, argv, &sample_setting);
    if (tcc_status == TCC_PARSE_ERROR) {
        print_usage();
        return -TCC_E_BAD_PARAM;
    }
    if (tcc_status == TCC_PRINT_HELP) {
        print_usage();
        return TCC_E_SUCCESS;
    }

    static const char measurement_multiplication[] = "Multiplication";
    static const char measurement_approximation[] = "Approximation";
    static const char measurement_cycle[] = "Cycle";

    /* Initialize the ITT domain to collect performance data for the sample workloads */
    domain = __itt_domain_create("TCC");

    /* Initialize the ITT handlers to collect performance data for sample workloads. */
    /* Creating the ITT handlers for matrix multiplication. */
    measurement_multiplication_handler = __itt_string_handle_create(measurement_multiplication);

    /* Creating the ITT handlers for 2/PI approximation. */
    measurement_approximation_handler = __itt_string_handle_create(measurement_approximation);

    /* Creating the ITT handlers for both: matrix multiplication and 2/PI approximation. */
    measurement_cycle_handler = __itt_string_handle_create(measurement_cycle);
    if (!domain || !measurement_multiplication_handler || !measurement_approximation_handler ||
        !measurement_cycle_handler) {
        printf("Unable to create ITT handles\n");
        return -TCC_E_NOT_AVAILABLE;
    }

    print_params(sample_setting);

    printf("Running workloads. This may take a while, depending on iteration values.\n");
    run_workload_and_measure_cycles(sample_setting);

    /* Application exits too fast. We use sleep here for the shared memory reader to read measurement results */
    sleep(1);
    return 0;
}


TCC_ARGS_PARSE_STATUS parse_args(int argc, char** argv, settings_t* sample_settings)
{
    static struct option long_options[] = {{"approximation", required_argument, NULL, 'a'},
        {"multiplication", required_argument, NULL, 'm'},
        {"iterations", required_argument, NULL, 'i'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int opt = 0;
    while (opt != -1) {
        opt = getopt_long(argc, argv, "a:m:i:h", long_options, &option_index);
        switch (opt) {
        case 'a': {
            if (optarg) {
                if (((sample_settings->approximation = strtoul(optarg, NULL, DECIMAL_NUM_SYS)) == 0) ||
                    (strchr(optarg, '-') != NULL)) {
                    printf("Invalid approximation value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case 'm': {
            if (optarg) {
                if (((sample_settings->multiplication = strtoul(optarg, NULL, DECIMAL_NUM_SYS)) == 0) ||
                    (strchr(optarg, '-') != NULL)) {
                    printf("Invalid multiplication value\n");
                    return TCC_PARSE_ERROR;
                }
            }
            break;
        }
        case 'i': {
            if (optarg) {
                if (((sample_settings->iterations = strtoul(optarg, NULL, DECIMAL_NUM_SYS)) == 0) ||
                    (strchr(optarg, '-') != NULL)) {
                    printf("Invalid iterations value\n");
                    return TCC_PARSE_ERROR;
                }
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
    if (sample_settings->approximation == 0) {
        printf("Approximation value was not set correctly.\n");
        return TCC_PARSE_ERROR;
    }
    if (sample_settings->multiplication == 0) {
        printf("Multiplication value was not set correctly.\n");
        return TCC_PARSE_ERROR;
    }

    return TCC_PARSE_SUCCESS;
}


void print_params(settings_t sample_settings)
{
    printf("Running with arguments:\n"
           "    approximation = %u,\n"
           "    multiplication = %u,\n"
           "    iterations = %u\n",
        sample_settings.approximation,
        sample_settings.multiplication,
        sample_settings.iterations);
}

void print_usage()
{
    printf("Usage: tcc_multiple_measurements_sample -a N -m N [-i N]\n\
Options:\n\
    -a | --approximation N     Required. Execute 2/PI approximation workload with N number.\n\
    -m | --multiplication N    Required. Execute matrices multiplication workload with N number.\n\
    -i | --iterations N        Execute N iterations of the main loop to gather\n\
                               more precise timing statistics. Default: 1,000.\n\
    -h | --help                Show this help message and exit.\n");
}

/* Measure latency of matrix multiplication, 2/PI approximation */
void run_workload_and_measure_cycles(const settings_t sample_settings)
{
    int* A = create_matrix(sample_settings.multiplication);
    int* B = create_matrix(sample_settings.multiplication);
    if (A == NULL || B == NULL) {
        goto error;
    }

    for (unsigned i = 0; i < sample_settings.iterations; ++i) {
        /* Collecting overall measurement was started. */
        __itt_task_begin(domain, __itt_null, __itt_null, measurement_cycle_handler);

        /* Collecting the measurement for multiplication matrix was started. */
        __itt_task_begin(domain, __itt_null, __itt_null, measurement_multiplication_handler);

        /* Running Matrix Multiplication function. */
        matrix_multiplication(A, B, sample_settings.multiplication);

        /* Collecting the measurement for multiplication matrix was ended. */
        __itt_task_end(domain);

        /* Collecting the measurement for PI approximation was started. */
        __itt_task_begin(domain, __itt_null, __itt_null, measurement_approximation_handler);

        /* Running Approximation function. */
        two_over_pi_approx(sample_settings.approximation);

        /* Collecting the measurement for PI approximation was ended. */
        __itt_task_end(domain);

        /* Collecting overall measurement was ended. */
        __itt_task_end(domain);
    }
    printf("Workloads were run successfully.\n");
error:
    free(A);
    free(B);
}

int* create_matrix(const unsigned matrix_size)
{
    int* x = (int*)malloc(matrix_size * matrix_size * sizeof(*x));
    if (x == NULL) {
        printf("Unable to allocate memory for matrix\n");
        return NULL;
    }

    for (unsigned i = 0; i < matrix_size; ++i) {
        for (unsigned j = 0; j < matrix_size; ++j) {
            /* Each element of matrix is equal to i. */
            x[i * matrix_size + j] = (int)i;
        }
    }
    return x;
}

/* The method computes multiplication of two square matrices with n size. */
void matrix_multiplication(const int* A, const int* B, unsigned int n)
{
    volatile int C[n][n];
    for (unsigned int i = 0; i < n; ++i) {
        for (unsigned int j = 0; j < n; ++j) {
            for (unsigned int k = 0; k < n; ++k) {
                /* Each element of C will be equal to the sum of the positive integer numbers from 1 to n. */
                C[i][j] += A[i * n + k] * B[k * n + j];
            }
        }
    }
}

/* The method computes sequential values of (2/PI) approximation using time-consuming operation of square root. */
double two_over_pi_approx(unsigned int n)
{
    double current = sqrt(0.5);

    /* Initializing the value on the 1st step. */
    double res = current;

    /* Every step the variable 'res' is getting closer to the exact value (2/PI). */
    for (unsigned int i = 1; i < n; ++i) {
        current = sqrt(0.5 + 0.5 * current);
        res *= current;
    }
    return res;
}
