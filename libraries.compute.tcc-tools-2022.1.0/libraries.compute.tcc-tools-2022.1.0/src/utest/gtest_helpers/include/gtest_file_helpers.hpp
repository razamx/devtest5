/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef __GTEST_FILE_HELPERS__
#define __GTEST_FILE_HELPERS__


#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string>
#include <memory>
#include <functional>

#include <iostream>

/**
 * Helper functions for testing files operations.
 *
 * create_* functions:
 * Create file descriptor/FILE* to the fake file in memory. Pass this descriptors
 * as a return values for mocked open/fopen functions.
 *
 * close functions:
 * Obtain data from fake file. Use it instead of normal close/fclose by passing
 * to the Invoke()
 */
namespace fake_file
{

int create_fd();
int create_fd(const void* data, size_t size);
int create_fd(const std::string& data);

FILE* create_file();
FILE* create_file(const void* data, size_t size);
FILE* create_file(const std::string& data);

/**
 * Create a close() function which obtain data from fake file and close file
 * descriptor
 * @param [out] data pointer to the data written to the file
 * @retval close() function with same signature as POSIX close
 *
 * NOTE: make sure that "data" object still alive when close() will be called
 */
std::function<int(int)> make_close_fd(std::unique_ptr<uint8_t[]>& data);

}

#endif
