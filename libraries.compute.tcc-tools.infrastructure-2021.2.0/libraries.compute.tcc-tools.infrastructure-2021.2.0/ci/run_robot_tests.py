#!/usr/bin/env python3
#/*******************************************************************************
#Copyright 2019 Intel Corporation.
#This software and the related documents are Intel copyrighted materials, and your use of them
#is governed by the express license under which they were provided to you (License).
#Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
#or transmit this software or the related documents without Intel's prior written permission.
#This software and the related documents are provided as is, with no express or implied warranties,
#other than those that are expressly stated in the License.
#
#*******************************************************************************/

# This script is meant to be run on a target or host platform. It takes file with robot tests' paths
# and run it one by one.


import os
import sys
import re
import argparse
import subprocess


def parse_args():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--argumentfile',
                        '-A',
                        help='Argument file for RF')
    parser.add_argument('--tests_list',
                        required=True,
                        help='List with tests\' path')
    parser.add_argument('--include',
                        '-i',
                        nargs='*',
                        help='Additional include tags. Provide all tags to RF command "--include" with OR separator')
    parser.add_argument('--exclude',
                        '-e',
                        nargs='*',
                        help='Additional exclude tags. Provide all tags to RF command "--exclude" with OR separator')
    parser.add_argument('-h', '--help', action='help',
                        default=argparse.SUPPRESS,
                        help='Show this help message and exit')
    parser.usage = '%(prog)s --tests_list <tests_list> [--argumentfile <argumentfile>] [--include <include>] [--exclude <exclude>]'
    return parser.parse_args()

def run_suite(suite_path, include, exclude, arg_file):
    suite_name = os.path.basename(suite_path)
    suite_name = re.sub(r'\.robot$', '', suite_name)
    command = "robot {0} {1} {2} --outputdir robot_result --loglevel TRACE --runemptysuite -l {3}_log.html -r {3}_report.html -o {3}_output.xml -x {3}_xoutput.xml {4}".format(
        arg_file,
        include,
        exclude,
        suite_name,
        suite_path
    )

    print( '\n' + 'Execute command: ' + command)
    try:
        output = subprocess.run(command, shell=True)
    except FileNotFoundError as err:
        print(err)

    return output.returncode

def main():
    args = parse_args()
    result = 0
    include = ''
    exclude = ''
    arg_file = ''
    if args.include:
        include = '--include ' + 'OR'.join(args.include)
    if args.exclude:
        exclude = '--exclude ' + 'OR'.join(args.exclude)
    if args.argumentfile:
        arg_file = '--argumentfile ' + args.argumentfile

    if not os.path.isfile(args.tests_list):
       print("File path {} does not exist".format(args.tests_list))
       return -1

    if args.argumentfile and not os.path.isfile(args.argumentfile):
       print("File path {} does not exist".format(args.argumentfile))
       return -1

    with open(args.tests_list, 'r') as f:
        for suite in f:
            suite = suite.rstrip()
            rc = run_suite(suite, include, exclude, arg_file)
            if rc != 0:
                result = -1

    return result

if __name__ == "__main__":
    sys.exit(main())
