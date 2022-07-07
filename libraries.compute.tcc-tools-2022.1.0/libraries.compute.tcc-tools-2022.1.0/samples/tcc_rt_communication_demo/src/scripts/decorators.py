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
from sys import exit
from os import EX_SOFTWARE
from typing import Dict, Any

config_handlers: Dict[str, Any] = {}


# Decorator for singleton class
def singleton_class(inner_class):
    classes = {}

    def get_created_classes(*args, **kwargs):
        if inner_class not in classes:
            classes[inner_class] = inner_class(*args, **kwargs)
        return classes[inner_class]
    return get_created_classes


# Decorator for registation of different callbacks in requested list
def local_handler(handlers_list):
    def register_local_handler(func):
        handlers_list.append(func)
    return register_local_handler


# Decorator for registration of configuration handler in configuration tree
# based on node_name which can be dot sepatared string with path to requested node,
# and also based on provided execution order.
# This decorator does not normalize tree by order, normalization is performing
# during execution of callbacks in lazy mode.
# Orders, currently used in the sample:
#  100) 'run-remote-apps';
#  200) 'eth-if';
#     10) 'eth-if.ip';
#     20) 'eth-if.ethtool';
#     30) 'eth-if.irq-affinity';
#  300) 'tsn-disc';
#  400) 'sync-clocks';
#     10) 'sync-clocks.ptp4l';
#     20) 'sync-clocks.phc2sys';
#  500) 'stress-apps';
#     10) 'stress-apps.best-effort';
# 1000) 'run-apps';
# 2000) 'finish-apps';
# 9000) 'finish-remote-apps';
def config_handler(handlers_dict, node_name, order=None):
    def register_config_handler(func):
        class _config_handler():  # noqa: SIM119  # Cannot use dataclass here
            def __init__(self, func, order):
                self.func = func
                self.order = order
                self.children = dict()

            def __repr__(self):
                return f'config_handler({str(self.func)}, {str(self.order)}, {str(self.children)})'

        target_node_path = node_name.split('.')
        target_dict = config_handlers
        path_length = len(target_node_path)
        for node in target_node_path:
            if node not in target_dict:
                if path_length == 1:
                    target_dict[node] = _config_handler(func, order)
                else:
                    target_dict[node] = _config_handler(None, None)
                    target_dict = target_dict[node].children
            else:
                if path_length == 1:
                    if target_dict[node].func is not None:
                        print(f'ERROR: attempt to register one more handler for {node_name} node, '
                              f'current {target_dict[node].func.__name__}, new {func.__name__}')
                    else:
                        target_dict[node].func = func
                        target_dict[node].order = order
                else:
                    target_dict = target_dict[node].children
            path_length -= 1
    return register_config_handler


if __name__ == '__main__':
    print('FATAL ERROR: this file is component of RTC sample and is not intended to be executed directly')
    exit(EX_SOFTWARE)
