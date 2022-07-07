/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#define INTEL_ITTNOTIFY_API_PRIVATE
#undef NO_TCC_MEASUREMENT  // Not needed for measurement library

#include <ittnotify/ittnotify.h>
#include <ittnotify/ittnotify_config.h>

#include "measurement.h"
#include "tcc/cache.h"
#include "tcc/measurement_helpers.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <tcc_log.h>

#define UNICODE_AGNOSTIC(name) name

#define UNUSED(x) ((void)(x))

typedef struct tcc_measurement tcc_measurement_t;
typedef struct tcc_measurmetn_stack_frame_t tcc_measurmetn_stack_frame_t;
typedef __itt_string_handle itt_string_handle_t;
typedef __itt_domain itt_domain_t;

itt_string_handle_t* ITTAPI UNICODE_AGNOSTIC(string_handle_create)(const char* name);
itt_domain_t* UNICODE_AGNOSTIC(domain_create)(const char* name);

// These functions are public
void task_begin(const __itt_domain* pDomain, __itt_id taskid, __itt_id parentid, __itt_string_handle* pName);
void task_end(const __itt_domain* pDomain);
int __tcc_measurement_get(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement);
void ITTAPI __itt_api_init(__itt_global* pGlob, __itt_group_id id);

#define ORIGINAL_FUNCTIONS()                            \
    ITT_STUB_IMPL_ORIG(UNICODE_AGNOSTIC(domain_create)) \
    ITT_STUB_IMPL_ORIG(UNICODE_AGNOSTIC(string_handle_create))

#define API_MAP()                                     \
    ITT_STUB_IMPL(task_begin)                         \
    ITT_STUB_NO_IMPL(task_begin_fn)                   \
    ITT_STUB_IMPL(task_end)                           \
    ITT_STUB_NO_IMPL(marker)                          \
    ITT_STUB_NO_IMPL(marker_ex)                       \
    ITT_STUB_NO_IMPL(counter_inc_delta_v3)            \
    ITT_STUB_NO_IMPL(counter_set_value)               \
    ITT_STUB_NO_IMPL(counter_set_value_ex)            \
    ITT_STUB_NO_IMPL(clock_domain_create)             \
    ITT_STUB_NO_IMPL(clock_domain_reset)              \
    ITT_STUB_NO_IMPL(task_begin_ex)                   \
    ITT_STUB_NO_IMPL(task_end_ex)                     \
    ITT_STUB_NO_IMPL(id_create)                       \
    ITT_STUB_NO_IMPL(set_track)                       \
    ITT_STUB_NO_IMPL(track_create)                    \
    ITT_STUB_NO_IMPL(track_group_create)              \
    ITT_STUB_NO_IMPL(task_begin_overlapped)           \
    ITT_STUB_NO_IMPL(task_begin_overlapped_ex)        \
    ITT_STUB_NO_IMPL(task_end_overlapped)             \
    ITT_STUB_NO_IMPL(task_end_overlapped_ex)          \
    ITT_STUB_NO_IMPL(id_destroy)                      \
    ITT_STUB_NO_IMPL(api_version)                     \
    ITT_STUB_NO_IMPL(frame_begin_v3)                  \
    ITT_STUB_NO_IMPL(frame_end_v3)                    \
    ITT_STUB_NO_IMPL(frame_submit_v3)                 \
    ITT_STUB_NO_IMPL(frame_begin)                     \
    ITT_STUB_NO_IMPL(frame_end)                       \
    ITT_STUB_NO_IMPL(region_begin)                    \
    ITT_STUB_NO_IMPL(region_end)                      \
    ITT_STUB_NO_IMPL(pause)                           \
    ITT_STUB_NO_IMPL(resume)                          \
    ITT_STUB_NO_IMPL(get_timestamp)                   \
    ITT_STUB_NO_IMPL(metadata_add)                    \
    ITT_STUB_NO_IMPL(sync_destroy)                    \
    ITT_STUB_NO_IMPL(sync_acquired)                   \
    ITT_STUB_NO_IMPL(sync_releasing)                  \
    ITT_STUB_NO_IMPL(sync_prepare)                    \
    ITT_STUB_NO_IMPL(sync_cancel)                     \
    ITT_STUB_NO_IMPL(relation_add_to_current)         \
    ITT_STUB_NO_IMPL(relation_add)                    \
    ITT_STUB_NO_IMPL(relation_add_to_current_ex)      \
    ITT_STUB_NO_IMPL(relation_add_ex)                 \
    ITT_STUB_NO_IMPL(heap_allocate_begin)             \
    ITT_STUB_NO_IMPL(heap_allocate_end)               \
    ITT_STUB_NO_IMPL(heap_free_begin)                 \
    ITT_STUB_NO_IMPL(heap_free_end)                   \
    ORIGINAL_FUNCTIONS()                              \
    ITT_STUB_NO_IMPL(thread_ignore)                   \
    ITT_STUB_NO_IMPL(thr_ignore)                      \
    ITT_STUB_NO_IMPL(counter_inc_delta)               \
    ITT_STUB_NO_IMPL(enable_attach)                   \
    ITT_STUB_NO_IMPL(suppress_push)                   \
    ITT_STUB_NO_IMPL(suppress_pop)                    \
    ITT_STUB_NO_IMPL(suppress_mark_range)             \
    ITT_STUB_NO_IMPL(suppress_clear_range)            \
    ITT_STUB_NO_IMPL(model_site_beginA)               \
    ITT_STUB_NO_IMPL(model_site_beginAL)              \
    ITT_STUB_NO_IMPL(model_site_end)                  \
    ITT_STUB_NO_IMPL(model_task_end)                  \
    ITT_STUB_NO_IMPL(model_lock_acquire)              \
    ITT_STUB_NO_IMPL(model_lock_release)              \
    ITT_STUB_NO_IMPL(model_record_allocation)         \
    ITT_STUB_NO_IMPL(model_record_deallocation)       \
    ITT_STUB_NO_IMPL(model_induction_uses)            \
    ITT_STUB_NO_IMPL(model_reduction_uses)            \
    ITT_STUB_NO_IMPL(model_observe_uses)              \
    ITT_STUB_NO_IMPL(model_clear_uses)                \
    ITT_STUB_NO_IMPL(model_site_begin)                \
    ITT_STUB_NO_IMPL(model_site_beginA)               \
    ITT_STUB_NO_IMPL(model_site_beginAL)              \
    ITT_STUB_NO_IMPL(model_task_begin)                \
    ITT_STUB_NO_IMPL(model_task_beginA)               \
    ITT_STUB_NO_IMPL(model_task_beginAL)              \
    ITT_STUB_NO_IMPL(model_iteration_taskA)           \
    ITT_STUB_NO_IMPL(model_iteration_taskAL)          \
    ITT_STUB_NO_IMPL(model_site_end_2)                \
    ITT_STUB_NO_IMPL(model_task_end_2)                \
    ITT_STUB_NO_IMPL(model_lock_acquire_2)            \
    ITT_STUB_NO_IMPL(model_lock_release_2)            \
    ITT_STUB_NO_IMPL(model_aggregate_task)            \
    ITT_STUB_NO_IMPL(model_disable_push)              \
    ITT_STUB_NO_IMPL(model_disable_pop)               \
    ITT_STUB_NO_IMPL(heap_reallocate_begin)           \
    ITT_STUB_NO_IMPL(heap_reallocate_end)             \
    ITT_STUB_NO_IMPL(heap_internal_access_begin)      \
    ITT_STUB_NO_IMPL(heap_internal_access_end)        \
    ITT_STUB_NO_IMPL(heap_record_memory_growth_begin) \
    ITT_STUB_NO_IMPL(heap_record_memory_growth_end)   \
    ITT_STUB_NO_IMPL(heap_reset_detection)            \
    ITT_STUB_NO_IMPL(heap_record)                     \
    ITT_STUB_NO_IMPL(task_group)                      \
    ITT_STUB_NO_IMPL(counter_inc_v3)                  \
    ITT_STUB_NO_IMPL(event_start)                     \
    ITT_STUB_NO_IMPL(event_end)                       \
    ITT_STUB_NO_IMPL(notify_sync_prepare)             \
    ITT_STUB_NO_IMPL(notify_sync_cancel)              \
    ITT_STUB_NO_IMPL(notify_sync_acquired)            \
    ITT_STUB_NO_IMPL(notify_sync_releasing)           \
    ITT_STUB_NO_IMPL(memory_read)                     \
    ITT_STUB_NO_IMPL(memory_write)                    \
    ITT_STUB_NO_IMPL(memory_update)                   \
    ITT_STUB_NO_IMPL(state_get)                       \
    ITT_STUB_NO_IMPL(state_set)                       \
    ITT_STUB_NO_IMPL(obj_mode_set)                    \
    ITT_STUB_NO_IMPL(thr_mode_set)                    \
    ITT_STUB_NO_IMPL(counter_destroy)                 \
    ITT_STUB_NO_IMPL(counter_inc)                     \
    ITT_STUB_NO_IMPL(counter_inc_v3)                  \
    ITT_STUB_NO_IMPL(mark_off)                        \
    ITT_STUB_NO_IMPL(mark_global_off)                 \
    ITT_STUB_NO_IMPL(stack_caller_create)             \
    ITT_STUB_NO_IMPL(stack_caller_destroy)            \
    ITT_STUB_NO_IMPL(stack_callee_enter)              \
    ITT_STUB_NO_IMPL(stack_callee_leave)              \
    ITT_STUB_NO_IMPL(id_create_ex)                    \
    ITT_STUB_NO_IMPL(id_destroy_ex)                   \
    ITT_STUB_NO_IMPL(task_begin_fn_ex)                \
    ITT_STUB_NO_IMPL(metadata_add_with_scope)

static void fill_api_list(__itt_api_info* api_list_ptr)
{
#define ITT_STUB_IMPL(fn)                                             \
    if (0 == strcmp("__itt_" ITT_TO_STR(fn), api_list_ptr[i].name)) { \
        *api_list_ptr[i].func_ptr = (void*)(fn);                      \
        continue;                                                     \
    }
#define ITT_STUB_IMPL_ORIG(name) ITT_STUB_IMPL(name)
#define ITT_STUB_NO_IMPL(fn)
    for (int i = 0; (api_list_ptr[i].name != NULL) && (*api_list_ptr[i].name != 0); ++i) {
        API_MAP();  // continue is called inside when function is found
    }
#undef ITT_STUB_IMPL
#undef ITT_STUB_IMPL_ORIG
#undef ITT_STUB_NO_IMPL
}


typedef struct string_handle_holder_t string_handle_holder_t;

struct string_handle_holder_t
{
    itt_string_handle_t* handle;
    string_handle_holder_t* next;
};

static string_handle_holder_t* string_handle_holders = NULL;
static itt_string_handle_t empty_string_handle;
static itt_string_handle_t* string_handels_stack = &empty_string_handle;

static void free_holders()
{
    TCC_TRACE_FUNC("void");
    while (string_handle_holders != NULL) {
        TCC_ERROR tcc_sts = tcc_measurement_destroy((struct tcc_measurement*)string_handle_holders->handle->extra2);
        if (tcc_sts != TCC_E_SUCCESS) {
            TCC_LOG_ERROR("Failed to destroy tcc_measurement, error %d", tcc_sts);
        }
        free(string_handle_holders->handle->extra2);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        free((char*)string_handle_holders->handle->strA);
#pragma GCC diagnostic pop
        free(string_handle_holders->handle->strW);
        free(string_handle_holders->handle);
        string_handle_holder_t* prev_holder = string_handle_holders;
        string_handle_holders = string_handle_holders->next;
        free(prev_holder);
    }
}

void tcc_itt_deinit()
{
    TCC_TRACE_FUNC("void");

    string_handels_stack = NULL;
    const char* file_name = tcc_measurement_get_dump_file_from_env();
    TCC_TIME_UNIT time_unit = tcc_measurement_get_time_unit_from_env();
    if (file_name == NULL) {
        free_holders();
        return;
    }
    // use clk as default time unit
    if (time_unit == TCC_TU_UNKNOWN) {
        time_unit = TCC_TU_CLK;
    }
    FILE* file = fopen(file_name, "w");
    if (file == NULL) {
        free_holders();
        TCC_LOG_ERROR("Can't open '%s':%s(%i)", SAFENULLSTR(file_name), strerror(errno), errno);
        return;
    }
    string_handle_holder_t* holder = string_handle_holders;

    while (holder != NULL) {
        tcc_measurement_print_history((tcc_measurement_t*)holder->handle->extra2, file, time_unit);
        string_handle_holder_t* new_holder = holder->next;
        holder = new_holder;
    }
    fflush(file);
    fclose(file);
    free_holders();
}

itt_domain_t* UNICODE_AGNOSTIC(domain_create)(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));
    static itt_domain_t tcc_domain;
    return &tcc_domain;
}

static int register_handle_if_not_registred(itt_string_handle_t* handle)
{
    TCC_TRACE_FUNC("handle=%p", handle);

    TCC_ERROR err;
    tcc_measurement_t* measure = NULL;
    string_handle_holder_t* holder = NULL;
    if (handle->extra2 != NULL) {
        return TCC_E_SUCCESS;
    }

    holder = calloc(1, sizeof(string_handle_holder_t));
    if (holder == NULL) {
        TCC_LOG_ERROR("Failed to allocate memory for string handle");
        err = -TCC_E_MEMORY;
        goto error;
    }

    measure = malloc(sizeof(tcc_measurement_t));
    if (measure == NULL) {
        TCC_LOG_ERROR("Failed to allocate memory for measurement structure");
        err = -TCC_E_MEMORY;
        goto error;
    }

    if ((err = tcc_measurement_init_ex(measure,
             tcc_measurement_get_buffer_size_from_env(handle->strA),
             0,
             NULL,
             tcc_measurement_shared_memory_flag_from_env(),
             handle->strA)) != TCC_E_SUCCESS) {
        TCC_LOG_ERROR("Failed to initialize tcc_measurement");
        goto error;
    }

    handle->next = NULL;
    handle->extra2 = measure;
    holder->next = string_handle_holders;
    holder->handle = handle;
    string_handle_holders = holder;
    return 0;
error:
    free(holder);
    free(measure);
    return err;
}

static itt_string_handle_t* find_string_handle(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));

    string_handle_holder_t* holder = string_handle_holders;
    while (holder != NULL) {
        itt_string_handle_t* handle = holder->handle;
        if (strcmp(handle->strA, name) != 0) {
            holder = holder->next;
            continue;
        }
        TCC_LOG_DEBUG("String handle with name %s found: %p", SAFENULLSTR(name), handle);
        return handle;
    }
    TCC_LOG_DEBUG("String handle with name %s not found", SAFENULLSTR(name));
    return NULL;
}

itt_string_handle_t* ITTAPI UNICODE_AGNOSTIC(string_handle_create)(const char* name)
{
    TCC_TRACE_FUNC("name=%s", SAFENULLSTR(name));

    if (name == NULL) {
        TCC_LOG_ERROR("Parameter \"name\" is NULL");
        return NULL;
    }

    itt_string_handle_t* handle = find_string_handle(name);
    if (handle != NULL) {
        return handle;
    }

    handle = calloc(1, sizeof(itt_string_handle_t));
    if (handle == NULL) {
        TCC_LOG_ERROR("Failed to allocate memory for string handle");
        goto error;
    }

    handle->strA = strdup(name);
    if (handle->strA == NULL) {
        TCC_LOG_ERROR("Failed to allocate memory for string handle name");
        goto error;
    }

    if (register_handle_if_not_registred(handle) != 0) {
        TCC_LOG_ERROR("Failed to register handle");
        goto error;
    }

    return handle;
error:
    if (handle != NULL) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        free((char*)handle->strA);
#pragma GCC diagnostic pop
        free((char*)handle->strW);
    }
    free(handle);
    return NULL;
}

void task_begin(const __itt_domain* pDomain, __itt_id taskid, __itt_id parentid, __itt_string_handle* pName)
{
    UNUSED(pDomain);
    UNUSED(taskid);
    UNUSED(parentid);

    if (__builtin_expect((pName->next != NULL), 0)) {
        return;
    }

    pName->next = string_handels_stack;
    string_handels_stack = pName;
    tcc_measurement_start((tcc_measurement_t*)(pName->extra2));
}

void task_end(__attribute__((unused)) const __itt_domain* pDomain)
{
    uint64_t clk_end = __tsc_end();
    tcc_measurement_t* measurement = string_handels_stack->extra2;
    /* Store raw TSC difference in clk_last_diff */
    measurement->clk_last_diff = clk_end - measurement->clk_start;
    __tcc_measurement_store_measure(measurement, 1);
    __itt_string_handle* prev = string_handels_stack;
    string_handels_stack = prev->next;
    prev->next = NULL;
}

__attribute__((visibility("default"))) void ITTAPI __itt_api_init(__itt_global* pGlob, __itt_group_id id)
{
    TCC_TRACE_FUNC("pGlob=%p id=%u", pGlob, id);

    fill_api_list(pGlob->api_list_ptr);
    itt_string_handle_t* next = pGlob->string_list;

    while (next != NULL) {
        itt_string_handle_t* current = next;
        next = next->next;
        register_handle_if_not_registred(current);
    }

    atexit(tcc_itt_deinit);
}

int __tcc_measurement_get(__itt_domain* domain, __itt_string_handle* handle, struct tcc_measurement** measurement)
{
    TCC_TRACE_FUNC("domain=%p handle=%p measurement=%p", domain, handle, measurement);

    (void)domain;
    (void)measurement;
    TCC_ERROR err;

    // Currently unused, but need to check this to confirm that it will work with another collector
    if (domain == NULL) {
        TCC_LOG_ERROR("Parameter \"domain\" is NULL");
        return -TCC_E_BAD_PARAM;
    }

    if (handle == NULL) {
        TCC_LOG_ERROR("Parameter \"handle\" is NULL");
        return -TCC_E_BAD_PARAM;
    }

    if (measurement == NULL) {
        TCC_LOG_ERROR("Parameter \"measurement\" is NULL");
        return -TCC_E_BAD_PARAM;
    }

    if ((err = register_handle_if_not_registred(handle)) != TCC_E_SUCCESS) {
        TCC_LOG_ERROR("Failed to register string handle");
        return err;
    }

    *measurement = (struct tcc_measurement*)handle->extra2;
    return TCC_E_SUCCESS;
}

#ifdef UNIT_TESTS
void __tcc_measurement_itt_clean_state()
{
    string_handels_stack = &empty_string_handle;
    free_holders();
}
#endif
