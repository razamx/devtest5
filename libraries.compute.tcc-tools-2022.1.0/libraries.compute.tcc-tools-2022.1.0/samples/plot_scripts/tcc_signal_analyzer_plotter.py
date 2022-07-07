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

"""
This script analyze periodic digital signals captured from logic analyzer
for tcc_tgpio_advanced_sample and tcc_ethernet_timestamps_sample

File format: CSV-file. First column is an edge time, other columns are edge
direction (0 - falling, 1 - rising). One edge per line.

For --period option:
One channel is required
Time[s],           Channel 0
0.000000000000000, 0
1.017330524000000, 1
2.131018628000000, 0
2.131057090000000, 1
2.131095558000000, 0
...

For --shift option:
Two channels are required
Time[s],           Channel 0,  Channel 1
0.000000000000000, 0           0
1.017330524000000, 1           0
2.131018628000000, 1           1
2.131057090000000, 0           1
2.131095558000000, 0           0
...

Time should be in seconds
"""

import os
import sys
import argparse
import csv
import numpy as np
import matplotlib.pyplot as plt
from common.log_helper import plot_scatter, convert_time_unit, check_file

def init_parser():
    parser = argparse.ArgumentParser(description="Analyzes the captured data from tcc_tgpio_advanced_sample and tcc_ethernet_timestamps_sample. Calculates jitter, other statistics, and plot distribution",
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("log_file", nargs="+", help="Signal file captured with logic analyzer, oscilloscope, etc.")
    g = parser.add_mutually_exclusive_group(required=True)
    g.add_argument("--period", action="store_true", help="Plot the periodic signal")
    g.add_argument("--shift", action="store_true", help="Plot the phase difference between two signals")
    parser.add_argument("--output", type=str, help='Output file; otherwise, display in window')
    parser.add_argument("--min", type=int, help="Minimum plotting boundary")
    parser.add_argument("--max", type=int, help="Maximum plotting boundary")
    parser.add_argument("--units", choices=['s', 'ms', 'us', 'ns'], default='ns', help='Time units to display (default: nanosecond)')
    return parser

# Trim several arrays to the shortest length
def trim_shortest(data):
    min_length = min([len(d) for d in data])
    return np.array([d[:min_length] for d in data])

# Read the log file and return the duration of every half-period
# (time between every pin change)
def get_periods(filename):
    check_file(filename)
    with open(filename, 'r') as f:
        try:
            header = next(f)  # Skip file header
        except:
            print("File '%s' is Empty" % filename)
            exit(-1)
        if len(header.split(',')) != 2:
            print('Invalid number of channels in file')
            print('--period mode requires one channel in file')
            exit(-1)

        data = np.array([[float(inst) for inst in line.split(',')] for line in f])[:,0]
        # Remove the first measurement because the logic analyzer can start recording
        # at an arbitrary time, not only the exact beginning of the period
        return (data[1:] - data[:-1])[1:]

def check_edge(cur, prev, index):
    if prev[index] == 0 and cur[index] == 1:
        return 1
    elif prev[index] == 1 and cur[index] == 0:
        return -1
    else:
        return 0

def parse_row(row):
    return [float(row[0])] + [int(channel) for channel in row[1:]]

# Read the log file and return the offset between corresponding edges of the two signals
def get_offsets(filename):
    check_file(filename)
    with open(filename, newline='') as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        try:
            header = next(reader) # Skip file header
        except:
            print("File '%s' is Empty" % filename)
            exit(-1)
        if len(header) != 3:
            print('Invalid number of channels in file')
            print('--shift mode requires two channels in file')
            exit(-1)

        fild_low = False
        rising_first = []
        rising_second = []

        dataset = [parse_row(row) for row in reader]

        for i in range (0, len(dataset)):
            if not fild_low and dataset[i][1] == 0 and dataset[i][2] == 0:
                fild_low = True
                continue
            if fild_low:
                if dataset[i-1][1] == 0 and dataset[i][1] == 1:
                    rising_first.append(dataset[i][0])
                if dataset[i-1][2] == 0 and dataset[i][2] == 1:
                    rising_second.append(dataset[i][0])

        offsets = [second - first for first, second in zip(rising_first, rising_second)]

    if offsets == []:
        print("The data in the file '%s' does not match the format" % filename)
        exit(-1)
    return np.array(offsets)[1:]

# Read and plot the duration of the signal periods
def plot_period(files, bounds, time_unit, fig, grid):
    data = trim_shortest([convert_time_unit(get_periods(file), time_unit) for file in files])
    return plot_scatter(data, files, 'Period jitter (half-period)', fig, grid, time_unit, bounds=bounds)

# Read and plot the phase shift between two signals
def plot_shift(files, bounds, time_unit, fig, grid):
    data = trim_shortest([convert_time_unit(get_offsets(file), time_unit) for file in files])
    return plot_scatter(data, files, 'Phase jitter', fig, grid, time_unit, bounds=bounds)

def main():
    parser = init_parser()
    if len(sys.argv) == 1:
        parser.print_help()
        exit(1)

    args = parser.parse_args()

    if args.output is not None:
        plt.switch_backend('Agg')  # set non gui backend

    bounds = (args.min, args.max)
    fig = plt.figure(figsize=(12,8))
    grid = plt.GridSpec(1, 5, hspace=0.2, wspace=0)
    if args.period:
        has_plot = plot_period(args.log_file, bounds, args.units, fig, grid)
    elif args.shift:
        has_plot = plot_shift(args.log_file, bounds, args.units, fig, grid)
    else:
        exit(0)

    if has_plot:
        if args.output is None:
            plt.show()
        else:
            plt.savefig(args.output, format='png')
            print('Image saved to %s' % os.path.realpath(args.output))
    plt.close(fig)

if __name__ == '__main__':
    main()
