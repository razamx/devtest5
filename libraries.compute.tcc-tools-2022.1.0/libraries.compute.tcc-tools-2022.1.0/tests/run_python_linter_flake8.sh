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

# set -x

TCC_ROOT="$(realpath -s "$(dirname "${BASH_SOURCE[0]}")"/..)"

cd ${TCC_ROOT}/tests

include=(
    # */target/*
    */scripts/setup_ssram/*
    */scripts/setup_ssram_v1/*
    */samples/tcc_rt_communication_demo/src/scripts/*
    */tools/*
)

exclude=(
    */build/*
    */tests/*
)

INCLUDES="-type f ( "
for include_folder in ${include[@]}; do
    INCLUDES+="-path ${include_folder} -o "
done
INCLUDES="${INCLUDES%????} ) "

EXCLUDES=""
for exclude_folder in ${exclude[@]}; do
    EXCLUDES+="-not -path ${exclude_folder} "
done
EXCLUDES="${EXCLUDES%?}"

SOURCE_LIST=$(find ${TCC_ROOT} -name "*.py" ${EXCLUDES} ${INCLUDES} | sort)

# echo $SOURCE_LIST

MUTEX="./mutex.lock"

atomic_task() {
    (
        flock -x 200  # Locks file descriptor
        ($1 "${@: 2}")  # Call func from 1st param with arguments from params from 2nd to last
    ) 200>${MUTEX}  # Unlocks file descriptor
}


exit_code=0

print_result() {
    if test $3 -ne 0; then
        echo -e "Found errors in file $1\n$2\n"
        exit_code=1
    else
        echo -e "No errors in file $1\n\n"
    fi
}

check_file() {
    out=$(python3 -m flake8 $1 --config ${TCC_ROOT}/.flake8 -j $(nproc))
    atomic_task print_result $1 "$out" $?
}


while read file; do
    while [ "$(jobs|wc -l)" -gt "$(($(nproc) * 2))" ]; do
        wait -n
    done
    check_file ${file} &
done <<< "${SOURCE_LIST}"

wait
rm ${MUTEX}

exit ${exit_code}
