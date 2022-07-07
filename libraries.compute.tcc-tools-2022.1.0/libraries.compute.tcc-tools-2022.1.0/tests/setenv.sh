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

#!/bin/sh

ORIGINAL_PATH=${BASH_SOURCE[0]}
if [[ -z "$ORIGINAL_PATH" ]]; then
	# try zsh approach
	ORIGINAL_PATH=$0
	if [[ -z "$ORIGINAL_PATH" ]]; then
		echo "Failed to get scripth path"
	fi
fi

refine_path()
{
	SELF_PATH=$(cd -P -- "$(dirname -- "$1")" && pwd -P) && SELF_PATH=$SELF_PATH/$(basename -- "$1")
	while [[ -h $SELF_PATH ]]; do
		DIR=$(dirname -- "$SELF_PATH")
		SYM=$(readlink "$SELF_PATH")
		SELF_PATH=$(cd "$DIR" && cd "$(dirname -- "$SYM")" && pwd)/$(basename -- "$SYM")
	done
	echo $SELF_PATH
}

REFINED_PATH=$(refine_path "$ORIGINAL_PATH")
SDK_PATH=$(dirname -- "$REFINED_PATH")
SDK_PATH=${SDK_PATH}/..

BASE_INSTALL_PREFIX=share/tcc_tools

################################################################################
# Set required variables
################################################################################

CPATH=${SDK_PATH}/include${CPATH+:${CPATH}}
LIBRARY_PATH=${SDK_PATH}/lib64${LIBRARY_PATH+:${LIBRARY_PATH}}
LD_LIBRARY_PATH=${SDK_PATH}/lib64:${SDK_PATH}/tests/lib64:${LD_LIBRARY_PATH+:${LD_LIBRARY_PATH}}
PATH=${SDK_PATH}/tests/robotframework/src/robot:${SDK_PATH}/tests/bin:${SDK_PATH}/bin${PATH+:${PATH}}
PYTHONPATH=${SDK_PATH}/tests/robotframework/src:${SDK_PATH}/bin/common${PYTHONPATH+:${PYTHONPATH}}

TCC_ROOT=${SDK_PATH}/${BASE_INSTALL_PREFIX}/
TCC_CONFIG_PATH=${TCC_ROOT}/config
TCC_TOOLS_PATH=${TCC_ROOT}/tools

################################################################################
# Print debug
################################################################################

echo "CPATH=${CPATH}"
echo "LIBRARY_PATH=${LIBRARY_PATH}"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
echo "PATH=${PATH}"
echo "PYTHONPATH=${PYTHONPATH}"

echo "TCC_ROOT=${TCC_ROOT}"
echo "TCC_CONFIG_PATH=${TCC_CONFIG_PATH}"
echo "TCC_TOOLS_PATH=${TCC_TOOLS_PATH}"

################################################################################
# Export variables
################################################################################

export CPATH
export LIBRARY_PATH
export LD_LIBRARY_PATH
export PATH
export PYTHONPATH

export TCC_ROOT
export TCC_CONFIG_PATH
export TCC_TOOLS_PATH
