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
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif
#include "gtest_file_helpers.hpp"
#include <string.h>
#include <exception>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <err.h>

static int memfd_create_syscall(const char *name, unsigned int flags)
{
	return syscall(SYS_memfd_create, name, flags);
}

namespace fake_file
{

int create_fd()
{
	return memfd_create_syscall("fake_file", O_RDWR);
}

int create_fd(const void* data, size_t size)
{
	int fd = create_fd();
	write(fd, data, size);
	lseek(fd, 0, SEEK_SET);
	return fd;
}

int create_fd(const std::string& data)
{
	return create_fd(data.c_str(), data.size()+1);
}

FILE* create_file()
{
	return fdopen(create_fd(), "w+");
}

FILE* create_file(const void* data, size_t size)
{
	return fdopen(create_fd(data, size), "w+");
}

FILE* create_file(const std::string& data)
{
	return fdopen(create_fd(data), "w+");
}

std::function<int(int)> make_close_fd(std::unique_ptr<uint8_t[]>& data)
{
	return [&data](int fd) -> int {
		size_t size = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		data = std::unique_ptr<uint8_t[]>(new uint8_t[size]);
		read(fd, reinterpret_cast<void*>(data.get()), size);
		close(fd);
		return 0;
	};
}

}