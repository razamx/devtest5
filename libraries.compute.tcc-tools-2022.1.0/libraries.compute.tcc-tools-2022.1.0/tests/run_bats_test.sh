#!/bin/bash
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

trap 'echo "error: TCC_LOG_LEVEL not set"' ERR
[ -n "${TCC_LOG_LEVEL}" ]

trap 'echo "error: Require single parametr with test name"' ERR
echo $1
[ $# == 1 ]
TEST="$1"

trap 'echo "error: File with test \"$TEST\" does not exist"' ERR
[ -e "${TEST}" ]

export PATH="$PATH:$PWD/bin" # bats executable

output_dir="bats_test_results"
trap 'echo "error: Cant create folder \"${output_dir}\""' ERR
mkdir -p "${output_dir}"

trap 'echo "${BASH_SOURCE[0]}:${BASH_LINENO[0]}:Error happened!" 1>&2' ERR

test_name="$(basename "${TEST/.bats/}")_${TCC_LOG_LEVEL}"

output_file="${output_dir}/${test_name}.txt"

bats_params=( "--tap" )
if [ -n "${CI}" ] ; then
    output_file="${output_dir}/${test_name}.xml"
    bats_params=( "--formatter" "junit" )
fi

bats_params+=( "$TEST" )

echo "${test_name} RUN"

trap 'echo "${test_name} FAILED"' ERR

export TCC_LOG_TO_FILE="${output_dir}/${test_name}.log"
bats "${bats_params[@]}" > "${output_file}"

echo "${test_name} SUCCESS"
