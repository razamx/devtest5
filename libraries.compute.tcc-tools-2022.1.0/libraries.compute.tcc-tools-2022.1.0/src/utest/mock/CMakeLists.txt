# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

cmake_minimum_required(VERSION 3.0)

add_library(tcc_common_mock INTERFACE)
target_include_directories(tcc_common_mock INTERFACE include)
target_link_libraries(tcc_common_mock INTERFACE gmock_main gmock tcc_log)

add_library(tccsdk_mocks INTERFACE)

macro (create_wrap_flags VAR)
  set(WRAP_PREFIX "--wrap=")
  set(TMP)
  foreach(W ${ARGN})
    list(APPEND TMP "${WRAP_PREFIX}${W}")
  endforeach()
  set(WRAPS ${TMP})
  string(REPLACE ";" "," WRAPS "${WRAPS}" )
  string(CONCAT WRAPS  "-Wl," ${WRAPS})
  set(${VAR} ${WRAPS})
endmacro(create_wrap_flags )
#---------------------------------------------------------------------------------
add_library(tccsdk_allock_mock STATIC src/alloc_mock.cpp)
target_include_directories(tccsdk_allock_mock PUBLIC include)
set_property(TARGET tccsdk_allock_mock PROPERTY CXX_STANDARD 11)
create_wrap_flags(ALLOC_MOCK_WRAPS
    malloc
    calloc
    realloc
    strdup
    asprintf
    vasprintf
    free
)
target_link_libraries(tccsdk_allock_mock
  PUBLIC
    ${ALLOC_MOCK_WRAPS}
    tcc_common_mock
    gtest_helpers
)
#---------------------------------------------------------------------------------
add_library(tccsdk_libc_mock STATIC src/libc_mock.cpp)
target_include_directories(tccsdk_libc_mock PUBLIC include)
set_property(TARGET tccsdk_libc_mock PROPERTY CXX_STANDARD 11)
create_wrap_flags(GLIBC_MOCK_WRAPS
    getenv
    setenv

    fopen
    fclose
    fprintf
    vfprintf
    fread
    fwrite
    fgetc

    open
    close
    read
    write
    ioctl
    lseek
    mmap
    munmap
    dprintf

    stat
    unlink

    pwrite
    pread

    clock_nanosleep
    clock_gettime
    nanosleep
    usleep

    select
    access

    get_nprocs
    sched_getaffinity
    sched_setaffinity
    sched_setscheduler

    pthread_getaffinity_np
    pthread_setaffinity_np
    pthread_getschedparam
    pthread_setschedparam

    ftruncate

    exit

    mount

    dlopen
    dlsym
    dlerror
    dlclose

    setmntent
    getmntent
    endmntent

    iopl

    gettimeofday
    localtime

    mlockall
    munlockall

    memfd_create

    uname

    strrchr
)
target_link_libraries(tccsdk_libc_mock
  PUBLIC
    ${GLIBC_MOCK_WRAPS}
    -Wl,--defsym=__fprintf_chk=__wrap___fprintf_chk
    tcc_common_mock
    gtest_helpers
)
#---------------------------------------------------------------------------------
target_link_libraries(tccsdk_mocks INTERFACE
    tccsdk_libc_mock
    tccsdk_allock_mock
    rt
)

add_subdirectory(itt_notify)
