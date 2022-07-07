
/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_VERSION_H_
#define _TCC_VERSION_H_

/*confuguration start */

#ifndef TCC_VER_MAJOR
#define TCC_VER_MAJOR 0
#endif

#ifndef TCC_VER_MINOR
#define TCC_VER_MINOR 0
#endif

#ifndef TCC_VER_PATCH
#define TCC_VER_PATCH 0
#endif

#ifndef CURRENT_DATE
#define CURRENT_DATE 02.71.82
#endif

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH local_build
#endif
/*confuguration end  */

#define TO_STR_T(A) #A
#define TO_STR(A) TO_STR_T(A)

#define TCC_GIT_COMMIT_HASH TO_STR(GIT_COMMIT_HASH)
#define TCC_CURRENT_DATE TO_STR(CURRENT_DATE)

#define CAT3_T(A, B, C) A##.##B##.##C
#define CAT3(A, B, C) CAT3_T(A, B, C)

#define CAT4_T(A, B, C, D) A##.##B##.##C##.##D
#define CAT4(A, B, C, D) CAT4_T(A, B, C, D)

#define CAT5_T(A, B, C, D, E) A##.##B##.##C##.##D##.##E
#define CAT5(A, B, C, D, E) CAT5_T(A, B, C, D, E)

#define TCC_VERSION TO_STR(CAT4(TCC_VER_MAJOR, TCC_VER_MINOR, TCC_VER_PATCH, TCC_BUILD_ID))
#define TCC_VERSION_WITH_DETAILS TO_STR(CAT4(TCC_VER_MAJOR, TCC_VER_MINOR, TCC_VER_PATCH, GIT_COMMIT_HASH))
#define TCC_VERSION_STRING static volatile char version_id[] = "VERSION: " TCC_VERSION_WITH_DETAILS;
TCC_VERSION_STRING

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"

#define TCC_BUILD_STRING static volatile char build[] = "BUILD DATE: " __DATE__ " " __TIME__;
TCC_BUILD_STRING

#pragma GCC diagnostic pop

#endif  // _TCC_VERSION_H_
