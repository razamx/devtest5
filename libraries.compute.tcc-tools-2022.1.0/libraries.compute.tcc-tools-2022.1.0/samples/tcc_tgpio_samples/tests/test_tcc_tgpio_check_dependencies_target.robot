#!/usr/bin/env robot

*** Variables ***
${RESOURCES}              ${CURDIR}

*** Settings ***
Library         OperatingSystem
Resource        ${RESOURCES}/robot/util/extensions.robot
Force Tags      TGPIO    DEPENDENCIES    BKC
Default Tags    TGL-U    TGL-H    EHL    ADL-S

*** Keywords ***
Check kernel boot parameters
    ${KERNEL_BOOT_PARAMS_PATH}=    Set Variable    /proc/cmdline
    
    ${COMMAND}=    Set Variable    cat ${KERNEL_BOOT_PARAMS_PATH} | grep 'art=virtallow'
    Run Command And Compare Status    ${COMMAND}    0

*** Test Cases ***
Check CC dependencies
    Check kernel boot parameters
