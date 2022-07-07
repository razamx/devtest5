#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_cache_allocation_sample

*** Settings ***
Library         OperatingSystem
Library         String
Library         ${RESOURCES}/robot/lib/save_env.py
Resource        ${RESOURCES}/robot/util/system_util.robot
Resource        ${RESOURCES}/robot/util/extensions.robot

Test Setup      Run Keywords
...             Save Environment Variables

Test Teardown   Run Keywords
...             Terminate All Processes    kill=True    AND
...             Restore Environment Variables

Force Tags      CACHE_SAMPLE    NO_PRE_COMMIT

*** Keywords ***
Run With Core And Get Real
    ${DEFAULT_CORE}=      Set Variable    bash -ce '${RUN_SAMPLE} -l 10000 -i 1 --sleep 2000000000 > /dev/null & sleep 1; ps -o psr $!| tail -1 | sed "s/^ *//"; wait'
    ${CORE_ONE}=      Set Variable    bash -ce '${RUN_SAMPLE} -p 1 -l 10000 -i 1 --sleep 2000000000 > /dev/null & sleep 1; ps -o psr $!| tail -1 | sed "s/^ *//"; wait'
    ${CORE_TWO}=      Set Variable    bash -ce '${RUN_SAMPLE} -p 2 -l 10000 -i 1 --sleep 2000000000 > /dev/null & sleep 1; ps -o psr $!| tail -1 | sed "s/^ *//"; wait'

    ${res_default}=    Run Command And Compare Status    ${DEFAULT_CORE}    0
    ${res_one}=    Run Command And Compare Status    ${CORE_ONE}    0
    ${res_two}=    Run Command And Compare Status    ${CORE_TWO}    0
    [Return]    ${res_default.stdout}    ${res_one.stdout}    ${res_two.stdout}

*** Test Cases ***

CPU Core Id
    ${res_default}    ${res_one}    ${res_two}    Run With Core And Get Real
    Should Be Equal    ${res_default}    3
    Should Be Equal    ${res_one}    1
    Should Be Equal    ${res_two}    2
    [Tags]    BKC

CPU Fail Core Id
    ${CORE_FAIL}=      Set Variable    bash -c 'tcc_cache_allocation_sample -p -1 -l 10000 -i 1 --sleep 2000000000'
    Run Command And Compare Status    ${CORE_FAIL}    250
    [Tags]    BKC
