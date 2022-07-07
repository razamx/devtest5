#!/usr/bin/env robot

*** Variables ***
${RUN_NUMBER}    20
${sleep}    1000000
${iterations}    1000
${with_lock}    0
${without_lock}    0
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_cache_allocation_sample
${TCC_RUN_PREFIX}    ${EMPTY}

*** Settings ***
Library     OperatingSystem
Library     String
Library     ${RESOURCES}/robot/lib/save_env.py
Resource    ${RESOURCES}/robot/util/system_util.robot
Resource    ${RESOURCES}/robot/util/extensions.robot
Suite Setup    Run Keywords
...            Executable Path    ${RUN_SAMPLE}    AND
...            Set Latency
Test Setup     Run Keywords
...            Start stress-ng    AND
...            Save Environment Variables
Test Teardown  Run Keywords
...            Terminate All Processes    kill=True    AND
...            Restore Environment Variables
Force Tags     CACHE_SAMPLE    PERFORMANCE
Default Tags   ALL    NO_PRE_COMMIT

*** Keywords ***
Get Maximum Latency    [Arguments]    ${str}
    ${matches}=    Get Regexp Matches    ${str}    Maximum total latency: \\d+(\.)?\\d*
    ${matches}=    Get Regexp Matches    ${matches[0]}    \\d+(\.)?\\d*
    [Return]    ${matches[0]}

*** Test Cases ***
With lock better than without lock
    Set Environment Variable    INTEL_LIBITTNOTIFY64  libtcc_collector.so
    ${LOCK_COMMAND}=      Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep ${sleep} --iterations ${iterations}
    ${NOLOCK_COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_DRAM_LATENCY} --sleep ${sleep} --iterations ${iterations}
    FOR    ${index}    IN RANGE    ${RUN_NUMBER}
        # Get maximum latency with lock
        ${res}=         Run Command And Compare Status    ${LOCK_COMMAND}   0
        ${cur_latency}=    Get Maximum Latency    ${res.stdout}
        ${with_lock}=      Evaluate    ${cur_latency} + ${with_lock}

        # Get maximum latency without lock
        ${res}=          Run Command And Compare Status    ${NOLOCK_COMMAND}    0
        ${cur_latency}=     Get Maximum Latency    ${res.stdout}
        ${without_lock}=    Evaluate    ${cur_latency} + ${without_lock}
    END
    ${with_lock}=       Evaluate    ${with_lock} / ${RUN_NUMBER}
    ${without_lock}=    Evaluate    ${without_lock} / ${RUN_NUMBER}
    Should Be True    ${with_lock} < ${without_lock}    msg=Latency with lock is more than without lock
