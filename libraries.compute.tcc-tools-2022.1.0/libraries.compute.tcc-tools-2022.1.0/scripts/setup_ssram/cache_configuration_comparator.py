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
from typing import List, Tuple, Type, Union, TypeVar
from dataclasses import dataclass

try:
    from common.version_checker import check_version
    check_version(3, 7)

    import common.dataclass as dc
    import common.rtct as rtct
    from common.log import trace
    from common.exceptions import ParsingError
except ImportError:
    logging.error('Failed to import common modules')
    exit(-1)


# Custom class with ony required fields
@dataclass(frozen=True)
class Waymask:
    level: int
    cache_id: int
    waymask: Union[int, List[int]]


def _parse_config(text: str) -> List[rtct.RTCT_Base]:
    """
    Parse any JSON object which contains "rtct" field, because we are not
    interested in other fields in full config (see SystemConfiguration class)
    """
    _rtct = json.loads(text).get('rtct')
    if _rtct is None:
        raise ParsingError('Configuration has wrong format: "rtct" field is missing')
    return dc.fromdict(_rtct, List[rtct.RTCT_Base])


@trace('Failed to parse configs')
def _parse_configs(real: str, expected: str) -> Tuple[List[rtct.RTCT_Base], List[rtct.RTCT_Base]]:
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


T = TypeVar('T', rtct.v2.SSRAM_WayMask, rtct.v2.IA_WayMasks, rtct.v2.GT_WayMasks, rtct.v2.WRC_WayMasks)


def _convert_agent_mask(agent: T) -> Waymask:
    return Waymask(agent.Level, agent.Cache_ID, agent.WayMask)


def _convert_agents(agents: List[T]) -> List[Waymask]:
    return [_convert_agent_mask(agent) for agent in agents]


def _get_agents(_rtct: List[rtct.RTCT_Base], _type: Type[T]) -> List[Waymask]:
    agents = rtct.rtct_get(_rtct, _type)
    return _convert_agents(agents)


def _compare_ssram_configuration(real: List[rtct.RTCT_Base], expected: List[rtct.RTCT_Base]) -> bool:
    # NOTE: SSRAM size is not used for comparison, only WayMask is used
    # 1. It isn't needed now, WayMasks describes SSRAM compleately, because
    #    size = way_size * num_set_bits  # noqa: E800
    # 2. It's difficult to generate expected Size (need to know way size)
    logging.debug('Comparing Software SRAMs')
    real_ssram = set(_get_agents(real, rtct.v2.SSRAM_WayMask))
    expected_ssram = set(_get_agents(expected, rtct.v2.SSRAM_WayMask))
    logging.debug('Real:     %s', real_ssram)
    logging.debug('Expected: %s', expected_ssram)

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

    is_ok = len(missing) == 0 and len(not_expected) == 0
    if is_ok:
        logging.debug('SSRAMs correct')
    return is_ok


def _compare_agent(real: List[rtct.RTCT_Base], expected: List[rtct.RTCT_Base], agent_type: Type[T]) -> bool:
    real_agent = _get_agents(real, agent_type)
    expected_agent = _get_agents(expected, agent_type)
    logging.debug('Comparing agent, type=%s', agent_type)
    logging.debug('Real:     %s', real_agent)
    logging.debug('Expected: %s', expected)

    # TODO: compare agents in order-agnostic manear
    if real_agent != expected_agent:
        logging.error(f'Agent {agent_type.__name__} does not match')
        logging.error(f'Expected: {expected_agent}')
        logging.error(f'Real:     {real_agent}')
        return False
    logging.debug('Agent correct')
    return True


def _compare_cache_configuration(real: List[rtct.RTCT_Base], expected: List[rtct.RTCT_Base]) -> bool:
    logging.debug('Comparing cache agents')
    compare = [
        _compare_agent(real, expected, rtct.v2.GT_WayMasks),
        _compare_agent(real, expected, rtct.v2.WRC_WayMasks),
        _compare_agent(real, expected, rtct.v2.IA_WayMasks),
    ]
    return all(compare)


@trace('Failed to compare configs')
def compare_configs(
        real: str, expected: str, compare_buffer: bool = True, compare_cache: bool = True) -> bool:
    """
    Compare two cache configurations in JSON format (see SystemInfo class)
    """
    logging.debug('Parsing configurations')
    real_cfg, expected_cfg = _parse_configs(real, expected)
    logging.debug('Comparing configurations')
    compare = [
        _compare_ssram_configuration(real_cfg, expected_cfg) if compare_buffer else True,
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
def input_data(args):
    real = _read_configuration_from_file(args.real) if args.real is not None else sys.stdin.read()
    expected = _read_configuration_from_file(args.expected)
    return real, expected


def main(args):
    real, expected = input_data(args)

    if not compare_configs(real, expected, args.buffer, args.cache):
        logging.error("Configurations don't match")
        return -1
    return 0


def init_parser():
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
    parser = init_parser()
    args = parser.parse_args()
    exit(main(args))
