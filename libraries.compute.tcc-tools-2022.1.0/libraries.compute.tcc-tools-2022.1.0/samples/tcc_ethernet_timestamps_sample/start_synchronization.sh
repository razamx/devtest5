#!/usr/bin/env bash
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

# This script starts ptp4l and ph2sys, which is required to do before tcc_ethernet_timestamps_sample run.

# Exit on Ctrl+C
trap script_exit SIGINT
script_exit() {
    rm ptp4l_output.txt
    echo -e "\nFinished successfully"
    exit
}

# Get network interface name for TSN AIC
ETH_INTERFACE_NAME=$(ifconfig | grep -o '^[^ ]*' | xargs -n1 bash -c 'echo "$0: $(lspci -s $(ethtool -i $0 2>/dev/null | grep bus-info | grep -o "[^ ]*$") 2>/dev/null)"' | grep -E "Ethernet controller: Intel Corporation Device a0ac|Device 4bb0|Device 43ac|Device 7aac" | head -n 1 | sed 's/:.*//')
if [ -z "$ETH_INTERFACE_NAME" ]; then
    echo "Error: No supported network controller found."
    exit
fi
echo -e "Ethernet interface name: $ETH_INTERFACE_NAME\n"

# Run ptp4l
echo -e "Starting ptp4l..."
ptp4l -A -4 -H -m -i ${ETH_INTERFACE_NAME} 1>ptp4l_output.txt &
sleep 5
echo -e "$(cat ptp4l_output.txt)\n"

# Parse PPS PTP device from stdout of ptp4l
IFS=' ' read -ra LINE <<< $(head -n 1 ptp4l_output.txt)
PTP_DEV=${LINE[2]}
echo -e "PTP device: $PTP_DEV\n"

# Run ph2sys
echo -e "Starting ph2sys..."
echo -e "Use Ctrl+C to exit.\n"
phc2sys -s CLOCK_REALTIME -c $PTP_DEV -m -O 0
