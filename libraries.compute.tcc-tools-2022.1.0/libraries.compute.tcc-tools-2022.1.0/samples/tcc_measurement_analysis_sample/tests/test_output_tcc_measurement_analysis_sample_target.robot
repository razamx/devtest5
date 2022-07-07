#!/usr/bin/env robot

*** Variables ***
${RESOURCES}                          ${CURDIR}
${OUTPUT_PATTERNS_PATH}               ${RESOURCES}/robot/data/output_patterns

${SAMPLE}                             tcc_measurement_analysis_sample

*** Settings ***
Library        OperatingSystem
Resource       ${RESOURCES}/robot/util/output_testing_util.robot
Resource       ${RESOURCES}/robot/util/system_util.robot
Resource       ${RESOURCES}/robot/util/extensions.robot

Suite Setup    Run Keywords
...            Create Output Tests Results Directory    AND
...            Set Common Pathes                        AND
...            Get Readme File                          AND
...            Executable Path    ${SAMPLE}

Test Setup     Run Keywords
...            Get Test Result File Name
Force Tags     MEASUREMENT_SAMPLE    OUTPUT_COMPARATION
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
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_analysis_sample_help_output.txt
    ...                        ${README_FILE}

Monitor Help Scenario
    Run Sample And Save Output    ${SAMPLE} monitor -h
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_analysis_sample_monitor_help_output.txt
    ...                        ${README_FILE}

Hist Help Scenario
    Run Sample And Save Output    ${SAMPLE} hist -h
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_analysis_sample_hist_help_output.txt
    ...                        ${README_FILE}

Monitor Scenario
    Run Sample And Save Output    ${SAMPLE} monitor "Workload:100" -time-units "ns" -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --iterations 5
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_analysis_sample_monitor_output.txt
    ...                        None

Hist Scenario
    Run Sample And Save Output    ${SAMPLE} hist "Workload:1000" -time-units "ns" -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_analysis_sample_hist_output.txt
    ...                        None
