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

# Script to enable/disable SlimVMM (RTCM) on system with SBL BIOS

check_return_code () {
	ret=$?
	message=$1
	if [[ $ret != 0 ]]; then
		echo "$message"
		exit $ret
	fi
}

function usage() {
	echo "control_rtcm_sbl.sh - Enable/disable SlimVMM (RTCM) on system with SBL BIOS"
	echo "Usage:"
	echo "control_rtcm_sbl.sh enable|disable|status"
	echo "    enable    Enable RTCM boot option"
	echo "    disable   Disable RTCM boot option"
	echo "    status    Show current RTCM status"
	exit 1
}

function enable_rtcm() {
	if [ ! -f "/boot/sbl_rtcm" ]; then
		mv /boot/sbl_rtcm_disabled /boot/sbl_rtcm
		check_return_code "ERROR: No boot file found"
		echo "Added sbl_rtcm to /boot"
	else
		echo "RTCM is already enabled, do nothing"
	fi
}

function disable_rtcm() {
	if [ -f "/boot/sbl_rtcm" ]; then
		mv /boot/sbl_rtcm /boot/sbl_rtcm_disabled
		check_return_code "ERROR: Failed to move boot file"
		echo "Removed sbl_rtcm from /boot"
	else
		echo "RTCM is already disabled, do nothing"
	fi
}

function get_status() {
	if [ -f "/boot/sbl_rtcm" ]; then
		echo "RTCM is ENABLED"
	else
		echo "RTCM is DISABLED"
	fi
}

if [[ "$#" != 1 ]]; then
	echo "Wrong number of arguments"
	usage
	exit
fi

if [[ "$1" == "enable" ]]; then
	enable_rtcm
elif [[ "$1" == "disable" ]]; then
	disable_rtcm
elif [[ "$1" == "status" ]]; then
	get_status
else
	echo "Unknown option $1"
	usage
fi
