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
import sys
import json
from typing import List, Tuple
from dataclasses import dataclass

try:
    from common.version_checker import check_version
    check_version(3, 7)

    import common.dataclass as dc
    from common.rtct import RTCT_Base, Rtct_Type, rtct_get
    from common.rtct.v1 import PseudoSRAM, GT_L3WayMasks, WRC_L3WayMasks, CAT_L2WayMasks, CAT_L3WayMasks
    from common.log import trace
    from common.exceptions import ParsingError
except ImportError:
    logging.error('Failed to import common modules')
    exit(-1)


# Custom class with ony required fields
@dataclass(frozen=True)
class TmpPseudoSRAM:
    int_cache_level: int
    int_ways: int
    int_size: int
    int_APIC_ID: Tuple  # noqa: N815


def _parse_config(text: str) -> List[RTCT_Base]:
    """
    Parse any JSON object which contains "rtct" field, because we are not
    interested in other fields in full config (see SystemConfiguration class)
    """
    rtct = json.loads(text).get('rtct')
    if rtct is None:
        raise ParsingError('Configuration has wrong format: "rtct" field is missing')
    return dc.fromdict(rtct, List[RTCT_Base])


@trace('Failed to parse configs')
def _parse_configs(real: str, expected: str) -> Tuple[List[RTCT_Base], List[RTCT_Base]]:
    try:
        real_cfg = _parse_config(real)
    except Exception:
        logging.error('Failed to parse real config')
        raise

    try:
        expected_cfg = _parse_config(expected)
    except Exception:
        logging.error('Failed to parse expected config')
        raise

    return real_cfg, expected_cfg


def _convert_ssram(ssram: PseudoSRAM) -> TmpPseudoSRAM:
    return TmpPseudoSRAM(ssram.int_cache_level, ssram.int_ways, ssram.int_size, tuple(ssram.int_APIC_ID))


def _convert_ssrams(ssrams: List[PseudoSRAM]) -> List[TmpPseudoSRAM]:
    return [_convert_ssram(ssram) for ssram in ssrams]


def _compare_buffer_configuration(real: List[RTCT_Base], expected: List[RTCT_Base]) -> bool:
    real_ssram = set(_convert_ssrams(rtct_get(real, PseudoSRAM)))
    expected_ssram = set(_convert_ssrams(rtct_get(expected, PseudoSRAM)))

    not_expected = real_ssram.difference(expected_ssram)
    missing = expected_ssram.difference(real_ssram)

    if len(missing) > 0:
        logging.error('Following expected SoftwareSRAM are missing:')
        for ssram in missing:
            logging.error(f'    {ssram}')

    if len(not_expected) > 0:
        logging.error('Following SoftwareSRAM are not expected:')
        for ssram in not_expected:
            logging.error(f'    {ssram}')

    return len(missing) == 0 and len(not_expected) == 0


def _compare_agent(real: List[RTCT_Base], expected: List[RTCT_Base], agent_type: Rtct_Type) -> bool:
    real_agent = rtct_get(real, agent_type)
    expected_agent = rtct_get(expected, agent_type)

    if real_agent != expected_agent:
        logging.error(f'Agent {agent_type.__name__} does not match')
        logging.error(f'Expected: {expected_agent}')
        logging.error(f'Real:     {real_agent}')
        return False
    return True


def _compare_cache_configuration(real: List[RTCT_Base], expected: List[RTCT_Base]) -> bool:
    compare = [
        _compare_agent(real, expected, GT_L3WayMasks),
        _compare_agent(real, expected, WRC_L3WayMasks),
        _compare_agent(real, expected, CAT_L3WayMasks),
        _compare_agent(real, expected, CAT_L2WayMasks)
    ]
    return all(compare)


@trace('Failed to compare configs')
def compare_configs_v1(
        real: str, expected: str, compare_buffer: bool = True, compare_cache: bool = True) -> bool:
    """
    Compare two cache configurations in JSON format (see SystemInfo class)
    """
    real_cfg, expected_cfg = _parse_configs(real, expected)
    compare = [
        _compare_buffer_configuration(real_cfg, expected_cfg) if compare_buffer else True,
        _compare_cache_configuration(real_cfg, expected_cfg) if compare_cache else True
    ]
    is_equal = all(compare)
    if is_equal:
        logging.info('Configuration are equal')
    else:
        logging.error('Configuration are different')
    return is_equal


@trace('Failed to read configuration from file')
def _read_configuration_from_file(file_path) -> str:
    with open(file_path, 'r') as f:
        return f.read()


@trace('Failed to read configurations')
def input_data_v1(args):
    real = _read_configuration_from_file(args.real) if args.real is not None else sys.stdin.read()
    expected = _read_configuration_from_file(args.expected)
    return real, expected


def main_v1(args):
    real, expected = input_data_v1(args)

    if not compare_configs_v1(real, expected, args.buffer, args.cache):
        logging.error("Configurations don't match")
        return -1
    return 0


def init_parser_v1():
    parser = argparse.ArgumentParser(
        description='Compare cache configuration with expected configuration.',
        usage='compare_cache_config.py [-h] -e|--expected <expected> '
              '[-r|--real <real>] [-b|--buffer] [-c|--cache]\n'
              'Real configuration in JSON format is read from stdin or from file.',
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument(
        '-e', '--expected', required=True, help='Path to the JSON file with expected configuration')
    parser.add_argument('-r', '--real', help='Path to the JSON file with real configuration')
    parser.add_argument('-b', '--buffer', action='store_true', help='Compare buffer configuration')
    parser.add_argument('-c', '--cache', action='store_true', help='Compare cache configuration')
    return parser


if __name__ == "__main__":
    parser = init_parser_v1()
    args = parser.parse_args()
    exit(main_v1(args))
