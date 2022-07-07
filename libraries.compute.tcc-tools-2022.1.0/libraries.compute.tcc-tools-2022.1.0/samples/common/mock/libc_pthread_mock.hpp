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

#ifndef LIBC_PTHREAD_MOCK_H
#define LIBC_PTHREAD_MOCK_H

#include "mock_common.hpp"
#include "gmock/gmock.h"

#include <pthread.h>

#include <sys/types.h>

struct pthread_mock {
    MOCK_METHOD4(pthread_create, int (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *));
    MOCK_METHOD1(pthread_detach, int (pthread_t));
    MOCK_METHOD3(pthread_setaffinity_np, int (pthread_t, size_t, const cpu_set_t *));
    MOCK_METHOD3(pthread_setschedparam, int (pthread_t, int, const struct sched_param *));
    MOCK_METHOD3(pthread_getschedparam, int (pthread_t, int *, struct sched_param *));
    MOCK_METHOD0(pthread_self_fake, pthread_t (void));
    MOCK_METHOD2(pthread_join, int (pthread_t, void **));
    MOCK_METHOD2(kill_fake, int (pid_t, int));

    COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(pthread_mock);
};

#endif // LIBC_PTHREAD_MOCK_H
