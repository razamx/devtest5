#!/bin/sh
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

source helpers.sh

if test $# -lt 2; then
    echo "Error: missing required arguments: $0 <interface> <affinity-definition-file>"
    exit 1
fi

IFACE=$1
AFFINITY_FILE=$2

echo "Applying IRQ affinity definitions in file ${AFFINITY_FILE} for NW interface ${IFACE}"

set_irq_smp_affinity ${IFACE} ${AFFINITY_FILE}
