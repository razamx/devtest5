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
#ifndef _TCC_MOCK_COMMON_H_
#define _TCC_MOCK_COMMON_H_
#include "tcc_log.h"
#include <errno.h>
#include <signal.h>

#define MOCK_FATAL_ERROR "\033[1;31mTCC_MOCK_FATAL ERROR:\033[0m"

#define ALLOW_DEFAULT_MOCK(interface, function) \
    ({                                          \
        if (interface::m_current != nullptr) {  \
            return interface::get().function;   \
        }                                       \
        __real_##function;                      \
    })

#define ALLOW_REGULAR_MOCK(interface, function) interface::get(__PRETTY_FUNCTION__).function


#define COMMON_MOCK_INTERFACE_DEFINITIONS(class_name)         \
public:                                                       \
    class_name();                                             \
    virtual ~class_name();                                    \
    static class_name& get(const char* function = "nullptr"); \
    static class_name* m_current;


#define COMMON_MOCK_INTERFACE_IMPLEMENTATION(class_name)                                                  \
    class_name::class_name()                                                                              \
    {                                                                                                     \
        if (m_current != nullptr) {                                                                       \
            std::cerr << MOCK_FATAL_ERROR << __PRETTY_FUNCTION__ << ": mock already created" << std::endl \
                      << std::flush;                                                                      \
            _exit(1);                                                                                     \
        }                                                                                                 \
        m_current = this;                                                                                 \
    }                                                                                                     \
    class_name::~class_name()                                                                             \
    {                                                                                                     \
        m_current = nullptr;                                                                              \
    }                                                                                                     \
    class_name& class_name::get(const char* function)                                                     \
    {                                                                                                     \
        if (m_current == nullptr) {                                                                       \
            std::cerr << MOCK_FATAL_ERROR << function << ": mock not created" << std::endl << std::flush; \
            _exit(1);                                                                                     \
        }                                                                                                 \
        return *m_current;                                                                                \
    }                                                                                                     \
    class_name* class_name::m_current = nullptr;

#define COMMON_MOCK_INTERFACE_DEFINITIONS_EXT(class_name)  \
    COMMON_MOCK_INTERFACE_DEFINITIONS(class_name)          \
    bool verify_and_clear();                               \
                                                           \
private:                                                   \
    void init();

#define COMMON_MOCK_INTERFACE_IMPLEMENTATION_EXT(class_name)                                              \
    class_name::class_name()                                                                              \
    {                                                                                                     \
        if (m_current != nullptr) {                                                                       \
            std::cerr << MOCK_FATAL_ERROR << __PRETTY_FUNCTION__ << ": mock already created" << std::endl \
                      << std::flush;                                                                      \
            _exit(1);                                                                                     \
        }                                                                                                 \
        m_current = this;                                                                                 \
        init();                                                                                           \
    }                                                                                                     \
    class_name::~class_name()                                                                             \
    {                                                                                                     \
        m_current = nullptr;                                                                              \
    }                                                                                                     \
    class_name& class_name::get(const char* function)                                                     \
    {                                                                                                     \
        if (m_current == nullptr) {                                                                       \
            std::cerr << MOCK_FATAL_ERROR << function << ": mock not created" << std::endl << std::flush; \
            _exit(1);                                                                                     \
        }                                                                                                 \
        return *m_current;                                                                                \
    }                                                                                                     \
    class_name* class_name::m_current = nullptr;                                                          \
    bool class_name::verify_and_clear()                                                                   \
    {                                                                                                     \
        bool status = ::testing::Mock::VerifyAndClearExpectations(this);                                  \
        init();                                                                                           \
        return status;                                                                                    \
    }

#define MOCK_RETURN_VAL_SET_ERRNO(ret_val, errno_val) \
    Invoke([&](...) {                                 \
        errno = (errno_val);                          \
        return (ret_val);                             \
    })
#define MOCK_RETURN_ERROR(errno_val, ret_val) \
    Invoke([&](...) {                         \
        errno = errno ? errno : (errno_val);  \
        return (ret_val);                     \
    })

#endif  // _TCC_MOCK_COMMON_H_
