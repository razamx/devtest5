#!/usr/bin/env robot

*** Variables ***
${RESOURCES}        ${CURDIR}

*** Settings ***
Library             OperatingSystem
Resource            ${RESOURCES}/robot/util/extensions.robot
Resource            ${RESOURCES}/robot/util/output_testing_util.robot
Force Tags          CACHE_LIBRARY    PERFORMANCE
Default Tags        ALL


*** Test Cases ***
Run allocator tests
    ${cmd}=    Set Variable
    ...    ${CURDIR}/payload_tcc_region_manager_driver_allocator_target
    Run Command And Compare Status    ${cmd}    0

Run performance tests
    ${cmd}=    Set Variable
    ...    ${CURDIR}/payload_tcc_region_manager_performance_target
    Run Command And Compare Status    ${cmd}    0
    [Tags]    TGL-U    TGL-H    NO_PRE_COMMIT    EHL

Run close handler tests
    ${cmd}=    Set Variable
    ...    ${CURDIR}/payload_tcc_region_manager_close_handler_target
    Run Command And Compare Status    ${cmd}    0
    [Tags]    TGL-U    TGL-H    NO_PRE_COMMIT    EHL
