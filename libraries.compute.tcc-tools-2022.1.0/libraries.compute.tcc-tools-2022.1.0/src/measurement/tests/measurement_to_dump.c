/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "tcc_log.h"
#include "tcc/measurement.h"
#include "tcc/measurement_helpers.h"

#define COLLECTOR_LIBRARY_VAR_NAME "INTEL_LIBITTNOTIFY64"
#define TCC_COLLECTOR_NAME "libtcc_collector.so"
#define WORKLOAD_NAME "tcc_benchmark"

int main()
{
    static __itt_domain* domain = NULL;
    static __itt_string_handle* handle = NULL;
    FILE* dump_file = NULL;
    int status = -1;

    if (setenv("TCC_LOG_LEVEL", "ERROR", 0)) {
        TCC_LOG_ERROR("error:setenv:%s", strerror(errno));
        goto error;
    }

    if (setenv(COLLECTOR_LIBRARY_VAR_NAME, TCC_COLLECTOR_NAME, 0)) {
        TCC_LOG_ERROR("setenv:%s", strerror(errno));
        goto error;
    }

    if (setenv("TCC_MEASUREMENTS_DUMP_FILE", "/measurement_output.txt", 0)) {
        TCC_LOG_ERROR("setenv:%s", strerror(errno));
        goto error;
    }

    if (setenv("TCC_MEASUREMENTS_BUFFERS", WORKLOAD_NAME ":1000", 0)) {
        TCC_LOG_ERROR("setenv:%s", strerror(errno));
        goto error;
    }

    domain = __itt_domain_create(WORKLOAD_NAME);

    if (domain == NULL) {
        TCC_LOG_ERROR("Can't init ITT domain");
        goto error;
    }

    handle = __itt_string_handle_create(WORKLOAD_NAME);

    if (handle == NULL) {
        TCC_LOG_ERROR("Can't init ITT domain handle");
        goto error;
    }

    __itt_task_begin(domain, __itt_null, __itt_null, handle);
    __itt_task_end(domain);

    struct tcc_measurement* measurement = NULL;

    if (tcc_measurement_get(domain, handle, &measurement) != TCC_E_SUCCESS) {
        TCC_LOG_ERROR("tcc_measurement_get");
        goto error;
    }

    const char* dump_file_name_initial = tcc_measurement_get_dump_file_from_env();

    if (dump_file_name_initial == NULL) {
        TCC_LOG_ERROR("Can't get dump_file name");
        goto error;
    }

    char* dump_file_name = getenv("TCC_TESTS_INSTALL_PATH");

    if (dump_file_name == NULL) {
        TCC_LOG_ERROR("Can't get dump_file path");
        goto error;
    }

    strcat(dump_file_name, dump_file_name_initial);

    dump_file = fopen(dump_file_name, "w");

    if (dump_file == NULL) {
        TCC_LOG_ERROR("fopen:%s", strerror(errno));
        goto error;
    }

    if (tcc_measurement_print_history(measurement, dump_file, TCC_TU_CLK) != TCC_E_SUCCESS) {
        TCC_LOG_ERROR("tcc_measurement_print_history");
        goto error;
    }

    status = 0;

error:

    if (dump_file) {
        fclose(dump_file);
    }

    return status;
}
