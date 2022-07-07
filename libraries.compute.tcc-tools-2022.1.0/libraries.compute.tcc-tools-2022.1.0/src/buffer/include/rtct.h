/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/
#ifndef _TCC_RTCT_H_
#define _TCC_RTCT_H_

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct rtct_header_t
{
    uint16_t format;
    uint16_t size;
    int type;
} rtct_header_t;

/**
 * @brief Function that tries to find an RTCT element with known type
 * @param RTCT pointer to first element of RTCT table
 * @param size size of RTCT table
 * @param type type of entry that should be found
 * @param prev previous result used to find multiple members with the same type. Otherwise set to NULL
 * @return The pointer to found entry with required type or NULL if not found
 */
rtct_header_t* rtct_get_next(rtct_header_t* rtct, size_t size, int type, rtct_header_t* prev);

#ifdef __cplusplus
}  // closing brace for extern "C"
#endif

#endif  // _TCC_RTCT_H_
