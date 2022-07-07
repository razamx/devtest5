# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

import sys
import os
# NOTE: workaround to import modules
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../'))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../tools'))

import unittest
import copy
import json
from dataclasses import dataclass, replace
from typing import List

from cache_configuration_comparator import compare_configs
import common.rtct as rtct
from common.exceptions import ParsingError
import common.dataclass as dc

@dataclass
class RTCT:
    rtct: List[rtct.RTCT_Base]

def make_config(entries):
        return dc.dumps(RTCT(entries), indent=2)

SSRAM_L3_SSRAM = rtct.v2.SoftwareSRAM(
    Level=3,
    Cache_ID=0,
    Base=12345,
    Size=1000,
    Shared=0
)
SSRAM_L3_MASK = rtct.v2.SSRAM_WayMask(
    Level=3,
    Cache_ID=0,
    WayMask=0x80
)
SSRAM_L3 = [SSRAM_L3_SSRAM, SSRAM_L3_MASK]

SSRAM_L2_SSRAM = rtct.v2.SoftwareSRAM(
    Level=2,
    Cache_ID=0,
    Base=12345,
    Size=400,
    Shared=0
)
SSRAM_L2_MASK = rtct.v2.SSRAM_WayMask(
    Level=2,
    Cache_ID=0,
    WayMask=0xC0
)
SSRAM_L2 = [SSRAM_L2_SSRAM, SSRAM_L2_MASK]

CACHE_CONFIG_EXAMPLE = [
    rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 2]),
    rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=1),
    rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
    rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
    rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[15, 15]),
    rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
    rtct.v2.IA_WayMasks(Level=3,  Cache_ID=0, WayMask=[4, 4, 4, 4]),
]

REAL_CONFIG_CORRECT_1_SSRAM = make_config(SSRAM_L3 + CACHE_CONFIG_EXAMPLE)
EXPECTED_CONFIG_CORRECT_1_SSRAM = make_config(SSRAM_L3 + CACHE_CONFIG_EXAMPLE)

REAL_CONFIG_CORRECT_2_SSRAM = make_config(SSRAM_L3 + SSRAM_L2 + CACHE_CONFIG_EXAMPLE)
EXPECTED_CONFIG_CORRECT_2_SSRAM = make_config(SSRAM_L3 + SSRAM_L2 + CACHE_CONFIG_EXAMPLE)

class TestCompareSsrams(unittest.TestCase):

    def test_bad_real_not_json_config_negative(self):
        with self.assertRaises(BaseException):
            compare_configs('bad json', EXPECTED_CONFIG_CORRECT_1_SSRAM)

    def test_bad_real_no_rtct_field_config_negative(self):
        with self.assertRaises(BaseException):
            compare_configs('{"a": "foo", "b": 123}', EXPECTED_CONFIG_CORRECT_1_SSRAM)

    def test_bad_expected_config_negative(self):
        with self.assertRaises(BaseException):
            compare_configs(REAL_CONFIG_CORRECT_1_SSRAM, 'bad_json')

    def test_no_ssram_with_given_cache_level_negative(self):
        real_cfg = [
            replace(SSRAM_L3_SSRAM, Level=2),
            replace(SSRAM_L3_MASK, Level=2),
        ]
        self.assertFalse(compare_configs(
            make_config(real_cfg + CACHE_CONFIG_EXAMPLE),
            EXPECTED_CONFIG_CORRECT_1_SSRAM))


    def test_no_ssram_with_given_cache_id_negative(self):
        real_cfg = [
            replace(SSRAM_L3_SSRAM, Cache_ID=2),
            replace(SSRAM_L3_MASK, Cache_ID=2),
        ]
        self.assertFalse(compare_configs(
            make_config(real_cfg + CACHE_CONFIG_EXAMPLE),
            EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_single_ssram_ways_is_different_negative(self):
        real_cfg = [
            SSRAM_L3_SSRAM,
            replace(SSRAM_L3_MASK, WayMask=0x100),
        ]
        self.assertFalse(compare_configs(
            make_config(real_cfg + CACHE_CONFIG_EXAMPLE),
            EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_single_ssram_positive(self):
        self.assertTrue(compare_configs(REAL_CONFIG_CORRECT_1_SSRAM, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_multiple_ssram_one_equal_other_is_not_equal_negative(self):
        real_cfg = [
            SSRAM_L3_SSRAM,
            replace(SSRAM_L3_MASK, WayMask=0x100),
            SSRAM_L2_SSRAM,
            SSRAM_L2_MASK
        ]
        self.assertFalse(compare_configs(
            make_config(real_cfg + CACHE_CONFIG_EXAMPLE),
            EXPECTED_CONFIG_CORRECT_2_SSRAM))

    def test_real_ssram_is_more_than_expected_ssram_negative(self):
        self.assertFalse(compare_configs(REAL_CONFIG_CORRECT_2_SSRAM, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_expected_ssram_is_more_than_real_ssram_negative(self):
        self.assertFalse(compare_configs(REAL_CONFIG_CORRECT_1_SSRAM, EXPECTED_CONFIG_CORRECT_2_SSRAM))

    def test_multiple_ssram_positive(self):
        self.assertTrue(compare_configs(REAL_CONFIG_CORRECT_2_SSRAM, EXPECTED_CONFIG_CORRECT_2_SSRAM))

class TestCompareCache(unittest.TestCase):

    def test_cache_gt_different_negative(self):
        wrong_cache = [
            rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 3]), # different
            rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=1),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=3,  Cache_ID=0, WayMask=[4, 4, 4, 4]),
        ]
        wrong_real_config = make_config(SSRAM_L3 + wrong_cache)
        self.assertFalse(compare_configs(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_wrc_different_negative(self):
        wrong_cache = [
            rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 2]),
            rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=2), # different
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=3,  Cache_ID=0, WayMask=[4, 4, 4, 4]),
        ]
        wrong_real_config = make_config(SSRAM_L3 + wrong_cache)
        self.assertFalse(compare_configs(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_l2_different_negative(self):
        wrong_cache = [
            rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 2]),
            rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=1),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[5, 5]), # different
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=3,  Cache_ID=0, WayMask=[4, 4, 4, 4]),
        ]
        wrong_real_config = make_config(SSRAM_L3 + wrong_cache)
        self.assertFalse(compare_configs(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_l3_different_negative(self):
        wrong_cache = [
            rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 2]),
            rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=1),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=3,  Cache_ID=0, WayMask=[4, 2, 4, 4]), # different
        ]
        wrong_real_config = make_config(SSRAM_L3 + wrong_cache)
        self.assertFalse(compare_configs(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_agent_is_missing_negative(self):
        wrong_cache = [
            rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 2]),
            rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=1),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
            # L3 is missing
        ]
        wrong_real_config = make_config(SSRAM_L3 + wrong_cache)
        self.assertFalse(compare_configs(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_positive(self):
        compare_configs(REAL_CONFIG_CORRECT_1_SSRAM, EXPECTED_CONFIG_CORRECT_1_SSRAM)

    def test_compare_only_buffer_positive(self):
        wrong_cache = [
            rtct.v2.GT_WayMasks(Level=3,  Cache_ID=0, WayMask=[2, 2]),
            rtct.v2.WRC_WayMasks(Level=3, Cache_ID=0, WayMask=2), # different
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=0, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=1, WayMask=[255, 255]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=2, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=2,  Cache_ID=3, WayMask=[15, 15]),
            rtct.v2.IA_WayMasks(Level=3,  Cache_ID=0, WayMask=[4, 4, 4, 4]),
        ]
        wrong_real_config = make_config(SSRAM_L3 + wrong_cache)
        # Cache is different, but result is True
        self.assertTrue(compare_configs(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM, compare_cache=False))

    def test_compare_only_cache_positive(self):
        real_cfg = [
            SSRAM_L3_SSRAM,
            replace(SSRAM_L3_MASK, WayMask=0x100),
        ]
        self.assertTrue(compare_configs(
            make_config(real_cfg + CACHE_CONFIG_EXAMPLE),
            EXPECTED_CONFIG_CORRECT_1_SSRAM, compare_buffer=False))


if __name__ == "__main__":
    unittest.main()
