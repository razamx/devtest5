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

import tracer
import decorators
from shell_run_utils import run_cmd

traces = tracer.traces()
IPERF_SERVER_PID_FILENAME = '/var/run/iperf3.pid'
IPERF_CLIENT_LOG = '/var/log/iperf3-client.log'
IPERF_SERVER_LOG = '/var/log/iperf3-server.log'


def is_iperf3_server_running():
    # Check if the PID file exists, the server is not running, if it does not exist
    if not os.access(IPERF_SERVER_PID_FILENAME, os.F_OK):
        return False
    pidfile = open(IPERF_SERVER_PID_FILENAME, 'r')
    pidfile.seek(0)
    server_pid = int(pidfile.readline())
    # Check if the process with the stored PID is running
    try:
        os.kill(server_pid, 0)
    except OSError:
        return False
    else:
        return True


def start_client(config):
    ip = config.get('ip', '169.254.0.2')
    bandwidth = config.get('bandwidth', '150M')
    packet_length = config.get('packet-length', 1448)
    duration = config.get('duration', 30)
    args = f'iperf3 --client {ip} --udp --bandwidth {bandwidth} --len {packet_length} --time {duration}'
    traces.notice('Starting iperf3 in client mode...')
    run_cmd(args, async_run=True, redirect_to=IPERF_CLIENT_LOG)


def start_server(config):
    traces.notice('Starting iperf3 in server mode...')
    if is_iperf3_server_running():
        traces.debug('Server iperf3 is already running')
        traces.notice('Server iperf3 listening on 5201')
        return
    args = 'iperf3 --server'
    process = run_cmd(args, async_run=True, redirect_to=IPERF_SERVER_LOG)
    with open(IPERF_SERVER_PID_FILENAME, 'w') as pidfile:
        pidfile.write(f'{process.pid}\n')
        pidfile.flush()
    traces.notice('Server iperf3 listening on 5201')


# Register handler for stress-apps.best-effort branch from json
@decorators.config_handler(decorators.config_handlers, 'stress-apps.best-effort', order=10)
def process_best_effort_node(config):
    role = config.get('role', 'server')
    if role == 'server':
        start_server(config)
    else:
        start_client(config)


# Register handler for stress-apps branch from json in global set of handlers,
# order=3, runs after eth-if, tsn-disc, sync-clocks handlers
@decorators.config_handler(decorators.config_handlers, 'stress-apps', order=300)
def process_stress_apps(config):
    traces.debug2('processing stress applications section')


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
