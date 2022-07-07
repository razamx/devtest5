#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_measurement_analysis_sample

*** Settings ***
Library          OperatingSystem
Library          String
Library          ${RESOURCES}/robot/lib/save_env.py
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Executable Path    ${RUN_SAMPLE}    AND
...              Set TCC Run Prefix
Test Setup       Run Keywords
...              Kill stress-ng    AND
...              Save Environment Variables
Test Teardown    Run Keywords
...              Kill stress-ng    AND
...              Restore Environment Variables
Force Tags       MEASUREMENT_SAMPLE    BKC
Default Tags     ALL

*** Test Cases ***
Usage is correct
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample
    Run Command And Check Output Contain    ${COMMAND}    0    usage

Check monitor
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libtcc_collector.so
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Workload:100" -deadline-only -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    Count of read data:
    Should Contain    ${res.stdout}    Maximum latency
    [Tags]    ALL    NO_PRE_COMMIT

Check stress-ng stop error handling
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Workload:100" -time-units ns -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --iterations 5
    Start Process    ${COMMAND}    alias=sample_run_process    shell=True
    Sleep    1
    Run    killall stress-ng
    ${res} =    Wait For Process    timeout=1min    on_timeout=kill
    Should Contain    ${res.stdout}    `stress-ng` finished earlier than expected. Please re-run the sample to collect correct sample statistics
    [Tags]    ALL    NO_PRE_COMMIT
