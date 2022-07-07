/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "json_helper.h"

#include "opcua_utils.h"

#ifndef TRUE
#define TRUE 1
#define FALSE (!TRUE)
#endif

typedef struct json_object JsObj;

static void ensureType(JsObj* toCheck, json_type type, JsObj* parent, const char* key)
{
    json_type objType = json_object_get_type(toCheck);
    if (objType != type) {
        LOG_ERR("Key '%s' in object '%s' has invalid type", key, json_object_to_json_string(parent));
        exit(EXIT_FAILURE);
    }
}

char* getOptionalStr(struct json_object* json, const char* key, const char* defaultVal)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_string, json, key);
        return strdup(json_object_get_string(val));
    }

    return strdup(defaultVal);
}

int getOptionalInt(struct json_object* json, const char* key, const int defaultVal)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_int, json, key);
        return json_object_get_int(val);
    }

    return defaultVal;
}

bool getOptionalBool(struct json_object* json, const char* key, const bool defaultVal)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_boolean, json, key);
        return json_object_get_boolean(val);
    }

    return defaultVal;
}

int64_t getOptionalInt64(struct json_object* json, const char* key, const int64_t defaultVal)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_int, json, key);
        return json_object_get_int64(val);
    }

    return defaultVal;
}

char* getString(struct json_object* json, const char* key)
{
    char* result = NULL;
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_string, json, key);
        result = strdup(json_object_get_string(val));
        if (!result) {
            LOG_ERR(
                "No memory for JSON value, key '%s' in object '%s'", key ?: "(null)", json_object_to_json_string(json));
            exit(EXIT_FAILURE);
        }
        return result;
    }

    LOG_ERR("Key '%s' not found in object '%s'", key ?: "(null)", json_object_to_json_string(json));
    exit(EXIT_FAILURE);
}

int getInt(struct json_object* json, const char* key)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_int, json, key);
        return json_object_get_int(val);
    }

    LOG_ERR("Key '%s' not found in object '%s'", key ?: "(null)", json_object_to_json_string(json));
    exit(EXIT_FAILURE);
}

bool getBool(struct json_object* json, const char* key)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_boolean, json, key);
        return json_object_get_boolean(val);
    }

    LOG_ERR("Key '%s' not found in object '%s'", key ?: "(null)", json_object_to_json_string(json));
    exit(EXIT_FAILURE);
}

int64_t getInt64(struct json_object* json, const char* key)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        ensureType(val, json_type_int, json, key);
        return json_object_get_int64(val);
    }

    LOG_ERR("Key '%s' not found in object '%s'", key ?: "(null)", json_object_to_json_string(json));
    exit(EXIT_FAILURE);
}

int countChildrenEntries(struct json_object* json)
{
    if (json == NULL) {
        LOG_ERR("JSON object cannot be NULL");
        exit(EXIT_FAILURE);
    }
    if (json_object_get_type(json) != json_type_object) {
        LOG_ERR("Cannot count children entries for non 'json_type_object'");
        exit(EXIT_FAILURE);
    }
    return json_object_object_length(json);
}

struct json_object* getValue(struct json_object* json, const char* key)
{
    json_bool found = FALSE;
    JsObj* val = NULL;

    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        return val;
    }

    LOG_ERR("Key '%s' not found in object '%s'", key ?: "(null)", json_object_to_json_string(json));
    exit(EXIT_FAILURE);
}

struct json_object* getOptionalValue(struct json_object* json, const char* key)
{
    json_bool found = FALSE;
    JsObj* val = NULL;
    if (key != NULL) {
        found = json_object_object_get_ex(json, key, &val);
    }

    if (found) {
        return val;
    }
    return NULL;
}

/* vim: set ts=4 sw=4 et: */
