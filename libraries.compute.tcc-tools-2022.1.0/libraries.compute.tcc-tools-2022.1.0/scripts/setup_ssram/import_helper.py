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

import logging
import os
import sys


def _add_env(varname, path=''):
    """
    Try to add path ${varname}/path to the PYTHONPATH
    """
    prefix = ''
    logging.info('Adding ${%s}/%s to the PYTHONPATH...', varname, path)
    value = os.environ.get(varname)
    if value is None:
        logging.debug('Environment variable %s does not exist, ignoring', varname)
        return
    prefix = value

    full_path = os.path.abspath(os.path.join(prefix, path))
    logging.debug('Full path: %s')
    if not os.path.isdir(full_path):
        logging.debug('Path %s does not exists, ignoring', full_path)
        return

    sys.path.append(full_path)


def _add_path(path):
    logging.debug('Adding %s to the PYTHONPATH...')
    if not os.path.isdir(path):
        logging.debug('Path %s does not exists, ignoring', path)
        return
    sys.path.append(path)


# Workaround to import TCC common python modules from wrong directories.
# Tries to add several possible locations to the PYTHONPATH
_add_env('TCC_TOOLS_PATH')
_add_path('/usr/share/tcc_tools/tools')
