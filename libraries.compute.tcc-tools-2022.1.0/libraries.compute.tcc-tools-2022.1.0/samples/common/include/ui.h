/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_UI_H_
#define _TCC_UI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draws a progress-bar using pseudographics in the standart output
 * @param percentage Progress par value [0; 1]
 */
void print_progressbar(double percentage);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif  //_TCC_UI_H_
