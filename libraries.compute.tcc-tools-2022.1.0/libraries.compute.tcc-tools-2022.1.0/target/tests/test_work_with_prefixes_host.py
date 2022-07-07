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

class TestUpdatePrefix(unittest.TestCase):
	def setUp(self):
		tcc_setup.install_prefix = "tmp/tcc"
		tcc_setup.install_prefix_bin = "tmp/tcc/bin"
		tcc_setup.install_prefix_lib = "tmp/tcc/lib64"
		tcc_setup.install_prefix_share = "tmp/tcc/share"
		tcc_setup.install_prefix_include = "tmp/tcc/include"

	def tearDown(self):
		tcc_setup.install_prefix = ""
		tcc_setup.install_prefix_bin = ""
		tcc_setup.install_prefix_lib = ""
		tcc_setup.install_prefix_share = ""
		tcc_setup.install_prefix_include = ""

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_update_prefix_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "tmp/tcc/file"
		value = tcc_setup.tcc_install_update_prefix("usr/file")
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_update_prefix_bin_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "tmp/tcc/bin/file"
		value = tcc_setup.tcc_install_update_prefix("usr/bin/file")
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_update_prefix_lib_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "tmp/tcc/lib64/file"
		value = tcc_setup.tcc_install_update_prefix("usr/lib64/file")
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_update_prefix_share_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "tmp/tcc/share/file"
		value = tcc_setup.tcc_install_update_prefix("usr/share/file")
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_update_prefix_include_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "tmp/tcc/include/file"
		value = tcc_setup.tcc_install_update_prefix("usr/include/file")
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_process_error_correct(
			self,
			mocked_exit,
			mocked_log):
		tcc_setup.tcc_install_update_prefix("/not_correct_input")
		mocked_exit.assert_called_once()

class TestParsePrefix(unittest.TestCase):

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_install_prefix_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "usr/bin"
		value = tcc_setup.parse_prefix("usr", None, "bin")
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_special_prefix_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = "/tmp"
		value = tcc_setup.parse_prefix(None, "/tmp", None)
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_process_error_correct(
			self,
			mocked_exit,
			mocked_log):
		tcc_setup.parse_prefix(None, None, "bin")
		mocked_exit.assert_called_once()

class TestParsePrefixes(unittest.TestCase):

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_parse_default_prefixes_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = [
			"/usr",
			"/usr/bin",
			"/usr/lib64",
			"/usr/share",
			"/usr/include"
		]

		config = mock.Mock()    
		config.prefix = "/usr"
		config.prefix_bin = None
		config.prefix_lib = None
		config.prefix_share = None
		config.prefix_include = None

		tcc_setup.parse_prefixes(config)
		value = [
			tcc_setup.install_prefix,
			tcc_setup.install_prefix_bin,
			tcc_setup.install_prefix_lib,
			tcc_setup.install_prefix_share,
			tcc_setup.install_prefix_include,
		]
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.exit')
	def test_parse_custom_prefixes_correct(
			self,
			mocked_exit,
			mocked_log):
		expected = [
			"/tmp",
			"/tmp/bin",
			"/tmp/lib64",
			"/tmp/share",
			"/tmp/include"
		]

		config = mock.Mock()    
		config.prefix = "/tmp"
		config.prefix_bin = "/tmp/bin" 
		config.prefix_lib = "/tmp/lib64"
		config.prefix_share = "/tmp/share"
		config.prefix_include = "/tmp/include"

		tcc_setup.parse_prefixes(config)

		value = [
			tcc_setup.install_prefix,
			tcc_setup.install_prefix_bin,
			tcc_setup.install_prefix_lib,
			tcc_setup.install_prefix_share,
			tcc_setup.install_prefix_include,
		]
		self.assertEqual(expected, value)

if __name__ == "__main__":
	unittest.main()
