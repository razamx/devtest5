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

import import_helper  # It's needed # noqa: F401

import argparse
import logging
import pathlib
import sys

BUFFER_GUID = '3A6F667B-CED4-481B-8FC3-C087A0F2DD53'
CACHE_GUID = '9A069A23-836F-41DF-AF02-F14BEC97DE72'

try:
    from common.version_checker import check_version
    check_version(3, 7)

    import common.supported_cpus as cpus
except ImportError:
    logging.error('Failed to import common modules')
    exit(-1)


def apply_config(path):
    print('applying', path)


def main():
    cpu_model = cpus.get_model_name()
    cpu_info = cpus.get_supported_cpu_info(cpu_model)
    logging.debug('CPU: %s', cpu_model)
    if cpu_info is None:
        logging.error('No configuration found for the CPU %s. Try to use manual input.', cpu_model)
        for cache_type in cpus.CacheType:
            print(f"Use option {cache_type.name.lower()} for:", file=sys.stderr)
            for cpu in cpus.SUPPORTED_CPUS:
                if cpu.cache_type == cache_type:
                    print(f"\t{cpu.cpu_model_name}", file=sys.stderr)
        return -1

    configs_root = pathlib.Path(__file__).parent / 'config'
    type_folder = cpu_info.cache_type.name.lower()
    config_dir = configs_root / type_folder
    logging.debug('Config dir for %s: %s', cpu_model, config_dir)
    if not config_dir.exists():
        logging.error('No configuration found for CPU %s', cpu_model)
        return -1

    print(config_dir)
    return 0


def parse_args():
    parser = argparse.ArgumentParser(
        description='Prints directory with configuration based on current CPU model')
    parser.parse_args()


if __name__ == "__main__":
    parse_args()
    exit(main())
