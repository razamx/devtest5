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

#ifndef ARGS_HELPER_HPP
#define ARGS_HELPER_HPP

#include <vector>

// CLI arguments helper wrapper

// This class encapsulates:
// - including executable path as first parameter
// - including null termination
// - size correcting (excludes null terminator)
// - const casting of args

class Args {
    std::vector<const char*> m_args;

public:
    Args(const std::vector<const char*> args_vec) {
        m_args = args_vec;
        m_args.insert(m_args.begin(), "/executable/path");
        m_args.push_back(nullptr);
    }

    Args(std::initializer_list<const char*> args_il) {
        m_args = args_il;
        m_args.insert(m_args.begin(), "/executable/path");
        m_args.push_back(nullptr);
    }

    Args() {
        m_args.push_back("/executable/path");
        m_args.push_back(nullptr);
    }

    operator char**() const {
        return const_cast<char**>(m_args.data());
    }

    int count() const {
        return m_args.size() - 1;
    }

    void push(const char* arg) {
        m_args.back() = arg;
        m_args.push_back(nullptr);
    }

    void clear() {
        m_args.clear();
        m_args.push_back("/executable/path");
        m_args.push_back(nullptr);
    }
};

#endif  // ARGS_HELPER_HPP
