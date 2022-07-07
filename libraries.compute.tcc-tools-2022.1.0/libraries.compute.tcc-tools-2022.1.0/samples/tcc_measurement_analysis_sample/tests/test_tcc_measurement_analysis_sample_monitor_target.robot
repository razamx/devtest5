#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_measurement_analysis_sample

*** Settings ***
Library          OperatingSystem
Library          String
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Executable Path    ${RUN_SAMPLE}    AND
...              Set TCC Run Prefix
Test Setup       Kill stress-ng
Test Teardown    Kill stress-ng
Force Tags       MEASUREMENT_SAMPLE
Default Tags     ALL

*** Test Cases ***
Check default
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    Latency(clk)
    Should Contain    ${res.stdout}    data: 1
    [Tags]    ALL    BKC

Check deadline
    # No deadline monitoring when deadline is not set${TCC_RUN_PREFIX} 
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    Run Command And Check Output Not Contain    ${COMMAND}    0    Latency exceeding deadline
    # Deadlines found
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100:1" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    Run Command And Check Output Contain    ${COMMAND}    0    Latency exceeding deadline
    # Deadlines not found
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100:10000" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    Run Command And Check Output Not Contain    ${COMMAND}    0    Latency exceeding deadline
    [Tags]    ALL    BKC

Check -time-units clk
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor -time-units clk "Cycle:100:1" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    Run Command And Check Output Contain    ${COMMAND}    0    Latency(clk)

Check -time-units ns
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor -time-units ns "Cycle:100:1" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    Run Command And Check Output Contain    ${COMMAND}    0    Latency(ns)

Check -time-units us
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor -time-units us "Cycle:100:1" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    Run Command And Check Output Contain    ${COMMAND}    0    Latency(us)

Check -deadline-only
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor -deadline-only "Cycle:100:10000" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain Any    ${res.stdout}    Latency exceeding deadline    NO DEADLINE VIOLATIONS HAPPENED

Check multiple measeurers
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100:1,Sensor:100:1,Model:100:1" -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 1
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Cycle]
    Should Contain    ${res.stdout}    [Sensor]
    Should Contain    ${res.stdout}    [Model]
    [Tags]    ALL    BKC

Check multiple measurements sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Multiplication:5" -- tcc_multiple_measurements_sample -a 100 -m 100 -i 5
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Multiplication] Count of read data: 5
    [Tags]    ALL    BKC

Check multiple measeurers for multiple measurements sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Multiplication:1,Approximation:1,Cycle:1" -- tcc_multiple_measurements_sample -a 100 -m 100 -i 1
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Multiplication] Count of read data: 1
    Should Contain    ${res.stdout}    [Approximation] Count of read data: 1
    Should Contain    ${res.stdout}    [Cycle] Count of read data: 1
    [Tags]    ALL    BKC

Check no deadline violations multiple measeurers for multiple measurements sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Multiplication:5,Approximation:5,Cycle:5" -deadline-only -- tcc_multiple_measurements_sample -a 100 -m 100 -i 5
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Multiplication] Count of read data: 5
    Should Contain    ${res.stdout}    [Approximation] Count of read data: 5
    Should Contain    ${res.stdout}    [Cycle] Count of read data: 5
    Should Contain    ${res.stdout}    NO DEADLINE VIOLATIONS HAPPENED
    [Tags]    ALL    BKC

Check deadline multiple measeurers for multiple measurements sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Multiplication:5:10,Approximation:5,Cycle:5" -deadline-only -- tcc_multiple_measurements_sample -a 100 -m 100 -i 5
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Multiplication] Count of read data: 5
    Should Contain    ${res.stdout}    [Approximation] Count of read data: 5
    Should Contain    ${res.stdout}    [Cycle] Count of read data: 5
    Should Contain    ${res.stdout}    Latency exceeding deadline
    [Tags]    ALL    BKC

Check single measurement sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Approximation:5" -- tcc_single_measurement_sample -a 100 -i 5
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Approximation]
    Should Contain    ${res.stdout}    data: 5
    [Tags]    ALL    BKC

Check no deadline violations single measurement sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Approximation:5" -deadline-only -- tcc_single_measurement_sample -a 100 -i 5
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Approximation]
    Should Contain    ${res.stdout}    NO DEADLINE VIOLATIONS HAPPENED
    Should Contain    ${res.stdout}    data: 5
    [Tags]    ALL    BKC

Check deadline single measurement sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Approximation:5:10" -deadline-only -- tcc_single_measurement_sample -a 100 -i 5
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    [Approximation]
    Should Contain    ${res.stdout}    Latency exceeding deadline
    Should Contain    ${res.stdout}    data: 5
    [Tags]    ALL    BKC

# This test was added to verify the following bug:
# tcc_measurement_analysis_sample monitor ignors -time-units for deadline monitoring
Check deadline with -time-units
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100:10" -deadline-only -time-units clk -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 100
    Run Command And Check Output Contain    ${COMMAND}    0    Latency exceeding deadline
    # Run with clk, will not found deadlines
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample monitor "Cycle:100:100" -deadline-only -time-units us -nn 2 -- ${CURDIR}/payload_syntetic_multiple_measurement_instances_target --latency ${ALWAYS_DRAM_LATENCY} --iterations 100
    Run Command And Check Output Contain    ${COMMAND}    0    NO DEADLINE VIOLATIONS HAPPENED
    [Tags]    NO_PRE_COMMIT
