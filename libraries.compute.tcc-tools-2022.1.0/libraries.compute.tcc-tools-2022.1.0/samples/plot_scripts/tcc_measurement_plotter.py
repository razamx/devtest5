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

import numpy as np
import matplotlib.pyplot as plt
from common.log_helper import plot_scatter, convert_time_unit, check_file
import argparse
import re

def init_parser():
    parser = argparse.ArgumentParser(description='Plot data from TCC Measurement')
    parser.add_argument('files', nargs='+', help='Log file or files')
    parser.add_argument('-w', '--workload', type=str, default='Workload', help='Name of the workload to plot')
    parser.add_argument('-t', '--title', type=str, default='Measurement', help='Title of the plot')
    parser.add_argument('-o', '--output', type=str, help='Output file; otherwise, display in window')
    parser.add_argument('-u', '--units', choices=['s', 'ms', 'us', 'ns', 'clk'], default='ns', help='Time units to display (default: ns)')
    parser.add_argument('--min', type=float, help='Minimum scale (in units)')
    parser.add_argument('--max', type=float, help='Maximum scale (in units)')
    return parser

# Reat TCC Measurement dump file
def read_dump(filename, workload_name):
    check_file(filename)
    with open(filename) as f:
        for workload in f:
            name, *raw_data = re.split(': |, ', workload)
            if name == workload_name:
                return np.array([int(x) for x in raw_data])
    print("WARNING: Don't find data for workload '%s'", workload_name)
    return np.array([])


def trim_shortest(data):
    min_length = min([len(d) for d in data])
    return np.array([d[:min_length] for d in data])

def main():
    parser = init_parser()
    args = parser.parse_args()

    data = trim_shortest([read_dump(file, args.workload) for file in args.files])
    fig = plt.figure(figsize=(12,8))
    grid = plt.GridSpec(1, 5, hspace=0.2, wspace=0)
    bounds = (args.min, args.max)
    has_plot = plot_scatter(data, args.files, args.title, fig, grid,
        units=args.units,
        bounds=bounds)
    if has_plot:
        if args.output == None:
            print('INFO:Show window with graph')
            plt.show()
        else:
            print('INFO:Image saved to %s' % args.output)
            plt.savefig(args.output)
    else:
        print("ERROR: Don't have plot. Can't print data.")

if __name__ == '__main__':
    main()
