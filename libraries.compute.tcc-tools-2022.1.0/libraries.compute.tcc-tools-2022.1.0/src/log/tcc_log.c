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
#include <libgen.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "tcc_log.h"
#include "version.h"

static FILE* log_to;
int tcc_log_level;

static const char* LOG_LEVEL_NAMES[] = {FOREACH_LOG_LEVEL(GENERATE_STRING)};

static void set_log_file(const char* file_name)
{
    if (file_name == NULL) {
        TCC_LOG_DEBUG("No TCC_LOG_TO_FILE environment variable");
        return;
    }
    FILE* log_file = fopen(file_name, "a+");
    if (log_file == NULL) {
        TCC_LOG_ERROR("Can't open file '%s' for logging: %s", file_name, strerror(errno));
        return;
    }
    if (log_to != NULL) {
        TCC_LOG_WARNING("set log file to '%s'", file_name);
        fclose(log_to);
    }
    log_to = log_file;
}

static void set_log_level(const char* log_level)
{
    if (log_level == NULL) {
        return;
    }
    for (size_t i = 0; i < sizeof(LOG_LEVEL_NAMES) / sizeof(LOG_LEVEL_NAMES[0]); i++) {
        if (!strncmp(log_level, LOG_LEVEL_NAMES[i], strlen(LOG_LEVEL_NAMES[i]))) {
            tcc_log_level = i;
        }
    }
}

void tcc_log_init(void)
{
    set_log_file(getenv("TCC_LOG_TO_FILE"));
    set_log_level(getenv("TCC_LOG_LEVEL"));
}

__attribute__((constructor)) static void tcc_log_init_constructor(void)
{
    log_to = NULL;
    tcc_log_level = LOG_NONE;
    tcc_log_init();
}

static char* vmake_message(const char* fmt, va_list ape)
{
    int size = 0;
    char* p = NULL;
    va_list ap;

    /* Determine required size */

    va_copy(ap, ape);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);

    if (size < 0) {
        return NULL;
    }

    size++; /* For '\0' */
    p = (char*)malloc(sizeof(char) * size);
    if (p == NULL) {
        return NULL;
    }

    va_copy(ap, ape);
    size = vsnprintf(p, size, fmt, ap);
    va_end(ap);

    if (size < 0) {
        free(p);
        return NULL;
    }

    return p;
}

static char* make_message(const char* fmt, ...)
{
    va_list ap;
    char* ret_string = NULL;
    va_start(ap, fmt);
    ret_string = vmake_message(fmt, ap);
    va_end(ap);
    return ret_string;
}

static char* make_time_stamp()
{
    char buf[128];
    struct tm* tm_info;
    struct timeval tv;
    char* msg = NULL;
    long ms;

    gettimeofday(&tv, NULL);
    ms = lrint((double)(tv.tv_usec / 1000.0L));
    if (ms >= 1000) {
        ms -= 1000;
        ++tv.tv_sec;
    }
    tm_info = localtime(&tv.tv_sec);
    if (tm_info) {
        strftime(buf, sizeof(buf), "%Y.%m.%d %H:%M:%S", tm_info);
        msg = make_message("%s.%03li", buf, ms);
    } else {
        msg = make_message("TIMESTAMP");
    }
    return msg;
}

static char* make_prefix(const char* file_name, int line, const char* function_name, int level, const char* time_stamp)
{
    char* path = strdup(file_name);
    char* base_file_name = strdup(basename(path));
    pid_t tmp = getpid();
    int num_of_digit_pid = 0;
    while (tmp) {
        tmp /= 10;
        ++num_of_digit_pid;
    }
    char* message = make_message("%s %*i %s %s:%i %s",
        time_stamp,
        num_of_digit_pid,
        getpid(),
        LOG_LEVEL_NAMES[level],
        base_file_name,
        line,
        function_name);
    free(path);
    free(base_file_name);
    return message;
}

int tcc_log_print(int level, const char* file_name, int line, const char* function_name, const char* fmt, ...)
{
    int retval = 0;
    char* prefix = NULL;
    char* time_stamp = NULL;
    char* message_body = NULL;
    FILE* log_file = log_to;
    va_list ap;
    if (log_file == NULL) {
        log_file = stderr;
    }

    time_stamp = make_time_stamp();
    prefix = make_prefix(file_name, line, function_name, level, time_stamp);
    va_start(ap, fmt);
    message_body = vmake_message(fmt, ap);
    va_end(ap);
    if (prefix && message_body) {
        fprintf(log_file, "%s | %s\n", prefix, message_body);
    }
    fflush(log_file);
    free(message_body);
    free(prefix);
    free(time_stamp);

    return retval;
}

int tcc_log_set_level(int level)
{
    int old_level = tcc_log_level;

    tcc_log_level = level;
    return old_level;
}

void __tcc_cs_trace_function_exit(struct tcc_function_trace* function)
{
    if (tcc_log_level == LOG_TRACE) {
        tcc_log_print(LOG_TRACE, function->file, 0, function->func, "exit");
    }
}
