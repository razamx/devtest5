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
for tcc_tgpio_frequency_discipline_sample sample

File format: CSV-file. First column is an edge time, other columns are edge
direction (0 - falling, 1 - rising). One edge per line.

One channel is required
Time[s],           Channel 0
0.000000000000000, 0
1.017330524000000, 1
2.131018628000000, 0
2.131057090000000, 1
2.131095558000000, 0
...

Time should be in seconds
"""

import numpy as np
import matplotlib.pyplot as plt
from common.log_helper import check_file
import argparse

time_units = {
    'ns': lambda x: x / 10**9,
    'us': lambda x: x / 10**6,
    'ms': lambda x: x / 10**3,
    's': lambda x: x,
    'Hz': lambda x: 1/x,
    'kHz': lambda x: 1 / (x * 10**3),
    'MHz': lambda x: 1 / (x * 10**6),
}

def pair_parser(arg):
    split = arg.split(',')
    if len(split) != 2:
        raise argparse.ArgumentError(None, 'File should be specified as <filename>,<period|frequency>')
    file, timeunit = split
    return file, float(timeunit)

def init_parser():
    parser = argparse.ArgumentParser(description="Analyzes the captured data from tcc_frequency_discipline_sample",
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("files", type=pair_parser, nargs='+', help='Log file or files: <path>,<period|frequency> [<path>,<period|frequency> ...]')
    parser.add_argument("--units", choices=time_units.keys(),
        default='ns', help='Time units to display')
    parser.add_argument("--output", type=str, help='Output file; otherwise, display in window')
    return parser

# Parse signal file and extract timestamps of the each edge
def parse_edges_timestamps(filename):
    check_file(filename)
    with open(filename, 'r') as f:
        # Skip file header
        next(f)
        data = np.array([[float(inst) for inst in line.split(',')] for line in f])
        # Find index of the first rising edge
        start_index = np.where(data[:,1] == 1)[0][0]

        # Workaround: sometimes TGPIO triggers after PTP_PEROUT_REQUEST and before
        # signal generation starts
        #     __________   _   _   _
        # ___|          |_| |_| |_| |_ ...
        # Remove this first long period from data
        PERIOD_THRESHOLD = 10
        if len(data) < start_index+3:
            raise ValueError('Not enough data in the signal file')
        T1 = data[start_index+1][0] - data[start_index][0]
        T2 = data[start_index+2][0] - data[start_index+1][0]
        if T1 > PERIOD_THRESHOLD * T2:
            start_index += 1

        return data[start_index:-1, 0]

# Calcs accumulated phase error
def calc_phase_error(timestamps, expected_period):
    # Assume accumulated phase increases every edge (half-period) by pi
    # Phase of the i-th edge of the real signal is "i (pi)"
    t0 = timestamps[0]
    phase_expected = (timestamps - t0) / expected_period * 2.0
    phase_real = np.arange(len(timestamps))
    return phase_real - phase_expected

def parse_args():
    parser = init_parser()
    args = parser.parse_args()
    return args

def convert_time_unit(period_or_freq, time_unit):
    return time_units[time_unit](period_or_freq)

def main():
    args = parse_args()

    for filename, period_or_freq in args.files:
        measurement = parse_edges_timestamps(filename)
        period = convert_time_unit(period_or_freq, args.units)
        error = calc_phase_error(measurement, period)
        plt.plot(measurement - measurement[0], error, label=filename)

    plt.title('Phase error')
    plt.xlabel('Time, s')
    plt.ylabel('Phase error, $\pi$')
    plt.legend()

    if args.output == None:
        plt.show()
    else:
        print(f'Image saved to {args.output}')
        plt.savefig(args.output)

if __name__ == '__main__':
    main()
