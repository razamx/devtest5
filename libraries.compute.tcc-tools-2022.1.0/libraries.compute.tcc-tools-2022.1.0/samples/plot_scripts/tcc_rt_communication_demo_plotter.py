#!/usr/bin/python3
# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

import os
import sys
import argparse
import numpy as np
import subprocess as sp
from typing import List
import matplotlib.pyplot as plt
from common.log_helper import plot_scatter, check_file

def init_parser():
    parser = argparse.ArgumentParser(
        description="Plotter for log files coming from tcc_rt_communication_demo",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "-i", "--input",
        type=str,
        nargs="+",
        help="filename with data",
        required=True
    )
    parser.add_argument(
        "-a", "--address",
        type=str,
        help='An optional hostname or IP address for the target where the input files are located',
        required=False
    )
    parser.add_argument(
        "-u", "--user",
        type=str,
        default='root',
        help='The user name for accessing the remote target systems, "root" on default',
        required=False
    )
    parser.add_argument(
        "-b", "--base-dir",
        type=str,
        default='tcc_rt_communication_demo_output',
        help='The base directory for specifying input files on remote target systems, '
             '"tcc_rt_communication_demo_output" on default',
        required=False
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        help='filename to save the image. GUI window will be opened if filename not specified',
        required=False
    )
    return parser


def check_input(filename):
    check_file(filename)


def check_output(filename):
    if os.path.splitext(filename)[-1].lower() != '.png':
        print('Output "%s" must be PNG file' % filename)
        exit(1)


def get_remote_data(address: str, user: str, remote_path: str) -> List[float]:
    print(f'Reading file "{remote_path}" on host "{address}"', end=': ')
    cmd_args = ['ssh', f'{user}@{address}', 'cat', remote_path]
    result = sp.run(cmd_args, stdout=sp.PIPE, stderr=sp.DEVNULL)
    print('OK' if result.returncode == 0 else 'FAILED')
    if result.returncode != 0:
        print(f'Error: failed to access file "{remote_path}" on host: "{address}", '
              f'return code: {result.returncode}.')
        exit(1)
    data = [float(line) / 1000 for line in result.stdout.splitlines()]
    return data


def main():
    # Initialize parser
    parser = init_parser()
    if len(sys.argv) == 1:
        parser.print_help()
        exit(1)

    args = parser.parse_args()

    if args.address is None:
        # Check if the local input files are available.
        # In case of remote files, the check is done in 'get_remote_data' function
        [check_input(file) for file in args.input]

    if args.output is not None: # required=False
        check_output(args.output)

    # Read out the data
    data=[]
    legend = []
    for file in args.input:
        if args.address is None:
            # Handling input data in local files
            legend.append(os.path.basename(file))
            with open(file, 'r') as f:
                # Read the input data in 'ns' and convert to 'us'
                file_data = np.array(np.fromfile(f, dtype=int, sep=" ") / 1000)
                if file_data is None or not len(file_data):
                    print('Unable to parse "%s"' % os.path.realpath(file))
                    exit(1)
                data.append(file_data)
        else:
            # Handling remote data
            legend.append(file)
            data_filename = file if os.path.isabs(file) else os.path.join(args.base_dir, file)
            remote_data = get_remote_data(args.address, args.user, data_filename)
            data.append(remote_data)

    # Let's plot
    if args.output is not None:
        plt.switch_backend('Agg')  # set non gui backend

    fig = plt.figure(figsize=(12,8), num=sys.argv[0])
    grid = plt.GridSpec(1, 5, hspace=0.3, wspace=0)

    has_plot = plot_scatter(
        data,
        legend,
        'Transmission Latency',
        fig,
        grid,
        'us', # time unit
        merge=True # Merge the plots
    )

    # Display results
    if has_plot:
        if args.output is None:
            plt.show()
        else:
            plt.savefig(args.output, format='png')
            print('Image saved to "%s"' % os.path.realpath(args.output))
    plt.close(fig)

if __name__ == '__main__':
    main()
