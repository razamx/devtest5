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
usage() { echo "Usage: $0 -s <SOURCE_PATH> -b <BUILD_DIR> [-l MIN_TOTAL_LINE_COVERAGE] [-t MIN_TOTAL_BRANCH_COVERAGE]" 1>&2; exit 1; }

TCC_ROOT=UNDEFINED
BUILD_DIR=UNDEFINED
LINE_FILE_COVERAGE=0
BRANCH_FILE_COVERAGE=0

while getopts ":s:b:l:t:e" opt; do
    case "${opt}" in
        s)
            TCC_ROOT=${OPTARG}
            ;;
        b)
            BUILD_DIR=${OPTARG}
            ;;
        l)
            LINE_FILE_COVERAGE=${OPTARG}
            ;;
        t)
            BRANCH_FILE_COVERAGE=${OPTARG}
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

COVERAGE_DIR=${BUILD_DIR}/coverage/cpp

GCOVR_TOOL="python3 -m gcovr"

# Prepare
SCRIPT=$(realpath -s $0)
SCRIPTPATH=$(dirname ${SCRIPT})

rm -rf ${COVERAGE_DIR}
mkdir -p ${COVERAGE_DIR}
cd ${COVERAGE_DIR}

# Clean old coverage data
find -name '*.gcda' | xargs rm -f

# Run CPP code coverage
EXCLUDE_FILES=(
    */googletest_github/*
    */mock/*
    */tests/*
    */utest/*
)

EXCLUDE_GCOVR_FLAGS=""
for EXCLUDE_FILE in ${EXCLUDE_FILES[@]}; do
    EXCLUDE_REGEXPS="$(echo "${EXCLUDE_FILE}" | sed 's/\./\\./g;s/*/.*/g;')"
    EXCLUDE_GCOVR_FLAGS+="$(echo "${EXCLUDE_REGEXPS}" | sed -r "s/.*/\-\-exclude '&'/" | xargs echo -n)"
    EXCLUDE_GCOVR_FLAGS+=" "
done

GCOVR_FLAGS="\
--root ${BUILD_DIR}/.. \
--branches \
--xml index.xml \
--xml-pretty \
--html-details index.html \
--object-directory ${BUILD_DIR} \
--html-medium-threshold 60 \
--html-high-threshold 80 \
--print-summary \
--fail-under-line ${LINE_FILE_COVERAGE} \
--fail-under-branch ${BRANCH_FILE_COVERAGE}
${EXCLUDE_GCOVR_FLAGS} \
"

$GCOVR_TOOL ${GCOVR_FLAGS}
status=$?

if [ ${status} -eq 2 ]; then
    echo "Total line coverage less than ${LINE_FILE_COVERAGE}"
    echo "Check ${COVERAGE_DIR}/index.html for details"
fi
if [ ${status} -eq 4 ]; then
    echo "Total branch coverage less than ${BRANCH_FILE_COVERAGE}"
    echo "Check ${COVERAGE_DIR}/index.html for details"
fi
if [ ${status} -eq 6 ]; then
    echo "Total line coverage less than ${LINE_FILE_COVERAGE}"
    echo "Total branch coverage less than ${BRANCH_FILE_COVERAGE}"
    echo "Check ${COVERAGE_DIR}/index.html for details"
fi
