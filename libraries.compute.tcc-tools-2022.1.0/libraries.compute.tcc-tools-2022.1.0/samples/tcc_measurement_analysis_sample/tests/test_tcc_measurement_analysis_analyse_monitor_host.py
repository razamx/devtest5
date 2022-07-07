#!/usr/bin/env python3
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
from os.path import dirname
sys.path.append(dirname(__file__)+'/../src')

import unittest
from unittest.mock import MagicMock
from unittest.mock import Mock
import modules.tcc_measurement_analysis_monitor as monitor

class test_time_units_to_clocks_host(unittest.TestCase):

    def test_time_units_to_clocks_ns_time_unit(self):
        time_interval = 987654321234567890
        expected_closk_number = 123
        time_units = "ns"
        monitor.tcc_lib.tcc_measurement_convert_timespec_to_clock= Mock();
        monitor.tcc_lib.tcc_measurement_convert_timespec_to_clock.side_effect= lambda timespec:(
            expected_closk_number,
            self.assertEqual(
                timespec.tv_nsec,
                234567890,
                "Wrong nanoseconds calculation"),
            self.assertEqual(
                timespec.tv_sec,
                987654321,
                "Wrong s4econds calculation"),
            )[0]
        self.assertEqual(
            monitor.time_units_to_clocks(time_interval, time_units),
            expected_closk_number,
            "Method return wrong value")

    def test_time_units_to_clocks_us_time_unit(self):
        time_interval = 987654321234567890
        expected_closk_number = 123
        time_units = "us"
        monitor.tcc_lib.tcc_measurement_convert_timespec_to_clock= Mock();
        monitor.tcc_lib.tcc_measurement_convert_timespec_to_clock.side_effect= lambda timespec:(
            expected_closk_number,
            self.assertEqual(
                timespec.tv_nsec,
                567890000,
                "Wrong nanoseconds calculation"),
            self.assertEqual(
                timespec.tv_sec,
                987654321234,
                "Wrong s4econds calculation"),
            )[0]
        self.assertEqual(
            monitor.time_units_to_clocks(time_interval, time_units),
            expected_closk_number,
            "Method return wrong value")

    def test_time_units_to_clocks_wrong_time_unit(self):
        time_interval = 1234567890
        time_interval_expected = 1234567890
        time_units = "ms"
        self.assertEqual(
             monitor.time_units_to_clocks(time_interval, time_units),
             time_interval_expected,
             "wrong behavior for wrong time unit")

if __name__ == '__main__':
    unittest.main()
