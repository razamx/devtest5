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
if [ "${TCC_DEBUG}" = ENABLE ] ;then set -x; fi
trap 'rm -rf bats-core-*; echo "${BASH_SOURCE[0]}:${BASH_LINENO[0]}:Error happened!" 1>&2' ERR
UTD_MESSAGE="-- Up-to-date:"
NAME=robotframework
ROBOTFRAMEWORK_VERSION=4.1.1

if test -d ${NAME}; then
    echo "${UTD_MESSAGE} RobotFramework already installed"
    exit 0
fi
wget https://github.com/robotframework/robotframework/archive/v$ROBOTFRAMEWORK_VERSION.zip -O ${NAME}-$ROBOTFRAMEWORK_VERSION.zip
unzip ${NAME}-$ROBOTFRAMEWORK_VERSION.zip
# Resolve issue with robot on BKC side
mv ${NAME}-$ROBOTFRAMEWORK_VERSION ${NAME}
rm ${NAME}-$ROBOTFRAMEWORK_VERSION.zip
cp ${NAME}/src/robot/__main__.py ${NAME}/src/robot/robot
dos2unix ${NAME}/src/robot/robot
sed -i 's|#!/usr/bin/env python|#!/usr/bin/env python3|' ${NAME}/src/robot/robot
# Resolve issue for oneCI
rm ${NAME}/.gitignore