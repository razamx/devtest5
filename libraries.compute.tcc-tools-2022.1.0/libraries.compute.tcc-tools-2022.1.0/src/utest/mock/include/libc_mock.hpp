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
#ifndef LIBC_MOCK_HPP
#define LIBC_MOCK_HPP

#include <errno.h>
#include "gmock/gmock.h"
#include "mock_common.hpp"
#include <sched.h>
#include <sys/types.h>
#include <pthread.h>
#include <mntent.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <string.h>
#include <time.h>

class libc_mock
{
public:
    MOCK_METHOD2(fopen, FILE*(const char* __restrict __filename, const char* __restrict __modes));
    MOCK_METHOD1(fclose, int(FILE* stream));

    MOCK_METHOD3(fprintf, int(FILE* __restrict __stream, const char* __restrict __fmt, va_list list));
    MOCK_METHOD3(vfprintf, int(FILE* __restrict __stream, const char* __restrict __fmt, va_list list));

    MOCK_METHOD4(fread, size_t(void* __ptr, size_t __size, size_t __n, FILE* __stream));
    MOCK_METHOD4(fwrite, size_t(const void* ptr, size_t size, size_t nmemb, FILE* stream));
    MOCK_METHOD1(fgetc, int(FILE* __stream));

    MOCK_METHOD1(getenv, char*(const char* __name));
    MOCK_METHOD3(setenv, int(const char* name, const char* value, int overwrite));

    MOCK_METHOD3(open, int(const char* __file, int __oflag, va_list list));
    MOCK_METHOD1(close, int(int __fd));
    MOCK_METHOD3(lseek, __off_t(int __fd, __off_t __offset, int __whence));
    MOCK_METHOD3(write, ssize_t(int __fd, const void* __buf, size_t __n));
    MOCK_METHOD3(read, ssize_t(int __fd, void* __buf, size_t __nbytes));
    MOCK_METHOD3(dprintf, int(int __fd, const char* __restrict __fmt, va_list list));

    MOCK_METHOD2(stat, int(const char* __restrict __filename, struct stat* __statbuf));
    MOCK_METHOD1(unlink, int(const char* path));

    MOCK_METHOD6(mmap, void*(void* __addr, size_t __length, int __prot, int __flags, int __fd, off_t __offset));
    MOCK_METHOD2(munmap, int(void* __addr, size_t __length));
    MOCK_METHOD3(ioctl, int(int fd, unsigned long request, va_list list));
    MOCK_METHOD4(clock_nanosleep,
        int(clockid_t clock_id, int flags, const struct timespec* request, struct timespec* remain));
    MOCK_METHOD2(nanosleep, int(const struct timespec* request, struct timespec* remain));
    MOCK_METHOD1(usleep, int(useconds_t usec));
    MOCK_METHOD2(clock_gettime, int(clockid_t clk_id, struct timespec* tp));
    MOCK_METHOD5(select,
        int(int __nfds,
            fd_set* __restrict __readfds,
            fd_set* __restrict __writefds,
            fd_set* __restrict __exceptfds,
            struct timeval* __restrict __timeout));
    MOCK_METHOD2(access, int(const char* pathname, int mode));
    MOCK_METHOD0(get_nprocs, int(void));
    MOCK_METHOD3(sched_getaffinity, int(pid_t pid, size_t cpusetsize, cpu_set_t* mask));
    MOCK_METHOD3(sched_setaffinity, int(pid_t pid, size_t cpusetsize, cpu_set_t* mask));
    MOCK_METHOD3(sched_setscheduler, int(pid_t pid, size_t cpusetsize, const cpu_set_t* cpuset));
    MOCK_METHOD3(pthread_getaffinity_np, int(pthread_t thread, size_t cpusetsize, cpu_set_t* cpuset));
    MOCK_METHOD3(pthread_setaffinity_np, int(pthread_t thread, size_t cpusetsize, const cpu_set_t* cpuset));
    MOCK_METHOD3(pthread_getschedparam, int(pthread_t thread, int* policy, struct sched_param* param));
    MOCK_METHOD3(pthread_setschedparam, int(pthread_t thread, int policy, const struct sched_param* param));
    MOCK_METHOD4(pread, ssize_t(int fd, void* buf, size_t count, off_t offset));
    MOCK_METHOD4(pwrite, ssize_t(int fd, void* buf, size_t count, off_t offset));
    MOCK_METHOD2(ftruncate, int(int fd, off_t length));

    MOCK_METHOD1(exit, void(int status));
    MOCK_METHOD5(mount,
        int(const char* source,
            const char* target,
            const char* filesystemtype,
            unsigned long mountflags,
            const void* data));

    MOCK_METHOD2(dlopen, void*(const char* filename, int flags));
    MOCK_METHOD2(dlsym, void*(void* handle, const char* symbol));
    MOCK_METHOD0(dlerror, char*());
    MOCK_METHOD1(dlclose, int(void* handle));

    MOCK_METHOD2(setmntent, FILE*(const char* filename, const char* type));
    MOCK_METHOD1(getmntent, struct mntent*(FILE* fp));
    MOCK_METHOD1(endmntent, int(FILE*));

    MOCK_METHOD1(iopl, int(int lvl));

    MOCK_METHOD2(gettimeofday, int(timeval* __restrict__ __tv, void* __restrict__ __tz));
    MOCK_METHOD1(localtime, tm*(const time_t* __timer));

    MOCK_METHOD1(mlockall, int(int flags));
    MOCK_METHOD0(munlockall, int(void));

    MOCK_METHOD2(memfd_create, int(const char* name, unsigned int flags));

    MOCK_METHOD1(uname, int(struct utsname* buf));

    MOCK_METHOD2(strrchr, char*(char* str, int character));

    COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(libc_mock)
};

extern "C" {
FILE* __real_fopen(const char* __restrict __filename, const char* __restrict __modes);
int __real_fclose(FILE* stream);
int __real_fprintf(FILE* __restrict __stream, const char* __restrict __fmt, ...);
int __real_vfprintf(FILE* stream, const char* format, va_list ap);
size_t __real_fread(void* __ptr, size_t __size, size_t __n, FILE* __stream);
size_t __real_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int __real_fgetc(FILE* stream);

char* __real_getenv(const char* __name);
int __real_setenv(const char* name, const char* value, int overwrite);


__off_t __real_lseek(int __fd, __off_t __offset, int __whence);
ssize_t __real_write(int __fd, const void* __buf, size_t __n);
ssize_t __real_read(int __fd, void* __buf, size_t __nbytes);
int __real_open(const char* __file, int __oflag, ...);
int __real_close(int __fd);
int __real_dprintf(int __fd, const char* __restrict __fmt, ...);

int __real_stat(const char* __restrict __filename, struct stat* __statbuf);
int __real_unlink(const char* pathname);

void* __real_mmap(void* __addr, size_t __length, int __prot, int __flags, int __fd, off_t __offset);
int __real_munmap(void* __addr, size_t __length);

int __real_ioctl(int fd, unsigned long request, ...);

int __real_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* request, struct timespec* remain);
int __real_nanosleep(const struct timespec* request, struct timespec* remain);
int __real_usleep(useconds_t usec);
int __real_clock_gettime(clockid_t clk_id, struct timespec* tp);

int __real_select(int __nfds,
    fd_set* __restrict __readfds,
    fd_set* __restrict __writefds,
    fd_set* __restrict __exceptfds,
    struct timeval* __restrict __timeout);
int __real_access(const char* pathname, int mode);

int __real_pthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t* cpuset);
int __real_pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t* cpuset);
int __real_pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param);
int __real_pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param);

int __real_ftruncate(int fd, off_t length);

int __real_get_nprocs(void);

ssize_t __real_pread(int fd, void* buf, size_t count, off_t offset);
ssize_t __real_pwrite(int fd, const void* buf, size_t count, off_t offset);

void __real_exit(int status);

int __real_mount(const char* source,
    const char* target,
    const char* filesystemtype,
    unsigned long mountflags,
    const void* data);

void* __real_dlopen(const char* filename, int flags);
void* __real_dlsym(void* handle, const char* symbol);
char* __real_dlerror(void);
int __real_dlclose(void* handle);

FILE* __real_setmntent(const char* filename, const char* type);
struct mntent* __real_getmntent(FILE* fp);
int __real_endmntent(FILE* fp);

int __real_iopl(int lvl);

int __real_gettimeofday(timeval* __restrict__ __tv, void* __restrict__ __tz);
tm* __real_localtime(const time_t* __timer);

int __real_mlockall(int flags);
int __real_munlockall(void);

int __real_memfd_create(const char* name, unsigned int flags);

int __real_uname(struct utsname* buf);

char* __real_strrchr(char* str, int character);
}

#endif  // LIBC_MOCK_HPP
