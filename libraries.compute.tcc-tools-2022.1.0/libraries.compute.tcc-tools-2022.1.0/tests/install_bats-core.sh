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

NAME=bats-core

trap 'echo "Error $? for:${BASH_SOURCE[0]}:${BASH_LINENO[0]}:${BAHSH_COMMAND}" 1>&2' ERR

BATS_CORE_VERIOSN=1.4.1

rm -rf ${NAME}
rm -rf ${NAME}-${BATS_CORE_VERIOSN}
rm -f ${NAME}-${BATS_CORE_VERIOSN}.zip

wget -O ${NAME}-${BATS_CORE_VERIOSN}.zip https://github.com/bats-core/bats-core/archive/refs/tags/v${BATS_CORE_VERIOSN}.zip

unzip ${NAME}-${BATS_CORE_VERIOSN}.zip
mv ${NAME}-${BATS_CORE_VERIOSN} ${NAME}
rm ${NAME}-${BATS_CORE_VERIOSN}.zip

bash -$- ./${NAME}/install.sh .
