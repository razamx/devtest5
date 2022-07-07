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
from enum import IntEnum
import json
import os
import re
from typing import List, Any
from time import clock_gettime, CLOCK_REALTIME

import decorators
import tracer

# The following lines indirectly import the configuration file helpers
import sync_clocks_helpers      # noqa: F401
import best_effort_helpers      # noqa: F401
import eth_tsn_helpers          # noqa: F401
import run_apps_helpers         # noqa: F401
import run_remote_apps_helpers  # noqa: F401

traces = tracer.traces()


# Indexes of possible profiles
class profile_ids(IntEnum):
    profile_undefined = -1
    basic_a_noopt = 0
    basic_b_noopt = 1
    siso_a_noopt = 2
    siso_b_noopt = 3
    basic_a_opt = 4
    basic_b_opt = 5
    siso_a_opt = 6
    siso_b_opt = 7


# Map profile name to profile idx
AVAIL_PROFILES = {
    'basic-a-noopt': profile_ids.basic_a_noopt,
    'basic-b-noopt': profile_ids.basic_b_noopt,
    'siso-single-a-noopt': profile_ids.siso_a_noopt,
    'siso-single-b-noopt': profile_ids.siso_b_noopt,
    'basic-a-opt': profile_ids.basic_a_opt,
    'basic-b-opt': profile_ids.basic_b_opt,
    'siso-single-a-opt': profile_ids.siso_a_opt,
    'siso-single-b-opt': profile_ids.siso_b_opt
}


# Map local profile ID to possible remote profile ID
REMOTE_PROFILES = {
    profile_ids.profile_undefined: profile_ids.profile_undefined,
    profile_ids.basic_a_noopt: profile_ids.basic_b_noopt,
    profile_ids.basic_b_noopt: profile_ids.profile_undefined,
    profile_ids.siso_a_noopt: profile_ids.siso_b_noopt,
    profile_ids.siso_b_noopt: profile_ids.profile_undefined,
    profile_ids.basic_a_opt: profile_ids.basic_b_opt,
    profile_ids.basic_b_opt: profile_ids.profile_undefined,
    profile_ids.siso_a_opt: profile_ids.siso_b_opt,
    profile_ids.siso_b_opt: profile_ids.profile_undefined
}


# Indexes of possible modes
class mode_ids(IntEnum):
    undefined = -1
    all = 0  # noqa: A003
    run = 1
    clock = 2
    tsn = 3
    setup = 4


# Map mode name to mode idx
AVAIL_MODES = {
    'all': mode_ids.all,
    'setup': mode_ids.setup,
    'run': mode_ids.run,
    'clock': mode_ids.clock,
    'tsn': mode_ids.tsn
}


# Available socket names
AVAIL_TARGETS = {
    'AUTO': 'auto',
    'EHL': 'Elkhart_Lake',
    'TGL-U': 'Tiger_Lake_U',
    'TGL-H': 'Tiger_Lake_H'
}


# Map profile name to log name
AVAIL_OUTPUT_FILES = {
    'basic-a-noopt': None,
    'basic-b-noopt': 'log_e2e.log',
    'siso-single-a-noopt': 'log-roundtrip.log',
    'siso-single-b-noopt': 'log-compute.log',
    'basic-a-opt': None,
    'basic-b-opt': 'log_e2e.log',
    'siso-single-a-opt': 'log-roundtrip.log',
    'siso-single-b-opt': 'log-compute.log'
}


# Map profile idx to config path+name
AVAIL_CONFIG_FILES = {
    profile_ids.basic_a_noopt: 'basic/{target}/talker-noopt.json',
    profile_ids.basic_b_noopt: 'basic/{target}/listener-noopt.json',
    profile_ids.siso_a_noopt: 'siso-single/{target}/monitor-noopt.json',
    profile_ids.siso_b_noopt: 'siso-single/{target}/compute-noopt.json',
    profile_ids.basic_a_opt: 'basic/{target}/talker-opt.json',
    profile_ids.basic_b_opt: 'basic/{target}/listener-opt.json',
    profile_ids.siso_a_opt: 'siso-single/{target}/monitor-opt.json',
    profile_ids.siso_b_opt: 'siso-single/{target}/compute-opt.json'
}


# Map mode idx to possible json config nodes (actions in other words)
AVAIL_ACTIONS_BY_MODES = {
    mode_ids.all: ['eth-if', 'tsn-disc', 'sync-clocks', 'run-apps', 'finish-apps', 'run-remote-apps', 'finish-remote-apps'],
    mode_ids.run: ['run-apps', 'finish-apps', 'run-remote-apps', 'finish-remote-apps'],
    mode_ids.clock: ['sync-clocks', 'run-remote-apps', 'finish-remote-apps'],
    mode_ids.tsn: ['tsn-disc', 'run-remote-apps', 'finish-remote-apps'],
    mode_ids.setup: ['eth-if', 'tsn-disc', 'sync-clocks', 'run-remote-apps', 'finish-remote-apps']
}


preprocess_list: List[Any] = list()


# Main class describes runtime configuration provided by user via CLI and
# detected other components during startup
@decorators.singleton_class
class runtime_config():
    def __init__(self, profile_name='', interface_name='', mode_name='',
                 use_stresses=False, config_path='', output_file=None, tcc_latency=-1,
                 socket='', target='', remote_interface='', remote_config_path='',
                 remote_exec_path='', remote_address=None, remote_profile_name=''):
        self.profile = AVAIL_PROFILES[profile_name] \
            if profile_name in AVAIL_PROFILES else profile_ids.profile_undefined
        self.interface = interface_name
        self.mode = AVAIL_MODES[mode_name] if mode_name in AVAIL_MODES \
            else mode_ids.undefined
        self.use_stresses = use_stresses
        self.config_path = config_path
        self.output_file = output_file
        self.output_path = 'tcc_rt_communication_demo_output'
        self.tcc_latency = tcc_latency
        self.socket = socket
        self.target = target
        self.verbosity = False
        self.base_time = 0
        self.remote_interface = remote_interface
        self.remote_config_path = remote_config_path
        self.remote_exec_path = remote_exec_path
        self.remote_address = remote_address
        self.remote_profile = AVAIL_PROFILES[remote_profile_name] \
            if remote_profile_name in AVAIL_PROFILES else profile_ids.profile_undefined
        self._update_base_time()

    # Set profile idx by profile name
    def set_profile(self, profile_name=''):
        if profile_name not in AVAIL_PROFILES:
            traces.panic('bad profile name was provided, check CLI handlers implementation')
        self.profile = AVAIL_PROFILES[profile_name]
        # Update the corresponding remote profile
        self.remote_profile = REMOTE_PROFILES[self.profile]

    # Set mode idx by mode name
    def set_mode(self, mode_name=''):
        if mode_name not in AVAIL_MODES:
            traces.panic('bad mode name was provided, check CLI handlers implementation')
        self.mode = AVAIL_MODES[mode_name]
        self._update_base_time()

    # Set output_file name
    def set_output_file(self, profile_name: str, output_file=''):
        if profile_name not in AVAIL_OUTPUT_FILES:
            traces.panic('bad output filename was provided, check CLI handlers implementation')
        if output_file != '':
            if AVAIL_OUTPUT_FILES[profile_name] is None:
                traces.warning(f'profile {profile_name} does not support creation of output files')
            else:
                # Check for existance of path to file should be already done
                # by CLI checker callback
                self.output_file = output_file
                self.output_path = os.path.dirname(self.output_file)
        else:
            output_file = AVAIL_OUTPUT_FILES.get(profile_name, None)
            if output_file is not None:
                self.output_file = os.path.join(self.output_path, output_file)
        if not os.path.exists(self.output_path):
            os.makedirs(self.output_path)
        register_preprocess_pair('output_path', self.output_path)

    # Set the base time
    def set_base_time(self, base_time):
        if base_time != 0:
            self.base_time = base_time
            register_preprocess_pair('base_time', self.base_time)
        else:
            self._update_base_time()

    # Method for data validation before processing of json configuration
    def check_config_data(self):
        if self.profile == profile_ids.profile_undefined:
            traces.panic('profile was not set')
        if self.interface == '':
            traces.panic('interface was not set')
        if self.mode == mode_ids.undefined:
            traces.panic('mode was not set')
        if self.config_path == '':
            traces.panic('config path was not set')
        config_file = os.path.join(
            self.config_path,
            AVAIL_CONFIG_FILES[self.profile].format(target=AVAIL_TARGETS[self.target]))
        if not os.path.exists(config_file):
            traces.panic(f'The config file {config_file} does not exist')
        if self.remote_address is not None:
            # The sample will launch the remote application automatically,
            # additional checks are required here
            if self.remote_profile == profile_ids.profile_undefined:
                traces.panic('remote profile was not set')
            if self.remote_interface == '':
                traces.panic('remote interface was not set')
            if self.remote_exec_path == '':
                traces.panic('remote exec path was not set')
            traces.debug2(f'check of runtime config data was passed, values are: '
                          f'profile:{self.profile} interface:{self.interface} mode:{self.mode} '
                          f'use_stresses:{self.use_stresses} config_file:{config_file} '
                          f'output_file:{self.output_file} remote_address: {self.remote_address} '
                          f'remote_profile: {self.remote_profile} remote_interface:{self.remote_interface}'
                          f'remote_config_path: {self.remote_config_path} '
                          f'remote_exec_path: {self.remote_exec_path}')
        else:
            traces.debug2(f'check of runtime config data was passed, values are: '
                          f'profile:{self.profile} interface:{self.interface} mode:{self.mode} '
                          f'use_stresses:{self.use_stresses} config_file:{config_file} '
                          f'output_file:{self.output_file}')

    # Update the base time
    def _update_base_time(self):
        base_time_delta = 10 if (mode_ids.run == self.mode and not xdp_bug_in_kernel()) else 75
        time_now = int(clock_gettime(CLOCK_REALTIME))
        self.base_time = ((time_now + 4) // 5) * 5 + base_time_delta
        register_preprocess_pair('base_time', self.base_time)
        traces.debug2(f"Updated the base time: {self.base_time}, time now: {time_now}")


# Get configuration name based on profile idx
def get_config_name(rt_conf):
    if rt_conf.profile not in AVAIL_CONFIG_FILES:
        traces.error('bad profile index was provided for config name searching')
        return None
    return os.path.join(
        rt_conf.config_path,
        AVAIL_CONFIG_FILES[rt_conf.profile].format(target=AVAIL_TARGETS[rt_conf.target]))


# Add pair template:replacer for json configuration preprocessing
def register_preprocess_pair(pattern, replacer):
    class preprocess_data():  # noqa: SIM119  # Using dataclass is not possible here
        def __init__(self, pattern, replacer):
            self.pattern = f'\\$\\{{{pattern}\\}}'
            self.replacer = str(replacer)
    pair = preprocess_data(pattern, replacer)
    preprocess_list.insert(0, pair)


# Preprocess json data using provided pair registered early
def preprocess_json_data(json_data, pair):
    debug_pattern = pair.pattern
    debug_pattern = re.sub('\\\\', '', debug_pattern)
    traces.debug2(f'preprocessing json config using variable {debug_pattern}={pair.replacer}')
    return re.sub(pair.pattern, pair.replacer, json_data)


# Load json config, preprocess data based on registered pairs, save preprocessed json
# for further usage by opcua_server
def load_json_config(rt_conf):
    json_file = get_config_name(rt_conf)
    if json_file is None:
        traces.panic('cannot get json file name by profile index')
    try:
        with open(json_file, 'r') as fd:
            json_data = fd.read()
    except Exception as e:
        traces.panic(f'cannot load data from json config ({e})')

    for pair in preprocess_list:
        json_data = preprocess_json_data(json_data, pair)

    json_data = json.loads(json_data)
    traces.debug2(f'json config data:\n{json.dumps(json_data, indent=4)}')

    saved_json = os.path.join(rt_conf.output_path, os.path.basename(json_file))
    traces.debug2(f'saving preprocessed json to file {saved_json}')
    with open(saved_json, 'w') as jf:
        json.dump(json_data, jf, indent=4)

    return json_data


# Test if required keys present in provided dict
def ensure_keys_exist(name, obj, *keys):
    for key in keys:
        if key not in obj:
            traces.panic(f'{key} not found in {name}')


# Core function for handling preprocessed json data, it aims to the following
# purposes:
# 1) select necessary actions for each mode
# 2) normalize set of handlers by order, on each level of sub-tree, handlers
# are organized as tree (see below)
# 3) Walk thru the the tree with handlers, run registered handler for each known
# json node
# As json structure provides configuration in tree representation it would be
# much easier to store handlers in tree as well. For example handler registered
# eth-if.ethtool will be called to {'eth-if': 'ethtool': {}} sub-tree data.
# Of course, we need to maintain mode option which can exclude set of actions/
# json nodes from processing, from other side we need to maintain order of
# execution. See decorators module for callback registering code.
def run_config_handlers(rt_conf, json_data, handlers_dict, global_handlers=False):  # noqa: CCR001
    def get_handler_order(sorting_object):
        # Packed key:value into tuple
        handler = sorting_object[1]
        if handler.order is None:
            # Let it be big enough
            handler.order = 1024 * 1024
        return handler.order

    # Pick set of handlers based on decision from mode
    planning_handlers = dict()
    if global_handlers:
        if rt_conf.use_stresses:
            if rt_conf.mode in (mode_ids.all, mode_ids.run):
                AVAIL_ACTIONS_BY_MODES[rt_conf.mode].append('stress-apps')
        # Append only allowed actions in case of global handlers
        for action in AVAIL_ACTIONS_BY_MODES[rt_conf.mode]:
            if action in handlers_dict:
                planning_handlers[action] = handlers_dict[action]
                continue
    else:
        planning_handlers = handlers_dict

    # Normalize handlers dict by order
    normalized_handlers = {k: v for k, v in sorted(planning_handlers.items(), key=get_handler_order)}
    traces.debug2('handlers execution order:')
    # Run handlers
    for handler in normalized_handlers.items():
        key = handler[0]
        value = handler[1]
        if key in json_data:
            traces.debug2(f'action:{key}, order:{value.order}, func:{value.func.__name__}, json:\n'
                          f'{json.dumps(json_data[key], indent=4)}')
            if value.func:
                value.func(json_data[key])
            if len(value.children) > 0:
                run_config_handlers(rt_conf, json_data[key], value.children)
    # Additional checks for json nodes with missing handlers, trigger warnings
    # for these nodes
    for item in json_data.items():
        json_action = item[0]
        if json_action not in handlers_dict:
            if json_action not in ('Note',):
                traces.warning(f'missing handler for json config node "{json_action}"')


# Check, if the kernel version matches the one that exposes an XDP communication bug
def xdp_bug_in_kernel() -> bool:
    profiles_with_bug = {
        profile_ids.basic_a_opt,
        profile_ids.basic_b_opt,
        profile_ids.siso_a_opt,
        profile_ids.siso_b_opt
    }

    # The XDP communication bug exists only for Optimized profiles
    if runtime_config().profile not in profiles_with_bug:
        return False

    # Get the Kernel version
    kernel_version = '.'.join(os.uname()[2].split('.')[:2])
    # Kernel version '5.10' has the XDP communication bug, report accordingly
    return kernel_version == '5.10'


# Apply workarounds for known bugs
def apply_workarounds() -> None:
    if not xdp_bug_in_kernel():
        # The workaround does not apply for this case
        return

    # For kernel 5.10 the NIC goes to reset after XDP handler is installed.
    # So, we need to restore the Network settings and restart the Time Syncronization
    # after the application is launched, moving the order after application run.
    traces.notice('Applying the workaround for XDP communication in Kernel 5.10')
    decorators.config_handlers['tsn-disc'].order += 1000
    decorators.config_handlers['sync-clocks'].order += 1000

    # For 'run' mode, we need to enable 'tsn-disc' and 'sync-clocks' handlers,
    # as they are required to run on Systems running Kernel 5.10
    AVAIL_ACTIONS_BY_MODES[mode_ids.run].append('tsn-disc')
    AVAIL_ACTIONS_BY_MODES[mode_ids.run].append('sync-clocks')


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
