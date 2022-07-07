#!/usr/bin/env robot

*** Variables ***
${PLOT_SCRIPTS_PATH}    ${EMPTY}
${RESOURCES}            ${CURDIR}
${PYTHON_VERSION}       3

${TEST_DATA}                         ${CURDIR}/data/plot_scripts_tests/tcc_rt_communication_demo_plotter
${PLOT_SCRIPTS_RESULTS_DIR}          plot_scripts_results
${SAMPLE}                            tcc_rt_communication_demo_plotter.py

*** Settings ***
Library          Process
Library          OperatingSystem
Library          String
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Check Host Environment    AND
...              Set Common Pathes
Force Tags       RTC_DEMO    PLOT_SCRIPT
Default Tags     HOST

*** Test Cases ***

Shows usage when called without arguments
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE}
    Run Command And Check Output Contain    ${COMMAND}    1    usage

Shows usage for -h (--help) option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -h
    Run Command And Check Output Contain    ${COMMAND}    0    usage
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --help
    Run Command And Check Output Contain    ${COMMAND}    0    usage

Fails without required parameter -i (--input)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -o /tmp/output_file.png
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: -i/--input

Fails with unsupported parameter
    ${UNSUP_PARAM}=    Set Variable    -x
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${TEST_DATA}/input_file.txt ${UNSUP_PARAM}
    Run Command And Check Output Contain    ${COMMAND}    2    unrecognized arguments: ${UNSUP_PARAM}

Fails when required parameter -i (--input) has no argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i
    Run Command And Check Output Contain    ${COMMAND}    2    argument -i/--input: expected at least one argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input
    Run Command And Check Output Contain    ${COMMAND}    2    argument -i/--input: expected at least one argument

Fails when argument for -i (--input) is nonexistent file
    ${BAD_FILE}=    Set Variable    /no/file/here
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    1    File ${BAD_FILE} not found
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    1    File ${BAD_FILE} not found

Fails when argument for -i (--input) is empty file
    ${BAD_FILE}=    Set Variable    /tmp/empty_file.txt
    Create File    ${BAD_FILE}
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    1    Unable to parse "${BAD_FILE}"
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    1    Unable to parse "${BAD_FILE}"
    Remove File    ${BAD_FILE}

Fails when parameter -o (--output) has no argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${TEST_DATA}/input_file.txt -o
    Run Command And Check Output Contain    ${COMMAND}    2    argument -o/--output: expected one argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input ${TEST_DATA}/input_file.txt --output
    Run Command And Check Output Contain    ${COMMAND}    2    argument -o/--output: expected one argument

Shows graph without -o (--output) option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${TEST_DATA}/input_file.txt
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    1024.327000 us
    Should Contain    ${res.stdout}    1028.876904 us
    Should Contain    ${res.stdout}    1124.536000 us
    Should Contain    ${res.stdout}    2.176452 us
    Should Contain    ${res.stdout}    100.209000 us
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input ${TEST_DATA}/input_file.txt
    ${res_2}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res_2.stdout}    1024.327000 us
    Should Contain    ${res_2.stdout}    1028.876904 us
    Should Contain    ${res_2.stdout}    1124.536000 us
    Should Contain    ${res_2.stdout}    2.176452 us
    Should Contain    ${res_2.stdout}    100.209000 us

Dumps graph to file with -o (--output) option
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file_1}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/tcc_rt_communication_demo_plotter_plot_-o.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${TEST_DATA}/input_file.txt -o ${output_file_1}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    1024.327000 us
    Should Contain    ${res.stdout}    1028.876904 us
    Should Contain    ${res.stdout}    1124.536000 us
    Should Contain    ${res.stdout}    2.176452 us
    Should Contain    ${res.stdout}    100.209000 us
    Should Contain    ${res.stdout}    Image saved to "${output_file_1}"
    File Should Exist         ${output_file_1}
    Should Be Equal By Md5    ${output_file_1}    ${TEST_DATA}/tcc_rt_communication_demo_plotter_plot.png
    Set Test Variable         ${output_file_2}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/tcc_rt_communication_demo_plotter_plot_--output.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input ${TEST_DATA}/input_file.txt --output ${output_file_2}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    1024.327000 us
    Should Contain    ${res.stdout}    1028.876904 us
    Should Contain    ${res.stdout}    1124.536000 us
    Should Contain    ${res.stdout}    2.176452 us
    Should Contain    ${res.stdout}    100.209000 us
    Should Contain    ${res.stdout}    Image saved to "${output_file_2}"
    File Should Exist         ${output_file_2}
    Should Be Equal By Md5    ${output_file_2}    ${TEST_DATA}/tcc_rt_communication_demo_plotter_plot.png
    Set Test Variable         ${output_file_3}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/tcc_rt_communication_demo_plotter_plot_2--output.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -i ${TEST_DATA}/input_file.txt ${TEST_DATA}/input_file_2.txt -o ${output_file_3}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    Statistics:
    Should Contain    ${res.stdout}    ---- input_file.txt ----
    Should Contain    ${res.stdout}    min:    1024.327000 us
    Should Contain    ${res.stdout}    avg:    1028.876904 us
    Should Contain    ${res.stdout}    max:    1124.536000 us
    Should Contain    ${res.stdout}    std:    2.176452 us
    Should Contain    ${res.stdout}    jitter: 100.209000 us
    Should Contain    ${res.stdout}    ---- input_file_2.txt ----
    Should Contain    ${res.stdout}    min:    1090.783000 us
    Should Contain    ${res.stdout}    avg:    1099.971549 us
    Should Contain    ${res.stdout}    max:    1107.965000 us
    Should Contain    ${res.stdout}    std:    2.660716 us
    Should Contain    ${res.stdout}    jitter: 17.182000 us
    Should Contain    ${res.stdout}    Image saved to "${output_file_3}"
    File Should Exist         ${output_file_3}
    Should Be Equal By Md5    ${output_file_3}    ${TEST_DATA}/tcc_rt_communication_demo_plotter_plot_2.png
    Set Test Variable         ${output_file_4}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/tcc_rt_communication_demo_plotter_plot_3--output.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --input ${TEST_DATA}/input_file.txt ${TEST_DATA}/input_file_2.txt --output ${output_file_4}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    Statistics:
    Should Contain    ${res.stdout}    ---- input_file.txt ----
    Should Contain    ${res.stdout}    min:    1024.327000 us
    Should Contain    ${res.stdout}    avg:    1028.876904 us
    Should Contain    ${res.stdout}    max:    1124.536000 us
    Should Contain    ${res.stdout}    std:    2.176452 us
    Should Contain    ${res.stdout}    jitter: 100.209000 us
    Should Contain    ${res.stdout}    ---- input_file_2.txt ----
    Should Contain    ${res.stdout}    min:    1090.783000 us
    Should Contain    ${res.stdout}    avg:    1099.971549 us
    Should Contain    ${res.stdout}    max:    1107.965000 us
    Should Contain    ${res.stdout}    std:    2.660716 us
    Should Contain    ${res.stdout}    jitter: 17.182000 us
    Should Contain    ${res.stdout}    Image saved to "${output_file_4}"
    File Should Exist         ${output_file_4}
    Should Be Equal By Md5    ${output_file_4}    ${TEST_DATA}/tcc_rt_communication_demo_plotter_plot_2.png
    [Tags]    TCCSDK-9031

