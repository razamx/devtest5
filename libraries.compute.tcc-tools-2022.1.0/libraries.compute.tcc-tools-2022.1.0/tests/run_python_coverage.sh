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
usage() { echo "Usage: $0 -s <SOURCE_PATH> -b <BUILD_DIR> [-f MIN_COVERAGE_FOR_FILE] [-t <TOTAL_COVERAGE>]" 1>&2; exit 1; }

TCC_ROOT=UNDEFINED
BUILD_DIR=UNDEFINED
FILE_COVERAGE=0
TOTAL_COVERAGE=0

while getopts ":s:b:f:t:" opt; do
    case "${opt}" in
        s)
            TCC_ROOT=${OPTARG}
            ;;
        b)
            BUILD_DIR=${OPTARG}
            ;;
        f)
            FILE_COVERAGE=${OPTARG}
            ;;
        t)
            TOTAL_COVERAGE=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ ${TCC_ROOT} = "UNDEFINED" ] || [ ${BUILD_DIR} = "UNDEFINED" ]; then
    usage
fi

COVERAGE_DIR=${BUILD_DIR}/coverage/python

PYTHON_TESTS_FILES=(
    ${TCC_ROOT}/scripts/setup_ssram/tests
    ${TCC_ROOT}/target/tests
    ${TCC_ROOT}/tools
    ${TCC_ROOT}/tools/demo/workloads/src/*
)

# Prepare
rm -rf ${COVERAGE_DIR}
mkdir -p ${COVERAGE_DIR}
report_file=coverage.txt
cd ${COVERAGE_DIR}
cp ${TCC_ROOT}/tests/.coveragerc .

# Run Python Code Coverage
for test_folder in "${PYTHON_TESTS_FILES[@]}"; do
    coverage3 run -a -m unittest discover ${test_folder}
done

# Create reports
coverage3 report --rcfile=.coveragerc > "${report_file}" 
coverage3 html -d ${COVERAGE_DIR}/coverage_html_report/

# Display text file to Build log
cat "${report_file}"

# Parse the report file to check the coverage of each separate and all files.

coverage_column=$(wc -w <<< "$(awk 'NR==3' "${report_file}")")
file_column=1
total_keyword=TOTAL
process_started=false

while read line; do
    if [[ "${line}" =~ ^-+$ ]]; then
        if [[ "${process_started}" = true ]]; then
            process_started=finished
            continue
        fi
        process_started=true
    fi
    if [[ "${process_started}" = false ]]; then
        continue
    fi
    #coverage_column=$(wc -w <<< "${line}")
    coverage=$(echo "${line}" | awk -v cov_col=${coverage_column} '{ print $cov_col }')
    file=$(echo "${line}" | awk -v file_col=${file_column} '{ print $file_col }')
    if [[ "${file}" =~ .*(cache_configurator/partitions|iview|_ioctl).* ]]; then
        continue
    fi
    if [ -z "${coverage}" ]; then
        continue
    fi
    coverage=${coverage::-1}
    # Specific case for total value
    if [[ "${process_started}" = finished ]]; then
        if [[ "${file}" != "${total_keyword}" ]]; then
            continue
        fi
        if [ "${coverage}" -lt "${TOTAL_COVERAGE}" ]; then
            echo "TOTAL coverage=${coverage}% results less than target limit=${TOTAL_COVERAGE}%"
        fi
        break
    fi
    if [ "${coverage}" -lt "${FILE_COVERAGE}" ]; then
       echo "File "$(basename -- ${file})" has coverage=${coverage}% less than target limit=${FILE_COVERAGE}%"
    fi
done < ${report_file}
