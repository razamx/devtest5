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
Check old results not used
    # Run correct command and expect histogram
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -time-units clk -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Run Command And Check Output Contain    ${COMMAND}    0    Latency Ranges [clk]
    # Run misspelled command and expect error
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -time-units clk -nn 2 -- tcc_cache_allocation_sample --unlock --sleep 10 --iterations 50
    Run Command And Compare Status    ${COMMAND}    1

Check clocks
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -time-units clk -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Run Command And Check Output Contain    ${COMMAND}    0    Latency Ranges [clk]
    [Tags]    ALL    BKC

Check default with -time-units
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -time-units year -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Run Command And Check Output Contain    ${COMMAND}    0    Latency Ranges [clk]

Check default without -time-units
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Run Command And Check Output Contain    ${COMMAND}    0    Latency Ranges [clk]

Check nanoseconds
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -time-units ns -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Run Command And Check Output Contain    ${COMMAND}    0    Latency Ranges [ns]
    [Tags]    ALL    BKC

Check multiple measurements sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Multiplication:50" -- tcc_multiple_measurements_sample -a 100 -m 100 -i 50
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    "Multiplication" histogram
    Should Contain    ${res.stdout}    Total counts: 50
    [Tags]    ALL    BKC

Check multiple measurements for multiple measurements sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Multiplication:50,Approximation:50,Cycle:50" -- tcc_multiple_measurements_sample -a 100 -m 100 -i 50
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    "Multiplication" histogram
    Should Contain    ${res.stdout}    "Approximation" histogram
    Should Contain    ${res.stdout}    "Cycle" histogram
    Should Contain    ${res.stdout}    Total counts: 50
    [Tags]    ALL    BKC

Check single measurement sample
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Approximation:50" -- tcc_single_measurement_sample -a 100 -i 50
    ${res}=    Run Command And Compare Status    ${COMMAND}    0
    Should Contain    ${res.stdout}    "Approximation" histogram
    Should Contain    ${res.stdout}    Total counts: 50
    [Tags]    ALL    BKC

Check microseconds
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} tcc_measurement_analysis_sample hist "Workload:1000" -dump-file result_for_ns.txt -time-units us -nn 2 -- tcc_cache_allocation_sample --latency ${ALWAYS_DRAM_LATENCY} --sleep 10 --iterations 50
    Run Command And Check Output Contain    ${COMMAND}    0    Latency Ranges [us]
