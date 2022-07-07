#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
${SAMPLE}              tcc_tgpio_info
${TGPIO_DEVICE}        /dev/ptp1

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

Check TGPIO device
    # Grep for list of pins
    ${cmd}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} --device ${TGPIO_DEVICE} | grep -v "Pin Name" | grep -E "\|" | awk -F "|" '{print $2}'
    ${ret}=    Run Command And Compare Status  ${cmd}    0
    Should Not Be Empty    ${ret.stdout}

Wrong Device
    ${cmd}=    Set Variable    ${TCC_RUN_PREFIX} ${SAMPLE} --device /nonexistent/device
    Run Command And Check Output Contain    ${cmd}    249    Unable to open device
    [Tags]    NO_PRE_COMMIT
