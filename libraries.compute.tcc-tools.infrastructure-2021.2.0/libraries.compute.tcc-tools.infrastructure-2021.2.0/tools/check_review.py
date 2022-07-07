#/*******************************************************************************
#INTEL CONFIDENTIAL
#Copyright 2021 Intel Corporation.
#This software and the related documents are Intel copyrighted materials, and your use of them
#is governed by the express license under which they were provided to you (License).
#Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
#or transmit this software or the related documents without Intel's prior written permission.
#This software and the related documents are provided as is, with no express or implied warranties,
#other than those that are expressly stated in the License.
#
#*******************************************************************************/

import sys
import os
import argparse
import requests
import rules_checker
import fnmatch
import logging
import copy

server = "https://api.github.com/repos/intel-innersource"


def get_process_pull_request_files(access_token, repository_name, id):

    headers = {'Authorization': f'token {access_token}'}
    request = server + "/" + repository_name + "/pulls/" + id + "/files"
    logging.debug("Git request: %s", request)
    response = requests.get(request, headers=headers)
    if response.status_code != 200:
        raise RuntimeError("Server returns: " + str(response.status_code) + "/" + response.reason)
    files = response.json()

    return map(lambda file: file['filename'],
            filter(lambda file: (file['status'] == 'added' or file['status'] == 'modified'),
                files))


def create_argparser():
    parser = argparse.ArgumentParser(
            description='Check all files in patchset using checker script')
    parser.add_argument('access_token', type=str)
    parser.add_argument('repository_name', type=str)
    parser.add_argument('pull_request_id', type=str)
    parser.add_argument('--filters', type=argparse.FileType())
    parser.add_argument('--root', type=str)
    parser.add_argument('-v', '--verbose', dest='logging_level',
        help='Turn on verbose output with debug messages.',
        action='store_const', const=logging.DEBUG, default=logging.INFO)
    return parser


def main():
    parser = create_argparser()
    arguments = parser.parse_args()
    logging.getLogger().setLevel(level=arguments.logging_level)
    files = get_process_pull_request_files(arguments.access_token,
                                            arguments.repository_name,
                                            arguments.pull_request_id)
    if arguments.filters is not None:
        filters = map(lambda x: x.rstrip('\n'), arguments.filters.readlines())
        print(filters)
        files = filter(
            lambda f: all(map(lambda c: not fnmatch.fnmatch(f,c), filters)),
            files)
    if arguments.root is not None:
        files = map(lambda x: arguments.root + x, files)
    files=list(files)
    files = map(lambda x: os.path.join(os.environ['TCCSDK_DIR'], x), files)
    
    filtered_files = filter(os.path.isfile, files)
    files = map(open, list(filtered_files))

    #use deepcopy to rally copy iterator
    logging.debug("Files: %s", list(copy.deepcopy(files)))

    if not files:
        print("No files to check")
        return 0
    return not rules_checker.process_files(files)


if __name__ == "__main__":
    sys.exit(main())
