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
import time
from os.path import join

import tracer
import decorators
import config_base
from shell_run_utils import run_cmd


ssh_process = None
traces = tracer.traces()


# Register handler for run-remote-apps branch from json in global set of handlers.
# We should run this handler first, so, we have a chance to run the remote part,
# before moving on to other handlers for local processing.
@decorators.config_handler(decorators.config_handlers, 'run-remote-apps', 100)
def process_start_remote_app_data(data):
    global ssh_process

    # Get reference to runtime config
    rt_conf = config_base.runtime_config()

    # Check if launcing the remote application is enabled
    if rt_conf.remote_address is None:
        traces.debug('Remote launch is not enabled, skipping')
        return

    # Check the configuration data
    if 'app' not in data:
        traces.panic('app is missing')
    remote_application = data.get('app')
    remote_application_arguments = ''
    if 'app-args' in data:
        remote_application_arguments += data.get('app-args')

    # Find the remote profile name
    remote_profile_name = None
    for profile_name, profile_id in config_base.AVAIL_PROFILES.items():
        if profile_id == rt_conf.remote_profile:
            remote_profile_name = profile_name
    if not remote_profile_name:
        traces.panic("Failed to find the remote profile name")
    # Find the current mode name
    remote_mode_name = None
    for mode_name, mode_id in config_base.AVAIL_MODES.items():
        if mode_id == rt_conf.mode:
            remote_mode_name = mode_name
    if remote_mode_name is None:
        traces.panic("Failed to find the remote mode name")

    # Check the configuration data
    if 'log-file' not in data:
        traces.panic('log-file is missing')
    remote_log_filename = data.get('log-file')

    # Get the remote network interface name,
    # if the remote interface if not specified, use the local interface name.
    remote_interface = rt_conf.remote_interface or rt_conf.interface

    # If the remote path is defined, prepend the path to the sample application
    if rt_conf.remote_exec_path is not None:
        remote_application = join(rt_conf.remote_exec_path, remote_application)

    # Print debug information
    traces.debug2(f'Running remote application {remote_application}, '
                  f'with initial arguments: {remote_application_arguments}, '
                  f'at host "{rt_conf.remote_address}", mode: {remote_mode_name}, '
                  f'interface: "{remote_interface}", '
                  f'profile: "{remote_profile_name}", '
                  f'exec path: "{rt_conf.remote_exec_path}", '
                  f'config path: "{rt_conf.remote_config_path}", '
                  f'verbosity: {rt_conf.verbosity}, '
                  f'best_effort: {rt_conf.use_stresses}, '
                  f'save logs to file: "{remote_log_filename}"')

    remote_working_directory = '/tmp'
    remote_cmd = f'cd {remote_working_directory} && ' \
                 f'{remote_application} {remote_application_arguments} --profile {remote_profile_name} ' \
                 f'--interface {remote_interface} ' \
                 f'--config-path {rt_conf.remote_config_path} --mode {remote_mode_name}'
    if rt_conf.remote_exec_path is not None:
        remote_cmd += f' --exec-path {rt_conf.remote_exec_path}'

    # Check the verbosity setting
    if rt_conf.verbosity:
        remote_cmd += ' --verbose'
    # Check the stress setting
    if not rt_conf.use_stresses:
        remote_cmd += ' --no-best-effort'

    cmd_ssh_wrapped = f'ssh {rt_conf.remote_address} \'{remote_cmd}\''

    # Start executing the remote command
    traces.debug2(f'Starting command on remote target: "{cmd_ssh_wrapped}"')
    ssh_process = run_cmd(cmd_ssh_wrapped, True, remote_log_filename, False)

    time.sleep(1.0)


# Register handler for 'finish-remote-apps' node, it should run last, after
# other local handlers are finished.
@decorators.config_handler(decorators.config_handlers, 'finish-remote-apps', 9000)
def process_finish_remote_app_data(data):
    global ssh_process

    if ssh_process is None:
        traces.debug2("No active remote session")
        return

    # Get reference to runtime config
    rt_conf = config_base.runtime_config()

    # Wait for the remote application to finish
    try:
        traces.debug("Waiting for the remote application to finish...")
        ssh_process.wait()
        traces.debug("The remote application has finished.")
    except KeyboardInterrupt:
        traces.notice('Teminated by: KeyboardInterrupt')
        exit(1)

    if 'copy-results' in data:
        copy_items = data.get('copy-results')
        for copy_item in copy_items:
            copy_from = copy_item.get('from')
            copy_to = copy_item.get('to')
            copy_cmd = f'scp {rt_conf.remote_address}:{copy_from} {copy_to}'
            traces.debug2(f'Copy remote file from: "{copy_from}", to local FS: "{copy_to}", command: {copy_cmd}')
            run_cmd(copy_cmd, disable_output=True)


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
