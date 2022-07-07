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
#set -x

PATH_TO_THIS_FILE="$(dirname "${BASH_SOURCE[0]}")"
INSTALL_ROBOT_TESTS_LIST_SCRIPT="${PATH_TO_THIS_FILE}/install_robot_tests_lists.sh"

# INSTALL ROBOTFRAMEWORK
bash $PATH_TO_THIS_FILE/install_robotframework.sh
${INSTALL_ROBOT_TESTS_LIST_SCRIPT} $1 $2 $3

set +x
set +e
