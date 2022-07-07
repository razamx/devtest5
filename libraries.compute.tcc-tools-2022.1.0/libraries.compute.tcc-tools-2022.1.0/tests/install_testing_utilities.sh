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
trap 'echo "${BASH_SOURCE[0]}:${BASH_LINENO[0]}:Error happened!" 1>&2' ERR

PATH_TO_THIS_FILE="$(dirname "${BASH_SOURCE[0]}")"
INSTALL_ROBOT_TESTS_LIST_SCRIPT="${PATH_TO_THIS_FILE}/install_robot_tests_lists.sh"


tools=(
    robotframework
)

pids=()

for tool in "${tools[@]}"; do
    bash -$- "$PATH_TO_THIS_FILE/install_${tool}.sh" &
    pids+=($!)
done

status=0
for i in $(seq 0 $(expr "${#pids[@]}" - 1)); do
    wait "${pids[$i]}"
    if [ 0 != "$?" ]; then
        echo "Failure during install ${tools[$i]}"
        status=1
    fi
done

[ ${status} = 0 ]

${INSTALL_ROBOT_TESTS_LIST_SCRIPT} $1 $2 $3
