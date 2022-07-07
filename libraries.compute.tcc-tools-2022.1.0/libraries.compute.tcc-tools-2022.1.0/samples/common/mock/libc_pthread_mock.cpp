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

#include "libc_pthread_mock.hpp"

using ::testing::_;
using ::testing::Return;

COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(pthread_mock)

extern "C" pthread_t pthread_self_fake(void);
extern "C" int kill_fake(pid_t pid, int sig);

void pthread_mock::init()
{
    ON_CALL(*this, pthread_create(_, _, _, _))
        .WillByDefault(Return(-EPERM));
    ON_CALL(*this, pthread_detach(_))
        .WillByDefault(Return(-EPERM));
    ON_CALL(*this, pthread_setaffinity_np(_, _, _))
        .WillByDefault(Return(-EPERM));
    ON_CALL(*this, pthread_setschedparam(_, _, _))
        .WillByDefault(Return(-EPERM));
    ON_CALL(*this, pthread_self_fake())
        .WillByDefault(Return(0));
    ON_CALL(*this, pthread_join(_, _))
        .WillByDefault(Return(-EPERM));
    ON_CALL(*this, kill_fake(_, _))
        .WillByDefault(Return(-EPERM));
}

int pthread_create(pthread_t *newthread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
    return pthread_mock::get().pthread_create(newthread, attr, start_routine, arg);
}

int pthread_detach(pthread_t thread)
{
    return pthread_mock::get().pthread_detach(thread);
}

int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t *cpuset)
{
    return pthread_mock::get().pthread_setaffinity_np(thread, cpusetsize, cpuset);
}

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param)
{
    return pthread_mock::get().pthread_setschedparam(thread, policy, param);
}

int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
{
    return pthread_mock::get().pthread_getschedparam(thread, policy, param);
}

pthread_t pthread_self_fake(void)
{
    return pthread_mock::get().pthread_self_fake();
}

int pthread_join(pthread_t thread, void **retval)
{
    return pthread_mock::get().pthread_join(thread, retval);
}

int kill_fake(pid_t pid, int sig)
{
    return pthread_mock::get().kill_fake(pid, sig);
}
