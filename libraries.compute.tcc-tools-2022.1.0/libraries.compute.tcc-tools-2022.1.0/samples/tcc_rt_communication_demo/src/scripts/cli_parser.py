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
import argparse
import os
import sys

import config_base
import decorators
import tracer
from typing import List, Any
from common import supported_cpus

incoming_arguments_handlers: List[Any] = []
traces = tracer.traces()


def parse_cli():
    profile_help_text = '''Define the name of the profile to be executed.
The following profiles are supported ('a' for Board A and 'b' for Board B):
    Basic, with and without optimizations:
        basic-a-opt
        basic-a-noopt
        basic-b-opt
        basic-b-noopt
    SISO, with and without optimizations:
        siso-single-a-opt
        siso-single-a-noopt
        siso-single-b-opt
        siso-single-b-noopt'''
    interface_help_text = 'Define the name of the network interface ' \
                          'to be used for TSN-based data transmission.'
    remote_interface_help_text = 'Define the name of the network interface ' \
                                 'on remote side to be used for TSN-based data transmission. ' \
                                 'If not specified, the local interface name will be used.'
    mode_help_text = '''Define the operation mode to be used for customization of sample behavior.
The following modes are supported:
    all (default): run all modes/stages: TSN configuration,
        clock synchronization, and run the sample application
    setup: configure the network interfaces, (re-)start
        time syncronization.
    clock: (Re-)start ptp4l and phc2sys clock synchronization
    tsn: Configure the network interface and the traffic control for TSN
    run: Only run the sample application; TSN and clock should already be configured'''
    best_effort_help_text = 'Do not use Best Effort traffic (iperf3) together ' \
                            'with other components of the sample application.'
    config_path_help_text = '''Define the directory that contains profiles configuration files.
'./cfg/' is used by default. The directory should contain 2 subdirectories:
'basic/' for basic-<...> profiles configuration files, and
'siso-single/' for siso-single-<...> profiles configuration files.'''
    remote_config_path_help_text = '''Define the directory on remote side that contains profiles configuration files.
'./cfg/' is used by default. The directory should contain 2 subdirectories:
'basic/' for basic-<...> profiles configuration files, and
'siso-single/' for siso-single-<...> profiles configuration files.'''
    output_file_help_text = '''Define the file name for storing the output statistics data.'''
    exec_path_help_text = '''Define the directory with binary executables such as opcua_server.
Sample uses these binary executables from general system directory with binaries as per
default and thus they are available by search from PATH environment.
Usage of this option prepends provided directory to PATH variable.'''
    remote_exec_path_help_text = '''Define the directory on remote side with binary executables such as opcua_server.
Sample uses these binary executables from general system directory with binaries as per
default and thus they are available by search from PATH environment.
Usage of this option prepends provided directory to PATH variable.'''
    remote_address_help = '''Defines the IP address for the 2nd remote target. If provided, the sample is launched
first on the remote side, making it possible to launch the whole RT communication Sample from one side.
This parameter can only be combined with 'basic-a-opt/noopt' or 'siso-single-a-opt/noopt' profiles.'''
    target_help = '''Define type of target board to be used for selection of configuration for execution.
Available types are:
    AUTO (default): autodetect the target
    EHL: use configuration for Intel Atom® x6000E Series processors
    TGL-U: use configuration for 11th Gen Intel® Core™ processors
    TGL-H: use configuration for Intel® Xeon® W-11000E Series processors'''
    base_time_help = '''Optional requested base time (UTC) to start communication.
If not provided, it is selected automatically'''
    verbose_help_text = '''Enable the verbose mode.'''
    parser_description = '''Profile name and interface name (described below) are required.'''
    parser_epilog = 'Example: tcc_rt_communication_demo --profile basic-a-opt ' \
                    '--interface enp1s0 --mode run'

    parser_usage = 'tcc_rt_communication_demo --profile <profile name> ' \
                   '--interface <interface name> [--mode <mode name>] ' \
                   '[--no-best-effort] [--config-path <directory with profiles configuration>] ' \
                   '[--output-file <output file>] [--exec-path <path to binary executables>] ' \
                   '[--target <target>] [--base-time <UTC base time>] ' \
                   '[--remote-address <remote-target-address>] ' \
                   '[--remote-interface <remote interface name>] ' \
                   '[--remote-config-path <remote directory with profiles configuration>] ' \
                   '[--remote-exec-path <remote path to binary executables>] [--verbose]'

    parser = argparse.ArgumentParser(description=parser_description,
                                     epilog=parser_epilog,
                                     formatter_class=argparse.RawTextHelpFormatter,
                                     usage=parser_usage)
    parser.add_argument('-p', '--profile', dest='profile', default=None,
                        type=str, help=profile_help_text, required=True)
    parser.add_argument('-i', '--interface', dest='interface',
                        type=str, help=interface_help_text, required=True)
    parser.add_argument('-r', '--remote-interface', dest='remote_interface',
                        type=str, help=remote_interface_help_text, default=None)
    parser.add_argument('-m', '--mode', dest='mode', default='all',
                        type=str, help=mode_help_text)
    parser.add_argument('-n', '--no-best-effort', dest='best_effort', default=True,
                        action='store_false', help=best_effort_help_text)
    parser.add_argument('-c', '--config-path', dest='config_path', default='cfg',
                        type=str, help=config_path_help_text)
    parser.add_argument('-g', '--remote-config-path', dest='remote_config_path',
                        default='cfg', type=str, help=remote_config_path_help_text)
    parser.add_argument('-o', '--output-file', dest='output_file', default=None,
                        type=str, help=output_file_help_text)
    parser.add_argument('-e', '--exec-path', dest='exec_path', default=None,
                        type=str, help=exec_path_help_text)
    parser.add_argument('-u', '--remote-exec-path', dest='remote_exec_path', default=None,
                        type=str, help=remote_exec_path_help_text)
    parser.add_argument('-a', '--remote-address', dest='remote_address', default=None,
                        type=str, help=remote_address_help)
    parser.add_argument('-t', '--target', dest='target', default='AUTO',
                        type=str, help=target_help)
    parser.add_argument('-b', '--base-time', dest='base_time', default=0,
                        type=int, help=base_time_help)
    parser.add_argument('-v', '--verbose', dest='verbose', default=False,
                        action='store_true', help=verbose_help_text)

    # show help if no arguments provided, default behavior of argument parser
    # above is showing error because required arguments are missing, moreover
    # it would be better to keep legacy behavior
    if len(sys.argv) < 2:
        parser.print_help(sys.stderr)
        sys.exit(os.EX_SOFTWARE)

    args = parser.parse_args()

    # set verbose mode as early as possible in order to print full scope of
    # debug messages if necessary
    if args.verbose:
        traces.set_log_level(tracer.log_level.DEBUG)

    # print all trace messages (including low level work scenarios)
    debug2 = os.environ.get('RTC_TRACE', '')
    if debug2 == 'DEBUG2':
        traces.set_log_level(tracer.log_level.DEBUG2)

    return args


# Handler for profile option
@decorators.local_handler(incoming_arguments_handlers)
def profile_handler(args, rt_conf):
    traces.debug2(f'Profile: {args.profile}')
    if args.profile not in config_base.AVAIL_PROFILES:
        traces.panic(f'Profile {args.profile} not supported. Use [-h] to see help information.')
    rt_conf.set_profile(profile_name=args.profile)


# Handler for interface option
@decorators.local_handler(incoming_arguments_handlers)
def interface_handler(args, rt_conf):
    traces.debug2(f'Interface: {args.interface}')
    sysfs_net_path = '/sys/class/net'
    if not os.path.exists(sysfs_net_path):
        traces.panic(f'path {sysfs_net_path} does not exist, update implementation')
    sysfs_if_path = os.path.join(sysfs_net_path, args.interface)
    if not os.path.exists(sysfs_if_path):
        traces.panic('Unknown interface name was specified. Specify valid interface '
                     'name (check with ifconfig tool, for example).')
    rt_conf.interface = args.interface
    config_base.register_preprocess_pair('interface', rt_conf.interface)


# Handler for config path option
@decorators.local_handler(incoming_arguments_handlers)
def config_path_handler(args, rt_conf):
    traces.debug2(f'Config path: {args.config_path}')
    if not os.path.isabs(args.config_path):
        current_executable = os.path.abspath(__file__)
        executable_dir = os.path.dirname(current_executable)
        args.config_path = os.path.join(executable_dir, args.config_path)
        traces.debug2(f'Config path (expanded): {args.config_path}')

    if not os.path.exists(args.config_path):
        traces.panic(f'Directory {args.config_path} with configuration profiles does not exist.')
    rt_conf.config_path = args.config_path
    config_base.register_preprocess_pair('config_path', rt_conf.config_path)


# Handler for output file option
@decorators.local_handler(incoming_arguments_handlers)
def output_file_handler(args, rt_conf):
    if args.output_file is None:
        rt_conf.set_output_file(profile_name=args.profile)
        config_base.register_preprocess_pair('output_file', rt_conf.output_file)
        return
    traces.debug2(f'Output file: {args.output_file}')
    output_directory = os.path.dirname(args.output_file)
    if output_directory == '':
        output_directory = os.getcwd()
    if not os.path.exists(output_directory):
        traces.panic(f'Path to output file ({output_directory}) does not exist.')
    if os.path.exists(args.output_file):
        traces.warning(f'output file {args.output_file} exists and may be overwritten')
    rt_conf.set_output_file(profile_name=args.profile, output_file=args.output_file)
    config_base.register_preprocess_pair('output_file', rt_conf.output_file)


# Handler for mode option
@decorators.local_handler(incoming_arguments_handlers)
def mode_handler(args, rt_conf):
    traces.debug2(f'Mode: {args.mode}')
    if args.mode not in config_base.AVAIL_MODES:
        traces.panic(f'Mode {args.mode} not supported. Use [-h] to see help information.')
    rt_conf.set_mode(mode_name=args.mode)
    config_base.register_preprocess_pair('mode', args.mode)


# Handler for exec path option
@decorators.local_handler(incoming_arguments_handlers)
def exec_path_handler(args, rt_conf):
    if args.exec_path is None:
        return
    traces.debug2(f'Exec path: {args.exec_path}')
    if not os.path.exists(args.exec_path):
        traces.panic(f'Path to directory with binary executables ({args.exec_path}) does not exist.')
    # Extend (prepend) ${PATH} here, it is appropriate place
    path = os.environ['PATH']
    path = f'{args.exec_path}:{path}'
    os.environ['PATH'] = path
    traces.debug2(f'PATH variable: {os.environ["PATH"]}')
    ld_path = os.environ.get('LD_LIBRARY_PATH', None)
    if ld_path is None:
        ld_path = args.exec_path
    else:
        ld_path = f'{args.exec_path}:{ld_path}'
    os.environ['LD_LIBRARY_PATH'] = ld_path
    traces.debug2(f'LD_LIBRARY_PATH variable: {os.environ["LD_LIBRARY_PATH"]}')
    traces.warning(f'directory {args.exec_path} has been prepended to environment variables')


# Handler for stress options
@decorators.local_handler(incoming_arguments_handlers)
def stresses_handler(args, rt_conf):
    rt_conf.use_stresses = args.best_effort


# Handler for remote interface options
@decorators.local_handler(incoming_arguments_handlers)
def remote_interface_handler(args, rt_conf):
    rt_conf.remote_interface = args.remote_interface \
        if args.remote_interface is not None else args.remote_interface
    config_base.register_preprocess_pair('remote_interface', rt_conf.remote_interface)


# Handler for remote config path options
@decorators.local_handler(incoming_arguments_handlers)
def remote_config_path_handler(args, rt_conf):
    rt_conf.remote_config_path = args.remote_config_path


# Handler for remote exec path options
@decorators.local_handler(incoming_arguments_handlers)
def remote_exec_path_handler(args, rt_conf):
    rt_conf.remote_exec_path = args.remote_exec_path \
        if args.remote_exec_path is not None else args.exec_path
    if rt_conf.remote_exec_path is not None:
        config_base.register_preprocess_pair('remote_app_path', rt_conf.remote_exec_path)


# Handler for remote address options
@decorators.local_handler(incoming_arguments_handlers)
def remote_address_handler(args, rt_conf):
    # If 'remote_address' is not None, we switch mode of operation to single-side launch
    # running all components from one side (A-side)
    rt_conf.remote_address = args.remote_address
    if rt_conf.remote_address is not None:
        print("[INFO] Automatic launch mode")


# Handler for remote address options
@decorators.local_handler(incoming_arguments_handlers)
def base_time_handler(args, rt_conf):
    if args.base_time:
        rt_conf.set_base_time(args.base_time)


# Dummy handler for remote working directory
@decorators.local_handler(incoming_arguments_handlers)
def remote_working_dir_handler(args, rt_conf):
    config_base.register_preprocess_pair('remote_working_directory', '/tmp')


# Dummy handler for remote output directory
@decorators.local_handler(incoming_arguments_handlers)
def remote_output_path_dir_handler(args, rt_conf):
    config_base.register_preprocess_pair('remote_output_path', '/tmp/tcc_rt_communication_demo_output')


# Handler for verbosity option
@decorators.local_handler(incoming_arguments_handlers)
def verbosity_handler(args, rt_conf):
    rt_conf.verbosity = args.verbose


# Handler for target option
@decorators.local_handler(incoming_arguments_handlers)
def target_handler(args, rt_conf):
    if args.target not in config_base.AVAIL_TARGETS:
        traces.panic(f'Target "{args.target}" not supported. Use [-h] to see help information')
    traces.debug(f'Target: {args.target}')
    if args.target == 'AUTO':
        # Autodetect the target
        model_name = supported_cpus.get_model_name()
        cpu_info = supported_cpus.get_supported_cpu_info(model_name)
        if cpu_info is None:
            traces.panic('Cannot auto-detect the target. Try manually specifying '
                         'the target with [-t] argument')
        if supported_cpus.Platforms.EHL.value == cpu_info.processor_name:
            rt_conf.target = 'EHL'
        elif supported_cpus.Platforms.TGL_U.value == cpu_info.processor_name:
            rt_conf.target = 'TGL-U'
        elif supported_cpus.Platforms.TGL_H.value == cpu_info.processor_name:
            rt_conf.target = 'TGL-H'
        else:
            traces.panic(f'Target autodetection failed, unknown platform: '
                         f'{cpu_info.processor_name}')
        traces.debug(f'Auto-detected target: {rt_conf.target}, processor name: '
                     f'{cpu_info.processor_name}')
    else:
        rt_conf.target = args.target


# Runner of registered callbacks in this module
def validate_incoming_arguments(cli_args, rt_conf):
    for checker in incoming_arguments_handlers:
        traces.debug2(f'executing {checker.__name__} callback')
        # Each callback function behaves as validator and propagator,
        # it aims to the following purposes:
        # 1. Validate incoming data from user
        # 2. Configure runtime environment (e.g. change $PATH if needed)
        # 3. Propagate values into config_base core singleton class'es instance
        # for further processing, in other words configure sample's runtime
        checker(cli_args, rt_conf)


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
