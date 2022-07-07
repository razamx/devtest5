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
set -x

#Test system configuration

if test -z $1; then
    iterations=1000
else
    iterations=$1
fi

efibootmgr| grep BootCurrent | grep $(efibootmgr  | grep RTCM |head -1 | grep -oE '[0-9A-F]{4}')
if test ! 0=$?; then
	echo "Wrong efi boot"
	efibootmgr
	exit 1
fi

lspsram | grep L2
if test ! 0=$?; then
	echo "L2 region not exist"
	exit 1
fi

lspsram | grep L3
if test ! 0=$?; then
	echo "L3 region not exist"
	exit 1
fi

lspsram | grep DRAM
if test ! 0=$?; then
	echo "DRAM region not exist"
	exit 1
fi

#preparation
rm -rf experiment 
mkdir experiment
cd experiment

#experiment
REGIONS="$(lspsram  | tee lspsram.out | grep -oE '#[0-9]+' | sed 's/#//' | xargs echo)"
for random in 0 42; do
	for interrupt in 0 1; do
		mkdir "rand${random}_int${interrupt}"
        cd "rand${random}_int${interrupt}"
		for region in $REGIONS; do
			cache_lock_benchmark_v2 --sleep 0 --region_index $region --interrupt $interrupt --random $random --iterations $iterations
		done
		cd -
	done
done