#!/usr/bin/env robot

*** Variables ***
${RESOURCES}                    ${CURDIR}

*** Settings ***
Library         OperatingSystem
Library         ${RESOURCES}/robot/lib/save_env.py
Resource        ${RESOURCES}/robot/util/extensions.robot
Resource        ${RESOURCES}/robot/util/system_util.robot
Test Setup      Save Environment Variables
Test Teardown   Restore Environment Variables
Force Tags      MEASUREMENT_LIBRARY
Default Tags    ALL     NO_PRE_COMMIT


*** Test Cases ***
Test validating dump file
    [Documentation]    This test was added to verify the dump file is not empty after calling 
    ...                tcc_measurement_print_history() several times

    Set Environment Variable            TCC_TESTS_INSTALL_PATH          ${CURDIR}
    Run Command And Compare Status      ${CURDIR}/payload_measurement_to_dump_target      0
    File Should Not Be Empty		    ${CURDIR}/measurement_output.txt
    Remove File                         ${CURDIR}/measurement_output.txt

Dump file variable is initialized manually
    [Documentation]    Test verifies the dump file is not empty after calling 
    ...                tcc_measurement_print_history() several times, while 
    ...                TCC_MEASUREMENTS_DUMP_FILE is initialized manually

    Set Environment Variable            TCC_TESTS_INSTALL_PATH          ${CURDIR}
    Set Environment Variable            TCC_MEASUREMENTS_DUMP_FILE      /log.txt
    Run Command And Compare Status      ${CURDIR}/payload_measurement_to_dump_target      0
    File Should Not Be Empty		    ${CURDIR}/log.txt                                                 
    Remove File                         ${CURDIR}/log.txt
