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

class TestInstallProcess(unittest.TestCase):

	def setUp(self):
		tcc_setup.config_lines = [
			"PASSLIST:usr/bin/*",
			"PASSLIST:usr/lib64/*",
			"PASSLIST:usr/share/*",
			"PASSLIST:usr/include/*",
			"PERMIT:777:usr/share/*"
		]

	def tearDown(self):
		tcc_setup.config_lines = []
		tcc_setup.install_cfg_file = None
		tcc_setup.install_cfg_filename = ""

	@mock.patch('os.readlink', return_value="../share/file")
	@mock.patch('os.remove')
	@mock.patch('os.symlink')
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	def test_symlink_correction_correct(
			self,
			mocked_tcc_install_update_prefix,
			mocked_symlink,
			mocked_remove,
			mocked_readlink):
		expected = "usr/share/file", "usr/bin/file"

		tcc_setup.symlink_correction("usr/bin/file", "usr/bin/file")

		mocked_symlink.assert_called_with(*expected)

	@mock.patch('tcc_setup.log_line')
	def test_create_passlist_correct(
			self,
			mocked_log):
		expected = [
			"usr/bin/file",
			"usr/lib64/file",
			"usr/share/file",
			"usr/include/file"
		]

		bin_file = mock.Mock()
		bin_file.name = "usr/bin/file"
		lib_file = mock.Mock()
		lib_file.name = "usr/lib64/file"
		share_file = mock.Mock()
		share_file.name = "usr/share/file"
		include_file = mock.Mock()
		include_file.name = "usr/include/file"
		no_passlist = mock.Mock()
		no_passlist.name = "/tmp"

		input = [
			bin_file,
			lib_file,
			share_file,
			include_file,
			no_passlist
		]

		value = tcc_setup.create_passlist(input)
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	def test_custom_mode_request_return_mode(
			self,
			mocked_log):
		expected = 511
		input = "usr/share/file"
		value = tcc_setup.custom_mode_request(input)
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	def test_custom_mode_request_return_none(
			self,
			mocked_log):
		expected = None
		input = "usr/bin/file"
		value = tcc_setup.custom_mode_request(input)
		self.assertEqual(expected, value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('tcc_setup.tcc_install_update_prefix', return_value='usr/share/file')
	@mock.patch('os.chmod')
	def test_apply_permissions_correct(
			self,
			mocked_chmod,
			mocked_prefix,
			mocked_log):
		expected = "usr/share/file", 511
		bin_file = mock.Mock()
		bin_file.name = "usr/bin/file"
		lib_file = mock.Mock()
		lib_file.name = "usr/lib64/file"
		share_file = mock.Mock()
		share_file.name = "usr/share/file"
		include_file = mock.Mock()
		include_file.name = "usr/include/file"
		
		input = [
			bin_file,
			lib_file,
			share_file,
			include_file
		]

		tcc_setup.apply_permissions(input)
		mocked_chmod.assert_called_with(*expected)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('tcc_setup.add_install_item')
	@mock.patch('os.path.isdir', return_value=True)
	@mock.patch('os.mkdir')
	def test_tcc_install_directory_correct(
			self,
			mocked_mkdir,
			mocked_isdir,
			mocked_add_install_item,
			mocked_log):
		expected = "usr/bin", "E"

		input = mock.Mock()
		input.name = "usr/bin"
		input.isdir.return_value = True

		tcc_setup.tcc_install_directory(mock.Mock(), input)
		mocked_add_install_item.assert_called_with(*expected)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('tcc_setup.report_preinstallation_error')
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('os.path.exists', return_value=False)
	@mock.patch('os.access', return_value=True)
	def test_tcc_check_install_correct(
			self,
			mocked_os_access,
			mocked_os_path_exists,
			mocked_tcc_install_update_prefix,
			mocked_report,
			mocked_log):
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		no_passlist = mock.Mock()
		no_passlist.name = "/tmp/test"
		no_passlist.isdir.return_value = True
		no_passlist.isfile.return_value = False
		no_passlist.issym.return_value = False

		items = [file, dir, symlink, no_passlist]
		passlist = ["usr/bin/file", "usr/lib64", "usr/share/file"]

		value = tcc_setup.tcc_check_install(items, passlist)

		self.assertTrue(value)

	@mock.patch('tcc_setup.log_line')
	@mock.patch('tcc_setup.report_preinstallation_error')
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('os.path.exists', return_value=True)
	@mock.patch('os.access', return_value=False)
	def test_tcc_check_install_exist_files_process_correct(
			self,
			mocked_os_access,
			mocked_os_path_exists,
			mocked_tcc_install_update_prefix,
			mocked_report,
			mocked_log):
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		no_passlist = mock.Mock()
		no_passlist.name = "/tmp/test"
		no_passlist.isdir.return_value = True
		no_passlist.isfile.return_value = False
		no_passlist.issym.return_value = False

		items = [file, dir, symlink, no_passlist]
		passlist = ["usr/bin/file", "usr/lib64", "usr/share/file"]

		value = tcc_setup.tcc_check_install(items, passlist)

		self.assertFalse(value)

	@mock.patch('os.path.abspath', side_effect=["old_path", "new_path"])
	@mock.patch('os.path.exists', side_effect=[True, False])
	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', mock.mock_open(read_data="D:usr/bin\nF:usr/lib64/file\nS:usr/share/file"))
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.rmdir')
	@mock.patch('os.remove')
	def test_tcc_unistall_correct(
			self,
			mocked_os_remove,
			mocked_os_rmdir,
			mocked_exit,
			mocked_print,
			mocked_log,
			mocked_os_path_exists,
			mocked_os_path_abspath):
		tcc_setup.shutil = mock.Mock()

		tcc_setup.tcc_uninstall(False)

		mocked_exit.assert_not_called()

	@mock.patch('os.path.abspath', side_effect=["old_path", "new_path"])
	@mock.patch('os.path.exists', side_effect=[True, False])
	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', mock.mock_open(read_data="D:usr/bin\nF:usr/lib64/file\nS:usr/share/file"))
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.rmdir', side_effect=OSError())
	@mock.patch('os.remove')
	def test_tcc_unistall_force_correct(
			self,
			mocked_os_remove,
			mocked_os_rmdir,
			mocked_exit,
			mocked_print,
			mocked_log,
			mocked_os_path_exists,
			mocked_os_path_abspath):
		tcc_setup.shutil = mock.Mock()

		tcc_setup.tcc_uninstall(True)

		mocked_exit.assert_not_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', side_effect=OSError())
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.rmdir')
	@mock.patch('os.remove')
	def test_tcc_unistall_process_error_correct(
			self,
			mocked_os_remove,
			mocked_os_rmdir,
			mocked_exit,
			mocked_print,
			mocked_open,
			mocked_log):
		tcc_setup.shutil = mock.Mock()

		tcc_setup.tcc_uninstall(False)

		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', mock.mock_open(read_data="D:usr/bin\nF:usr/lib64/file\nS:usr/share/file"))
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.rmdir')
	@mock.patch('os.remove', side_effect=OSError())
	def test_tcc_unistall_process_remove_error_correct(
			self,
			mocked_os_remove,
			mocked_os_rmdir,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.shutil = mock.Mock()

		tcc_setup.tcc_uninstall(False)

		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', mock.mock_open(read_data="D:usr/bin\nF:usr/lib64/file\nS:usr/share/file"))
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.rmdir')
	@mock.patch('os.remove', side_effect=FileNotFoundError())
	def test_tcc_unistall_process_file_not_found_correct(
			self,
			mocked_os_remove,
			mocked_os_rmdir,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.shutil = mock.Mock()

		tcc_setup.tcc_uninstall(False)

		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', mock.mock_open())
	@mock.patch('builtins.exit')
	def test_tcc_install_open_config_file_correct(
			self,
			mocked_exit,
			mocked_log):
		tcc_setup.tcc_install_open_config_file("default")
		mocked_exit.assert_not_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', side_effect=OSError())
	@mock.patch('builtins.exit')
	def test_tcc_install_open_config_file_process_error_correct(
			self,
			mocked_exit,
			mocked_open,
			mocked_log):
		tcc_setup.tcc_install_open_config_file("default")
		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', mock.mock_open())
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	def test_tcc_install_create_install_configuration_file_correct(
			self,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.tcc_install_create_install_configuration_file()
		mocked_exit.assert_not_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.open', side_effect=OSError())
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	def test_tcc_install_create_install_configuration_file_process_error_correct(
			self,
			mocked_exit,
			mocked_print,
			mocked_open,
			mocked_log):
		tcc_setup.tcc_install_create_install_configuration_file()
		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch.object(tcc_setup.tarfile, 'open')
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	def test_open_tarfile_file_correct(
			self,
			mocked_exit,
			mocked_print,
			mocked_tarfile,
			mocked_log):
		tcc_setup.open_tarfile("file")
		mocked_exit.assert_not_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch.object(tcc_setup.tarfile, 'open', side_effect=OSError())
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	def test_open_tarfile_file_process_error_correct(
			self,
			mocked_exit,
			mocked_print,
			mocked_tarfile,
			mocked_log):
		tcc_setup.open_tarfile("file")
		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.path.exists', return_value=False)
	@mock.patch('os.path.isfile', return_value=True)
	@mock.patch('os.remove')
	@mock.patch('tcc_setup.open_tarfile')
	@mock.patch('tcc_setup.tcc_install_create_install_configuration_file')
	@mock.patch('tcc_setup.create_passlist', return_value=["usr/bin/file", "usr/lib64", "usr/share/file"])
	@mock.patch('tcc_setup.tcc_check_install', return_value=True)
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('tcc_setup.tcc_install_directory')
	@mock.patch('tcc_setup.add_install_item')
	@mock.patch('tcc_setup.apply_permissions')
	@mock.patch('tcc_setup.symlink_correction')
	def test_tcc_install_correct(
			self,
			mocked_symlink_correction,
			mocked_apply_premissions,
			mocked_add_install_item,
			mocked_tcc_install_directory,
			mocked_tcc_install_update_prefix,
			mocked_tcc_check_install,
			mocked_create_passlist,
			mocked_tcc_install_create_install_configuration_file,
			mocked_open_tarfile,
			mocked_os_remove,
			mocked_os_path_isfile,
			mocked_os_path_exists,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.shutil = mock.Mock()
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		items = [file, dir, symlink]
		ifile = mock.Mock()
		ifile.getmembers.return_value = items
		mocked_open_tarfile.return_value = ifile

		tcc_setup.tcc_install("file")

		mocked_exit.assert_not_called()

	@mock.patch('builtins.input', side_effect=["No"])
	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.path.exists', return_value=True)
	@mock.patch('os.path.isfile', return_value=True)
	@mock.patch('os.remove')
	@mock.patch('tcc_setup.open_tarfile')
	@mock.patch('tcc_setup.tcc_install_create_install_configuration_file')
	@mock.patch('tcc_setup.create_passlist', return_value=["usr/bin/file", "usr/lib64", "usr/share/file"])
	@mock.patch('tcc_setup.tcc_check_install', return_value=True)
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('tcc_setup.tcc_install_directory')
	@mock.patch('tcc_setup.add_install_item')
	@mock.patch('tcc_setup.apply_permissions')
	@mock.patch('tcc_setup.symlink_correction')
	def test_tcc_install_process_exist_file_correct(
			self,
			mocked_symlink_correction,
			mocked_apply_premissions,
			mocked_add_install_item,
			mocked_tcc_install_directory,
			mocked_tcc_install_update_prefix,
			mocked_tcc_check_install,
			mocked_create_passlist,
			mocked_tcc_install_create_install_configuration_file,
			mocked_open_tarfile,
			mocked_os_remove,
			mocked_os_path_isfile,
			mocked_os_path_exists,
			mocked_exit,
			mocked_print,
			mocked_log,
			mocked_input):
		tcc_setup.shutil = mock.Mock()
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		items = [file, dir, symlink]
		ifile = mock.Mock()
		ifile.getmembers.return_value = items
		mocked_open_tarfile.return_value = ifile

		tcc_setup.tcc_install("file")

		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.path.exists', return_value=False)
	@mock.patch('os.path.isfile', return_value=False)
	@mock.patch('os.remove')
	@mock.patch('tcc_setup.open_tarfile')
	@mock.patch('tcc_setup.tcc_install_create_install_configuration_file')
	@mock.patch('tcc_setup.create_passlist', return_value=["usr/bin/file", "usr/lib64", "usr/share/file"])
	@mock.patch('tcc_setup.tcc_check_install', return_value=True)
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('tcc_setup.tcc_install_directory')
	@mock.patch('tcc_setup.add_install_item')
	@mock.patch('tcc_setup.apply_permissions')
	@mock.patch('tcc_setup.symlink_correction')
	def test_tcc_install_process_archive_not_exist_correct(
			self,
			mocked_symlink_correction,
			mocked_apply_premissions,
			mocked_add_install_item,
			mocked_tcc_install_directory,
			mocked_tcc_install_update_prefix,
			mocked_tcc_check_install,
			mocked_create_passlist,
			mocked_tcc_install_create_install_configuration_file,
			mocked_open_tarfile,
			mocked_os_remove,
			mocked_os_path_isfile,
			mocked_os_path_exists,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.shutil = mock.Mock()
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		items = [file, dir, symlink]
		ifile = mock.Mock()
		ifile.getmembers.return_value = items
		mocked_open_tarfile.return_value = ifile

		tcc_setup.tcc_install("file")

		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.path.exists', return_value=False)
	@mock.patch('os.path.isfile', return_value=True)
	@mock.patch('os.remove')
	@mock.patch('tcc_setup.open_tarfile')
	@mock.patch('tcc_setup.tcc_install_create_install_configuration_file')
	@mock.patch('tcc_setup.create_passlist', return_value=["usr/bin/file", "usr/lib64", "usr/share/file"])
	@mock.patch('tcc_setup.tcc_check_install', return_value=False)
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('tcc_setup.tcc_install_directory')
	@mock.patch('tcc_setup.add_install_item')
	@mock.patch('tcc_setup.apply_permissions')
	@mock.patch('tcc_setup.symlink_correction')
	def test_tcc_install_process_install_failed_correct(
			self,
			mocked_symlink_correction,
			mocked_apply_premissions,
			mocked_add_install_item,
			mocked_tcc_install_directory,
			mocked_tcc_install_update_prefix,
			mocked_tcc_check_install,
			mocked_create_passlist,
			mocked_tcc_install_create_install_configuration_file,
			mocked_open_tarfile,
			mocked_os_remove,
			mocked_os_path_isfile,
			mocked_os_path_exists,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.shutil = mock.Mock()
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		items = [file, dir, symlink]
		ifile = mock.Mock()
		ifile.getmembers.return_value = items
		mocked_open_tarfile.return_value = ifile

		tcc_setup.tcc_install("file")

		mocked_exit.assert_called()

	@mock.patch('tcc_setup.log_line')
	@mock.patch('builtins.print')
	@mock.patch('builtins.exit')
	@mock.patch('os.path.exists', return_value=False)
	@mock.patch('os.path.isfile', return_value=True)
	@mock.patch('os.remove', side_effect=OSError())
	@mock.patch('tcc_setup.open_tarfile')
	@mock.patch('tcc_setup.tcc_install_create_install_configuration_file')
	@mock.patch('tcc_setup.create_passlist', return_value=["usr/bin/file", "usr/lib64", "usr/share/file"])
	@mock.patch('tcc_setup.tcc_check_install', return_value=True)
	@mock.patch('tcc_setup.tcc_install_update_prefix', side_effect=(lambda arg: arg))
	@mock.patch('tcc_setup.tcc_install_directory')
	@mock.patch('tcc_setup.add_install_item')
	@mock.patch('tcc_setup.apply_permissions')
	@mock.patch('tcc_setup.symlink_correction')
	def test_tcc_install_process_cant_remove_config_correct(
			self,
			mocked_symlink_correction,
			mocked_apply_premissions,
			mocked_add_install_item,
			mocked_tcc_install_directory,
			mocked_tcc_install_update_prefix,
			mocked_tcc_check_install,
			mocked_create_passlist,
			mocked_tcc_install_create_install_configuration_file,
			mocked_open_tarfile,
			mocked_os_remove,
			mocked_os_path_isfile,
			mocked_os_path_exists,
			mocked_exit,
			mocked_print,
			mocked_log):
		tcc_setup.shutil = mock.Mock()
		file = mock.Mock()
		file.name = "usr/bin/file"
		file.isdir.return_value = False
		file.isfile.return_value = True
		file.issym.return_value = False
		dir = mock.Mock()
		dir.name = "usr/lib64"
		dir.isdir.return_value = True
		dir.isfile.return_value = False
		dir.issym.return_value = False
		symlink = mock.Mock()
		symlink.name = "usr/share/file"
		symlink.isdir.return_value = False
		symlink.isfile.return_value = False
		symlink.issym.return_value = True
		items = [file, dir, symlink]
		ifile = mock.Mock()
		ifile.getmembers.return_value = items
		mocked_open_tarfile.return_value = ifile

		tcc_setup.tcc_install("file")

		mocked_exit.assert_not_called()

if __name__ == "__main__":
	unittest.main()
