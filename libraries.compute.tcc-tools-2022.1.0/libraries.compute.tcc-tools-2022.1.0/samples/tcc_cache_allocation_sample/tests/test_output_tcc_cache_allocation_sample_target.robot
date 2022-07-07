#!/usr/bin/env robot

*** Variables ***
${RESOURCES}                          ${CURDIR}
${OUTPUT_PATTERNS_PATH}               ${RESOURCES}/robot/data/output_patterns

${SAMPLE}                             tcc_cache_allocation_sample

*** Settings ***
Library        OperatingSystem
Resource       ${RESOURCES}/robot/util/output_testing_util.robot
Resource       ${RESOURCES}/robot/util/system_util.robot
Resource       ${RESOURCES}/robot/util/extensions.robot

Suite Setup    Run Keywords
...            Create Output Tests Results Directory    AND
...            Set Common Pathes                        AND
...            Get Readme File                          AND
...            Set Latency                              AND
...            Executable Path    ${SAMPLE}

Test Setup     Run Keywords
...            Get Test Result File Name
Force Tags     CACHE_SAMPLE    OUTPUT_COMPARATION
Default Tags   ALL

*** Keywords ***
Get Readme File
    Set Suite Variable    ${README_FILE}    ${TCC_SAMPLES_PATH}/${SAMPLE}/README.md

Run Sample And Save Output    [Arguments]    ${COMMAND}
    ${output}=    Run    ${COMMAND}
    Create File    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt    ${output}

*** Test Cases ***
Help Scenario
    Run Sample And Save Output    ${SAMPLE} -h
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_cache_allocation_sample_help_output.txt
    ...                        ${README_FILE}

Lock Scenario With Sleep
    Run Sample And Save Output    INTEL_LIBITTNOTIFY64=libtcc_collector.so ${SAMPLE} --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 100 --iterations 100
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_cache_allocation_sample_lock_nolock_output.txt
    ...                        None

Lock Scenario With Internal Stress On The Same Core
    Run Sample And Save Output    INTEL_LIBITTNOTIFY64=libtcc_collector.so ${SAMPLE} --latency ${CACHE_LEVEL_L2_LATENCY} --stress --iterations 100
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_cache_allocation_sample_lock_nolock_output.txt
    ...                        None

Lock Scenario Without Stress
    Run Sample And Save Output    INTEL_LIBITTNOTIFY64=libtcc_collector.so ${SAMPLE} --latency ${CACHE_LEVEL_L2_LATENCY} --nostress --iterations 100
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_cache_allocation_sample_lock_nolock_output.txt
    ...                        None

Nolock Scenario
    Run Sample And Save Output    INTEL_LIBITTNOTIFY64=libtcc_collector.so ${SAMPLE} --latency ${CACHE_LEVEL_DRAM_LATENCY} --sleep 100 --iterations 100
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_cache_allocation_sample_lock_nolock_output.txt
    ...                        ${README_FILE}
