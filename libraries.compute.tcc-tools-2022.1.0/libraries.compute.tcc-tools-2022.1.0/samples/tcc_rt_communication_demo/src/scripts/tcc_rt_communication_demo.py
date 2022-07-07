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
from os import getenv, path, environ
from site import addsitedir
from sys import version_info, exit
import tracer
from shell_run_utils import is_application_available, supports_hw_timestamping_caps

traces = tracer.traces()
REQUIRED_STRESS_APPS = ['iperf3']
REQUIRED_REMOTE_APPS = ['ssh', 'scp']
REQUIRED_APPS = ['ip', 'tc', 'ethtool', 'ptp4l', 'phc2sys', 'pmc', 'hwstamp_ctl', 'taskset', 'killall']

# Look for the 'tools' directory
tools_path = getenv('TCC_TOOLS_PATH', None)
if not tools_path:
    # Search for the 'tools' directory
    path_base = path.dirname(path.realpath(__file__))
    while path_base and path_base != '/':
        path_name = "{}/tools".format(path_base)
        if path.isdir(path_name) and path.isdir("{}/common".format(path_name)):
            tools_path = path_name
            break
        path_base = path.dirname(path_base)

# If found, add the 'tools' directory to 'sys.path'
if tools_path:
    addsitedir(tools_path)


# Append the sample's main directory to the PATH environment variable,
# after all other defined locations.
# The directory contains a few required scripts that need to be available
# for the RTC sample:
# - helpers.sh;
# - apply_irq_affinities.sh;
sample_dir = path.dirname(path.realpath(__file__))
path_spec = environ['PATH']
path_spec = f'{path_spec}:{sample_dir}'
environ['PATH'] = path_spec


# Check the python runtime version
PYTHON_REQUIRED_MAJOR_VERSION = 3
PYTHON_REQUIRED_MINOR_VERSION = 6

if version_info < (PYTHON_REQUIRED_MAJOR_VERSION, PYTHON_REQUIRED_MINOR_VERSION):
    exit(f'You are running Python {version_info[0]}.{version_info[1]}\n'
         'Python {PYTHON_REQUIRED_MAJOR_VERSION}.{PYTHON_REQUIRED_MINOR_VERSION} '
         'or newer is required\n')


# We need to import these files after updating the search paths
import cli_parser   # noqa: E402
import config_base  # noqa: E402
import decorators   # noqa: E402


# Check if the requested applications are available in the system
def are_applications_available(applications, info_message: str, extra_info: str = "") -> None:
    traces.debug(f'{info_message}...')
    for application in applications:
        if not is_application_available(application):
            traces.panic(f'Required tool "{application}" is not available in the system'
                         f'{extra_info}')
    traces.debug(f'{info_message}: DONE')


def main():
    # Parse CLI arguments, store them in argparse object
    input_data = cli_parser.parse_cli()

    # Prepare runtime config, run validation and population handlers, perform
    # final check for input data consistency
    rt_conf = config_base.runtime_config()
    cli_parser.validate_incoming_arguments(input_data, rt_conf)
    rt_conf.check_config_data()

    # Check the run-time environment
    are_applications_available(REQUIRED_APPS, "Checking for required applications")
    if rt_conf.remote_address is not None:
        are_applications_available(REQUIRED_REMOTE_APPS,
                                   'Checking for required applications for the remote mode')
    if rt_conf.use_stresses:
        are_applications_available(REQUIRED_STRESS_APPS,
                                   'Checking for required stress applications',
                                   ', install the missing tool or try the sample without stress'
                                   ', passing \'--no-best-effort\' argument')

    # Check if the provided network interface supports TSN timestamping
    if not supports_hw_timestamping_caps(rt_conf.interface):
        traces.panic(f'Cannot identify presence hardware timestamping feature on device '
                     f'related to network interface "{rt_conf.interface}". '
                     f'You may check device hardware timestamping settings using command '
                     f'"hwstamp_ctl -i {rt_conf.interface}"')

    # Get configuration setting from json file, these settings are already
    # preprocessed with CLI input data
    json_settings = config_base.load_json_config(rt_conf)

    config_base.apply_workarounds()

    # Run registered handlers/callbacks from helper modeuled for available
    # json nodes from config (set of json nodes may vary due to provided mode)
    config_base.run_config_handlers(rt_conf, json_settings, decorators.config_handlers,
                                    global_handlers=True)


if __name__ == '__main__':
    main()
