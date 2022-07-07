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
# set -x

function transform_src2dst()
{
	#TRANSFORM SORCES PATHS TO DESTINATION PATHS
	if [ -f $1 ]; then
		#transform base names from recived file to regexp "((filename1)|(filename2)|(filename3))"
		#and after that we grep this from dst files and we get only dst files that compatible with recived file
		TESTS="$(basename -a $(cat $1 | xargs echo) 2>/dev/null | xargs echo)"
		if test -z "${TESTS}"; then
			echo -n > $1
		else
			echo "${ROBOT_DST_TESTS}" | grep -E "(($(echo "${TESTS}" | sed -r 's/ +/)|(/g')))" > $1
		fi
		echo "-- Installing: ${PWD}/$1"
	fi
}

# Check that arguments are correct
if [ -z "$1" ] || [ -z "$2" ]; then
	echo "Usage: $0 <ROBOT_TESTS_LIST> <PLATFORM>" 1>&2
	exit 1
fi

# Get path to generate test list script
PATH_TO_THIS_FILE="$(dirname "${BASH_SOURCE[0]}")"
SCRIPT="${PATH_TO_THIS_FILE}/generate_test_list.py"

# Setup variables for host or target platforms
if [ "$2" = "HOST" ]; then
	TEST_TYPES=(HOST)
	WORKFLOWS=(COMPATIBLE DAILY PRE_COMMIT PV)
	DIRRECTORY_FOR_LISTS="tests_lists/host"
else
	TEST_TYPES=(TARGET HOST_TARGET TWO_TARGETS)
	WORKFLOWS=(COMPATIBLE DAILY BKC PRE_COMMIT)
	PLATFORM_NAME=$2
	LOWER_PLATFORM_NAME=${PLATFORM_NAME,,}
	DIRRECTORY_FOR_LISTS="tests_lists/${LOWER_PLATFORM_NAME}"
fi

# Parse robot tests list
ROBOT_TESTS_LIST=$1
# The ROBOT_TESTS_LIST have the format for each string:
#     DST_FILE_PATH_1 SRC_FILE_PATH_1
#     DST_FILE_PATH_2 SRC_FILE_PATH_2
ROBOT_SRC_TESTS="$(cat ${ROBOT_TESTS_LIST} | awk '{print $1}')" # list of sorce robot files
ROBOT_DST_TESTS="$(cat ${ROBOT_TESTS_LIST} | awk '{print $2}')" # list of destination robot files

# Setup robotframework environment
PYTHONPATH=${PWD}/robotframework/src:${PYTHONPATH+:${PYTHONPATH}}
export PYTHONPATH

# Prepare directory with tests lists
rm -rf ${DIRRECTORY_FOR_LISTS}
mkdir -p tests_lists
cd tests_lists

# Create robot tests lists for each target and platforms
echo "${ROBOT_SRC_TESTS}" > tmp_tests_list_robot_src.txt
pids=()
for test_type in "${TEST_TYPES[@]}"; do
	lower_test_type=${test_type,,}
	for workflow in "${WORKFLOWS[@]}"; do
		lower_workflow=${workflow,,}
		if [ "$2" = "HOST" ]; then
			${SCRIPT} ${lower_test_type} --input_file tmp_tests_list_robot_src.txt \
			--workflow ${workflow} \
			--output_file "host/tests_list_robot_${lower_workflow}.txt" &
			pids+=($!)
		else
			${SCRIPT} ${lower_test_type} --input_file tmp_tests_list_robot_src.txt \
			--platform_name ${PLATFORM_NAME} --workflow ${workflow} \
			--output_file "${LOWER_PLATFORM_NAME}/${lower_test_type}/tests_list_robot_${lower_workflow}.txt" &
			pids+=($!)
		fi
	done
done

for pid in "${pids[@]}"; do wait $pid; done

rm tmp_tests_list_robot_src.txt

# Install robot tests lists for each platforms
for test_type in "${TEST_TYPES[@]}"; do
	lower_test_type=${test_type,,}
	for workflow in "${WORKFLOWS[@]}"; do
		lower_workflow=${workflow,,}
		if [ "$2" = "HOST" ]; then
			transform_src2dst "host/tests_list_robot_${lower_workflow}.txt"
		else
			transform_src2dst "${LOWER_PLATFORM_NAME}/${lower_test_type}/tests_list_robot_${lower_workflow}.txt"
		fi
	done
done

wait
set +x
set +e
