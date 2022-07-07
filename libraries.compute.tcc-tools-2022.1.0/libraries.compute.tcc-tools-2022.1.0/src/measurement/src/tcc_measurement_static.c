/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#undef NO_TCC_MEASUREMENT  // Not needed for measurement library

#include "tcc/measurement.h"
#include <assert.h>
#include <dlfcn.h>
#include <ittnotify/ittnotify.h>
#include <ittnotify/ittnotify_config.h>
#include <stdlib.h>
#include <string.h>

#ifndef TCC_LIB_NAME
#if ITT_ARCH == ITT_ARCH_IA32 || ITT_ARCH == ITT_ARCH_ARM
#define TCC_LIB_NAME INTEL_LIBITTNOTIFY32
#else
#define TCC_LIB_NAME INTEL_LIBITTNOTIFY64
#endif
#endif

typedef void* lib_t;

static lib_t library = NULL;
static int is_initialyzed = 0;
static struct tcc_functions
{
    int (*tcc_measurement_get)(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement);
} functions;

static const char* tcc_measurement_get_library_name()
{
    const char* env_name = ITT_TO_STR(TCC_LIB_NAME);
    const char* library_name = getenv(env_name);
    return library_name;
}

static void deinit_tcc_static()
{
    functions.tcc_measurement_get = NULL;
    if (library != NULL) {
        dlclose(library);
    }
    is_initialyzed = 0;
}

static void init_tcc_static()
{
    const char* labrary_name = tcc_measurement_get_library_name();
    if (labrary_name == NULL) {
        return;
    }
    library = dlopen(labrary_name, RTLD_LAZY);
    if (library == NULL) {
        fprintf(stderr, "error: can't open library '%s'\n", labrary_name);
        return;
    }

    functions.tcc_measurement_get = dlsym(library, "__tcc_measurement_get");
    atexit(deinit_tcc_static);
}

static TCC_ERROR check_and_init(void** ptr)
{
    if (*ptr == NULL) {
        if (!is_initialyzed) {
            init_tcc_static();
            is_initialyzed = 1;
            if (*ptr == NULL) {
                return -TCC_E_NOT_AVAILABLE;
            }
        } else {
            return -TCC_E_NOT_AVAILABLE;
        }
    }
    return TCC_E_SUCCESS;
}

int tcc_measurement_get(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement)
{
    TCC_ERROR err;
    if ((err = check_and_init((void**)&functions.tcc_measurement_get)) != TCC_E_SUCCESS) {
        return err;
    }
    return functions.tcc_measurement_get(domain, handle, measurement);
}
