#! /bin/bash
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

set -e

NAME=bash_shell_mock
BASH_SHELL_MOCK_VERSION=1.3

#--------------------------------------------------------------------------------------------
trap 'echo "Error $? for:${BASH_SOURCE[0]}:${BASH_LINENO[0]}:${BAHSH_COMMAND}" 1>&2' ERR
#--------------------------------------------------------------------------------------------

rm -rf "${NAME}"
rm -rf "${NAME}-${BASH_SHELL_MOCK_VERSION}"
rm -rf "${NAME}-${BASH_SHELL_MOCK_VERSION}.zip"

wget -O "${NAME}-${BASH_SHELL_MOCK_VERSION}.zip" "https://github.com/capitalone/bash_shell_mock/archive/refs/tags/${BASH_SHELL_MOCK_VERSION}.zip"
unzip "${NAME}-${BASH_SHELL_MOCK_VERSION}.zip"
rm "${NAME}-${BASH_SHELL_MOCK_VERSION}.zip"
mv ${NAME}-${BASH_SHELL_MOCK_VERSION} ${NAME}

cd ${NAME}

bash -$- ./install.sh ..
