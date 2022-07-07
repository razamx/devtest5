#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
${INSTALL_PREFIX}      ./usr

*** Settings ***
Library          OperatingSystem
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Get Sources Path
Force Tags       SOURCES_BUILD
Default Tags     HOST    PV

*** Keywords ***

Get Sources Path
    Check Host Environment
    Set Common Pathes
    Set Suite Variable    ${TCC_SOURCES_PATH}    ${TCC_ROOT}/sources/

*** Test Cases ***
Build sources
    Create Directory    ${TCC_SOURCES_PATH}/build
    Empty Directory     ${TCC_SOURCES_PATH}/build

    ${CMAKE_COMMAND} =    Set Variable    cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=Release ../
    Run Command And Compare Status    ${CMAKE_COMMAND}    0    cwd=${TCC_SOURCES_PATH}/build
    ${BUILD_COMMAND} =    Set Variable    make install -j$(nproc --all)
    Run Command And Compare Status    ${BUILD_COMMAND}    0    cwd=${TCC_SOURCES_PATH}/build
    List Directory    ${TCC_SOURCES_PATH}/build/${INSTALL_PREFIX}
