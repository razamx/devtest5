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
SCRIPT=`realpath -s $0`
SCRIPTPATH=`dirname $SCRIPT`
green_color="\e[1m\e[32m"
red_color="\e[1m\e[31m"
clear_color="\e[0m"
export code_exit=0

if test -z "$1"; then
    echo "error: SOURCE_PATH zero lenght"
    exit 1
fi
SOURCE_PATH="`realpath -s $1`"

if test -z "$2"; then
    echo "error: BUILD_PATH zero lenght"
    exit 1
fi
BUILD_PATH="`realpath -s $2`"

if test -z "$3"; then
    echo "error: TEST_PATH zero lenght"
    exit 1
fi
TEST_PATH="`realpath -s $3`"

mkdir -p $TEST_PATH
print_status(){
    if test $1 = 0; then
        echo $green_color"[SUCCESS]    "$clear_color
    else
        printf $red_color"[FAILURE] % 3s"$clear_color $1
    fi
}

run_test()
{
    test_name=$1
    log_level=$2
    command=$(echo "TCC_LOG_LEVEL=${log_level} ${TCC_RUN_PREFIX} ${test_name}" | sed 's/ +/ /g') #create commnad and clear duplicate spaces
    printf "$green_color[RUN    ]     $clear_color$command\n" | tee -a $TEST_PATH/tests_result.txt
    set +e
    eval ${command} &>> $TEST_PATH/$(basename ${test_name}.${log_level}.result)
    rc=$?
    if test ${rc} != 0; then
    code_exit=${rc}
    fi
    set -e
    printf "$(print_status ${rc}) ${command}\n" | tee -a $TEST_PATH/tests_result.txt

    # workaround for c/c++ coverage
    find -name '*.gcda' | while read f; do chmod +rw $f; done
}

echo "[COMMAND] $0 $*"

# remove old tests results
rm -rf $TEST_PATH/tests_result.txt
rm -rf $TEST_PATH/*.result

echo "[       ] RUN C/C++ tests:"
echo "[       ] ================"


while read test_name; do
    run_test $test_name NONE
    run_test $test_name TRACE
done < <(find $TEST_PATH -name 'test_*_host' | sort)

echo "[       ] RUN Python tests:"
echo "[       ] ================="

# we set omit rules in coverage cfg relate to source root.
# and we should run python tests from source root to get right omit applying from config
cd $SOURCE_PATH

TCC_PYTHON_RUN_PREFIX="TCC_BUILD_PATH=$BUILD_PATH python3"

#setup run prefix to add coverage command to test
export TCC_RUN_PREFIX="${TCC_RUN_PREFIX} ${TCC_PYTHON_RUN_PREFIX}"
while read test_name; do
    run_test $test_name NONE
    run_test $test_name TRACE
done < <(find $SOURCE_PATH -name 'test_*_host.py' )

exit ${code_exit}
