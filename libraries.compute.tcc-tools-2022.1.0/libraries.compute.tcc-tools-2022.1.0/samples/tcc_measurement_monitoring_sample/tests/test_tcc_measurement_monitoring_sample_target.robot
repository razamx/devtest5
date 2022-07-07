#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}
${OUTPUT_PATTERNS_PATH} 	${RESOURCES}/robot/data/output_patterns
${RUN_SAMPLE}        tcc_measurement_monitoring_sample

*** Settings ***
Library          Process
Library          OperatingSystem
Library          String
Library          ${RESOURCES}/robot/lib/save_env.py
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/output_testing_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot

Suite Setup      Run Keywords
...              Executable Path    ${RUN_SAMPLE}    AND
...              Set TCC Run Prefix

Test Setup       Run Keywords
...              Get Test Result File Name    AND
...              Save Environment Variables

Test Teardown    Restore Environment Variables
Force Tags       MEASUREMENT_SAMPLE
Default Tags     ALL    BKC

*** Test Cases ***
Test help with measurement buffers set
    [Documentation]    This test was added to verify the following bug:\n
    ...                tcc_measurement_monitoring_sample continues running when called with --help
    Set Environment Variable    TCC_MEASUREMENTS_BUFFERS    Approximation:100:20000
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_monitoring_sample -h
    Run Command And Check Output Contain    ${COMMAND}    0    Show this help message and exit

Test with default single_measurement_sample parameters  
    Set Environment Variable    TCC_USE_SHARED_MEMORY    true
    Set Environment Variable    TCC_MEASUREMENTS_BUFFERS    Approximation:10:20000
    Start Process    tcc_measurement_monitoring_sample    alias=measurements_process    shell=True
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_single_measurement_sample -a 5
    Run Command And Compare Status    ${COMMAND}    0
    ${result}=    Wait For Process    measurements_process    timeout=5    on_timeout=kill
    Should Be Equal As Integers    ${result.rc}    0

Test monitor measurements only 
    Set Environment Variable    TCC_USE_SHARED_MEMORY    true
    Set Environment Variable    TCC_MEASUREMENTS_BUFFERS    Approximation:10
    Start Process    tcc_measurement_monitoring_sample    alias=measurements_process    stdout=output.txt    shell=True
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_single_measurement_sample -a 10 -i 1
    Run Command And Compare Status    ${COMMAND}    0
    ${result}=    Wait For Process    measurements_process    timeout=5    on_timeout=kill
    Should Be Equal As Integers    ${result.rc}    0
    Check Output Compliance    output.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_monitoring_sample_single_measurement.txt
    ...                        None
    [Tags]    ALL


Test monitor measurements and deadline violations 
    Set Environment Variable    TCC_USE_SHARED_MEMORY    true
    Set Environment Variable    TCC_MEASUREMENTS_TIME_UNIT    ns
    Set Environment Variable    TCC_MEASUREMENTS_BUFFERS    Approximation:10:1
    Start Process    tcc_measurement_monitoring_sample    alias=measurements_process    stdout=output.txt    shell=True
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_single_measurement_sample -a 40 -i 1
    Run Command And Compare Status    ${COMMAND}    0
    ${result}=    Wait For Process    measurements_process    timeout=5    on_timeout=kill
    Should Be Equal As Integers    ${result.rc}    0
    Check Output Compliance    output.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_measurement_monitoring_sample_single_measurement_dl.txt
    ...                        None
    [Tags]    ALL
