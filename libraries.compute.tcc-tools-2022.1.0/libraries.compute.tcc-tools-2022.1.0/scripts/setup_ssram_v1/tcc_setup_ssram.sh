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

# Script to prepare target for cache allocation library

# Path to the script dir
BASE_PATH="$(realpath $(dirname $(realpath $0)))"

CONFIGS_DIR="$BASE_PATH/config"
DEFAULT_TOOLS_PATH="/usr/share/tcc_tools/tools"

CONTROL_RTCM="$BASE_PATH/control_rtcm.sh"
CONTROL_DRIVER="$BASE_PATH/control_tcc_driver.sh"
COMPARE_CACHE="$BASE_PATH/cache_configuration_comparator_v1.py"
SELECT_CONFIG="$BASE_PATH/select_config.py"
CACHE_READ="tcc_cache_info_read"
DRIVER="tcc_buffer"

BUFFER_GUID="3A6F667B-CED4-481B-8FC3-C087A0F2DD53"
CACHE_GUID="9A069A23-836F-41DF-AF02-F14BEC97DE72"

CPUS=(atom atom_6200fe core_i3 core_i5 core_i7)

check_return_code () {
	ret=$?
	message=$1
	if [[ $ret != 0 ]]; then
		echo "$message"
		exit $ret
	fi
}

function usage() {
	echo "tcc_setup_ssram.sh - prepare target for cache allocation library"
	echo ""
	echo "Usage:"
	echo "tcc_setup_ssram.sh enable|disable [-c <cpu_model>|--cpu <cpu_model>] [-v|--verify] [-h|--help]"
	echo "    enable        Enable cache locking capabilities (enable RTCM, driver, and create software SRAM regions) and reboot"
	echo "    disable       Disable cache locking capabilities (disable RTCM and delete software SRAM regions) and reboot"
	echo "Optional arguments"
	echo "    -c, --cpu     CPU to configure. Without the argument, the script will automatically detect the processor. Supported: ${CPUS[@]}"
	echo "    -h, --help    Show this help message and exit"
	echo "    -v, --verify  Do not perform the action, but verify that the system is configured"
}

function argparse() {

	if [[ "$#" == 1 && ( "$1" == "-h" || "$1" == "--help" ) ]]; then
		usage
		exit 0
	fi

	if [[ "$#" != 1 && "$#" != 2 && "$#" != 3 && "$#" != 4 ]]; then
		echo "Wrong number of arguments. See --help."
		exit 1
	fi

	if [[ "$1" == "enable" ]]; then
		MODE="enable"
	elif [[ "$1" == "disable" ]]; then
		MODE="disable"
	else
		echo "Unknown option $1"
		exit 1
	fi

	VERIFY=0
	MANUAL=0
	cpu_configur=""

	if [[ "$#" == 2 ]]; then
		if [[ "$2" == "-v"  || "$2" == "--verify" ]]; then
			VERIFY=1
		else
			echo "Unknown option $2";
			exit 1
		fi
	fi

	if [[ "$#" == 3 ]]; then
		if [[ "$2" == "-c"  || "$2" == "--cpu" ]]; then
			MANUAL=1
			cpu_configur="$3"
		else
			echo "Unknown option $2";
			exit 1
		fi
	fi

	if [[ "$#" == 4 ]]; then
		if [[ "$2" == "-c"  || "$2" == "--cpu" ]]; then
			MANUAL=1
			cpu_configur="$3"
			if [[ "$4" == "-v"  || "$4" == "--verify" ]]; then
				VERIFY=1
			else
				echo "Unknown option $4";
				exit 1
			fi
		elif [[ "$2" == "-v"  || "$2" == "--verify" ]]; then
			VERIFY=1
			if [[ "$3" == "-c"  || "$3" == "--cpu" ]]; then
				MANUAL=1
				cpu_configur="$4"
			else
				echo "Unknown option $3";
				exit 1
			fi
		else
			echo "Unknown option $2";
			exit 1
		fi
	fi
}

function apply_capsules() {
	capsules_dir=$1
	fwupdate --apply $BUFFER_GUID "$capsules_dir/buffer_capsule.out.bin"
	check_return_code "ERROR: Failed to apply buffer capsule"
	fwupdate --apply $CACHE_GUID "$capsules_dir/cache_capsule.out.bin"
	check_return_code "ERROR: Failed to apply cache capsule"
}

function configure() {
	echo "Apply configuration:"
	if [[ $MANUAL == 1 ]]; then
		cpu_dir="$CONFIGS_DIR/$cpu_configur"
	else
		cpu_dir=$(python3 $SELECT_CONFIG)
		check_return_code "ERROR: Failed to find configuration"
	fi
	capsules_dir="$cpu_dir/$MODE"
	apply_capsules "$capsules_dir"

	echo "Configure RTCM:"
	bash -$- "$CONTROL_RTCM" $MODE
	check_return_code "ERROR: Failed to $MODE RTCM"

	echo "Configure driver:"
	bash -$- "$CONTROL_DRIVER" $MODE
	check_return_code "ERROR: Failed to $MODE driver"
}

function check_cache_configuration() {
	if [[ $MANUAL == 1 ]]; then
		cpu_dir="$CONFIGS_DIR/$cpu_configur"
	else
		cpu_dir=$(python3 $SELECT_CONFIG)
		check_return_code "ERROR: Failed to find configuration"
	fi
	real_rtct=$("$CACHE_READ")
	check_return_code "ERROR: Failed to read RTCT"
	expected_rtct_path="$cpu_dir/$PLATFORM/$MODE/expected_rtct.json"
	# NOTE: Compare only buffer, because CLOSes don't affect tests
	echo "$real_rtct" | python3 "$COMPARE_CACHE" --expected "$expected_rtct_path" --buffer
	status=$?
	if [[ $status == 0 ]]; then
		echo "[VALID   ] Cache configuration (from RTCT)"
	else
		echo "[INVALID ] Cache configuration (from RTCT)"
	fi
	return $status
}

function verify() {
	echo "verify"
	bash -$- "$CONTROL_RTCM" status
	hypervisor_status=$?
	bash -$- "$CONTROL_DRIVER" status
	driver_status=$?

	if [[ $MODE == "enable" ]]; then
		expected_status=0
	else
		expected_status=2
	fi

	# Need to load driver to read RTCT even if driver should be unloaded
	modprobe $DRIVER
	check_cache_configuration
	cache_status=$?

	# RTCM and driver expected status depends on enable/disable mode
	# Cache comparator will return 0 if configuration matches expected one
	if [[ $hypervisor_status == $expected_status && \
	      $driver_status == $expected_status && \
	      $cache_status == 0 ]];
	then
		echo "SYSTEM CONFIGURATION IS CORRECT"
		status=0
	else
		echo "SYSTEM CONFIGURATION IS INCORRECT"
		status=-1
	fi

	if [[ $MODE == "disable" ]]; then
		rmmod $DRIVER
	fi
	exit $status
}

argparse $@

# Workaround to import common modules
if [[ -z "$TCC_TOOLS_PATH" ]]; then
	TCC_TOOLS_PATH=$DEFAULT_TOOLS_PATH
fi
export PYTHONPATH=$TCC_TOOLS_PATH:$PYTHONPATH

if [[ $VERIFY == 1 ]]; then
	verify
else
	echo "NOTE: System will be rebooted after configuration complete!"
	configure
	echo "NOTE: Rebooting system!"
	reboot
fi
