#!/usr/bin/env robot

*** Variables ***
${iterations}        100
${TCC_RUN_PREFIX}    TCC_MEASUREMENTS_BUFFERS=Workload:${iterations}
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_cache_allocation_sample

*** Settings ***
Library         OperatingSystem
Library         String
Library         ${RESOURCES}/robot/lib/save_env.py
Resource        ${RESOURCES}/robot/util/system_util.robot
Resource        ${RESOURCES}/robot/util/extensions.robot

Suite Setup     Run Keywords
...             Executable Path    ${RUN_SAMPLE}    AND
...             Set Latency    AND
...             Set Common Pathes

Test Setup      Run Keywords
...             Start stress-ng    AND
...             Save Environment Variables

Test Teardown   Run Keywords
...             Terminate All Processes    kill=True    AND
...             Restore Environment Variables

Force Tags      CACHE_SAMPLE    PERFORMANCE    NO_PRE_COMMIT

*** Keywords ***
Run Latency And Get Difference     [Arguments]    ${Platform}
    ${L2_COMMAND}=      Set Variable    ${TCC_RUN_PREFIX} TCC_MEASUREMENTS_DUMP_FILE=dump_l2.txt tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --stress --iterations ${iterations} -c
    ${L3_COMMAND}=      Set Variable    ${TCC_RUN_PREFIX} TCC_MEASUREMENTS_DUMP_FILE=dump_l3.txt tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L3_LATENCY} --stress --iterations ${iterations} -c
    ${DRAM_COMMAND}=      Set Variable    ${TCC_RUN_PREFIX} TCC_MEASUREMENTS_DUMP_FILE=dump_dram.txt tcc_cache_allocation_sample --latency ${CACHE_LEVEL_DRAM_LATENCY} --stress --iterations ${iterations} -c
    ${res_dram}=    Run Command And Compare Status    ${DRAM_COMMAND}    0
    ${res_l2}=    Run Command And Compare Status    ${L2_COMMAND}    0
    ${res_l3}=    Run Command And Compare Status    ${L3_COMMAND}    0
    ${res}=    Read Tcc Cache Output    ${res_dram.stdout}    ${res_l2.stdout}    ${res_l3.stdout}    ${Platform}
    [Return]    ${res}

*** Test Cases ***
LATENCY CACHE TGL-H
    ${res}=    Run Latency And Get Difference    TGL-H
    Should Be Equal    ${res}   0
    [Tags]    TGL-H     BKC

LATENCY CACHE TGL-U
    ${res}=    Run Latency And Get Difference    TGL-U
    Should Be Equal    ${res}   0
    [Tags]    TGL-U    BKC

LATENCY CACHE EHL
    ${res}=    Run Latency And Get Difference    EHL
    Should Be Equal    ${res}   0
    [Tags]    EHL      BKC

LATENCY CACHE ADL-S
    ${res}=    Run Latency And Get Difference    ADL-S
    Should Be Equal    ${res}   0
    [Tags]    ADL-S    BKC
