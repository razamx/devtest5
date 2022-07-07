#!/usr/bin/env robot

*** Variables ***
${RESOURCES}			    ${CURDIR}
${OUTPUT_PATTERNS_PATH} 	${RESOURCES}/robot/data/output_patterns
${SAMPLE} 			        tcc_single_measurement_sample

*** Settings ***
Library         OperatingSystem
Resource        ${RESOURCES}/robot/util/output_testing_util.robot
Resource        ${RESOURCES}/robot/util/extensions.robot

Suite Setup     Run Keywords
...             Create Output Tests Results Directory    AND
...             Set Common Pathes                        AND
...             Get Readme File                          AND
...             Executable Path    ${SAMPLE}

Test Setup      Run Keywords
...             Get Test Result File Name
Force Tags      MEASUREMENT_SAMPLE    OUTPUT_COMPARATION
Default Tags    ALL    BKC

*** Keywords ***
Get Readme File
    Set Suite Variable    ${README_FILE}  ${TCC_SAMPLES_PATH}/${SAMPLE}/README.md
    
Run Sample And Save Output    [Arguments]    ${COMMAND}
        ${output} =    Run    ${COMMAND}
        Create File    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt    ${output}

*** Test Cases ***

Test help
    Run Sample And Save Output    ${SAMPLE} -h
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_single_measurement_sample_help_output.txt
    ...                        ${README_FILE}

Test iterations
    Run Sample And Save Output    ${SAMPLE} -a 50 --iterations 2000
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_single_measurement_sample_output_iterations.txt
    ...                        None

Test deadline
    Run Sample And Save Output    ${SAMPLE} -a 50 --deadline 1,ns -i 1
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_single_measurement_sample_output_deadline.txt
    ...                        None

Test outliers
    Run Sample And Save Output    ${SAMPLE} -a 10 --deadline 6000,ns -o -i 25
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_single_measurement_sample_output_outliers.txt
    ...                        None

Test measurement run with another shared memory size fail after interrupt 
    Run Command And Compare Status
    ...    ${CURDIR}/test_output_tcc_single_measurement_sample_target/measurement_run_with_another_shared_memory_size_fail_after_interrupt.sh
    ...    0
    [Tags]    ALL

