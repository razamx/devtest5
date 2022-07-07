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

# Script to enable tcc_buffer driver

DRIVER="tcc_buffer"
DEVNODE="/dev/tcc"
FILE=/etc/modules-load.d/$DRIVER.conf

function usage() {
	echo "control_tcc_driver.sh - Enable/disable $DRIVER driver"
	echo "Usage:"
	echo "control_tcc_driver.sh enable|disable|status"
	echo "    enable    Load driver and add it to auto-load"
	echo "    disable   Unload driver and remove it from auto-load"
	echo "    status    Show current $DRIVER driver status"
}

function enable_driver() {
	modprobe $DRIVER
	if [[ $? != 0 ]]; then
		echo "ERROR: Failed to load $DRIVER driver"
		exit 1
	fi

	echo "$DRIVER driver is loaded"

	if [[ ! -f  "$FILE" ]]; then
		depmod -a
		echo "$DRIVER" > "$FILE"
		echo "$DRIVER driver is added to auto-load"
	fi
}

function disable_driver() {
	rmmod $DRIVER &> /dev/null
	if [[ $? != 0 ]]; then
		echo "$DRIVER is not loaded"
	else
		echo "$DRIVER driver is unloaded"
	fi

	if [[ -f  "$FILE" ]]; then
		depmod -a
		rm "$FILE"
		echo "$DRIVER driver is removed from auto-load"
	fi
}

function get_status() {
	if ls "$DEVNODE" &> /dev/null ; then
		echo "[ENABLED ] Driver"
		loaded=1
	else
		echo "[DISABLED] Driver"
		loaded=0
	fi

	if [[ -f  "$FILE" ]]; then
		echo "[ENABLED ] Driver auto-load"
		autoload=1
	else
		echo "[DISABLED] Driver auto-load"
		autoload=0
	fi

	if [[ $loaded == 1 && $autoload == 1 ]]; then
		exit 0
	elif [[ $loaded == 0 && $autoload == 0 ]]; then
		exit 2
	else
		exit 1
	fi
}

if [[ "$#" != 1 ]]; then
	echo "Wron number of arguments"
	usage
	exit
fi

if [[ "$1" == "enable" ]]; then
	enable_driver
elif [[ "$1" == "disable" ]]; then
	disable_driver
elif [[ "$1" == "status" ]]; then
	get_status
else
	echo "Unknown option $1"
	usage
fi
