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
import math
from os.path import isfile

import config_base
import decorators
import tracer
from contextlib import suppress
from shell_run_utils import run_cmd, run_and_get_output

traces = tracer.traces()


def get_show_qdisc_cmd(interface):
    return f'tc qdisc show dev {interface}'


def init_ingress(iface):
    clear_rx_cmd = "tc qdisc del dev {} parent ffff:".format(iface)
    add_ingress_cmd = "tc qdisc add dev {} ingress".format(iface)

    run_cmd(clear_rx_cmd, disable_output=True)
    run_cmd(add_ingress_cmd, disable_output=True)


def set_vlanrx(iface, config):
    config_base.ensure_keys_exist(
        'vlanrx',
        config,
        'vlan_priority',
        'rx_hw_q'
    )
    vlan_priority = str(config['vlan_priority'])
    rx_hw_q = str(config['rx_hw_q'])
    set_vlanrx_cmd = f'tc filter add dev {iface} '
    set_vlanrx_cmd += f'parent ffff: protocol 802.1Q flower vlan_prio {vlan_priority} '
    set_vlanrx_cmd += f'hw_tc {rx_hw_q}'

    run_cmd(set_vlanrx_cmd, disable_output=True)


# Register handler for eth-if.ip branch from json
@decorators.config_handler(decorators.config_handlers, 'eth-if.ip', order=10)
def process_ip_data(config):
    traces.notice("Configuring network interface(s)...")

    has_vlan_config = False
    config_base.ensure_keys_exist(
        'eth-if.ip',
        config,
        'interface',
        'if-address',
        'if-net-bits-count',
        'if-broadcast-address',
        'if-mac-address'
    )

    if 'vlan-id' in config:
        has_vlan_config = True
        config_base.ensure_keys_exist(
            'eth-if.ip',
            config,
            'vlan-address',
            'vlan-net-bits-count',
            'vlan-broadcast-address'
        )

    interface = config.get('interface')
    if_address = config.get('if-address')
    if_net_bits_count = config.get('if-net-bits-count')
    if_broadcast_address = config.get('if-broadcast-address')
    if_mac_address = config.get('if-mac-address')

    traces.debug(f'setting MAC address for interface "{interface}"')
    run_cmd(f'ip link set {interface} down', disable_output=True)
    run_cmd(f'ip link set dev {interface} address {if_mac_address}', disable_output=True)
    run_cmd(f'ip link set {interface} up', disable_output=True)

    traces.debug(f'flushing network interface "{interface}"')
    run_cmd(f'ip addr flush dev {interface}', disable_output=True)

    traces.debug(f'setting up adresses for interface "{interface}"')
    cmd = f'ip addr add {if_address}/{if_net_bits_count}'
    cmd += f' brd {if_broadcast_address}'
    cmd += f' dev {interface}'
    run_cmd(cmd, disable_output=True)

    traces.debug(f'flush neighbours on interface "{interface}"')
    run_cmd(f'ip neigh flush all dev {interface}', disable_output=True)

    if has_vlan_config:
        vlan_id = config.get('vlan-id')
        vlan_address = config.get('vlan-address')
        vlan_net_bits_count = config.get('vlan-net-bits-count')
        vlan_broadcast_address = config.get('vlan-broadcast-address')

        traces.debug(f'remove previously created vlan on "{interface}"')
        run_cmd(f'ip link delete dev {interface}.vlan', disable_output=True)

        traces.debug(f'adding vlan with id {vlan_id} on interface "{interface}"')
        run_cmd(f'ip link add link {interface} name {interface}.vlan type vlan id {vlan_id}',
                disable_output=True)

        traces.debug(f'flushing network interface "{interface}.vlan"')
        run_cmd(f'ip addr flush dev {interface}.vlan', disable_output=True)

        traces.debug(f'setting up adresses for interface "{interface}.vlan"')
        cmd = f'ip addr add {vlan_address}/{vlan_net_bits_count}'
        cmd += f' brd {vlan_broadcast_address}'
        cmd += f' dev {interface}.vlan'
        run_cmd(cmd, disable_output=True)

        traces.debug('map socket priority to vlan priority')
        for _it in range(1, 6):
            run_cmd(f'ip link set {interface}.vlan type vlan egress-qos-map {_it}:{_it}', disable_output=True)

        traces.debug(f'flush neighbours on interface "{interface}.vlan"')
        run_cmd(f'ip neigh flush all dev {interface}.vlan', disable_output=True)


def get_qdisc_parent(cmd, token):
    output = run_and_get_output(cmd).decode('utf8')
    for item in output.split('\n'):
        if token in item:
            res = item.strip().split()
            return res[res.index(token) + 1].replace(":", "")
    return None


def set_taprio(iface, maps, config, basetime, clkid):
    schedules = ""
    for each in config["schedule"]:
        schedules += "sched-entry S {} {} ".format(each['gate_mask'], str(each['duration']))

    num_tc = 0
    queues = 0
    handle = 0
    for each in config.keys():
        if each == "num_tc":
            num_tc = config["num_tc"]
        if each == "queues":
            queues = config["queues"]
        if each == "handle":
            handle = config["handle"]

    if not num_tc:
        traces.panic("num_tc not defined")
    if not queues:
        traces.panic("queues not defined")
    if not handle:
        traces.panic("handle not defined")

    # Final taprio command to run
    set_taprio_cmd = f"tc -d qdisc replace dev {iface} "
    set_taprio_cmd += f"parent root handle {handle} "
    set_taprio_cmd += f"taprio num_tc {num_tc} map {maps} "
    set_taprio_cmd += f"queues {queues} base-time {basetime} "
    set_taprio_cmd += f"{schedules} flags 0x2"

    # Parse hardware offload option
    # It's specified in json as "offload": true|false
    if 'offload' in config and config.get('offload'):
        set_taprio_cmd += ' offload 1'

    run_obj = None
    with suppress(Exception):
        run_obj = run_cmd(set_taprio_cmd, disable_output=True)
    if (run_obj is None) or (run_obj.returncode != 0):
        traces.error(f'Cannot configure TAPRIO TSN qdisc, jitter of transmission '
                     f'latency can be unpredictable. Run command "{set_taprio_cmd}" to retry '
                     f'attempt of configuration manually and check output.')


def set_mqprio(iface, maps, config):
    num_tc = 0
    queues = 0
    handle = 0
    for each in config.keys():
        if each == "num_tc":
            num_tc = config["num_tc"]
        if each == "queues":
            queues = config["queues"]
        if each == "handle":
            handle = config["handle"]

    if not num_tc:
        traces.panic("num_tc not defined")
    if not queues:
        traces.panic("queues not defined")
    if not handle:
        traces.panic("handle not defined")

    # Final mqprio command to run
    set_mqprio_cmd = f"tc qdisc add dev {iface} "
    set_mqprio_cmd += f"parent root handle {handle} "
    set_mqprio_cmd += f"mqprio num_tc {num_tc} "
    set_mqprio_cmd += f"map {maps} queues {queues} hw 0"

    run_obj = None
    with suppress(Exception):
        run_obj = run_cmd(set_mqprio_cmd, disable_output=True)

    if (run_obj is None) or (run_obj.returncode != 0):
        traces.error(f'Cannot configure MQPRIO TSN qdisc, jitter of transmission '
                     f'latency can be unpredictable. Run command "{set_mqprio_cmd}" to retry '
                     f'attempt of configuration manually and check output.')


def set_etf(iface, clkid, config, show_cmd, use_taprio):
    # unpack config variable
    delta = str(config['delta'])

    # naming convention for queue is <parent>:<queue>
    # since <queue> naming starts from 1, we add 1 to queue.
    queue = str(config['queue'] + 1)

    # deadline_mode and offload are turned off by default
    deadline_mode = False
    offload = False

    # find the parent for specific qdisc
    if use_taprio is True:
        parent = get_qdisc_parent(show_cmd, 'taprio')
    else:
        parent = get_qdisc_parent(show_cmd, 'mqprio')

    # check if config specifies deadline_mode or offload
    for each in config.keys():
        if each == 'deadline_mode':
            deadline_mode = config['deadline_mode']
        if each == 'offload':
            offload = config['offload']
        if each == 'parent':
            parent = str(config['parent'])

    if not parent:
        traces.error('Missing parent for ETF TSN qdisc, incomplete TSN configuration.')
        return

    # generate etf command
    etf_default_cmd = f'tc qdisc replace dev {iface} parent {parent}:{queue} etf '
    etf_default_cmd += f' clockid {clkid}'
    etf_default_cmd += f' delta {delta}'
    if offload:
        etf_default_cmd += ' offload'
    if deadline_mode:
        etf_default_cmd += ' deadline_mode'
    traces.debug(f'Adding etf qdisc on queue {queue}...')

    run_obj = None
    with suppress(Exception):
        run_obj = run_cmd(etf_default_cmd, disable_output=True)

    if (run_obj is None) or (run_obj.returncode != 0):
        traces.error(f'Cannot configure ETF TSN qdisc, jitter of transmission '
                     f'latency can be unpredictable. Run command "{etf_default_cmd}" to retry '
                     f'attempt of configuration manually and check output.')


def set_mapping(scenario_config, use_mapping) -> str:
    maps = [str(scenario_config[use_mapping]["mapping"]["default"])]
    maps *= 16
    # apply mapping for specific queue
    for each in scenario_config[use_mapping]["mapping"]:
        if each == 'default':
            continue
        priority = int(each[1:])
        queue = str(scenario_config[use_mapping]["mapping"][each])
        maps[priority] = queue
    return ' '.join(maps)


def set_cbs(iface, scenario_config):
    handle = scenario_config["handle"]
    parent = scenario_config["parent"]
    queue = scenario_config["queue"] + 1
    sendslope = scenario_config["sendslope"]
    idleslope = scenario_config["idleslope"]
    hicredit = scenario_config["locredit"]
    locredit = scenario_config["locredit"]
    offload = scenario_config["offload"]

    # generate cbs command
    cbs_default_cmd = f'tc qdisc replace dev {iface} '
    cbs_default_cmd += f'handle {handle} parent {parent}:{queue} '
    cbs_default_cmd += f'cbs idleslope {idleslope} sendslope {sendslope} '
    cbs_default_cmd += f'hicredit {hicredit} locredit {locredit} '
    cbs_default_cmd += f'offload {offload} '

    traces.debug(f'adding cbs qdisc on queue {queue}...')

    run_obj = None
    with suppress(Exception):
        run_obj = run_cmd(cbs_default_cmd, disable_output=True)

    if (run_obj is None) or (run_obj.returncode != 0):
        traces.error(f'Cannot configure CBS TSN qdisc, jitter of transmission '
                     f'latency can be unpredictable. Run command "{cbs_default_cmd}" to retry '
                     f'attempt of configuration manually and check output.')

# END COPIED FROM OLD SCRIPT


def ensure_keys_exist(name, mapd, *keys):
    for key in keys:
        if key not in mapd:
            traces.panic(f'key {key} does not exist in {name}')


def process_ets(iface, data):
    ensure_keys_exist('ethtool-ets', data, 'gcl_path')
    gcl_path = data.get('gcl_path')

    cycle_time = 0.0
    with open(gcl_path, 'r') as f:
        lengths = [int(r.split()[2]) for r in f.readlines()]
        cycle_time = sum(lengths) / 1000000000

    # 3 secs in the future
    base_time = math.ceil(time.time()) + 3
    traces.debug('cycle time: ', cycle_time, ' base: ', base_time)

    try:
        run_cmd(f'ethtool -K {iface} tx-est on', disable_output=True)
        # Set GCL
        run_cmd(f'ethtool --set-gcl {iface} own admin file {gcl_path}', disable_output=True)
        # Set base and cycle time
        run_cmd(f'ethtool --set-est-info {iface} own admin cycle {cycle_time} '
                f'base {base_time} ext 0.0', disable_output=True)
        # Show resulting config
        run_cmd(f'ethtool --get-est-info {iface} own admin', disable_output=True)
    except Exception:
        traces.error('Cannot configure Gate Control List, jitter of transmission '
                     'latency can be unpredictable')


def delete_qdiscs(iface):
    traces.debug('deleting existing qdiscs')
    run_cmd(f'tc qdisc del dev {iface} root', disable_output=True)
    time.sleep(5)


# Register handler for "tsn-disc" branch from json in global set of handlers
@decorators.config_handler(decorators.config_handlers, 'tsn-disc', order=300)
def process_tc_data(data):
    traces.notice('Setting up TSN queues disciplines...')
    if 'interface' not in data:
        return

    interface = data.get('interface')
    delete_qdiscs(interface)

    # Moved from existing script
    use_scheduler = ''
    is_using_taprio = False
    if 'mqprio' in data:
        use_scheduler = 'mqprio'
    if 'taprio' in data:
        is_using_taprio = True
        use_scheduler = 'taprio'

    clock_id = 'CLOCK_TAI'
    if use_scheduler:
        traces.debug(f"setting up {use_scheduler} qdisc")
        maps = set_mapping(data, use_scheduler)

        if use_scheduler == 'taprio':
            base_time = (int(time.time()) + 1) * 1000000000
            set_taprio(interface, maps, data["taprio"], base_time, clock_id)

        elif use_scheduler == 'mqprio':
            set_mqprio(interface, maps, data["mqprio"])

    show_qdisc_cmd = get_show_qdisc_cmd(interface)

    if 'ethtool-est' in data:
        traces.debug('Set up EST with ethtool')
        process_ets(interface, data.get('ethtool-est'))

    if 'cbs' in data:
        traces.debug('Set up CBS')
        set_cbs(interface, data["cbs"])

    if 'etf' in data:
        traces.debug('Set up ETF')
        for each_config in data["etf"]:
            set_etf(interface, clock_id, each_config, show_qdisc_cmd,
                    is_using_taprio)

    if 'ethtool' in data:
        ethtool_cfg = data['ethtool']
        traces.debug(f'Apply ethtool configurations: "{ethtool_cfg}"')
        _process_ethtool_config(ethtool_cfg, interface)

    if 'vlanrx' in data:
        traces.debug('Set VLAN RX steering')
        init_ingress(interface)
        for each_config in data["vlanrx"]:
            set_vlanrx(interface, each_config)

    traces.debug('TC setup complete')
    output = run_and_get_output(show_qdisc_cmd)
    traces.debug('TSN discipline settings:', output.decode('utf8'))


# Register handler for eth-if.ethtool branch from json
@decorators.config_handler(decorators.config_handlers, 'eth-if.ethtool', order=20)
def process_ethtool_data(data):
    iface = data.get('interface', None)

    _process_ethtool_config(data, iface)


# Register handler for eth-if.irq-affinity branch from json
@decorators.config_handler(decorators.config_handlers, 'eth-if.irq-affinity', order=30)
def process_irq_affinity_data(data):
    irq_file = data.get('file', None)
    iface = data.get('interface', None)
    if irq_file is None:
        traces.error(f'Missing required argument "file" in "eth-if.irq-affinity: {data}"')
        return
    if not isfile(irq_file):
        traces.error(f'The file {irq_file} provided for "eth-if.irq-affinity.file" does not exist')
        return
    if iface is None:
        traces.error('Missing required argument "interface" in "eth-if.irq-affinity"')
        return
    traces.debug("Applying affinity requirements: file: {}, iface: {}".format(irq_file, iface))
    run_cmd(f'sh apply_irq_affinities.sh {iface} {irq_file}', disable_output=False)


# Register handler for eth-if branch from json in global set of handlers,
# set order=50, which is initial stage of local configuration
@decorators.config_handler(decorators.config_handlers, 'eth-if', order=200)
def eth_if_main(data):
    traces.debug2('start processing eth-if section')


def _process_ethtool_config(ethtool_cfg, iface) -> None:
    if iface is None:
        return

    if 'tso' in ethtool_cfg:
        tso_status = ethtool_cfg.get('tso')
        if tso_status in ('on', 'off'):
            tso_cmd = f'ethtool -K {iface} tso {tso_status}'
            run_cmd(tso_cmd, disable_output=True)
        else:
            traces.error(f'Unexpected action for ethtool TSO handler: {tso_status}')

    if ('rx' in ethtool_cfg) and ('tx' in ethtool_cfg):
        rx_num = ethtool_cfg.get('rx')
        tx_num = ethtool_cfg.get('tx')
        traces.debug(f'Setting queues for interface "{iface}": rx: {rx_num}, tx: {tx_num}')
        run_cmd(f'ethtool -L {iface} rx {rx_num} tx {tx_num}', disable_output=True)

    if 'rxvlan' in ethtool_cfg:
        rxvlan_status = ethtool_cfg.get('rxvlan', None)
        if rxvlan_status in ('on', 'off'):
            rxvlan_cmd = f'ethtool -K {iface} rxvlan {rxvlan_status}'
            run_cmd(rxvlan_cmd, disable_output=True)
        else:
            traces.error(f'Unexpected action for ethtool rxvlan handler: {rxvlan_status}')


if __name__ == '__main__':
    traces.panic('this file is component of RTC sample and is not intended to be executed directly')
