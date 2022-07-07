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

#tcc_measurement_analysis_sample is an example of a profiling application.

from sys import version_info, exit

PYTHON_REQUIRED_MAJOR_VERSION = 3
PYTHON_REQUIRED_MINOR_VERSION = 5

if version_info < (PYTHON_REQUIRED_MAJOR_VERSION, PYTHON_REQUIRED_MINOR_VERSION):
    exit('You are running Python ' + str(version_info[0]) + '.' + str(version_info[1]) + '\n'
          + 'Python ' + str(PYTHON_REQUIRED_MAJOR_VERSION) + '.' + str(PYTHON_REQUIRED_MINOR_VERSION)
          + ' or newer is required\n')

import argparse
import os
import subprocess
import sys
# hotspot modules:
import modules.tcc_measurement_analysis_utils as utils
import modules.tcc_measurement_analysis_hist as hist
import modules.tcc_measurement_analysis_monitor as monitor


def create_parser():
    """Create parser"""
    parser = argparse.ArgumentParser(
        prog=utils.hotspot_tool_name,
        formatter_class=argparse.RawTextHelpFormatter,
        add_help=False
    )
    parser.add_argument('-h','--help', action='help', help='Show this help message and exit')
    subparsers = parser.add_subparsers(help='Supported commands',
                                       dest='command')                                   

    # hist command:
    parser_hist = subparsers.add_parser('hist',
                                        prog=utils.hotspot_tool_name,
                                        help='Show histogram from data collected by measurement library',
                                        formatter_class=argparse.RawTextHelpFormatter,
                                        add_help=False)
    # monitor command:
    parser_monitor = subparsers.add_parser('monitor',
                                        prog=utils.hotspot_tool_name,
                                        help='Monitor measurements collected by measurement library',
                                        formatter_class=argparse.RawTextHelpFormatter,
                                        add_help=False)
    parser_monitor.add_argument(
        '-deadline-only',
        help='Print only the values exceeding the deadline.\n',
        action='store_true'
    )

    positionals_title = 'commands'
    optionals_title = 'options'

    # common options | styling:
    for _, subparser in subparsers.choices.items():
        # Arguments
        subparser.add_argument(
            'measurement_instances',
            type=str,
            help='Specify the measurement_instances inside the profiled application to use to collect the measurement history.\n'
                 'This argument can be omitted if environment variable TCC_MEASUREMENTS_BUFFERS is specified\n'
                 '(TCC_MEASUREMENTS_BUFFERS serves the same purpose). This argument overrides TCC_MEASUREMENTS_BUFFERS\n'
                 'environment variable.\n'
                 'Format: <measurement_instance_name>:<buffer_size>[,<measurement_instance_name>:<buffer_size> ...],\n'
                 'where the measurement instances name is the measurement name passed to __itt_string_handle_create()\n'
                 'in the profiled application, and the buffer size is the maximum number of measurements that the buffer can store.\n'
                 'Specifying too high a value will require lots of memory for the profiled application.\n'
                 'Example: "Cycle:1000,Sensor:1000"\n'
                 'NOTE: If the buffer size defined in the measurement_instances argument does not match the total number of\n'
                 'measurements (that is, only N last samples are collected), the full data statistics\n'
                 '(average, maximum, and minimum) may not match the histogram statistics.',
            nargs='?',
            default=''
        )
        subparser.add_argument(
            'app',
            nargs='?',
            help='Specify the path to the profiled application (example: ./tcc_multiple_measurements_sample).',
            default=''
        )
        subparser.add_argument(
            'args',
            nargs='*',
            type=str,
            help='Command-line arguments for the application being profiled.',
            default=''
        )

        # Options
        subparser.add_argument('-h','--help', action='help', help='Show this help message and exit')
        subparser.add_argument(
            '-dump-file',
            metavar='file',
            type=str,
            help='Specify the history dump file path from the profiled application.\n'
                 'The file contains measurements collected by the measurement library during application run.\n'
                 'This option overrides TCC_MEASUREMENTS_DUMP_FILE environment variable',
            default=''
        )
        subparser.add_argument(
            '-time-units',
            metavar='units',
            type=str,
            help=
            'Specify time units\n'
            '"ns" for nanoseconds\n'
            '"us" for microseconds\n'
            '"clk" for CPU clock cycles (default)\n'
            'This option overrides TCC_MEASUREMENTS_TIME_UNIT environment variable\n',
            default=None
        )
        subparser.add_argument(
            '-nn',
            metavar='cpu',
            help='Run noisy neighbor (stress-ng) on cpu id, for example, -nn 3'
        )
        # styling:
        positionals_title_for_subparsers = 'command arguments'
        subparser._positionals.title = positionals_title_for_subparsers
        subparser._optionals.title = optionals_title

    # styling:
    parser.usage = '%(prog)s hist | monitor\n'
    parser._positionals.title = positionals_title
    parser._optionals.title = optionals_title
    parser.epilog = 'See \'%(prog)s <command> --help(-h)\'\nto get information about specific command.'

    parser_hist.usage = '%(prog)s hist [<measurement_instances>] [-dump-file] [-time-units] [-nn cpu] -- <app> [<args>]'
    parser_monitor.usage = '%(prog)s monitor [<measurement_instances>] [-deadline-only] [-dump-file] [-time-units] [-nn cpu] -- <app> [<args>]'
    return parser

def main():
    """Main logic """
    app = ""
    args = []
    p = create_parser()
    for arg in sys.argv:
        if arg == '--' and ((sys.argv.index(arg)+1) < len(sys.argv)):
            app = sys.argv[sys.argv.index(arg)+1]
            args = sys.argv[sys.argv.index(arg)+2:]
            sys.argv = sys.argv[:sys.argv.index(arg)]
    cli_args = p.parse_args()
    if len(sys.argv) <= 1 or app == "":
        p.print_help()
        return

    exit_sts = 0  # function exit status

    # noise
    if cli_args.nn is not None:
        res = open('/proc/cpuinfo').read().count('processor\t:')
        if res < 0:
            sys.stderr.write("Error! Unable to open file /proc/cpuinfo.\n")
            return 1
        if int(cli_args.nn) >= res:
            sys.stderr.write("Invalid core number in '-nn'.\n")
            return 1
        proc = utils.start_noisy_neighbor(cli_args.nn)
    ############################
    # sample commands execution

    # Set the itt notify environment
    setenv_res = utils._setenv_libittnotify()
    if setenv_res != 0:
        return setenv_res

    # Get time units
    if cli_args.time_units is not None:
        time_unit = cli_args.time_units
    elif os.getenv("TCC_MEASUREMENTS_TIME_UNIT") is not None:
        time_unit = os.getenv("TCC_MEASUREMENTS_TIME_UNIT")
    else:
        time_unit = 'clk'

    if cli_args.command == 'hist':
        exit_sts = hist.make_hist(cli_args.measurement_instances, cli_args.dump_file, time_unit, app, args)
    elif cli_args.command == 'monitor':
        exit_sts = monitor.monitor(cli_args.measurement_instances, cli_args.dump_file, time_unit, cli_args.deadline_only, app, args)
    # sample commands execution
    ############################

    # noise
    if cli_args.nn is not None:
        utils.stop_noisy_neighbor(proc)

    return exit_sts


if __name__ == "__main__":
    utils.init_globals(os.path.basename(__file__))
    sys.exit(main())
