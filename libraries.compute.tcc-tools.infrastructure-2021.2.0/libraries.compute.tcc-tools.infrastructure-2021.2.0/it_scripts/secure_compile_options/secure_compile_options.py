#!/usr/bin/env python3.7
# /*******************************************************************************
# Copyright 2020-2021 Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

import argparse
import sys
import os

# List of recommended compiler options from this link:
# https://wiki.ith.intel.com/display/SecTools/Compiler+Defenses+BKMs
# -Werror Makes all warnings into errors. Replaces -Werror=format-security
# -Wformat=2 Enable -Wformat plus additional format checks. 
#   Equivalent to -Wformat -Wformat-nonliteral -Wformat-security -Wformat-y2k.
#   Replaces -Wformat -Wformat-security
REQUIRED_COMPILER_OPTIONS = [
    '-Wformat=2',
    '-Werror',
    '-D_FORTIFY_SOURCE=2',
    '-fstack-protector-strong',
    '-fPIE',
    '-fPIC',
]
ADDITIONAL_COMPILER_OPTIONS = [
    '-fcf-protection=full -mcet',
    '-fstack-clash-protection',
    '-Wall',
    'mconditional-branch=keep',
    'mconditional-branch=pattern-report',
    'mconditional-branch=pattern-fix',
    '-mindirect-branch=choice',
    '-mfunction-return=choice',
    '-mindirect-branch-register',
]
REQUIRED_SO_LINKER_OPTIONS = [
    '-shared',
    '-z,relro',
    '-z,now',
    '-z noexecstack',
]
REQUIRED_EXEC_LINKER_OPTIONS = [
    '-pie',
    '-z,relro',
    '-z,now',
    '-z noexecstack',
]

# Coloring
CRED = '\33[31m'
CEND = '\33[0m'
CGREEN  = '\33[32m'
# CBOLD     = '\33[1m'
# CITALIC   = '\33[3m'
# CURL      = '\33[4m'
# CBLINK    = '\33[5m'
# CBLINK2   = '\33[6m'
# CSELECTED = '\33[7m'
#
# CBLACK  = '\33[30m'
# CYELLOW = '\33[33m'
# CBLUE   = '\33[34m'
# CVIOLET = '\33[35m'
# CBEIGE  = '\33[36m'
# CWHITE  = '\33[37m'
#
# CBLACKBG  = '\33[40m'
# CREDBG    = '\33[41m'
# CGREENBG  = '\33[42m'
# CYELLOWBG = '\33[43m'
# CBLUEBG   = '\33[44m'
# CVIOLETBG = '\33[45m'
# CBEIGEBG  = '\33[46m'
# CWHITEBG  = '\33[47m'
#
# CGREY    = '\33[90m'
# CRED2    = '\33[91m'
# CGREEN2  = '\33[92m'
# CYELLOW2 = '\33[93m'
# CBLUE2   = '\33[94m'
# CVIOLET2 = '\33[95m'
# CBEIGE2  = '\33[96m'
# CWHITE2  = '\33[97m'
#
# CGREYBG    = '\33[100m'
# CREDBG2    = '\33[101m'
# CGREENBG2  = '\33[102m'
# CYELLOWBG2 = '\33[103m'
# CBLUEBG2   = '\33[104m'
# CVIOLETBG2 = '\33[105m'
# CBEIGEBG2  = '\33[106m'
# CWHITEBG2  = '\33[107m'

# Global variable to store file output
g_file_out = list()


def parse_arguments():
    """ Parse input arguments"""
    try:
        parser = argparse.ArgumentParser(
            description="Parse file to find compiler options.",
            epilog="""Secure compiler options are defined by SDL.""",
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        parser.add_argument(
            '-o', '--output',
            default="",
            metavar='DIR',
            help="save test results in %(metavar)s (default: '%(default)s')")
        parser.add_argument(
            '-i', '--input',
            default="",
            metavar='DIR',
            help='load test results (for aggregation) ' +
                 "from %(metavar)s (default: '%(default)s')")
        parser.add_argument(
            '-v', '--verbose',
            action='store_true',
            help='verbose mode')

        args = parser.parse_args()
    except Exception as exception:
        sys.stderr.write('{0}: {1}'.format(
            os.path.basename(__file__), str(exception)))

    return args


def is_compile_line(log_line):
    """Verify if input line are compilation line"""

    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-o', '--output',
        default=None,
        metavar='DIR',
        help="save test results in %(metavar)s (default: '%(default)s')")

    args, unknown = parser.parse_known_args(log_line.split())
    if args.output is None:
        return False

    # All files which starts with test_ are ignored
    if 'test_' in args.output:
        return False

    if args.output.endswith(".o") or args.output.endswith(".s"):
        if ("/usr/bin/c++" in log_line) or ("/usr/bin/cc" in log_line) or (
                "/usr/lib/gcc" in log_line):
            out = "Compile: {}".format(args.output)
            print(out)
            g_file_out.append(out)
            return True

    return False


def is_so_linker_line(log_line):
    """Verify if input line are compilation line"""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-o', '--output',
        default=None,
        metavar='DIR',
        help="save test results in %(metavar)s (default: '%(default)s')")

    args, unknown = parser.parse_known_args(log_line.split())
    if args.output is None:
        return False

    # All files which starts with test_ are ignored
    if 'test_' in args.output:
        return False
    # Skip object files
    if args.output.endswith(".o"):
        return False
    if args.output.endswith(".s"):
        return False
    # Skip files not starting with lib
    if not args.output.startswith("lib"):
        return False
    # Skip files without .so in name
    if '.so' not in args.output:
        return False

    if ("/usr/bin/c++" in log_line) or ("/usr/bin/cc" in log_line) or (
            "/usr/lib/gcc" in log_line):
        out = "Link so: {}".format(args.output)
        print(out)
        g_file_out.append(out)
        return True

    return False


def is_exec_linker_line(log_line):
    """Verify if input line are compilation line"""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '-o', '--output',
        default=None,
        metavar='DIR',
        help="save test results in %(metavar)s (default: '%(default)s')")

    args, unknown = parser.parse_known_args(log_line.split())
    if args.output is None:
        return False

    # All files which starts with test_ are ignored
    if 'test_' in args.output:
        return False
    # Skip object files
    if args.output.endswith(".o"):
        return False
    if args.output.endswith(".s"):
        return False
    # Skip files starting with lib
    if args.output.startswith("lib"):
        return False
    # Skip files with .so in name
    if '.so' in args.output:
        return False

    if ("/usr/bin/c++" in log_line) or ("/usr/bin/cc" in log_line) or (
            "/usr/lib/gcc" in log_line):
        out = "Link exec: {}".format(args.output)
        print(out)
        g_file_out.append(out)
        return True

    return False


def print_required(present, missing, verbose):
    """Print required present and missing options"""
    if verbose:
        if present != "":
            out = '  Required present: {}'.format(present)
            print(CGREEN + out + CEND)
            g_file_out.append(out)
        if missing != "":
            out = '  Required MISSING: {}'.format(missing)
            print(CRED + out + CEND)
            g_file_out.append(out)
    else:
        if missing != "":
            out = '  Required MISSING: {}'.format(missing)
            print(CRED + out + CEND)
            g_file_out.append(out)


def print_optional(present, missing, verbose):
    """Print optional present and missing options"""
    if verbose:
        if present != "":
            out = '  Optional present: {}'.format(present)
            print(CGREEN + out + CEND)
            g_file_out.append(out)
        if missing != "":
            out = '  Optional MISSING: {}'.format(missing)
            print(CRED + out + CEND)
            g_file_out.append(out)
    # else:
    #     if missing != "":
    #         out = '  Optional missing: {}'.format(missing)
    #         print(CRED + out + CEND)
    #         g_file_out.append(out)


def inspect_line(log_line, verbose):
    """Verify eligible log file line for given secure options"""
    result = 0
    if is_compile_line(log_line):
        present = ""
        missing = ""
        for option in REQUIRED_COMPILER_OPTIONS:
            if option in log_line:
                present += '{}, '.format(option)
            else:
                missing += '{}, '.format(option)
                result += 1
        print_required(present, missing, verbose)

        present = ""
        missing = ""
        for option in ADDITIONAL_COMPILER_OPTIONS:
            if option in log_line:
                present += '{}, '.format(option)
            else:
                missing += '{}, '.format(option)
        print_optional(present, missing, verbose)

    elif is_so_linker_line(log_line):
        present = ""
        missing = ""
        for option in REQUIRED_SO_LINKER_OPTIONS:
            if option in log_line:
                present += '{}, '.format(option)
            else:
                missing += '{}, '.format(option)
                result += 1
        print_required(present, missing, verbose)

    elif is_exec_linker_line(log_line):
        present = ""
        missing = ""
        for option in REQUIRED_EXEC_LINKER_OPTIONS:
            if option in log_line:
                present += '{}, '.format(option)
            else:
                missing += '{}, '.format(option)
                result += 1
        print_required(present, missing, verbose)

    return result


def main():
    """Execute script logic to find compiler options"""
    args = parse_arguments()

    with open(args.input, "r") as input_log_file:
        input_log = input_log_file.readlines()

    result = 0
    for line in input_log:
        result += inspect_line(line, args.verbose)

    if args.output != "":
        with open(args.output, "w") as output_log_file:
            output_log_file.writelines("%s\n" % item for item in g_file_out)

    print('Exit: {}'.format(result))
    return result


if __name__ == "__main__":
    sys.exit(main())
