#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
${RUNNER}              ${RESOURCES}/util/run_with_delay.sh
${SAMPLE}              tcc_ethernet_timestamps_sample
${RUN_DURATION}        10
${TGPIO_DEVICE}        /dev/ptp0
${PPS_DEVICE}          /dev/ptp2

*** Settings ***
Library          OperatingSystem
Library          Process
Library          String
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Suite Setup
Force Tags       ETHERNET_SAMPLE
Default Tags     ALL

*** Keywords ***
Suite Setup
    Executable Path    ${SAMPLE}
    Set Common Pathes
    Set TCC Run Prefix

*** Test Cases ***
Shows usage for -h option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} -h
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Shows usage for --help option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} --help
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Run the sample
    ${COMMAND}=    Set Variable    tcc_ethernet_sample_start_synchronization
    Start Process    ${COMMAND}    alias=sync_script_process    shell=True    stdout=${RESOURCES}/run_sample.txt
    Sleep    10
    ${PPS_DEVICE}=    Parse Ptp Clock    ${RESOURCES}/run_sample.txt
    ${COMMAND}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} ${SAMPLE} -t ${TGPIO_DEVICE} 1 -p ${PPS_DEVICE} 0 -T 1000000000
    Run Command And Check Output Contain    ${COMMAND}    0    Generated single pulse on TGPIO. Start time:    timeout=1min    on_timeout=kill
    Terminate Process    sync_script_process    kill=True
    [Tags]    ALL    NO_PRE_COMMIT    TCCSDK-8887
