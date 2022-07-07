#!/usr/bin/env robot

*** Variables ***
${RESOURCES}			    ${CURDIR}
${OUTPUT_PATTERNS_PATH} 	${RESOURCES}/robot/data/output_patterns
${SAMPLE} 			        tcc_multiple_measurements_sample

*** Settings ***
Library         OperatingSystem
Resource        ${RESOURCES}/robot/util/output_testing_util.robot
Resource        ${RESOURCES}/robot/util/extensions.robot

Suite Setup    Run Keywords
...            Check Host Environment                   AND
...            Create Output Tests Results Directory    AND
...            Set Common Pathes                        AND
...            Get Readme File                          AND
...            Executable Path    ${SAMPLE}

Test Setup    Run Keywords
...           Get Test Result File Name
Force Tags     MEASUREMENT_SAMPLE    OUTPUT_COMPARATION
Default Tags    HOST    PV

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
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_multiple_measurements_sample_help_output.txt
    ...                        ${README_FILE}

Test iterations
    Run Sample And Save Output    ${SAMPLE} -a 100 -m 100 -i 1000
    Check Output Compliance    ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.txt
    ...                        ${OUTPUT_TESTS_RESULTS_PATH}/${TEST_RESULT_NAME}.html
    ...                        ${OUTPUT_PATTERNS_PATH}/tcc_multiple_measurements_sample_output_iterations.txt
    ...                        None
