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
#include "alloc_mock.hpp"

using namespace ::testing;

void* __wrap_malloc(size_t size)
{
	return ALLOW_DEFAULT_MOCK(alloc_mock, malloc(size));
}

void __wrap_free(void *ptr)
{
	return ALLOW_DEFAULT_MOCK(alloc_mock, free(ptr));
}

void* __wrap_calloc(size_t num, size_t size)
{
	return ALLOW_DEFAULT_MOCK(alloc_mock, calloc(num, size));
}

void* __wrap_realloc(void* ptr, size_t size)
{
	return  ALLOW_DEFAULT_MOCK(alloc_mock, realloc(ptr, size));
}

char* __wrap_strdup(const char* __s)
{
	return  ALLOW_DEFAULT_MOCK(alloc_mock, strdup(__s));
}

int __wrap_asprintf(char **strp, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int status;
	if(alloc_mock::m_current != nullptr){
		status = alloc_mock::get().asprintf(strp, fmt, args);
	}else{
		TCC_LOG_WARNING("called without mock");
		status = __real_vasprintf(strp, fmt, args);
	}
	va_end(args);
	return status;
}
int __wrap_vasprintf(char **strp, const char *fmt, va_list ap)
{
	return ALLOW_DEFAULT_MOCK(alloc_mock, vasprintf(strp, fmt, ap));
}

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(alloc_mock)

void alloc_mock::init()
{
	ON_CALL(*this, malloc(_)).WillByDefault(Invoke(__real_malloc));
	ON_CALL(*this, calloc(_, _)).WillByDefault(Invoke(__real_calloc));
	ON_CALL(*this, free(_)).WillByDefault(Invoke(__real_free));
	ON_CALL(*this, realloc(_, _)).WillByDefault(Invoke(__real_realloc));
	ON_CALL(*this, strdup(_)).WillByDefault(Invoke(__real_strdup));
	ON_CALL(*this, asprintf(_, _, _)).WillByDefault(Invoke(__real_vasprintf));
	ON_CALL(*this, vasprintf(_, _, _)).WillByDefault(Invoke(__real_vasprintf));
}
