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
from contextlib import suppress

import config_base
import decorators
import tracer
from shell_run_utils import run_cmd, is_running

traces = tracer.traces()

PTP4L_LOG_FILENAME = '/var/log/ptp4l.log'
PHC2SYS_LOG_FILENAME = '/var/log/phc2sys.log'
PMC_LOG_FILENAME = '/var/log/pmc.log'


# Register handler for sync-clocks.ptp4l  branch from json, set order=75 because
# this handler should be executed after general network interface setup and before phc2sys
@decorators.config_handler(decorators.config_handlers, 'sync-clocks.ptp4l', order=10)
def process_ptp(obj):
    config_base.ensure_keys_exist('ptp4l', obj, 'interface')
    config_path = config_base.runtime_config().config_path
    iface = obj.get('interface')
    ignore_existing = obj.get('ignore_existing', False)
    socket_prio = str(obj.get('socket_prio', 1))
    gPTP_file = str(obj.get('gPTP_file', f'{config_path}/ptp/gPTP.cfg'))  # noqa: N806

    if ignore_existing and is_running('ptp4l'):
        return

    # Remove old log file and terminate all old processes
    if os.path.exists(PTP4L_LOG_FILENAME) and os.path.isfile(PTP4L_LOG_FILENAME):
        os.remove(PTP4L_LOG_FILENAME)
    run_cmd('killall -TERM ptp4l', disable_output=True)

    arglist = f'taskset -c 1 ptp4l -mP2Hi {iface} --step_threshold=2 ' \
              f'-f {gPTP_file} --socket_priority {socket_prio}'
    traces.notice('Starting service for PTP hardware clock time synchronization over network...')
    traces.notice(f'You may use command "tail -f {PTP4L_LOG_FILENAME}" in order to check '
                  f'quality of synchronization process')

    run_cmd(arglist, async_run=True, redirect_to=PTP4L_LOG_FILENAME)

    traces.notice('Waiting 30sec for PTP hardware clock time synchronization...')
    time.sleep(30)
    traces.notice('Finished waiting for PTP hardware clock time synchronization over network, '
                  'going to next step...')


# Register handler for sync-clocks.phc2sys branch from json, set order=1, i.e.
# after ptp4l
@decorators.config_handler(decorators.config_handlers, 'sync-clocks.phc2sys', order=20)
def process_phc2sys(obj):
    config_base.ensure_keys_exist('phc2sys', obj, 'clock', 'interface')
    clock = obj.get('clock')
    iface = obj.get('interface')
    ignore_existing = obj.get('ignore_existing', False)

    if ignore_existing and is_running('phc2sys'):
        return

    # Remove old log file and terminate all old processes
    if os.path.exists(PHC2SYS_LOG_FILENAME) and os.path.isfile(PHC2SYS_LOG_FILENAME):
        os.remove(PHC2SYS_LOG_FILENAME)
    run_cmd('killall -TERM phc2sys', disable_output=True)

    argl = 'pmc -u -b 0 -t 1 "SET GRANDMASTER_SETTINGS_NP clockClass 248 clockAccuracy 0xfe ' \
           'offsetScaledLogVariance 0xffff currentUtcOffset 37 leap61 0 leap59 0 ' \
           'currentUtcOffsetValid 1 ptpTimescale 1 timeTraceable 1 ' \
           'frequencyTraceable 0 timeSource 0xa0"'
    run_cmd(argl, async_run=True, redirect_to=PMC_LOG_FILENAME)

    argl = f'taskset -c 1 phc2sys -s {iface} -c {clock} -w -ml 7 -O 0 ' \
           f'--transportSpecific=1 --step_threshold=1'

    traces.notice(f'Starting service for time synchronization between '
                  f'PTP hardware clock and {clock} system clock...')
    traces.notice(f'You may use command "tail -f {PHC2SYS_LOG_FILENAME}" in order '
                  f'to check quality of synchronization process')
    run_cmd(argl, async_run=True, redirect_to=PHC2SYS_LOG_FILENAME)
    traces.notice(f'Waiting 30sec for time synchronization between PTP '
                  f'hardware clock and {clock} system clock...')
    time.sleep(30)
    traces.notice(f'Finished waiting for time synchronization between PTP hardware clock '
                  f'and {clock} system clock, going to next step...')


# Register handler for sync-clocks branch from json global handlers,
# set order=2, i.e. after eth-if and tsn-disc
@decorators.config_handler(decorators.config_handlers, 'sync-clocks', order=400)
def sync_clocks_main(json_node):
    traces.debug('start processing sync-clocks section')
    # Get reference to runtime config
    rt_conf = config_base.runtime_config()
    run_obj = None
    with suppress(Exception):
        run_obj = run_cmd(f'hwstamp_ctl -i {rt_conf.interface}', disable_output=True)
    if (run_obj is None) or (run_obj.returncode != 0):
        traces.panic(f'Cannot identify presence hardware timestamping feature '
                     f'on device related to network interface "{rt_conf.interface}". '
                     f'You may check device hardware timestaming settings using command '
                     f'"hwstamp_ctl -i {rt_conf.interface}".')


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
