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
...             Kill stress-ng    AND
...             Set TCC Run Prefix
Test Setup      Save Environment Variables
Test Teardown   Restore Environment Variables
Force Tags      CACHE_SAMPLE
Default Tags    ALL

# This test temporary disable because we run pre-commit with CMAKE_BUILD_TYPE=Debug but pathes are
# stripped only for Release
# Test DE8229: tcc_cache_allocation_sample contain full source path in error output
#    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample -l1
#    Run Command And Check Output Contain    ${COMMAND}    255    from samples/tcc_cache_allocation_sample/src/main.c

*** Test Cases ***
Shows usage when called without argumnents
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample
    Run Command And Check Output Contain    ${COMMAND}    250    Usage

Simple test with lock with sleep
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5
    Run Command And Check Output Contain    ${COMMAND}    0    Running workload
    [Tags]    ALL    BKC

Simple test with lock with internal stress on the same core
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --stress
    Run Command And Check Output Contain    ${COMMAND}    0    Running workload
    [Tags]    ALL    BKC

Simple test with lock without stress
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --nostress
    Run Command And Check Output Contain    ${COMMAND}    0    Running workload
    [Tags]    ALL    BKC

Simple test with nolock
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_DRAM_LATENCY} --sleep 5
    Run Command And Check Output Contain    ${COMMAND}    0    Running workload
    [Tags]    ALL    BKC

Extended test with lock
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    Running workload

Test with getting measurement structure
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libtcc_collector.so
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    Maximum latency

Test with getting measurement structure and default set collector using '-c'
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample -c --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    Maximum latency

Test with getting measurement structure and default set collector using '--collect'
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --collect --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    Maximum latency

Check that used collector provided via INTEL_LIBITTNOTIFY64
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libbad_collector.so
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample -c --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    collector \= libbad_collector.so

Test with other collector
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libbad_collector.so
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Not Contain    ${COMMAND}    0    Maximum latency

Test no dumping to file
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libtcc_collector.so
    Remove File    ${FILE}
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Compare Status    ${COMMAND}    0
    File Should Not Exist    ${FILE}

Test dumping to file
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libtcc_collector.so
    Set Environment Variable    TCC_MEASUREMENTS_DUMP_FILE    ${FILE}
    Set Environment Variable    TCC_MEASUREMENTS_TIME_UNIT    clk
    Remove File    ${FILE}
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Compare Status    ${COMMAND}    0
    File Should Exist    ${FILE}
    ${file_content}=    Grep File    ${FILE}    Workload:

Test flag params showing no -c or --collect flag
    Set Environment Variable    INTEL_LIBITTNOTIFY64    libtcc_collector.so
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    collector \= libtcc_collector.so

Test flag params showing -c flag
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample -c --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    collector \= libtcc_collector.so

Test flag params showing --collect flag
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --collect --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Contain    ${COMMAND}    0    collector \= libtcc_collector.so

Test no collector and no --collect or -c flags
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_cache_allocation_sample --latency ${CACHE_LEVEL_L2_LATENCY} --sleep 5 --iterations 5
    Run Command And Check Output Not Contain    ${COMMAND}    0    Statistics for workload
