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
#include "libc_mock.hpp"
#include <stdarg.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "test_log.hpp"

#define UNUSED(x) ((void)x)

using namespace ::testing;

extern "C" {
FILE* __wrap_fopen(const char* __restrict __filename, const char* __restrict __modes);
int __wrap_fclose(FILE* stream);

int __wrap_fprintf(FILE* __restrict __stream, const char* __restrict __fmt, ...);
int __wrap___fprintf_chk(FILE* stream, int flag, const char* format, ...);
int __wrap_vfprintf(FILE* stream, const char* format, va_list ap);

size_t __wrap_fread(void* __ptr, size_t __size, size_t __n, FILE* __stream);
size_t __wrap_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int __wrap_fgetc(FILE* __stream);

char* __wrap_getenv(const char* __name);
int __wrap_setenv(const char* name, const char* value, int overwrite);

__off_t __wrap_lseek(int __fd, __off_t __offset, int __whence);

int __wrap_open(const char* __file, int __oflag, ...);
ssize_t __wrap_write(int __fd, const void* __buf, size_t __n);
ssize_t __wrap_read(int __fd, void* __buf, size_t __nbytes);
int __wrap_ioctl(int fd, unsigned long request, ...);
int __wrap_close(int __fd);

int __wrap_dprintf(int __fd, const char* __restrict __fmt, ...);

int __wrap_stat(const char* __restrict __filename, struct stat* __statbuf);
int __wrap_unlink(const char* path);

void* __wrap_mmap(void* __addr, size_t __length, int __prot, int __flags, int __fd, off_t __offset);

int __wrap_munmap(void* __addr, size_t __length);

int __wrap_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* request, struct timespec* remain);

int __wrap_nanosleep(const struct timespec* request, struct timespec* remain);

int __wrap_usleep(useconds_t usec);

int __wrap_clock_gettime(clockid_t clk_id, struct timespec* tp);

int __wrap_select(int __nfds,
    fd_set* __restrict __readfds,
    fd_set* __restrict __writefds,
    fd_set* __restrict __exceptfds,
    struct timeval* __restrict __timeout);

int __wrap_access(const char* pathname, int mode);
int __wrap_get_nprocs();
int __wrap_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t* mask);
int __wrap_sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t* mask);
int __wrap_sched_setscheduler(pid_t pid, size_t cpusetsize, const cpu_set_t* cpuset);


int __wrap_pthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t* cpuset);
int __wrap_pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t* cpuset);

int __wrap_pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param);
int __wrap_pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param);

ssize_t __wrap_pread(int fd, void* buf, size_t count, off_t offset);
ssize_t __wrap_pwrite(int fd, void* buf, size_t count, off_t offset);

int __wrap_ftruncate(int fd, off_t length);

void __wrap_exit(int status);
int __wrap_mount(const char* source,
    const char* target,
    const char* filesystemtype,
    unsigned long mountflags,
    const void* data);

void* __wrap_dlopen(const char* filename, int flags);
void* __wrap_dlsym(void* handle, const char* symbol);
char* __wrap_dlerror(void);
int __wrap_dlclose(void* handle);

FILE* __wrap_setmntent(const char* filename, const char* type);
struct mntent* __wrap_getmntent(FILE* fp);
int __wrap_endmntent(FILE* fp);

int __wrap_iopl(int lvl);

int __wrap_gettimeofday(timeval* __restrict__ __tv, void* __restrict__ __tz);
tm* __wrap_localtime(const time_t* __timer);

int __wrap_mlockall(int flags);
int __wrap_munlockall(void);

int __wrap_memfd_create(const char* name, unsigned int flags);

int __wrap_uname(struct utsname* buf);

char* __wrap_strrchr(char* str, int character);
}

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(libc_mock)

#define get() get(__PRETTY_FUNCTION__)

void libc_mock::init()
{
    ON_CALL(*this, fopen(_, _)).WillByDefault(ReturnNull());
    ON_CALL(*this, fclose(_)).WillByDefault(Return(EOF));
    ON_CALL(*this, fprintf(_, _, _)).WillByDefault(Return(-1));
    EXPECT_CALL(*this, vfprintf(_, _, _)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*this, fwrite(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_fwrite));
    ON_CALL(*this, fread(_, _, _, _)).WillByDefault(Return(0));
    EXPECT_CALL(*this, fgetc(_)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_fgetc));

    ON_CALL(*this, getenv(NotNull())).WillByDefault(ReturnNull());
    ON_CALL(*this, setenv(_, _, _)).WillByDefault(Invoke(__real_setenv));

    ON_CALL(*this, lseek(_, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, write(_, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, read(_, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, open(NotNull(), _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, close(_)).WillByDefault(Return(-1));
    ON_CALL(*this, dprintf(_, _, _)).WillByDefault(Return(-1));

    ON_CALL(*this, stat(_, _)).WillByDefault(Return(-1));
    ON_CALL(*this, unlink(_)).WillByDefault(Return(-1));

    ON_CALL(*this, mmap(_, _, _, _, _, _)).WillByDefault(Return(MAP_FAILED));
    ON_CALL(*this, munmap(_, _)).WillByDefault(Return(-1));
    ON_CALL(*this, ioctl(_, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, clock_nanosleep(_, _, _, _)).WillByDefault(Return(EINVAL));
    ON_CALL(*this, nanosleep(_, _)).WillByDefault(Return(EINVAL));
    ON_CALL(*this, usleep(_)).WillByDefault(Return(-1));
    ON_CALL(*this, clock_gettime(_, _)).WillByDefault(Return(-1));
    ON_CALL(*this, select(_, _, _, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, access(_, _)).WillByDefault(Return(-1));
    ON_CALL(*this, get_nprocs()).WillByDefault(Return(1));
    ON_CALL(*this, sched_getaffinity(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, sched_setaffinity(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, sched_setscheduler(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, pthread_getaffinity_np(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, pthread_setaffinity_np(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, pthread_getschedparam(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, pthread_setschedparam(_, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, pread(_, _, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, pwrite(_, _, _, _)).WillByDefault(Return(-1));
    ON_CALL(*this, exit(_)).WillByDefault(Invoke(__real_exit));
    ON_CALL(*this, mount(_, _, _, _, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, dlopen(_, _)).WillByDefault(Return(nullptr));
    ON_CALL(*this, dlsym(_, _)).WillByDefault(Return(nullptr));
    ON_CALL(*this, dlerror()).WillByDefault(Return(nullptr));
    ON_CALL(*this, dlclose(_)).WillByDefault(Return(-1));
    ON_CALL(*this, setmntent(_, _)).WillByDefault(Return(nullptr));
    ON_CALL(*this, getmntent(_)).WillByDefault(Return(nullptr));
    ON_CALL(*this, endmntent(_)).WillByDefault(Return(1));
    ON_CALL(*this, iopl(_)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));

    ON_CALL(*this, gettimeofday(_, _)).WillByDefault(MOCK_RETURN_VAL_SET_ERRNO(-1, -EPERM));
    ON_CALL(*this, localtime(_)).WillByDefault(Return(nullptr));

    ON_CALL(*this, mlockall(_)).WillByDefault(Return(-1));
    ON_CALL(*this, munlockall()).WillByDefault(Return(-1));
    ON_CALL(*this, memfd_create(_, _)).WillByDefault(Invoke(__real_memfd_create));

    EXPECT_CALL(*this, uname(_)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_uname));

    EXPECT_CALL(*this, strrchr(_, _)).Times(AnyNumber()).WillRepeatedly(Invoke(__real_strrchr));
}

FILE* __wrap_fopen(const char* __restrict __filename, const char* __restrict __modes)
{
    return ALLOW_DEFAULT_MOCK(libc_mock, fopen(__filename, __modes));
}

int __wrap_fclose(FILE* stream)
{
    return ALLOW_DEFAULT_MOCK(libc_mock, fclose(stream));
}

int __wrap_fprintf(FILE* __restrict __stream, const char* __restrict __fmt, ...)
{
    va_list args;
    va_start(args, __fmt);
    int status = libc_mock::get().fprintf(__stream, __fmt, args);
    va_end(args);
    return status;
}

int __wrap___fprintf_chk(FILE* stream, int flag, const char* format, ...)
{
    UNUSED(flag);
    va_list args;
    va_start(args, format);
    if (libc_mock::m_current != nullptr) {
        int status = libc_mock::get().fprintf(stream, format, args);
        return status;
    }
    int status = __real_vfprintf(stream, format, args);
    va_end(args);
    return status;
}

int __wrap_vfprintf(FILE* __restrict __stream, const char* __restrict __fmt, va_list args)
{
    return libc_mock::get().vfprintf(__stream, __fmt, args);
}


size_t __wrap_fread(void* __ptr, size_t __size, size_t __n, FILE* __stream)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, fread(__ptr, __size, __n, __stream));
}

size_t __wrap_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, fwrite(ptr, size, nmemb, stream));
}

int __wrap_fgetc(FILE* stream)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, fgetc(stream));
}

char* __wrap_getenv(const char* __name)
{
    return ALLOW_DEFAULT_MOCK(libc_mock, getenv(__name));
}

int __wrap_setenv(const char* name, const char* value, int overwrite)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, setenv(name, value, overwrite));
}

__off_t __wrap_lseek(int __fd, __off_t __offset, int __whence)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().lseek(__fd, __offset, __whence);
}

ssize_t __wrap_write(int __fd, const void* __buf, size_t __n)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, write(__fd, __buf, __n));
}

ssize_t __wrap_read(int __fd, void* __buf, size_t __nbytes)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, read(__fd, __buf, __nbytes));
}

int __wrap_open(const char* __file, int __oflag, ...)
{
    va_list args;
    va_start(args, __oflag);
    va_end(args);
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, open(__file, __oflag, args));
}

int __wrap_close(int __fd)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, close(__fd));
}

int __wrap_dprintf(int __fd, const char* __restrict __fmt, ...)
{
    va_list args;
    va_start(args, __fmt);
    va_end(args);
    TEST_LOG_TRACE("called");
    return libc_mock::get().dprintf(__fd, __fmt, args);
}

int __wrap_stat(const char* __restrict __filename, struct stat* __statbuf)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, stat(__filename, __statbuf));
}

int __wrap_unlink(const char* path)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, unlink(path));
}

void* __wrap_mmap(void* __addr, size_t __length, int __prot, int __flags, int __fd, off_t __offset)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().mmap(__addr, __length, __prot, __flags, __fd, __offset);
}

int __wrap_munmap(void* __addr, size_t __length)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().munmap(__addr, __length);
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
    va_list args;
    va_start(args, request);
    va_end(args);
    TEST_LOG_TRACE("called");
    return libc_mock::get().ioctl(fd, request, args);
}

int __wrap_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec* request, struct timespec* remain)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().clock_nanosleep(clock_id, flags, request, remain);
}

int __wrap_nanosleep(const struct timespec* request, struct timespec* remain)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().nanosleep(request, remain);
}

int __wrap_usleep(useconds_t usec)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().usleep(usec);
}

int __wrap_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, clock_gettime(clk_id, tp));
}

int __wrap_select(int __nfds,
    fd_set* __restrict __readfds,
    fd_set* __restrict __writefds,
    fd_set* __restrict __exceptfds,
    struct timeval* __restrict __timeout)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, select(__nfds, __readfds, __writefds, __exceptfds, __timeout));
}

int __wrap_access(const char* pathname, int mode)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, access(pathname, mode));
}

int __wrap_get_nprocs()
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().get_nprocs();
}

int __wrap_sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t* mask)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().sched_getaffinity(pid, cpusetsize, mask);
}

int __wrap_sched_setaffinity(pid_t pid, size_t cpusetsize, cpu_set_t* mask)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().sched_setaffinity(pid, cpusetsize, mask);
}

int __wrap_sched_setscheduler(pid_t pid, size_t cpusetsize, const cpu_set_t* cpuset)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().sched_setscheduler(pid, cpusetsize, cpuset);
}

int __wrap_pthread_getaffinity_np(pthread_t thread, size_t cpusetsize, cpu_set_t* cpuset)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, pthread_getaffinity_np(thread, cpusetsize, cpuset));
}

int __wrap_pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t* cpuset)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, pthread_setaffinity_np(thread, cpusetsize, cpuset));
}

int __wrap_pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, pthread_getschedparam(thread, policy, param));
}

int __wrap_pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, pthread_setschedparam(thread, policy, param));
}

ssize_t __wrap_pread(int fd, void* buf, size_t count, off_t offset)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().pread(fd, buf, count, offset);
}

ssize_t __wrap_pwrite(int fd, void* buf, size_t count, off_t offset)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().pwrite(fd, buf, count, offset);
}

int __wrap_ftruncate(int fd, off_t length)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, ftruncate(fd, length));
}

void __wrap_exit(int status)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, exit(status));
}

int __wrap_mount(const char* source,
    const char* target,
    const char* filesystemtype,
    unsigned long mountflags,
    const void* data)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, mount(source, target, filesystemtype, mountflags, data));
}

void* __wrap_dlopen(const char* filename, int flags)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().dlopen(filename, flags);
}

void* __wrap_dlsym(void* handle, const char* symbol)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().dlsym(handle, symbol);
}

char* __wrap_dlerror(void)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().dlerror();
}

int __wrap_dlclose(void* handle)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().dlclose(handle);
}

FILE* __wrap_setmntent(const char* filename, const char* type)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().setmntent(filename, type);
}

struct mntent* __wrap_getmntent(FILE* fp)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().getmntent(fp);
}

int __wrap_endmntent(FILE* fp)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().endmntent(fp);
}

int __wrap_iopl(int lvl)
{
    TEST_LOG_TRACE("called");
    return libc_mock::get().iopl(lvl);
}

int __wrap_gettimeofday(timeval* __restrict__ __tv, void* __restrict__ __tz)
{
    return ALLOW_DEFAULT_MOCK(libc_mock, gettimeofday(__tv, __tz));
}

tm* __wrap_localtime(const time_t* __timer)
{
    return ALLOW_DEFAULT_MOCK(libc_mock, localtime(__timer));
}

int __wrap_mlockall(int flags)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, mlockall(flags));
}

int __wrap_munlockall(void)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, munlockall());
}

int __wrap_memfd_create(const char* name, unsigned int flags)
{
    return ALLOW_DEFAULT_MOCK(libc_mock, memfd_create(name, flags));
}

int __wrap_uname(struct utsname* buf)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, uname(buf));
}

char* __wrap_strrchr(char* str, int character)
{
    TEST_LOG_TRACE("called");
    return ALLOW_DEFAULT_MOCK(libc_mock, strrchr(str, character));
}
