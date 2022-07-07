#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_measurement_monitoring_sample

*** Settings ***
Library          Process
Library          OperatingSystem
Library          String
Library          ${RESOURCES}/robot/lib/save_env.py
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Executable Path    ${RUN_SAMPLE}    AND
...              Set TCC Run Prefix
Test Setup       Save Environment Variables
Test Teardown    Restore Environment Variables
Force Tags       MEASUREMENT_SAMPLE
Default Tags     HOST    PV

*** Test Cases ***
Test help with measurement buffers set
    [Documentation]    This test was added to verify the following bug:\n
    ...                tcc_measurement_monitoring_sample continues running when called with --help
    Set Environment Variable    TCC_MEASUREMENTS_BUFFERS    Approximation:100:20000
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_monitoring_sample -h
    Run Command And Check Output Contain    ${COMMAND}    0    Show this help message and exit
