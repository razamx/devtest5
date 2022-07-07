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
import time

import config_base
import decorators
import tracer
from shell_run_utils import run_cmd

tcp_proc = None
server_proc = None
traces = tracer.traces()


# Register handler for run-apps branch from json in global set of handlers,
# omitting order, these tasks should be executed only when other tasks
# will be finished, i.e. system is configured and ready for sample's
# payload processing
@decorators.config_handler(decorators.config_handlers, 'run-apps', order=1000)
def process_rt_data(data):  # noqa: CCR001  # High complexity
    global tcp_proc
    global server_proc

    # Get reference to runtime config
    rt_conf = config_base.runtime_config()

    enable_tcpdump = 'tcpdump' in data
    tcpdump_iface = data.get('tcpdump', 'eth0')

    if 'carrier' not in data:
        traces.panic('carrier is missing')
    if 'carrier-args' not in data:
        traces.panic('carrier arguments is missing')

    # Handling 'carrier' as main application
    carrier = data.get('carrier')
    carrier_args = data.get('carrier-args')
    if rt_conf.verbosity:
        carrier_args += ' --log-level=debug'
    server_cmd = carrier + ' ' + carrier_args
    # Check for existance of carrier in $PATH
    bin_paths = os.environ['PATH'].split(':')
    carrier_exists = False
    for bin_path in bin_paths:
        if os.path.exists(os.path.join(bin_path, carrier)):
            carrier_exists = True
            break

    if not carrier_exists:
        traces.panic(f'Binary "{carrier}" was not found in '
                     f'directories provided by PATH environment variable. '
                     f'Try to launch script with [-e] option and provide path '
                     f'to binary executables manually.')

    tcp_proc = None
    # This is here as a workaround because NIC have to be moved to
    # promisc mode in case if MAC addresses were not changed in json
    # file pointed by 'path'
    if enable_tcpdump:
        cmd = f'tcpdump -i {tcpdump_iface} -q ether proto 0xb62c ' \
              f'-Q in --time-stamp-precision=nano -j adapter_unsynced'
        pcap_file = os.path.join(rt_conf.output_path, "tcc_rt_communication_demo_incoming.pcap")
        cmd += ' -w ' + pcap_file
        if rt_conf.verbosity:
            tcp_proc = run_cmd(cmd, async_run=True)
        else:
            tcp_proc = run_cmd(cmd, async_run=True, disable_output=True)
        time.sleep(2)  # tcpdump takes some time to initialise itself

    # Run *-app here; it will launch the opcua server
    server_proc = run_cmd(server_cmd, async_run=True)


@decorators.config_handler(decorators.config_handlers, 'finish-apps', order=2000)
def finish_processing_rt_data(data):
    if (server_proc):
        try:
            traces.debug("Waiting for the server...")
            server_proc.wait()
            traces.debug("The server has finished.")
        except KeyboardInterrupt:
            traces.notice('Teminated by: KeyboardInterrupt')
            if tcp_proc:
                tcp_proc.terminate()
            exit(1)
    if tcp_proc:
        traces.debug("Terminating tcpdump process")
        tcp_proc.terminate()


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
