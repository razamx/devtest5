/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "tcc_config.h"

#include "memory_properties.h"
#include <errno.h>
#include <fcntl.h>
#include <json-c/json_object.h>
#include <json-c/json_object_iterator.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tcc_log.h"

////////////////////////////////////////////////////////////////////////////////

#define MAX_CONFIG_CNT (TCC_MEM_DRAM - TCC_MEM_L1)

#ifndef TRUE
#define TRUE 1
#define FALSE (!TRUE)
#endif

struct tcc_config
{
    size_t count;
    tcc_config_item_t* items;
};

////////////////////////////////////////////////////////////////////////////////

static tcc_config_t* parse_config(const char* filename);
static tcc_config_t* json_parse_file(int fd);
static enum tcc_memory_level str_to_mem_level(const char* str);
static tcc_config_t* allocate_config(size_t items_count);
static int json_parse_level(tcc_config_item_t* item, const char* level_name, json_object* level_config);
static int getInt64(struct json_object* parent, const char* key, int64_t* result);

////////////////////////////////////////////////////////////////////////////////

tcc_config_t* tcc_config_read(const char* filename)
{
    TCC_TRACE_FUNC("filename=%s", SAFENULLSTR(filename));

    tcc_config_t* config = NULL;

    if (filename == NULL) {
        TCC_LOG_ERROR("Parameter \"filename\" is NULL");
        goto err;
    }

    if ((config = parse_config(filename)) == NULL) {
        goto err;
    }

    return config;

err:
    tcc_config_destroy(config);
    return NULL;
}

size_t tcc_config_count(const tcc_config_t* config)
{
    TCC_TRACE_FUNC("config=%p", config);

    if (config == NULL) {
        TCC_LOG_ERROR("Parameter is NULL");
        return 0;
    }
    return config->count;
}

const tcc_config_item_t* tcc_config_get(const tcc_config_t* config, size_t index)
{
    TCC_TRACE_FUNC("config=%p index=%zu", config, index);

    if (config == NULL) {
        TCC_LOG_ERROR("Parameter \"config\" is NULL");
        return NULL;
    }
    if (index >= config->count) {
        TCC_LOG_ERROR("Index out of range");
        return NULL;
    }
    return &(config->items[index]);
}

void tcc_config_destroy(tcc_config_t* config)
{
    TCC_TRACE_FUNC("%p", config);
    if (config == NULL) {
        return;
    }
    free(config->items);
    free(config);
}

static tcc_config_t* parse_config(const char* filename)
{
    TCC_TRACE_FUNC("filename=%s", SAFENULLSTR(filename));
    int fd = -1;
    tcc_config_t* config = NULL;
    if (filename == NULL) {
        TCC_LOG_ERROR("Parameter \"filename\" is NULL");
        goto exit;
    }

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        TCC_LOG_ERROR("Unable to open configuration file %s: %s(%d)", filename, strerror(errno), errno);
        goto exit;
    }

    config = json_parse_file(fd);
    if (config == NULL) {
        TCC_LOG_ERROR("Unable to parse configuration file");
        goto exit;
    }

exit:
    if (fd != -1) {
        close(fd);
    }
    return config;
}

static tcc_config_t* json_parse_file(int fd)
{
    TCC_TRACE_FUNC("fd=%i", fd);
    tcc_config_t* config = NULL;
    struct json_object* json = NULL;

    if (fd == -1) {
        TCC_LOG_ERROR("Invalid file descriptor. fd=%i", fd);
        goto err;
    }

    if ((json = json_object_from_fd(fd)) == NULL) {
        TCC_LOG_ERROR("Failed to create a JSON object from descriptor.");
        goto err;
    }

    int items_in_config = json_object_object_length(json);
    if (items_in_config < 0) {
        TCC_LOG_ERROR("Wrong number of items in config. "
                      "Possible, JSON parsing issue");
        goto err;
    }

    if ((config = allocate_config(items_in_config)) == NULL) {
        goto err;
    }

    size_t index = 0;
    struct json_object_iterator it = json_object_iter_begin(json);
    struct json_object_iterator it_end = json_object_iter_end(json);

    while (!json_object_iter_equal(&it, &it_end)) {
        const char* level_name = json_object_iter_peek_name(&it);
        struct json_object* level_config = json_object_iter_peek_value(&it);
        if (json_parse_level(&config->items[index], level_name, level_config) != 0) {
            TCC_LOG_ERROR("Failed to parse config item \"%s\"", level_name);
            goto err;
        }
        index++;
        json_object_iter_next(&it);
    }

    json_object_put(json);  // destructor
    return config;

err:
    if (json != NULL) {
        json_object_put(json);  // destructor
    }
    tcc_config_destroy(config);
    return NULL;
}

static enum tcc_memory_level str_to_mem_level(const char* str)
{
    TCC_TRACE_FUNC("str=%s", SAFENULLSTR(str));

    if (strncmp(str, "L1", sizeof("L1")) == 0) {
        return TCC_MEM_L1;
    }
    if (strncmp(str, "L2", sizeof("L2")) == 0) {
        return TCC_MEM_L2;
    }
    if (strncmp(str, "L3", sizeof("L3")) == 0) {
        return TCC_MEM_L3;
    }
    if (strncmp(str, "EDRAM", sizeof("EDRAM")) == 0) {
        return TCC_MEM_EDRAM;
    }
    if (strncmp(str, "DRAM", sizeof("DRAM")) == 0) {
        return TCC_MEM_DRAM;
    }
    TCC_LOG_WARNING("Failed to get memory level from string. str=%s", SAFENULLSTR(str));
    return TCC_MEM_UNKNOWN;
}

static tcc_config_t* allocate_config(size_t items_count)
{
    TCC_TRACE_FUNC("items_count=%zu", items_count);

    tcc_config_t* config = (tcc_config_t*)malloc(sizeof(tcc_config_t));
    if (config == NULL) {
        TCC_LOG_ERROR("Unable to allocate config structure %s(%d)", strerror(errno), errno);
        return NULL;
    }

    config->count = items_count;
    config->items = (tcc_config_item_t*)calloc(items_count, sizeof(tcc_config_item_t));
    if (config->items == NULL) {
        TCC_LOG_ERROR("Unable to allocate config structure %s(%d)", strerror(errno), errno);
        free(config);
        return NULL;
    }
    return config;
}

static int json_parse_level(tcc_config_item_t* item, const char* level_name, json_object* level_config)
{
    TCC_TRACE_FUNC("item=%p level_name=%s level_config=%p", item, level_name, level_config);

    int64_t tmp_size = 0;

    item->level = str_to_mem_level(level_name);
    if (item->level == TCC_MEM_UNKNOWN) {
        TCC_LOG_ERROR("Wrong memory level \"%s\" in the config file", level_name);
        goto err;
    }

    if (getInt64(level_config, "buffer_size_bytes", &tmp_size) != 0) {
        TCC_LOG_ERROR("Memory level \"%s\" has no \"buffer_size_bytes\" field in the config file", level_name);
        goto err;
    }
    item->size = (size_t)tmp_size;

    return 0;

err:
    return -1;
}

static int getInt64(struct json_object* parent, const char* key, int64_t* result)
{
    TCC_TRACE_FUNC("parent=%p key=%s result=%ld", parent, key, *result);

    struct json_object* val = NULL;
    if (key != NULL) {
        if (json_object_object_get_ex(parent, key, &val) == TRUE) {
            *result = json_object_get_int64(val);
            return 0;
        }
    }
    goto err;

err:
    return -1;
}
