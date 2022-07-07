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

###############################################################################
# Helper functions
###############################################################################

function logging()
{
    echo "$(date --iso-8601=seconds) $*"
}

exit_with_error_message () {
    message=$1
    echo "ERROR: $message"
    exit -1
}

check_return_code () {
    ret=$?
    message=$1
    if [[ $ret != 0 ]]; then
        echo "ERROR: $message"
        exit $ret
    fi
}

logging "Testing started"

###############################################################################
# Setup enviroment
###############################################################################

logging "Setup enviroment"

if [ -z ${DASHBOARD_INSTALL_DIR} ]; then
    exit_with_error_message "DASHBOARD_INSTALL_DIR variable should be set"
fi

TCC_TOOLS_INSTALL_DIR=${DASHBOARD_INSTALL_DIR}/tcc_tools/latest

if [ ! -d $TCC_TOOLS_INSTALL_DIR ]; then
    exit_with_error_message "TCC Tools directroy doen't exist in path ${TCC_TOOLS_INSTALL_DIR}"
fi

TEST_PACKAGE_DIR=$(realpath $(dirname ${BASH_SOURCE[0]}))

export PATH=${PATH}:${TEST_PACKAGE_DIR}/tests/robotframework/src/bin
export PYTHONPATH=${TEST_PACKAGE_DIR}/tests/robotframework/src:${PYTHONPATH+:${PYTHONPATH}}

###############################################################################
# Source env file
###############################################################################
source ${TCC_TOOLS_INSTALL_DIR}/env/vars.sh
check_return_code "env file source failed"

###############################################################################
# Install host dependencies
###############################################################################
logging "Install host dependencies"
bash ${TCC_TOOLS_INSTALL_DIR}/scripts/install_host_dependencies.sh
check_return_code "dependencies installation failed"

###############################################################################
# Run robot tests
###############################################################################
logging "Run robot tests"
status=0

function run_robot_test() {
    suite_path="$1"
    suite_name="$(basename $suite_path)"
    suite_name=${suite_name%.*}
    robot \
        --argumentfile ${TEST_PACKAGE_DIR}/tests/argument_files/host/argumentfile_pv.txt \
        --outputdir "logs/${suite_name}" \
        --loglevel TRACE \
        --runemptysuite \
        -l ${suite_name}_log.html \
        -r ${suite_name}_report.html \
        -o ${suite_name}_output.xml \
        -x ${suite_name}_xoutput.xml \
        ${TEST_PACKAGE_DIR}/${suite_path}
}
ROBOT_TEST_LIST="$(cat ${TEST_PACKAGE_DIR}/tests/tests_lists/host/tests_list_robot_pv.txt)"

for test_name in $ROBOT_TEST_LIST; do
    run_robot_test ${test_name}
    status=$((status || $?))
done

logging "All test cases executed"

###############################################################################
# Check result
###############################################################################
if [ '0' != $status ]; then
    exit_with_error_message "tests FAILED"
fi

logging "Testing SUCCESS"
