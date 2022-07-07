#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
${RUNNER}              ${RESOURCES}/util/run_with_delay.sh
${SAMPLE}              tcc_tgpio_basic_input
${TGPIO_DEVICE}        /dev/ptp0
${RUN_DURATION}        10

*** Settings ***
Library          OperatingSystem
Library          String
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Executable Path    ${SAMPLE}    AND
...              Set TCC Run Prefix
Force Tags       TGPIO    BKC    ALL

*** Test Cases ***
Shows usage for -h option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} -h
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Shows usage for --help option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} --help
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Run With Default Settings
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} ${SAMPLE} --device ${TGPIO_DEVICE}
    Run Command And Check Output Contain    ${cmd}    0    Start input sample.    timeout=1min    on_timeout=kill
    [Tags]    NO_PRE_COMMIT

Run Pin 1
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} ${SAMPLE} --pin 1 --channel 1 --device ${TGPIO_DEVICE}
    Run Command And Check Output Contain    ${cmd}    0    Start input sample.    timeout=1min    on_timeout=kill
    [Tags]    NO_PRE_COMMIT

Wrong Device
    ${cmd}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} --device /nonexistent/device
    Run Command And Check Output Contain    ${cmd}    249    Unable to open device
    [Tags]    NO_PRE_COMMIT
