#!/usr/bin/env robot

*** Variables ***
${TCC_RUN_PREFIX}      ${EMPTY}
${RESOURCES}           ${CURDIR}
${RUNNER}              ${RESOURCES}/util/run_with_delay.sh
${TGPIO_DEVICE}        /dev/ptp0
${RUN_DURATION}        10

*** Settings ***
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Suite Setup
Force Tags       TGPIO    SOURCES_BUILD
Default Tags     NO_PRE_COMMIT    BKC    TGL-U    TGL-H    EHL    ADL-S


*** Keywords ***

Suite Setup
    Set Common Pathes
    Set Suite Variable    ${BASIC_SAMPLES_PATH}    ${TCC_SAMPLES_PATH}/tcc_tgpio_samples/tcc_tgpio_basic_samples
    Run Command And Compare Status    make    0    cwd=${BASIC_SAMPLES_PATH}

*** Test Cases ***

Run TGPIO Periodic Output
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ./tcc_tgpio_basic_periodic_output --device ${TGPIO_DEVICE}
    Run Command And Check Output Contain    ${cmd}    0    Start periodic output.    cwd=${BASIC_SAMPLES_PATH}

Run TGPIO One Shot Output
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ./tcc_tgpio_basic_oneshot_output --device ${TGPIO_DEVICE}
    Run Command And Check Output Contain    ${cmd}    0    Single shot output requested on    cwd=${BASIC_SAMPLES_PATH}

Run TGPIO Basic Input
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ./tcc_tgpio_basic_input --device ${TGPIO_DEVICE}
    Run Command And Check Output Contain    ${cmd}    0    Start input sample.    cwd=${BASIC_SAMPLES_PATH}
