/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tcc_config_finder.h"
#include "tcc_log.h"

#define ENV_CONFIG_PATH "TCC_CONFIG_PATH"
#define ENV_HOME "HOME"
#define CONFIG_SDK_PATH "/usr/share/tcc_tools/config"
#define CONFIG_FILE ".tcc.config"

static char* check_config_from_env_path(const char* env_name);
static char* check_confg_from_sdk_path();
static char* get_available_file(const char* dir, const char* file_name);

const char* get_available_config_file()
{
    static char* path = NULL;
    TCC_TRACE_FUNC();

    free(path);
    path = NULL;
    if (!((path = check_config_from_env_path(ENV_CONFIG_PATH)) || (path = check_config_from_env_path(ENV_HOME)) ||
            (path = check_confg_from_sdk_path()))) {
        TCC_LOG_WARNING("Unable to get path");
    }
    return path;
}

static char* check_config_from_env_path(const char* env_name)
{
    TCC_TRACE_FUNC("env_name=%s", SAFENULLSTR(env_name));
    char* dir = NULL;
    if (!(dir = getenv(env_name))) {
        TCC_LOG_WARNING("Environment variable \"%s\" is not set", env_name);
        return NULL;
    } else {
        return get_available_file(dir, CONFIG_FILE);
    }
}

static char* check_confg_from_sdk_path()
{
    TCC_TRACE_FUNC();
    return get_available_file(CONFIG_SDK_PATH, CONFIG_FILE);
}

static char* get_available_file(const char* dir, const char* file_name)
{
    TCC_TRACE_FUNC("dir=%s name=%s", dir, file_name);

    char* dest_path = NULL;
    if (asprintf(&dest_path, "%s/%s", dir, file_name) == -1) {
        TCC_LOG_ERROR("asprintf(%p, %%s/%%s, %s, %s) error: %s(%d)",
            SAFENULLSTR(dest_path),
            dir,
            file_name,
            strerror(errno),
            errno);
        goto error;
    }
    TCC_LOG_TRACE("dest_path: %s", SAFENULLSTR(dest_path));
    if (access(dest_path, R_OK) != 0) {
        TCC_LOG_WARNING("File %s unavailable: %s(%d)", SAFENULLSTR(dest_path), strerror(errno), errno);
        goto error;
    }
    return dest_path;
error:
    free(dest_path);
    return NULL;
}
