#!/usr/bin/env robot

*** Variables ***
${FILE}              dump.txt
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_cache_allocation_sample

*** Settings ***
Library         OperatingSystem
Library         String
Library         ${RESOURCES}/robot/lib/save_env.py
Resource        ${RESOURCES}/robot/util/system_util.robot
Resource        ${RESOURCES}/robot/util/extensions.robot

Suite Setup     Run Keywords
...             Set Latency    AND
...             Set Environment Variable    TCC_MEASUREMENTS_BUFFERS    Workload:100    AND
...             Executable Path    ${RUN_SAMPLE}    AND
...             Set TCC Run Prefix

Test Setup      Save Environment Variables
Test Teardown   Restore Environment Variables

Force Tags      CACHE_SAMPLE    HOST
Default Tags    PV

*** Test Cases ***
Shows usage when called without argumnents
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample
    Run Command And Check Output Contain    ${COMMAND}    250    Usage
