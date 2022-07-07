/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_MEASUREMENT_HELPERS_H_
#define _TCC_MEASUREMENT_HELPERS_H_

#include "measurement.h"


#ifdef NO_TCC_MEASUREMENT
#define USE_TCC_MEASUREMENT 0
#else
#define USE_TCC_MEASUREMENT 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if USE_TCC_MEASUREMENT
/**
 * @file measurement_helpers.h
 * @brief Intel(R) Time Coordinated Computing Tools (Intel® TCC Tools) measurement library helper functions
 *
 * The helper functions defined in this file simplify work with the environment settings,
 * controlling the behavior of the measurement library.
 */

/**
 * @brief Extracts the history buffer size from the environment variable.
 *
 * @param [in] name String identifier of the measurement
 * @retval >0 Success, Buffer size for the measurement with supplied \a name
 * as specified in the TCC_MEASUREMENTS_BUFFERS environment variable.
 * @retval 0 Measurement with supplied \a name doesn't exist
 */
size_t tcc_measurement_get_buffer_size_from_env(const char* name);

/**
 * @brief Extracts the maximum tolerable latency for the measurement from the environment variable.
 *
 * @param [in] name String identifier of the measurement
 * @retval >0 Success, Maximum tolerable latency for the measurement with supplied \a name
 * as specified in the TCC_MEASUREMENTS_BUFFERS environment variable.
 * @retval 0 Measurement with supplied \a name doesn't exist or deadline isn't set
 */
uint64_t tcc_measurement_get_deadline_from_env(const char* name);

/**
 * @brief Extracts the dump file name from the environment variable.
 *
 * @return Dumps the file name as specified by the TCC_MEASUREMENTS_DUMP_FILE environment variable
 */
const char* tcc_measurement_get_dump_file_from_env();

/**
 * @brief Extracts the time unit from the environment variable.
 *
 * @return Time unit as specified by the TCC_MEASUREMENTS_TIME_UNIT environment variable
 * or ::TCC_TU_UNKNOWN if it is not set
 */
TCC_TIME_UNIT tcc_measurement_get_time_unit_from_env();

/**
 * @brief Converts TCC_TIME_UNIT enum to string name.
 *
 * @param [in] time_units Time unit enum value
 * @return Name of the specified time unit
 * @retval "clk", "us", "ns" Valid enum value
 * @retval "invalid" Invalid enum value
 */
const char* tcc_measurement_get_string_from_time_unit(TCC_TIME_UNIT time_units);

/**
 * @brief Parses the time unit string name and converts it to the TCC_TIME_UNIT enum.
 *
 * @param [in] string Time unit name [clk, ns, us]
 * @return Value from the TCC_TIME_UNIT enum for the specified time unit name
 * @retval TCC_TU_CLK, TCC_TU_NS, TCC_TU_US Valid string
 * @retval TCC_TU_UNKNOWN Invalid string
 */
TCC_TIME_UNIT tcc_measurement_get_time_unit_from_string(const char* string);

#else  // USE_TCC_MEASUREMENT == 0

static inline const char* tcc_measurement_get_string_from_time_unit(__attribute__((unused)) TCC_TIME_UNIT time_units)
{
    return "invalid";
}

static inline TCC_TIME_UNIT tcc_measurement_get_time_unit_from_string(__attribute__((unused)) const char* string)
{
    return TCC_TU_UNKNOWN;
}

static inline size_t tcc_measurement_get_buffer_size_from_env(__attribute__((unused)) const char* name)
{
    return 0;
}

static inline uint64_t tcc_measurement_get_deadline_from_env(__attribute__((unused)) const char* name)
{
    return 0;
}

static inline const char* tcc_measurement_get_dump_file_from_env()
{
    return (const char*)NULL;
}

static inline TCC_TIME_UNIT tcc_measurement_get_time_unit_from_env()
{
    return TCC_TU_UNKNOWN;
}

#endif

#ifdef __cplusplus
}
#endif

#endif  // _TCC_MEASUREMENT_HELPERS_H_
