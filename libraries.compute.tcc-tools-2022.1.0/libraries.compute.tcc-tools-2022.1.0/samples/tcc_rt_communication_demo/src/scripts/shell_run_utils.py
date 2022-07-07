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
import shlex
import subprocess as sp
import tracer

traces = tracer.traces()
DRY_RUN = False


# Run command according to requested modes (backgrounded, redirected, silent)
def run_cmd(cmd, async_run=False, redirect_to=None, disable_output=False):
    traces.debug(f'Running command (async={async_run}, redirect={redirect_to}, '
                 f'silent={disable_output}): {cmd}')
    if redirect_to is not None and disable_output is True:
        traces.error(f'mutually exclusive argument used for running "{cmd}"')
    if DRY_RUN:
        return None
    popen_obj = None
    if disable_output:
        popen_obj = sp.Popen(shlex.split(cmd), stdin=sp.DEVNULL, stdout=sp.DEVNULL, stderr=sp.DEVNULL)
    else:
        if redirect_to is not None:
            fd = open(redirect_to, 'w')
            popen_obj = sp.Popen(shlex.split(cmd), stdin=sp.DEVNULL, stdout=fd, stderr=fd)
        else:
            popen_obj = sp.Popen(shlex.split(cmd), stdin=sp.DEVNULL)

    if async_run:
        return popen_obj
    else:
        popen_obj.wait()
        return popen_obj


# Run command, collect output and return it to caller
def run_and_get_output(cmd):
    traces.debug(f'Running: {cmd}')
    if DRY_RUN:
        return None
    popen_obj = sp.Popen(shlex.split(cmd), stdin=sp.DEVNULL, stdout=sp.PIPE, stderr=sp.PIPE)
    popen_obj.wait()
    output, _ = popen_obj.communicate()
    return output


# Check if process running by name of the process
def is_running(pname):
    proc = sp.run(['pgrep', pname], stdout=sp.PIPE)
    return len(proc.stdout.splitlines()) > 0


# Check if the application/tool/execuable available in the system
def is_application_available(name: str) -> bool:
    return sp.run(['which', name], stdout=sp.DEVNULL, stderr=sp.DEVNULL).returncode == 0


# Check the HW timestamping capabilities for the specified network interface
def supports_hw_timestamping_caps(interface: str) -> bool:
    cap = run_and_get_output(f'ethtool --show-time-stamping {interface}')
    if not cap:
        return False
    caps_lines = cap.decode('utf-8').splitlines()
    for line in caps_lines:
        if line.startswith('PTP Hardware Clock: '):
            # Extract the PTP Hardware Clock setting
            spec = line.split(': ')[1]
            return spec and spec != 'none'
    return False


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
