#!/usr/bin/env python3
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
import math
import shutil
import subprocess
from collections import defaultdict, Counter
# hotspot utils
import modules.tcc_measurement_analysis_utils as utils


def _parse_hist_data():
    """
    Parse measurement library raw data

    _parse_hist_data() -> dict:
        { <measurement_instance_name>: (<counter>, <size>) }
        <measurement_instance_name> -- Measurement instance name (e.g., "Sensor")
        <counter> -- Counter() instance to aggregate sample data
        <size> -- Number of samples for a specific measurement
    """
    msmt_dump_filepath = os.environ['TCC_MEASUREMENTS_DUMP_FILE']
    # file exists:
    if not os.path.isfile(msmt_dump_filepath):
        utils.colored_print(
            'Cannot open TCC_MEASUREMENTS_DUMP_FILE to collect measurement history.\n'
            'Path: {0}'.format(msmt_dump_filepath),
            style=utils.AnsiStyles.FAIL
        )
        return None
    # get measurement_instances that need histograms:
    measurement_instances = os.environ['TCC_MEASUREMENTS_BUFFERS']
    measurement_instances = [value.split(':')[0].strip() for value in measurement_instances.split(',')]

    # read file and parse measurement history:
    hist_f = open(msmt_dump_filepath, 'r')
    history_per_measurement_instance = defaultdict(lambda: (Counter(), 0))
    for line in hist_f:
        if not line.strip():
            continue
        measurement_name, history_str = line.split(':')
        measurement_name = measurement_name.strip()
        history_str = history_str.strip()
        if not history_str or not measurement_name or measurement_name not in measurement_instances:
            continue
        measurement_history = list(map(int, history_str.split(',')))
        counter, number_of_elements = history_per_measurement_instance[measurement_name]
        counter.update(measurement_history)
        number_of_elements += len(measurement_history)
        history_per_measurement_instance[measurement_name] = counter, number_of_elements

    return history_per_measurement_instance


def _create_and_draw_hist(history_per_measurement_instance):
    """
    Create histogram and draw it to terminal

    Arguments:
    history_per_measurement_instance -- each measurement instance's history data (samples and size)
    """
    def find_index(latency_ranges, value):
        for i, latency_ranges_pair in enumerate(latency_ranges):
            if value >= latency_ranges_pair[0] and value <= latency_ranges_pair[1]:
                return i
        return len(latency_ranges) - 1

    def get_n_latency_ranges(num_elems):
        return (math.ceil(math.log(num_elems, 2)) + 1) * 3

    def make_relative(count, num_elems):
            return count / num_elems

    def create_hist_column(rel_count, multiplier):
        if rel_count == 0.0:
            return ''
        mult = int(rel_count * multiplier)
        mult = mult if mult > 0 else 1
        return '|'*mult

    for cycle_name, cycle_data in history_per_measurement_instance.items():
        counter, number_of_elements = cycle_data
        if number_of_elements <= 0:
            continue
        n_latency_ranges = get_n_latency_ranges(number_of_elements)
        min_val, max_val = min(counter.keys()), max(counter.keys())
        latency_ranges_size = int((max_val - min_val) / n_latency_ranges) + 1
        latency_ranges = []
        for i in range(n_latency_ranges):
            latency_ranges.append((min_val + i * latency_ranges_size, min_val + (i + 1) * latency_ranges_size))

        histogram = dict(map(lambda x: (x, 0), latency_ranges))
        hist = list(map(lambda x: 0, latency_ranges))
        for number, count in counter.items():
            index = find_index(latency_ranges, number)
            hist_value = histogram.setdefault(latency_ranges[index], 0)
            histogram[latency_ranges[index]] = hist_value + count
            hist[index] += count

        unit = os.environ.get('TCC_MEASUREMENTS_TIME_UNIT').lower()
        unit_print = unit if unit in ["clk", "ns", "us"] else "clk"
        latency_ranges_col_title = 'Latency Ranges'+ ' [' + unit_print + ']'
        count_col_title = 'Count'
        max_length = max(max(map(lambda x: len(str(x)), latency_ranges)), len(latency_ranges_col_title))
        digits = max(int(math.log10(number_of_elements)) + 1, len(count_col_title))
        table_template = '{LATENCY_RANGES:{LATENCY_RANGES_ALIGN}%d}   {COUNT:{COUNT_ALIGN}%d}   {HIST}' % (
            max_length,
            digits
        )
        terminal_cols = shutil.get_terminal_size().columns
        print('-'*(terminal_cols))
        print('"{0}" histogram\n'.format(cycle_name))

        print(table_template.format(
            LATENCY_RANGES=latency_ranges_col_title,
            LATENCY_RANGES_ALIGN='<',
            COUNT=count_col_title,
            COUNT_ALIGN='<',
            HIST='Histogram'
        ))
        index=0
        for interval, count in sorted(histogram.items()):
            if index <= find_index(latency_ranges, max_val):
                rel_count = make_relative(count, number_of_elements)
                print(table_template.format(
                    LATENCY_RANGES=str(interval),
                    LATENCY_RANGES_ALIGN='<',
                    COUNT=count,
                    COUNT_ALIGN='<',
                    HIST=create_hist_column(rel_count, terminal_cols / 2)
                ))
            index += 1

        print('\n Total counts: {0}'.format(number_of_elements))
        elements = [e for e in counter.elements()]
        if number_of_elements > 1:
            avg = sum(elements) / number_of_elements
            stddev = math.sqrt(sum([math.pow(e - avg, 2) for e in elements]) / (number_of_elements - 1))
            print(' Avg. Value: {0}\n Std. Deviation: {1}'.format(math.floor(avg), math.floor(stddev)))
        else:
            utils.colored_print(
                'Standard deviation is undefined for {0} counts'.format(number_of_elements),
                style=utils.AnsiStyles.WARNING
            )

def make_hist(measurement_instances_string, dump_file, time_units, app, app_args):
    """
    Main function for drawing histogram:
        Set environment
        Run application
        Collect measurement library data
        Draw histogram

    Arguments:
    measurement_instances_string -- measurement instances names for which data is to be collected
    dump_file -- history data dump file populated by measurement library
    app -- application path
    app_args -- application command-line arguments as list
    """
    setenv_res = utils._setenv_dump_file(dump_file, app)
    if setenv_res != 0:
        return setenv_res
    setenv_res = utils._setenv_time_units(time_units)
    if setenv_res != 0:
        return setenv_res
    setenv_res = utils._setenv_buffer(measurement_instances_string, 'hist')
    if setenv_res != 0:
        return setenv_res

    # clean up old instance of measurement dump file
    utils._cleanup_dump_file()

    if shutil.which(app) is None:
        utils.colored_print(
            'Application [{0}] does not exist'.format(os.path.basename(app)),
            style=utils.AnsiStyles.FAIL
        )
        return 1

    print('\nAPPLICATION OUTPUT:\n', flush=True)
    command = '{0} {1}'.format(app, ' '.join(app_args))
    completed_proc = subprocess.run(command.split())
    if completed_proc.returncode != 0:
        utils.colored_print(
            'Application [{0}] failed to run\n'
            'Arguments: {1}\n'
            'Return code {2}:'.format(os.path.basename(app), app_args, completed_proc.returncode),
            style=utils.AnsiStyles.FAIL
        )
        return 1

    history_per_measurement_instance = _parse_hist_data()
    if history_per_measurement_instance is None:
        return 1
    print('\n'+'-'*30)
    print('PROFILING OUTPUT:\n\n')
    _create_and_draw_hist(history_per_measurement_instance)
    return 0


if __name__ == "__main__":
    utils.print_unsupported_module_call()
