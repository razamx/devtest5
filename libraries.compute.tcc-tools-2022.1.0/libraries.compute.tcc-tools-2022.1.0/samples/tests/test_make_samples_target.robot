#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
@{SAMPLES_EXCLUDE}     tcc_measurement_analysis_sample

*** Settings ***
Library          OperatingSystem
Library          String
Library          Process
Library          ${RESOURCES}/robot/lib/DynamicTestCases.py
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Set Common Pathes                AND
...              Set Samples List                 AND
...              Setup One Test For Each Sample    AND
...              Set TCC Run Prefix
Force Tags       SOURCES_BUILD    BKC
Default Tags     ALL


*** Keywords ***

Set Samples List
    @{ALL_SAMPLES} =    List Directory    ${TCC_SAMPLES_PATH}    *sample*
    ${all_samples_string}=    Catenate    @{ALL_SAMPLES}
    FOR    ${SAMPLE}    IN    @{SAMPLES_EXCLUDE}
        ${all_samples_string}=    Remove String    ${all_samples_string}    ${SAMPLE}
    END
    @{ALL_SAMPLES} =    Split String    ${all_samples_string}
    Set Suite Variable    @{ALL_SAMPLES}

Build Sample    [arguments]    ${SAMPLE}
    ${SAMPLE_PATH}=    Set Variable    ${TCC_SAMPLES_PATH}/${SAMPLE}
    Run Make And Check Status    all      ${SAMPLE_PATH}
    Run Make And Check Status    clean    ${SAMPLE_PATH}

Run Make And Check Status    [arguments]    ${target}    ${working_directory}
    ${result}=    Run Process    make    ${target}    cwd=${working_directory}
    Log    ${result.rc}
    Log    ${result.stdout}
    Log    ${result.stderr}
    Should Be Equal As Integers    ${result.rc}    0

Setup One Test For Each Sample
    FOR    ${SAMPLE}    IN    @{ALL_SAMPLES}
        Add Test Case   Build sample ${SAMPLE}    Build Sample    ${SAMPLE}
    END

*** Test Cases ***
Placeholder test
    Log    Placeholder test required by Robot Framework
