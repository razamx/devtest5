#!/bin/bash
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

TCC_ROOT="$(realpath -s "$(dirname "${BASH_SOURCE[0]}")"/..)"

packages=(
    target
    tools/demo/workloads/src/mrl_workload/mrl_validation_script.py
    tools/internal
    tools/isd_module
    tools/target_scripts
    tools/target_scripts_win
    tools/tcc_cache_configurator.py
    tools/tcc_cache_info_read.py
    tools/tcc_data_streams_optimizer_preprod.py
    tools/tcc_data_streams_optimizer_prod.py
    tools/tcc_rt_checker.py
    samples/tcc_rt_communication_demo/src/scripts/tcc_rt_communication_demo.py
    scripts/setup_ssram
    scripts/setup_ssram_v1
)

echo "Run Linter - Mypy"
exit_code=0
for module in ${packages[@]}; do
    echo "Scan module: ${TCC_ROOT}/${module}"
    python3 -m mypy ${TCC_ROOT}/${module} --config-file ${TCC_ROOT}/.mypy
    status=$?
    if test ${status} -ne 0; then
        exit_code=1
    fi
done

if test ${exit_code} -eq 0; then
    echo "No errors found"
fi

exit ${exit_code}
