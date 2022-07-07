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
import logging

import unittest
import unittest.mock as mock

# NOTE: workaround to import modules
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../'))
import import_helper

FAKE_VARNAME = "FAKE_VAR_NAME"
FAKE_PATH = "PATH"
FAKE_PATH_PREFIX = "FAKE_PATH_PREFIX"
FAKE_FULL_PATH = os.path.abspath(os.path.join(FAKE_PATH_PREFIX, FAKE_PATH))

class TestImportHelper(unittest.TestCase):

    @mock.patch('import_helper.os.environ.get', return_value = None)
    def test_add_env_not_existed_variable_negative(self, env):
        with self.assertLogs(level=logging.DEBUG) as logs:
            import_helper._add_env(FAKE_VARNAME)
        self.assertEqual(len(logs.records), 2)
        self.assertEqual(logs.records[1].getMessage(), f'Environment variable {FAKE_VARNAME} does not exist, ignoring')

    @mock.patch('import_helper.os.environ.get', return_value = FAKE_PATH_PREFIX)
    @mock.patch('import_helper.os.path.isdir', return_value = False)
    def test_add_env_not_existed_path_negative(self, env, path):
        with self.assertLogs(level=logging.DEBUG) as logs:
            import_helper._add_env(FAKE_VARNAME, path=FAKE_PATH)
        self.assertEqual(len(logs.records), 3)
        self.assertEqual(logs.records[2].getMessage(), f'Path {FAKE_FULL_PATH} does not exists, ignoring')


if __name__ == '__main__':
    unittest.main()
