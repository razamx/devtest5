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
#ifndef ALLOC_MOCK_HPP
#define ALLOC_MOCK_HPP

#include "gmock/gmock.h"
#include "mock_common.hpp"
#include "errno.h"
#include <cstdarg>

#define RETURN_NULL() Invoke([](...){errno = ENOMEM; return nullptr;})

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnNull;

extern "C"
{
void *__wrap_malloc(size_t size);
void *__real_malloc(size_t size);

void __wrap_free(void *ptr);
void __real_free(void *ptr);

void* __wrap_calloc(size_t num, size_t size);
void* __real_calloc(size_t num, size_t size);

void* __wrap_realloc(void *ptr, size_t size);
void* __real_realloc(void *ptr, size_t size);

char* __wrap_strdup(const char*);
char* __real_strdup(const char*);

int __wrap_asprintf(char **strp, const char *fmt, ...);
int __real_asprintf(char **strp, const char *fmt, ...);

int __wrap_vasprintf(char **strp, const char *fmt, va_list ap);
int __real_vasprintf(char **strp, const char *fmt, va_list ap);

}

struct alloc_mock {

	MOCK_METHOD1(malloc, void*(size_t size));
	MOCK_METHOD2(calloc, void*(size_t num , size_t size));
	MOCK_METHOD1(free, void(void* ptr));
	MOCK_METHOD2(realloc, void*(void* ptr, size_t size));
	MOCK_METHOD1(strdup, char*(const char *__s));
	MOCK_METHOD3(asprintf, int(char **strp, const char *fmt, va_list list));
	MOCK_METHOD3(vasprintf, int(char **strp, const char *fmt, va_list list));

	COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(alloc_mock)
};

#endif // ALLOC_MOCK_HPP
