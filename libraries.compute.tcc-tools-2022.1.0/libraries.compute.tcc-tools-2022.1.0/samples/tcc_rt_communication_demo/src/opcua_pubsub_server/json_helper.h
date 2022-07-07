/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_JSON_HELPER_H_
#define _TCC_JSON_HELPER_H_

/**
 * @file json_helper.h
 * @brief This header file defines the API for reading configuration settings in JSON files
 */

#include <json-c/json.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 * Get an optional JSON string value
 *
 * @param[in] json        The pointer to the JSON object for reading its child string element
 * @param[in] key         The child element key to read its string value
 * @param[in] defaultVal  The default value to be returned if the child element with \c key does
 *            not exist
 * @return The newly allocated string value corresponding to the child matching \c key.
 *         The returned string should be freed with \c free call.
 * @note If the child element with \c key exists, but its type is not \c string, a fatal error
 *       will be reported using \c log_error and the current process will finish. This needs
 *       to be fixed in the configuration file.
 */
char* getOptionalStr(struct json_object* json, const char* key, const char* defaultVal);

/**
 * Get an optional JSON integer value
 *
 * @param[in] json        The pointer to the JSON object for reading its child integer element
 * @param[in] key         The key string to search the child element for reading its integer value
 * @param[in] defaultVal  The default value to be returned if the parameter with \c key does
 *            not exist
 * @return The value of the \c key integer element if it exists or \c defaultVal
 * @note If the child element with \c key exists, but its type is not \c integer, a fatal error
 *       will be reported using \c log_error and the current process will finish. The type of
 *       the element needs to be updated in the configuration file.
 */
int getOptionalInt(struct json_object* json, const char* key, const int defaultVal);

/**
 * Get an optional JSON Boolean value
 *
 * @param[in] json        The pointer to the JSON object for reading its child Boolean element
 * @param[in] key         The key string to search the child element for reading its Boolean value
 * @param[in] defaultVal  The default value to be returned if the parameter with \c key does
 *                        not exist
 * @return The value of the \c key Boolean element if it exists or \c defaultVal
 * @note If the child element with \c key exists, but its type is not \c Boolean, a fatal error
 *       will be reported using \c log_error and the current process will finish. The type of
 *       the element needs to be updated in the configuration file.
 */
bool getOptionalBool(struct json_object* json, const char* key, const bool defaultVal);

/**
 * Get an optional JSON 64-bit integer value
 *
 * @param[in] json        The pointer to the JSON object for reading its child integer element
 * @param[in] key         The key string to search the child element for reading its integer value
 * @param[in] defaultVal  The default value to be returned if the child element with \c key does
 *                        not exist
 * @return The value of the \c key 64-bit integer element if it exists or \c defaultVal
 * @note If the child element with \c key exists, but its type is not \c integer, a fatal error
 *       will be reported using \c log_error and the current process will finish. The type of
 *       the element needs to be updated in the configuration file.
 */
int64_t getOptionalInt64(struct json_object* json, const char* key, const int64_t defaultVal);


/**
 * Get a mandatory JSON string value
 *
 * @param[in] json  The pointer to the JSON object for reading its child string element
 * @param[in] key   The child element key to read its string value
 * @return The newly allocated string value corresponding to the child matching \c key.
 *         The returned string should be freed with \c free call.
 * @note If the child element with \c key does not exist, or it exists but its type is not
 *       \c string, a fatal error will be reported using \c log_error and the current process
 *       will finish. This needs to be fixed in the configuration file.
 */
char* getString(struct json_object* json, const char* key);

/**
 * Get a mandatory JSON integer value
 *
 * @param[in] json  The pointer to the JSON object for reading its child integer element
 * @param[in] key   The key string to search the child element for reading its integer value
 * @return The value of the \c key integer element
 * @note If the child element with \c key does not exist, or it exists but its type is not
 *       \c integer, a fatal error will be reported using \c log_error and the current process
 *       will finish. The type of the element needs to be updated in the configuration file.
 */
int getInt(struct json_object* json, const char* key);

/**
 * Get a mandatory JSON Boolean value
 *
 * @param[in] json  The pointer to the JSON object for reading its child Boolean element
 * @param[in] key   The key string to search the child element for reading its Boolean value
 * @return The value of the \c key Boolean element
 * @note If the child element with \c key does not exist, or it exists but its type is not
 *       \c Boolean, a fatal error will be reported using \c log_error and the current process
 *       will finish. The type of the element needs to be updated in the configuration file.
 */
bool getBool(struct json_object* json, const char* key);

/**
 * Get a mandatory JSON 64-bit integer value
 *
 * @param[in] json  The pointer to the JSON object for reading its child integer element
 * @param[in] key   The key string to search the child element for reading its integer value
 * @return The value of the \c key 64-bit integer element
 * @note If the child element with \c key does not exist, or it exists but its type is not
 *       \c integer, a fatal error will be reported using \c log_error and the current process
 *       will finish. The type of the element needs to be updated in the configuration file.
 */
int64_t getInt64(struct json_object* json, const char* key);

/**
 * Get the number of child elements for the JSON object
 *
 * @param[in] json  The pointer to the JSON object, for which the number of child elements
 *                  needs to be calculated
 * @return The number of child elements for the \c json object
 */
int countChildrenEntries(struct json_object* json);

/**
 * Get the child element that matches the \c key for the current JSON object \c json
 *
 * @param[in] json  The pointer to the JSON object to search for the child element that matches the \c key
 * @param[in] key   The key to search for the required child element
 * @return The pointer to the child JSON object that matches the \c key
 * @note Terminates the application if the child element was not found
 */
struct json_object* getValue(struct json_object* json, const char* key);

/**
 * Get the child element that matches the \c key for the current JSON object \c json
 *
 * @param[in] json  The pointer to the JSON object to search for the child element that matches the \c key
 * @param[in] key   The key to search for the required child element
 * @return The pointer to the child JSON object that matches the \c key or NULL value if the child object was not found
 * @note Behavior differs from that of the \c getValue function. The \c getOptionalValue function does not
 *        \c terminate the application if the child element was not found, it returns NULL instead.
 */
struct json_object* getOptionalValue(struct json_object* json, const char* key);

#ifdef __cplusplus
} /* extern "C" { */
#endif /* __cplusplus */

#endif  // _TCC_JSON_HELPER_H_
