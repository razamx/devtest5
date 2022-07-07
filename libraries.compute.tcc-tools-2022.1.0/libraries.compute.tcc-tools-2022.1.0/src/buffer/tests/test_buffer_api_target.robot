#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}

*** Settings ***
Library         OperatingSystem
Library         ${RESOURCES}/robot/lib/save_env.py
Resource        ${RESOURCES}/robot/util/extensions.robot
Resource        ${RESOURCES}/robot/util/system_util.robot
Suite Setup     Run Keywords
...             Set Latency    AND
...             Set TCC Run Prefix
Test Setup      Save Environment Variables
Test Teardown   Restore Environment Variables
Force Tags      CACHE_LIBRARY
Default Tags    ALL    BKC

*** Test Cases ***
Run payload
    Set Environment Variable    TEST_L2_LATENCY    ${CACHE_LEVEL_L2_LATENCY}
    Set Environment Variable    TEST_DRAM_LATENCY    ${CACHE_LEVEL_DRAM_LATENCY}
    Run Command And Compare Status    ${TCC_RUN_PREFIX} ${CURDIR}/payload_test_buffer_api_target    0
