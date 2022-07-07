#/*******************************************************************************
#INTEL CONFIDENTIAL
#Copyright 2018-2020 Intel Corporation.
#This software and the related documents are Intel copyrighted materials, and your use of them
#is governed by the express license under which they were provided to you (License).
#Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
#or transmit this software or the related documents without Intel's prior written permission.
#This software and the related documents are provided as is, with no express or implied warranties,
#other than those that are expressly stated in the License.
#
#*******************************************************************************/

import os
import sys
import argparse
import re
import fnmatch
import functools
import itertools
import datetime

year = str(datetime.date.today().year)

copyright_year = ('''Copyright (2\d{3}\-)?''' + year ,
"Wrong year of last file modification in copyright header. Year should be " + year,
lambda x: not x)

c_copyright_gplv2 = ('''/\* SPDX-License-Identifier: GPL-2.0\+ WITH Linux-syscall-note \*/
/\*
 \*.*
 \*
 \*.*
 \*
 \*  This program is free software; you can redistribute it and/or modify
 \*  it under the terms of the GNU General Public License as published by
 \*  the Free Software Foundation; either version 2 of the License, or
 \*  \(at your option\) any later version.
 \*
 \*  This program is distributed in the hope that it will be useful,
 \*  but WITHOUT ANY WARRANTY; without even the implied warranty of
 \*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 \*  GNU General Public License for more details.
 \*
 \*  You should have received a copy of the GNU General Public License
 \*  along with this program; if not, write to the Free Software
 \*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 \*/
''',
'There is no GPLv2 copyright header',
lambda x: not x)

c_copyright_no_conf = ('''/\\*+\\r?
Copyright (2\d{3}\-)?''' + year + ''' Intel Corporation.\\s*\\r?
This software and the related documents are Intel copyrighted materials, and your use of them\\s*\\r?
is governed by the express license under which they were provided to you \(License\).\\s*\\r?
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose\\s*\\r?
or transmit this software or the related documents without Intel's prior written permission.\\s*\\r?
This software and the related documents are provided as is, with no express or implied warranties,\\s*\\r?
other than those that are expressly stated in the License.\\s*\\r?
\\*+/''',
"There is no C style copyright header",
lambda x: not x)


c_copyright = ('''/\\*+\\r?
INTEL CONFIDENTIAL\\s*\\r?
Copyright (2\d{3}\-)?''' + year + ''' Intel Corporation.\\s*\\r?
This software and the related documents are Intel copyrighted materials, and your use of them\\s*\\r?
is governed by the express license under which they were provided to you \(License\).\\s*\\r?
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose\\s*\\r?
or transmit this software or the related documents without Intel's prior written permission.\\s*\\r?
This software and the related documents are provided as is, with no express or implied warranties,\\s*\\r?
other than those that are expressly stated in the License.\\s*\\r?
\\*+/''',
"There is no CONFIDENTIAL C style copyright header",
lambda x: not x)

tsc_copyright = (r"/\*+\r?\n"
	r" \* BSD LICENSE\s*\r?\n"
	r" \*\s*\r?\n"
	r" \* Copyright\(c\) 2016 Intel Corporation. All rights reserved.\s*\r?\n"
	r" \* All rights reserved.\s*\r?\n"
	r" \*\s*\r?\n"
	r" \* Redistribution and use in source and binary forms, with or without\s*\r?\n"
	r" \* modification, are permitted provided that the following conditions\s*\r?\n"
	r" \* are met:\s*\r?\n"
	r" \*\s*\r?\n"
	r" \*   \* Redistributions of source code must retain the above copyright\s*\r?\n"
	r" \*     notice, this list of conditions and the following disclaimer.\s*\r?\n"
	r" \*   \* Redistributions in binary form must reproduce the above copyright\s*\r?\n"
	r" \*     notice, this list of conditions and the following disclaimer in\s*\r?\n"
	r" \*     the documentation and/or other materials provided with the\s*\r?\n"
	r" \*     distribution.\s*\r?\n"
	r" \*   \* Neither the name of Intel Corporation nor the names of its\s*\r?\n"
	r" \*     contributors may be used to endorse or promote products derived\s*\r?\n"
	r" \*     from this software without specific prior written permission.\s*\r?\n"
	r" \*\s*\r?\n"
	r" \* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\s*\r?\n"
	r" \* \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\s*\r?\n"
	r" \* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\s*\r?\n"
	r" \* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\s*\r?\n"
	r" \* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\s*\r?\n"
	r" \* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES \(INCLUDING, BUT NOT\s*\r?\n"
	r" \* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\s*\r?\n"
	r" \* DATA, OR PROFITS; OR BUSINESS INTERRUPTION\) HOWEVER CAUSED AND ON ANY\s*\r?\n"
	r" \* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\s*\r?\n"
	r" \* \(INCLUDING NEGLIGENCE OR OTHERWISE\) ARISING IN ANY WAY OUT OF THE USE\s*\r?\n"
	r" \* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\s*\r?\n"
	r" \*/\s*\r?",
"There is no BSD copyright header",
lambda x: not x)

sharp_copyright_no_conf = ('''#\s*/\\*+\\r?
# Copyright (2\d{3}\-)?''' + year + ''' Intel Corporation.\\s*\\r?
# This software and the related documents are Intel copyrighted materials, and your use of them\\s*\\r?
# is governed by the express license under which they were provided to you \(License\).\\s*\\r?
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose\\s*\\r?
# or transmit this software or the related documents without Intel's prior written permission.\\s*\\r?
# This software and the related documents are provided as is, with no express or implied warranties,\\s*\\r?
# other than those that are expressly stated in the License.\\s*\\r?
#\\s*\\r?
# \\*+/''',
"There is no sharped copyright header (Without CONFIDENTIAL)",
lambda x: not x)

sharp_copyright = ('''#\s*/\\*+\\r?
# INTEL CONFIDENTIAL\\s*\\r?
# Copyright (2\d{3}\-)?''' + year + ''' Intel Corporation.\\s*\\r?
# This software and the related documents are Intel copyrighted materials, and your use of them\\s*\\r?
# is governed by the express license under which they were provided to you \(License\).\\s*\\r?
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose\\s*\\r?
# or transmit this software or the related documents without Intel's prior written permission.\\s*\\r?
# This software and the related documents are provided as is, with no express or implied warranties,\\s*\\r?
# other than those that are expressly stated in the License.\\s*\\r?
#\\s*\\r?
# \\*+/''',
"There is no sharped copyright header (CONFIDENTIAL)",
lambda x: not x)

using_in_header = (
    "using namespace [^;]*;",
    "There is using in header",
    lambda x: x)

windows_line_endings = (
    ".*\r$",
    "There is windows line endings in header",
    lambda x: x)

def regular_expression_check(regexp, error_msg, result_checker, content):
    re_check = re.compile(regexp, re.DOTALL)
    result = re_check.search(content)
    if result_checker(result):
        return error_msg

c_copyright_exist = functools.partial(
        regular_expression_check,
        *c_copyright)
c_copyright_no_conf_exist = functools.partial(
        regular_expression_check,
        *c_copyright_no_conf)
tsc_copyright_exist = functools.partial(
        regular_expression_check,
        *tsc_copyright)
sharp_copyright_exist = functools.partial(
        regular_expression_check,
        *sharp_copyright)
sharp_copyright_no_conf_exist = functools.partial(
                regular_expression_check,
                *sharp_copyright_no_conf)
using_in_header_exist = functools.partial(
        regular_expression_check,
        *using_in_header)
windows_line_ending_exist = functools.partial(
        regular_expression_check,
        *windows_line_endings)
right_copyright_year_exist = functools.partial(
                regular_expression_check,
                *copyright_year)
c_copyright_gplv2_exists = functools.partial(
    regular_expression_check,
    *c_copyright_gplv2
)

def line_length_check(string):
    if len(string) > 120:
        return "Line string length is great than 120"

def process_file(file, rulename, checkers):
    # Whole content check
    file.seek(0)
    content = file.read()
    whole_file_errors = list(filter(
        lambda e: e is not None,
        map(lambda checker: checker(content), checkers[0])))
    # Per line check
    lines = content.split("\n")
    line_errors = list(filter(
        lambda el: el[0] != None,
        list(itertools.chain.from_iterable(map(
            lambda check: [(check(line), idx) for (idx, line) in enumerate(lines)],
            checkers[1])))))
    if whole_file_errors or line_errors:
        #print("Errors in " + file.name + " with rules: " + rulename)
        print(file.name)
        for error in whole_file_errors:
            print("\tError: " + error)
        for error in line_errors:
            print("\tError: {0} on line {1}".format(*error))
        #print("\tTotal number of errors: {0}".format(
        #    len(whole_file_errors) + len(line_errors)))
        return False
    else:
        return True

def create_parser():
    parser = argparse.ArgumentParser(
            description='Checking file againt predefined set of rules')
    parser.add_argument('file',
                        type = argparse.FileType(mode='r'),
                        nargs = '+')
    return parser

# Checkers configuration
# (Rule name, File name pattern,  whole file checkers,     line-by-line checkers)
file_types_descs = [
# All files in test folder should have confidentional header
    ("Header checks",                u"*test*",                                                    u"*.h",    [c_copyright_exist], []),
    ("C++ Header checks",            u"*test*",                                                    u"*.hpp",  [c_copyright_exist], []),
    ("C++ Source file check",        u"*test*",                                                    u"*.cpp",  [c_copyright_exist], []),
    ("C++ Template file check",      u"*test*",                                                    u"*.tpp",  [c_copyright_exist], []),
    ("C Source file check",          u"*test*",                                                    u"*.c",    [c_copyright_exist], []),
    ("QML Source file check",        u"*test*",                                                    u"*.qml",  [c_copyright_exist], []),
    ("CMakeLists check",             u"*test*",                                                    u"CMakeLists.txt", [sharp_copyright_exist], []),
    ("Python files check",           u"*test*",                                                    u"*.py",   [sharp_copyright_exist], []),
# All files in mock folder should have confidentional header
    ("Header checks",                u"*mock*",                                                    u"*.h",    [c_copyright_exist], []),
    ("C++ Header checks",            u"*mock*",                                                    u"*.hpp",  [c_copyright_exist], []),
    ("C++ Source file check",        u"*mock*",                                                    u"*.cpp",  [c_copyright_exist], []),
    ("C++ Template file check",      u"*mock*",                                                    u"*.tpp",  [c_copyright_exist], []),
    ("C Source file check",          u"*mock*",                                                    u"*.c",    [c_copyright_exist], []),
    ("QML Source file check",        u"*mock*",                                                    u"*.qml",  [c_copyright_exist], []),
    ("CMakeLists check",             u"*mock*",                                                    u"CMakeLists.txt", [sharp_copyright_exist], []),
    ("Python files check",           u"*mock*",                                                    u"*.py",   [sharp_copyright_exist], []),
# Repository root should be named as tccsdk
    ("CMakeLists check",             u"*tccsdk",                                                   u"CMakeLists.txt",    [sharp_copyright_no_conf_exist], []),
    ("CMakeLists check",             u"*tccsdk/config",                                            u"CMakeLists.txt",    [sharp_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tccsdk/include*",                                          u"*.h",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tccsdk/src*",                                              u"*.h",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*tccsdk/src*",                                              u"*.c",    [c_copyright_no_conf_exist], []),
    ("CMakeLists check",             u"*tccsdk/src*",                                              u"CMakeLists.txt",    [sharp_copyright_no_conf_exist], []),
    ("Public C++ Source file check", u"*tccsdk/src/inih*",                                         u"*.cpp",  [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tccsdk/src",                                               u"empty_source.c",    [c_copyright_no_conf_exist], []),
    ("Shell files check",            u"*tccsdk/scripts",                                           u"*.sh",   [sharp_copyright_no_conf_exist], []),
    ("Shell files check",            u"*tccsdk/scripts/setup_ssram",                               u"*.sh",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tccsdk/scripts/setup_ssram",                               u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Shell files check",            u"*tccsdk/scripts/setup_ssram_v1",                            u"*.sh",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tccsdk/scripts/setup_ssram_v1",                            u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("GPL v2 checker",               u"*samples/common/include*",                                  u"tgpio_defs.h",    [c_copyright_gplv2_exists], []),
    ("Public Header file check",     u"*samples/common/include*",                                  u"*.h",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/common/src*",                                      u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_cache_allocation_sample*",                     u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*samples/tcc_cache_allocation_sample*",                     u"*.h",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_sensor_data_sample*",                          u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*samples/tcc_sensor_data_sample*",                          u"*.h",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_single_measurement_sample*",                   u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_multiple_measurements_sample*",                u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_measurement_monitoring_sample*",               u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*samples/tcc_measurement_monitoring_sample*",               u"*.h",    [c_copyright_no_conf_exist], []),
    ("Python files check",           u"*samples/tcc_measurement_analysis_sample*",                 u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_rt_communication_demo*",                       u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*samples/tcc_rt_communication_demo*",                       u"*.h",    [c_copyright_no_conf_exist], []),
    ("Python files check",           u"*samples/tcc_rt_communication_demo*",                       u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Shell files check",            u"*samples/tcc_rt_communication_demo*",                       u"*.sh",   [sharp_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_tgpio_sample*",                                u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*samples/tcc_tgpio_sample*",                                u"*.h",    [c_copyright_no_conf_exist], []),
    ("Shell files check",            u"*samples/tcc_tgpio_sample*",                                u"*.sh",   [sharp_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*samples/tcc_ethernet_timestamps_sample*",                  u"*.c",    [c_copyright_no_conf_exist], []),
    ("Python files check",           u"*samples/plot_scripts*",                                    u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*scripts/tcccommon*",                                       u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*target*",                                                  u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/rt_checker/[!e]*",                                   u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("CMakeLists check",             u"*tools/rt_checker/[!e]*",                                   u"CMakeLists.txt",    [sharp_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*tools/rt_checker/[!e]*",                                   u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tools/rt_checker/[!e]*",                                   u"*.h",    [c_copyright_no_conf_exist], []),
    ("C Source file check",          u"*tools/rt_checker/efi*",                                    u"*.c",    [c_copyright_exist], []),
    ("С Header file check",          u"*tools/rt_checker/efi*",                                    u"*.h",    [c_copyright_exist], []),
    ("CMakeLists check",             u"*tools/rt_checker/efi*",                                    u"CMakeLists.txt",    [sharp_copyright_exist], []),
    ("INF file check",               u"*tools/rt_checker/efi*",                                    u"*.inf",  [sharp_copyright_exist], []),
    ("Python files check",           u"*tools/tcc_data_streams_optimizer*",                        u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*tools/tcc_data_streams_optimizer*",                        u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tools/tcc_data_streams_optimizer*",                        u"*.h",    [c_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*tools/demo/workloads*",                                    u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tools/demo/workloads*",                                    u"*.h",    [c_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/cache_configurator*",                                u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/cache_info_read*",                                   u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/cache_viewer*",                                      u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/cache_viewer*",                                      u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/isd_module*",                                        u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/tcc_cache_configurator*",                            u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/tcc_cache_configurator*",                            u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools/common*",                                            u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("CMakeLists check",             u"*tools/common_lib*",                                        u"CMakeLists.txt",    [sharp_copyright_no_conf_exist], []),
    ("Public C Source file check",   u"*tools/common_lib*",                                        u"*.c",    [c_copyright_no_conf_exist], []),
    ("Public Header file check",     u"*tools/common_lib*",                                        u"*.h",    [c_copyright_no_conf_exist], []),
    ("Python files check",           u"*tools*",                                                   u"*.py",   [sharp_copyright_no_conf_exist], []),
    ("CMakeLists check",             u"*tools",                                                    u"CMakeLists.txt",    [sharp_copyright_no_conf_exist], []),
    ("TSC BSD checks",               u"*",                                                         u"tsc*.*", [tsc_copyright_exist], []),
    ("Header checks",                u"*",                                                         u"*.h",    [c_copyright_exist], [using_in_header_exist]),
    ("C++ Header checks",            u"*",                                                         u"*.hpp",  [c_copyright_exist], []),
    ("C++ Source file check",        u"*",                                                         u"*.cpp",  [c_copyright_exist], []),
    ("C++ Template file check",      u"*",                                                         u"*.tpp",  [c_copyright_exist], []),
    ("C Source file check",          u"*",                                                         u"*.c",    [c_copyright_exist], []),
    ("QML Source file check",        u"*",                                                         u"*.qml",  [c_copyright_exist], []),
    ("CMakeLists check",             u"*",                                                         u"CMakeLists.txt", [sharp_copyright_exist], []),
    ("Python files check",           u"*",                                                         u"*.py",   [sharp_copyright_exist], []),
    ("CMakeLists check",             u"*",                                                         u"*.cmake", [sharp_copyright_exist], [])
]
common_copyright_checkers=[
    ("Copyright year checker workaround",      u"*samples/common/include*",                        u"tgpio_defs.h",    [c_copyright_gplv2_exists], []),
    ("Copyright year checker C headers",       u"*",                                               u"*.h",    [right_copyright_year_exist], []),
    ("Copyright year checker C++ headers",     u"*",                                               u"*.hpp",  [right_copyright_year_exist], []),
    ("Copyright year checker C++ sources",     u"*",                                               u"*.cpp",  [right_copyright_year_exist], []),
    ("Copyright year checker 2",               u"*",                                               u"*.tpp",  [right_copyright_year_exist], []),
    ("Copyright year checker 3",               u"*",                                               u"*.c",    [right_copyright_year_exist], []),
    ("Copyright year checker 4",               u"*",                                               u"*.qml",  [right_copyright_year_exist], []),
    ("Copyright year checker 5",               u"*",                                               u"CMakeLists.txt", [right_copyright_year_exist], []),
    ("Copyright year checker 6",               u"*",                                               u"*.py",   [right_copyright_year_exist], []),
    ("Copyright year checker 7",               u"*",                                               u"*.sh",   [right_copyright_year_exist], []),
    ("Copyright year checker 8",               u"*",                                               u"*.cmake",   [right_copyright_year_exist], [])
]

def process_rule(file, rules):
    name = os.path.basename(file.name)
    path = os.path.dirname(file.name)
    rule = next((x for x in rules if (fnmatch.fnmatch(path, x[1]) and fnmatch.fnmatch(name, x[2]))), None)
    if rule is None:
        return True
    result = process_file(file, rule[0], rule[3:])
    return result

def process_files(files):
    status = True
    for file in files:
        status = process_rule(file, common_copyright_checkers) and status
        status = process_rule(file, file_types_descs) and status
    return status

def main():
    if(len(sys.argv) == 1):
        return 0
    parser = create_parser()
    arguments = parser.parse_args()
    return not process_files(arguments.file)

if __name__ == "__main__":
    sys.exit(main())
