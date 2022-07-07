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

# Script to enable/disable SlimVMM (RTCM) on system with UEFI BIOS

BOOT_NAME="RTCM"

check_return_code () {
	ret=$?
	message=$1
	if [[ $ret != 0 ]]; then
		echo "$message"
		exit $ret
	fi
}

function usage() {
	echo "control_rtcm_uefi.sh - Enable/disable SlimVMM (RTCM) on system with UEFI BIOS"
	echo "Usage:"
	echo "control_rtcm_uefi.sh enable|disable|status"
	echo "    enable    Enable RTCM boot option or add a new one"
	echo "    disable   Disable RTCM boot option"
	echo "    status    Show current RTCM status"
	exit 1
}

function find_boot_option() {
	out=$(efibootmgr)
	check_return_code "ERROR: Failed to list boot options"
	bootnum=$(echo "$out" | grep "$1" | sed -E "s/Boot([0-9A-Z]+).*/\1/")
}

function find_boot_partition() {
	# Assume, that boot partition is the first partition
	# sda
	# |-sda1    /boot/efi
	# ...
	# So, just take the line above to get disk name
	line=$(lsblk | grep -n /boot/efi | cut -d: -f1)
	if [[ $? != 0 ]]; then
		echo "ERROR: No boot partition found"
		exit 1
	fi
	line=$((line - 1))
	disk=$(lsblk | sed -n ${line}p  | awk '{print $1}')
	partition=1
}

function add_boot_option() {
	find_boot_partition
	efibootmgr -c -l '\EFI\BOOT\rtcm_boot.efi' -d /dev/$disk -p $partition -L $BOOT_NAME
	check_return_code "ERROR: Failed to add boot option"
}

function delete_boot_option() {
	bootnum=$1
	efibootmgr -B -b $bootnum
	check_return_code "ERROR: Failed to delete boot option"
}

function enable_rtcm() {
	find_boot_option $BOOT_NAME
	if [[ -z $bootnum ]]; then
		echo "No $BOOT_NAME boot entry, adding new"
		add_boot_option
	else
		echo "Found $BOOT_NAME ($bootnum), deleting and adding new"
		delete_boot_option $bootnum
		add_boot_option
	fi

}

function disable_rtcm() {
	find_boot_option $BOOT_NAME
	if [[ -z $bootnum ]]; then
		echo "No $BOOT_NAME boot entry, do nothing"
	else
		echo "Found $BOOT_NAME ($bootnum), deleting"
		delete_boot_option $bootnum
	fi
}

function get_status() {
	find_boot_option $BOOT_NAME
	efibootmgr | grep "BootCurrent: $bootnum" &> /dev/null
	res=$?
	if  [[ ! -z $bootnum && $res == 0 ]]; then
		echo "[ENABLED ] $BOOT_NAME"
		exit 0
	else
		echo "[DISABLED] $BOOT_NAME"
		exit 2
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
