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

echo "RUN: $0 $*"

STATUS=0
FF_MASK=255
ERROR_LOW_LINE_COV_MSK=2 # 1<<1
ERROR_LOW_BRANCH_COV_MSK=4 # 1<<2
ERROR_PYTHON_LOW_TOTAL_COVERAGE_MSK=8 # 1<<3
ERROR_PYTHON_LOW_LINE_COVERAGE_MSK=16 # 1<<4

CPP_MIN_LINE_COVERAGE=80
CPP_MIN_BRANCH_COVERAGE=75
PYTHON_MIN_TOTAL_COVERAGE=84
PYTHON_MIN_LINE_COVERAGE=70

SCRIPT=$(realpath -s $0)
SCRIPTPATH=$(dirname $SCRIPT)

EXCLUDED_FILES=(
    */.local/*
    /usr/*
    */test_*
    */googletest_github/*
    */mock/*
    */tests/*
    */utest/*
)

# collect_coiverage <build_path> <source_path>
#
# on success it return 0
#
# on coverage levels error it return mask: 1<<1 for less total line coverage for c/c++
#                                          1<<2 for less total branch coverage for c/c++
#                                          1<<3 for less total coverage for python
#                                          1<<4 for less line coverage for python

if test -z "$1"; then
	echo "error: BUILD_PATH zero lenght"
	exit 1
fi
BUILD_PATH="`realpath -s $1`"

if test -z "$2"; then
	echo "error: SOURCE_PATH zero lenght"
	exit 1
fi
SOURCE_PATH="`realpath -s $2`"

COVERAGE_PATH=${BUILD_PATH}/coverage

echo "C/C++ coverage results:"
echo "======================="

CPP_RESULTS="$(${SOURCE_PATH}/tests/run_cpp_coverage.sh -s ${SOURCE_PATH} -b ${BUILD_PATH} -l ${CPP_MIN_LINE_COVERAGE} -t ${CPP_MIN_BRANCH_COVERAGE})"
echo "${CPP_RESULTS}"

if test ! -z "$(echo "${CPP_RESULTS}" | grep "Some file have total line coverage less")"; then
	STATUS=$((${STATUS} + ${ERROR_LOW_LINE_COV_MSK}))
fi
if test ! -z "$(echo "${CPP_RESULTS}" | grep "Some file have total branch coverage less")"; then
	STATUS=$((${STATUS} + ${ERROR_LOW_BRANCH_COV_MSK}))
fi

echo "Python coverage results:"
echo "========================"

PYTHON_RESULTS="$(${SOURCE_PATH}/tests/run_python_coverage.sh -s ${SOURCE_PATH} -b ${BUILD_PATH} -f ${PYTHON_MIN_LINE_COVERAGE} -t ${PYTHON_MIN_TOTAL_COVERAGE})"
echo "${PYTHON_RESULTS}"

if test ! -z $(echo "${PYTHON_RESULTS}" | grep "TOTAL coverage=\d+% results less"); then
	STATUS=$((${STATUS} + ${ERROR_PYTHON_LOW_TOTAL_COVERAGE_MSK}))
fi
if test ! -z $(echo "${PYTHON_RESULTS}" | grep "File [a-zA-Z_/-]* has coverage=\d+% less"); then
	STATUS=$((${STATUS} + ${ERROR_PYTHON_LOW_LINE_COVERAGE_MSK}))
fi

#FIND_UNCOVERED FILES
CPP_COVERAGE_REPORT=${BUILD_PATH}/coverage/cpp/index.html
PY_COVERAGE_REPORT=${BUILD_PATH}/coverage/python/coverage_html_report/index.html

COVERED_CPP_FILES="$(cat ${CPP_COVERAGE_REPORT} | grep 'td class="coverFile"' | sed -r 's/.*a href=[^>]*//;s/<\/a>.*//;s/>//'| sed 's/^/.\//')"
COVERED_PY_FILES="$(cat ${PY_COVERAGE_REPORT} | grep 'td class="name left"' | sed -r 's/.*a href=[^>]*//;s/<\/a>.*//;s/>//'| grep '\.py$'| sed 's/^/.\//')"
COVERED_FILES="${COVERED_CPP_FILES} ${COVERED_PY_FILES}"
COVERED_FILES_REGEXP="$(echo ${COVERED_FILES} | xargs echo -n | sed -r 's/^/((/;s/$/))/;s/ +/)|(/g;s/\./\\./g')"
EXCLUDE_REGEXPS="$(echo "${EXCLUDED_FILES}" | sed 's/\./\\./g;s/*/.*/g;')"
FILES_REQURE_COVERAGE="$(cd $SOURCE_PATH; for ext in c py; do find -name "*.$ext"; done | grep -Ev "${EXCLUDE_REGEXPS}")"
NON_COVERED_FILES="$(echo "${FILES_REQURE_COVERAGE}" | grep -Ev "${COVERED_FILES_REGEXP}"| sort)"

if test ! -z "${NON_COVERED_FILES}"; then
	echo "UNCOVERED FILES FOUND"
	echo  "${NON_COVERED_FILES}"
fi

echo
echo "C/C++ coverage report: ${CPP_COVERAGE_REPORT}"
echo "Python coverage report: ${PY_COVERAGE_REPORT}"

if test  $((${ERROR_LOW_LINE_COV_MSK} & ${STATUS})) != 0; then
	echo "error:C/C++ total line coverage to less"
fi
if test  $((${ERROR_LOW_BRANCH_COV_MSK} & ${STATUS})) != 0; then
	echo "error:C/C++ total branch coverage to less"
fi
if test  $((${ERROR_PYTHON_LOW_TOTAL_COVERAGE_MSK} & ${STATUS})) != 0; then
	echo "error:python total coverage to less"
fi
if test  $((${ERROR_PYTHON_LOW_LINE_COVERAGE_MSK} & ${STATUS})) != 0; then
	echo "error:python line coverage to less for some file"
fi

echo "coverage status:${STATUS}"
exit ${STATUS}
