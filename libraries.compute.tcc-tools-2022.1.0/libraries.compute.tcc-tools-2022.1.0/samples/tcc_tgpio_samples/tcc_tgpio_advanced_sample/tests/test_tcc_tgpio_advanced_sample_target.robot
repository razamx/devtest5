#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
${RUNNER}              ${RESOURCES}/util/run_with_delay.sh
${SAMPLE}              tcc_tgpio_advanced_sample
${SAMPLE_DIR}          tcc_tgpio_samples/tcc_tgpio_advanced_sample
${RUN_DURATION}        10

*** Settings ***
Library          OperatingSystem
Library          String
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Suite Setup
Force Tags       TGPIO
Default Tags     BKC    TGL-U    TGL-H    EHL    ADL-S

*** Keywords ***
Suite Setup
    Executable Path    ${SAMPLE}
    Set Common Pathes
    Set Suite Variable    ${CFG_PATH}    ${TCC_SAMPLES_PATH}/${SAMPLE_DIR}/cfg
    Set TCC Run Prefix

*** Test Cases ***
Shows usage when called without arguments
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample
    Run Command And Check Output Contain    ${COMMAND}    250    Usage

Shows usage for -h option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample -h
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Shows usage for --help option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample --help
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Prepare TGL-U
    Set Suite Variable    ${CFG_PLATFORM}    ${CFG_PATH}/TGL-U
    [Tags]    BKC    TGL-U    NO_PRE_COMMIT

Prepare TGL-H
    Set Suite Variable    ${CFG_PLATFORM}    ${CFG_PATH}/TGL-H
    [Tags]    BKC    TGL-H    NO_PRE_COMMIT

Prepare EHL
    Set Suite Variable    ${CFG_PLATFORM}    ${CFG_PATH}/EHL
    [Tags]    BKC    EHL    NO_PRE_COMMIT

Run SWGPIO Single Channel
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample --mode soft --signal_file soft_signal.cfg
    Run Command And Check Output Contain    ${cmd}    0    Start software GPIO output scenario    cwd=${CFG_PLATFORM}    timeout=1min    on_timeout=kill
    [Tags]    BKC    TGL-U    TGL-H    EHL    NO_PRE_COMMIT

Run TGPIO Single Channel
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample --mode tgpio --signal_file tgpio_signal.cfg
    Run Command And Check Output Contain     ${cmd}    0    Start TGPIO output scenario.    cwd=${CFG_PLATFORM}    timeout=1min    on_timeout=kill
    [Tags]    BKC    TGL-U    TGL-H    EHL    NO_PRE_COMMIT

Run SWGPIO Two Channels
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample --mode soft --signal_file soft_two_signals.cfg
    Run Command And Check Output Contain    ${cmd}    0    Start software GPIO output scenario    cwd=${CFG_PLATFORM}    timeout=1min    on_timeout=kill
    [Tags]    BKC    TGL-U    TGL-H    EHL    NO_PRE_COMMIT

Run TGPIO Two Channels
    ${cmd}=    Set Variable    ${RUNNER} ${RUN_DURATION} ${TCC_RUN_PREFIX} tcc_tgpio_advanced_sample --mode tgpio --signal_file tgpio_two_signals.cfg
    Run Command And Check Output Contain    ${cmd}    0    Start TGPIO output scenario.    cwd=${CFG_PLATFORM}    timeout=1min    on_timeout=kill
    [Tags]    BKC    TGL-U    TGL-H    EHL    NO_PRE_COMMIT
