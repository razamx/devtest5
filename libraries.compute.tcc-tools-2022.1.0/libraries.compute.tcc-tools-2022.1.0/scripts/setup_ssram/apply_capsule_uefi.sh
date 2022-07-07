#!/bin/bash
# /*******************************************************************************
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

check_return_code () {
	ret=$?
	message=$1
	if [[ $ret != 0 ]]; then
		echo "$message"
		exit $ret
	fi
}

CACHE_GUID="9A069A23-836F-41DF-AF02-F14BEC97DE72"

function apply_capsules() {
	capsules_dir=$1
	fwupdate --apply $CACHE_GUID "$capsules_dir/cache_capsule.out.bin"
	check_return_code "ERROR: fwupdate exits with non-zero return code"
}

apply_capsules $1
