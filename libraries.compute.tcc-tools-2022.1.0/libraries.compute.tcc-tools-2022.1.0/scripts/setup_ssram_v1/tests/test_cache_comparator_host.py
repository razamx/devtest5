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

from cache_configuration_comparator_v1 import compare_configs_v1
from common.exceptions import ParsingError


def make_config(entries):
        config = copy.deepcopy(CONFIG_TEMPLATE)
        config['rtct'] = entries
        return json.dumps(config)

CONFIG_TEMPLATE = {
    "rtct": [],
    "foo": "hello world",
    "bar": 123
}

SSRAM_L3_v1 = {
    "int_cache_level": 3,
    "int_base": 12345,
    "int_ways": 8,
    "int_size": 1000,
    "int_APIC_ID": [ 0, 2, 4, 6 ],
    "Type": "common.rtct.v1.rtct.PseudoSRAM"
}

SSRAM_L2_v1 = {
    "int_cache_level": 2,
    "int_base": 12345,
    "int_ways": 240,
    "int_size": 400,
    "int_APIC_ID": [ 4 ],
    "Type": "common.rtct.v1.rtct.PseudoSRAM"
}

CACHE_CONFIG_EXAMPLE = [
    {
        "int_way_mask": [ 2, 2 ],
        "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
    },
    {
        "int_way_mask": [ 1 ],
        "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
    },
    {
        "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 15, 15 ], [ 15, 15]],
        "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
    },
    {
        "int_way_mask": [4, 4, 4, 4],
        "Type": "common.rtct.v1.rtct.CAT_L3WayMasks"
    }
]


REAL_CONFIG_CORRECT_1_SSRAM = make_config([SSRAM_L3_v1] + CACHE_CONFIG_EXAMPLE)
EXPECTED_CONFIG_CORRECT_1_SSRAM = make_config([SSRAM_L3_v1] + CACHE_CONFIG_EXAMPLE)

REAL_CONFIG_CORRECT_2_SSRAM = make_config([SSRAM_L3_v1, SSRAM_L2_v1] + CACHE_CONFIG_EXAMPLE)
EXPECTED_CONFIG_CORRECT_2_SSRAM = make_config([SSRAM_L3_v1, SSRAM_L2_v1] + CACHE_CONFIG_EXAMPLE)

class TestCacheComparator(unittest.TestCase):


    def test_bad_real_not_json_config_negative(self):
        with self.assertRaises(BaseException):
            compare_configs_v1('bad json', EXPECTED_CONFIG_CORRECT_1_SSRAM)

    def test_bad_real_no_rtct_field_config_negative(self):
        with self.assertRaises(BaseException):
            compare_configs_v1('{"a": "foo", "b": 123}', EXPECTED_CONFIG_CORRECT_1_SSRAM)

    def test_bad_expected_config_negative(self):
        with self.assertRaises(BaseException):
            compare_configs_v1(REAL_CONFIG_CORRECT_1_SSRAM, 'bad_json')

    def test_no_ssram_with_given_cache_level_negative(self):
        wrong_ssram = {
            "int_cache_level": 2, # different
            "int_base": 12345,
            "int_ways": 8,
            "int_size": 1000,
            "int_APIC_ID": [ 0, 2, 4, 6 ],
            "Type": "common.rtct.v1.rtct.PseudoSRAM"
        }
        wrong_real_config = make_config([wrong_ssram] + CACHE_CONFIG_EXAMPLE)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))


    def test_no_ssram_with_given_apic_ids_negative(self):
        wrong_ssram = {
            "int_cache_level": 3,
            "int_base": 12345,
            "int_ways": 8,
            "int_size": 1000,
            "int_APIC_ID": [ 0 ], # different
            "Type": "common.rtct.v1.rtct.PseudoSRAM"
        }
        wrong_real_config = make_config([wrong_ssram] + CACHE_CONFIG_EXAMPLE)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_single_ssram_ways_is_different_negative(self):
        wrong_ssram = {
            "int_cache_level": 3,
            "int_base": 12345,
            "int_ways": 14, # different
            "int_size": 1000,
            "int_APIC_ID": [ 0, 2, 4, 6 ],
            "Type": "common.rtct.v1.rtct.PseudoSRAM"
        }
        wrong_real_config = make_config([wrong_ssram] + CACHE_CONFIG_EXAMPLE)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_single_ssram_ssram_size_is_different_negative(self):
        wrong_ssram = {
            "int_cache_level": 3,
            "int_base": 12345,
            "int_ways": 8,
            "int_size": 2000, # different
            "int_APIC_ID": [ 0, 2, 4, 6 ],
            "Type": "common.rtct.v1.rtct.PseudoSRAM"
        }
        wrong_real_config = make_config([wrong_ssram] + CACHE_CONFIG_EXAMPLE)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_single_ssram_positive(self):
        self.assertTrue(compare_configs_v1(REAL_CONFIG_CORRECT_1_SSRAM, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_multiple_ssram_one_equal_other_is_not_equal_negative(self):
        wrong_ssram = {
            "int_cache_level": 2,
            "int_base": 12345,
            "int_ways": 240,
            "int_size": 800, # different
            "int_APIC_ID": [ 4 ],
            "Type": "common.rtct.v1.rtct.PseudoSRAM"
        }
        wrong_real_config = make_config([SSRAM_L3_v1, wrong_ssram] + CACHE_CONFIG_EXAMPLE)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_2_SSRAM))

    def test_real_ssram_is_more_than_expected_ssram_negative(self):
        self.assertFalse(compare_configs_v1(REAL_CONFIG_CORRECT_2_SSRAM, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_expected_ssram_is_more_than_real_ssram_negative(self):
        self.assertFalse(compare_configs_v1(REAL_CONFIG_CORRECT_1_SSRAM, EXPECTED_CONFIG_CORRECT_2_SSRAM))

    def test_multiple_ssram_positive(self):
        self.assertTrue(compare_configs_v1(REAL_CONFIG_CORRECT_2_SSRAM, EXPECTED_CONFIG_CORRECT_2_SSRAM))

    def test_cache_gt_different_negative(self):
        wrong_cache = [
            {
                "int_way_mask": [ 2, 3 ], # different
                "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
            },
            {
                "int_way_mask": [ 1 ],
                "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
            },
            {
                "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 15, 15 ], [ 15, 15]],
                "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
            },
            {
                "int_way_mask": [4, 4, 4, 4],
                "Type": "common.rtct.v1.rtct.CAT_L3WayMasks"
            }
        ]
        wrong_real_config = make_config([SSRAM_L3_v1] + wrong_cache)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_wrc_different_negative(self):
        wrong_cache = [
            {
                "int_way_mask": [ 2, 2 ],
                "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
            },
            {
                "int_way_mask": [ 1, 2, 3 ], # different
                "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
            },
            {
                "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 15, 15 ], [ 15, 15]],
                "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
            },
            {
                "int_way_mask": [4, 4, 4, 4],
                "Type": "common.rtct.v1.rtct.CAT_L3WayMasks"
            }
        ]
        wrong_real_config = make_config([SSRAM_L3_v1] + wrong_cache)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_l2_different_negative(self):
        wrong_cache = [
            {
                "int_way_mask": [ 2, 2 ],
                "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
            },
            {
                "int_way_mask": [ 1 ],
                "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
            },
            {
                "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 5, 5 ], [ 15, 15]], # different
                "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
            },
            {
                "int_way_mask": [4, 4, 4, 4],
                "Type": "common.rtct.v1.rtct.CAT_L3WayMasks"
            }
        ]
        wrong_real_config = make_config([SSRAM_L3_v1] + wrong_cache)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_l3_different_negative(self):
        wrong_cache = [
            {
                "int_way_mask": [ 2, 2 ],
                "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
            },
            {
                "int_way_mask": [ 1 ],
                "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
            },
            {
                "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 15, 15 ], [ 15, 15]],
                "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
            },
            {
                "int_way_mask": [4, 4, 2, 2, 2], # different
                "Type": "common.rtct.v1.rtct.CAT_L3WayMasks"
            }
        ]
        wrong_real_config = make_config([SSRAM_L3_v1] + wrong_cache)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_agent_is_missing_negative(self):
        wrong_cache = [
            {
                "int_way_mask": [ 2, 2 ],
                "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
            },
            {
                "int_way_mask": [ 1 ],
                "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
            },
            {
                "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 15, 15 ], [ 15, 15]],
                "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
            }
            # l3 missing
        ]
        wrong_real_config = make_config([SSRAM_L3_v1] + wrong_cache)
        self.assertFalse(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM))

    def test_cache_positive(self):
        compare_configs_v1(REAL_CONFIG_CORRECT_1_SSRAM, EXPECTED_CONFIG_CORRECT_1_SSRAM)

    def test_compare_only_buffer_positive(self):
        wrong_cache = [
            {
                "int_way_mask": [ 2, 3 ], # different
                "Type": "common.rtct.v1.rtct.GT_L3WayMasks"
            },
            {
                "int_way_mask": [ 1 ],
                "Type": "common.rtct.v1.rtct.WRC_L3WayMasks"
            },
            {
                "int_way_mask": [[ 255, 255 ], [ 255, 255 ], [ 15, 15 ], [ 15, 15]],
                "Type": "common.rtct.v1.rtct.CAT_L2WayMasks"
            },
            {
                "int_way_mask": [4, 4, 4, 4],
                "Type": "common.rtct.v1.rtct.CAT_L3WayMasks"
            }
        ]
        wrong_real_config = make_config([SSRAM_L3_v1] + wrong_cache)
        self.assertTrue(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM, compare_cache=False))

    def test_compare_only_cache_positive(self):
        wrong_ssram = {
            "int_cache_level": 2, # different
            "int_base": 12345,
            "int_ways": 8,
            "int_size": 1000,
            "int_APIC_ID": [ 0, 2, 4, 6 ],
            "Type": "common.rtct.v1.rtct.PseudoSRAM"
        }
        wrong_real_config = make_config([wrong_ssram] + CACHE_CONFIG_EXAMPLE)
        self.assertTrue(compare_configs_v1(wrong_real_config, EXPECTED_CONFIG_CORRECT_1_SSRAM, compare_buffer=False))


if __name__ == "__main__":
    unittest.main()
