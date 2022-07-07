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
sys.path.append(dirname(__file__)+'/..')

import unittest
import unittest.mock as mock
import tcc_setup

class TestLogging(unittest.TestCase):

	def tearDown(self):
		tcc_setup.log_file = None
		tcc_setup.install_cfg_file = None
		tcc_setup.error_reported = False

	def test_log_line_correct(self):
		expected = mock.call.write("log\r\n")

		log = mock.Mock()
		tcc_setup.log_file = log
		tcc_setup.log_line("log")
		value = log.mock_calls[0]

		self.assertEqual(expected, value)

	def test_add_install_item_correct(self):
		expected = mock.call.write("D:dir\r\n")

		install_cfg_file = mock.Mock()
		tcc_setup.install_cfg_file = install_cfg_file
		tcc_setup.add_install_item("dir", "D")
		value = install_cfg_file.mock_calls[0]

		self.assertEqual(expected, value)

	@mock.patch("builtins.print")
	def test_first_call_report_preinstallation_error_correct(
			self,
			mocked_print):
		expected = [
			mock.call("Error: pre-installation check failed, the following issue(s) detected:"),
			mock.call("* Error body")
		]

		tcc_setup.report_preinstallation_error("Error body")
		value = mocked_print.mock_calls

		self.assertEqual(expected, value)

	@mock.patch("builtins.print")
	def test_second_call_report_preinstallation_error_correct(
			self,
			mocked_print):
		expected = [
			mock.call("* Error body")
		]

		tcc_setup.error_reported = True
		tcc_setup.report_preinstallation_error("Error body")
		value = mocked_print.mock_calls

		self.assertEqual(expected, value)

if __name__ == "__main__":
	unittest.main()
