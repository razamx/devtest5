/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_TCC_LOG_H_
#define _TCC_TCC_LOG_H_


#define GENERATE_ENUM(ENUM) LOG_##ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define SAFENULLSTR(str) ((str) != NULL ? (str) : "(null)")

#ifdef __cplusplus
extern "C" {
#endif

#define FOREACH_LOG_LEVEL(DO_ACTION) \
    DO_ACTION(TRACE)                 \
    DO_ACTION(DEBUG)                 \
    DO_ACTION(INFO)                  \
    DO_ACTION(WARNING)               \
    DO_ACTION(ERROR)                 \
    DO_ACTION(FATAL)                 \
    DO_ACTION(NONE)

enum
{
    FOREACH_LOG_LEVEL(GENERATE_ENUM)
};

struct tcc_function_trace
{
    const char* file;
    const char* func;
};

void __tcc_cs_trace_function_exit(struct tcc_function_trace* function);

#ifndef TCC_LOG_LEVEL
#ifndef NO_DEBUG
#define TCC_LOG_LEVEL LOG_TRACE
#else /* NO_DEBUG */
#define TCC_LOG_LEVEL LOG_ERROR
#endif /* NO_DEBUG */
#endif /* TCC_LOG_LEVEL*/


extern int tcc_log_level;
#define TCC_LOG(level, fmt, ...)                                                    \
    if (level >= TCC_LOG_LEVEL)                                                     \
    ({                                                                              \
        if (level >= tcc_log_level) {                                               \
            tcc_log_print(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        }                                                                           \
    })
#ifndef TCC_LOGGING_DISABLED

#define TCC_LOG_TRACE(fmt, ...) TCC_LOG(LOG_TRACE, fmt, ##__VA_ARGS__)
#define TCC_LOG_DEBUG(fmt, ...) TCC_LOG(LOG_DEBUG, fmt, ##__VA_ARGS__)
#define TCC_LOG_INFO(fmt, ...) TCC_LOG(LOG_INFO, fmt, ##__VA_ARGS__)
#define TCC_LOG_WARNING(fmt, ...) TCC_LOG(LOG_WARNING, fmt, ##__VA_ARGS__)
#define TCC_LOG_ERROR(fmt, ...) TCC_LOG(LOG_ERROR, fmt, ##__VA_ARGS__)
#define TCC_LOG_FATAL(fmt, ...) TCC_LOG(LOG_FATAL, fmt, ##__VA_ARGS__)
#define TCC_TRACE_FUNC(...)                                                                                       \
    struct tcc_function_trace __tcc_function_trace                                                                \
        __attribute__((__cleanup__(__tcc_cs_trace_function_exit))) = {__FILE__, __func__};                        \
    if (tcc_log_level == LOG_TRACE) {                                                                             \
        tcc_log_print(LOG_TRACE, __tcc_function_trace.file, 0, __tcc_function_trace.func, "enter: " __VA_ARGS__); \
    }

int tcc_log_print(int level, const char* file_name, int line, const char* function_name, const char* fmt, ...)
    __attribute__((format(printf, 5, 6)));
void tcc_log_init(void);

int tcc_log_set_level(int level);


#else  //// TCC_LOGGING_DISABLED

#define TCC_LOG_TRACE(fmt, ...) ({})
#define TCC_LOG_DEBUG(fmt, ...) ({})
#define TCC_LOG_INFO(fmt, ...) ({})
#define TCC_LOG_WARNING(fmt, ...) ({})
#define TCC_LOG_ERROR(fmt, ...) ({})
#define TCC_LOG_FATAL(fmt, ...) ({})
#define TCC_TRACE_FUNC(...) ({})

#endif  // TCC_LOGGING_DISABLED

#ifdef __cplusplus
}
#endif


#endif  // _TCC_TCC_LOG_H_
