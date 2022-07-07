#!/usr/bin/env python3.7
# /*******************************************************************************
# Copyright 2020 Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

import subprocess
import argparse
import shutil
import sys
import os

# Script to run the SNYK command line scanner.
# SNYK executable should be available from the command line.

def parse_arguments():
    """ Parse input arguments"""
    try:
        parser = argparse.ArgumentParser(
            description="Run SNYK command line scanner.",
            epilog="""Scan dependencies for components.""",
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
            help='load list of dependecies ' +
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


def snyk_version(snyk_path):
    """Function to print SNYK version"""
    snyk_args = [
        '{0}'.format(snyk_path),
        '--version',
    ]

    proc = subprocess.run(snyk_args, capture_output=True, text=True)
    std_out = proc.stdout
    return std_out


def run_snyk(snyk_path, input_dep_file, severity):
    """Function to execute SNYK cli application"""
    #./snyk-linux test --file=prerequisites.txt --command=python3 --package-manager=pip
    snyk_args = [
        '{0}'.format(snyk_path),
        'test',
        '--file={0}'.format(input_dep_file),
        '--command=python3',
        '--package-manager=pip',
        '--severity-threshold={}'.format(severity)
    ]

    proc = subprocess.run(snyk_args, capture_output=True, text=True)
    std_out = proc.stdout
    result = proc.returncode
    return result, std_out


def main():
    """Execute script logic to find compiler options"""
    args = parse_arguments()
    
    try:
        snyk_path = shutil.which("snyk-linux")
        if snyk_path is None:
            print("SNYK binanary not found.")
            return 0
        print("Running with: {0} v{1}".format(snyk_path,snyk_version(snyk_path)))

        #Run SNYK for all severities
        result, output = run_snyk(snyk_path, args.input,'low')
        if args.verbose:
            print("SNYK full: ", output)
            print('Exit: {}'.format(result))
        full_out = output
        
        #Run SNYK for high severity only
        exit_code, output = run_snyk(snyk_path, args.input,'high')
        print("SNYK high severity: ", output)
        print('Exit: {}'.format(exit_code))
        full_out += output
        
        #Write log file if specified
        if args.output != "":
            with open(args.output, "w") as output_log_file:
                output_log_file.write(full_out)
   
    except Exception as exception:
        sys.stderr.write('{0}: {1}'.format(
            os.path.basename(__file__), str(exception)))
    
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
