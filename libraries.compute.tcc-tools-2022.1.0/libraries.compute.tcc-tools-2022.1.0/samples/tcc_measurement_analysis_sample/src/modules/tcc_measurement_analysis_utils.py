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

import sys
import os
import signal
import subprocess
import shlex

def init_globals(tool_name):
    """Init global hotspot tool name"""
    global hotspot_tool_name
    hotspot_tool_name = tool_name


class AnsiStyles:
    """Enum-like list of ansi styles for colored output"""
    FAIL = '\u001b[31m'
    SUCCESS = '\u001b[32m'
    WARNING = '\u001b[33m'
    RESET = '\u001b[0m'
    BOLD = '\u001b[1m'
    UNDERLINE = '\u001b[4m'
    REVERSED = '\u001b[7m'


def colored_print(*objects, style, tool_named=False, sep='', end='\n', file=sys.stdout, flush=False):
    """
    Print with color support. Color works for terminal only
    See `python print()` for arguments description

    Specific arguments:
    style -- AnsiStyles value
    tool_named -- Flag to specify if tool name should be printed before `objects` (default False)
    """
    to_be_printed = objects  # objects is tuple
    if tool_named:
        to_be_printed = (hotspot_tool_name, ': ') + to_be_printed

    if sys.stdout.isatty():  # if stdout is directed into terminal
        print(
            style, *to_be_printed, AnsiStyles.RESET,
            sep=sep, end=end, file=file, flush=flush
        )
    else:  # if stdout is redirected (e.g., into file)
        print(
            *to_be_printed,
            sep=sep, end=end, file=file, flush=flush
        )


def to_ascii(bstring):
    """Transform binary string to ascii format"""
    return bstring.decode('ascii').strip(' \t\n\r')


def print_unsupported_module_call():
    """Print 'unsupported to call module' message"""
    colored_print(
        'Error: not supposed to call this module as a standalone component',
        style=AnsiStyles.FAIL
    )

def _setenv_dump_file(dump_file, app):
    """Set environment for measurement library"""
    msmt_dump_filepath = dump_file
    if not msmt_dump_filepath:
        msmt_dump_filepath = os.environ.get('TCC_MEASUREMENTS_DUMP_FILE', '')
        if not msmt_dump_filepath:
            msmt_dump_filepath = '{0}.hist'.format(os.path.basename(app))
    os.environ['TCC_MEASUREMENTS_DUMP_FILE'] = msmt_dump_filepath
    return 0

def _cleanup_dump_file():
    """Clean up measurement library dump file"""
    msmt_dump_filepath = os.environ.get('TCC_MEASUREMENTS_DUMP_FILE', '')
    if msmt_dump_filepath and os.path.isfile(msmt_dump_filepath):
        os.remove(msmt_dump_filepath)


def _setenv_time_units(time_units):
    """Set environment for measurement library"""
    if not time_units:
        time_units  = "CLK"
    os.environ['TCC_MEASUREMENTS_TIME_UNIT'] = time_units
    return 0

def _setenv_buffer(measurement_instances_string, mode):
    """Set environment for measurement library"""
    measurement_instances = measurement_instances_string
    if not measurement_instances:
        colored_print(
            'No measurement instances specified with `-measurement_instances` option. Trying TCC_MEASUREMENTS_BUFFERS',
            style=AnsiStyles.WARNING
        )
        measurement_instances = os.environ.get('TCC_MEASUREMENTS_BUFFERS', '')
        if not measurement_instances:
            if mode == "hist":
                colored_print(
                    'No measurement instances specified with TCC_MEASUREMENTS_BUFFERS to collect history dump for histogram',
                    style=AnsiStyles.FAIL
                )
            if mode == "monitor":
                colored_print(
                    'No measurement instances specified with TCC_MEASUREMENTS_BUFFERS to monitor data',
                    style=AnsiStyles.FAIL
                )
            return 1
    os.environ['TCC_MEASUREMENTS_BUFFERS'] = measurement_instances
    return 0

def _setenv_libittnotify():
    os.environ['INTEL_LIBITTNOTIFY64'] = 'libtcc_collector.so'
    return 0

def _setenv_shared_memory():
    os.environ['TCC_USE_SHARED_MEMORY'] = 'true'
    return 0


def start_noisy_neighbor(cpu_id):
    """Start stress-ng on certain CPU ID"""
    command ='stress-ng -t 0 -C 10 --cache-level 2 --taskset '\
             '{0} --aggressive -v --metrics-brief > stress-ng.log '\
             '2> /dev/null'.format(cpu_id)

    try:
        proc = subprocess.Popen(
            shlex.split(command),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
    except FileNotFoundError:
        colored_print('`stress-ng` not found. Install `stress-ng v.0.10` or newer and try again', style=AnsiStyles.FAIL)
        sys.exit(os.EX_USAGE)
    except Exception as e:
        colored_print('{0}\n`{1}` command failed'.format(str(e), command), style=AnsiStyles.FAIL)
        sys.exit(os.EX_SOFTWARE)

    try:
        out, err = proc.communicate(timeout=3)
    except subprocess.TimeoutExpired:
        pass

    return proc


def stop_noisy_neighbor(stress_proc):
    """Stop stress-ng"""

    stress_proc.poll()
    if stress_proc.returncode is not None:
        colored_print(
            "`stress-ng` finished earlier than expected. " \
            "Please re-run the sample to collect correct sample statistics",
            style=AnsiStyles.FAIL)
        sys.exit(os.EX_SOFTWARE)

    stress_proc.terminate()
    stress_proc.wait()

if __name__ == "__main__":
    print_unsupported_module_call()
