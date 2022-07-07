#!/usr/bin/env robot

*** Variables ***
${RESOURCES}         ${CURDIR}
${RUN_SAMPLE}        tcc_rt_communication_demo

*** Settings ***
Library          OperatingSystem
Library          String
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Executable Path    ${RUN_SAMPLE}    AND
...              Get Interface    AND
...              Set TCC Run Prefix
Force Tags       RTC_DEMO
Default Tags     EHL

*** Keywords ***

Get Interface
    ${COMMAND}=    Set Variable    ifconfig | grep -oE '^[^ ]+' | head -1
    ${IFCONFIG_OUT}=    Run Command And Compare Status    ${COMMAND}    0
    ${INTERFACE}=  Set Variable    ${IFCONFIG_OUT.stdout}
    Set Suite Variable    ${INTERFACE}

*** Test Cases ***

Shows usage when called without arguments
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE}
    Run Command And Check Output Contain    ${COMMAND}    70    Usage

Shows usage for -h (--help) option
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -h
    Run Command And Check Output Contain    ${COMMAND}    0    Usage
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --help
    Run Command And Check Output Contain    ${COMMAND}    0    Usage

Fails with unsupported parameter
    ${UNSUP_PARAM}=    Set Variable    z
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -p basic-a-noopt -i ${INTERFACE} -${UNSUP_PARAM}
    Run Command And Check Output Contain    ${COMMAND}    2    unrecognized arguments: -${UNSUP_PARAM}

Fails without required parameter -p (--profile)
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE}
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: -p/--profile
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE}
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: -p/--profile

Fails when required parameter -p (--profile) has no argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p
    Run Command And Check Output Contain    ${COMMAND}    2    argument -p/--profile: expected one argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile
    Run Command And Check Output Contain    ${COMMAND}    2    argument -p/--profile: expected one argument

Fails when argument for -p (--profile) is unsupported profile
    ${UNSUP_PROFILE}=    Set Variable    unsup_profile
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p ${UNSUP_PROFILE}
    Run Command And Check Output Contain    ${COMMAND}    70    Profile ${UNSUP_PROFILE} not supported. Use [-h] to see help information.
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile ${UNSUP_PROFILE}
    Run Command And Check Output Contain    ${COMMAND}    70    Profile ${UNSUP_PROFILE} not supported. Use [-h] to see help information.

Fails without required parameter -i (--interface)
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -p basic-a-noopt
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: -i/--interface
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --profile basic-a-noopt
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: -i/--interface

Fails when required parameter -i (--interface) has no argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i -p basic-a-noopt
    Run Command And Check Output Contain    ${COMMAND}    2    argument -i/--interface: expected one argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface --profile basic-a-noopt
    Run Command And Check Output Contain    ${COMMAND}    2    argument -i/--interface: expected one argument

Fails when argument for -i (--interface) is invalid network interface
    ${INVALID_INTERFACE}=    Set Variable    invalid_interface
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INVALID_INTERFACE} -p basic-a-noopt
    Run Command And Check Output Contain    ${COMMAND}    70    Unknown interface name was specified. Specify valid interface name (check with ifconfig tool, for example).
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INVALID_INTERFACE} --profile basic-a-noopt
    Run Command And Check Output Contain    ${COMMAND}    70    Unknown interface name was specified. Specify valid interface name (check with ifconfig tool, for example).

Fails when parameter -m (--mode) has no argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -m
    Run Command And Check Output Contain    ${COMMAND}    2    argument -m/--mode: expected one argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --mode
    Run Command And Check Output Contain    ${COMMAND}    2    argument -m/--mode: expected one argument

Fails when argument for -m (--mode) is unsupported mode
    ${UNSUP_MODE}=    Set Variable    unsup_mode
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -m ${UNSUP_MODE}
    Run Command And Check Output Contain    ${COMMAND}    70    Mode ${UNSUP_MODE} not supported. Use [-h] to see help information.
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --mode ${UNSUP_MODE}
    Run Command And Check Output Contain    ${COMMAND}    70    Mode ${UNSUP_MODE} not supported. Use [-h] to see help information.

Fails when parameter -c (--config-path) has no argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -c
    Run Command And Check Output Contain    ${COMMAND}    2    argument -c/--config-path: expected one argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --config-path
    Run Command And Check Output Contain    ${COMMAND}    2    argument -c/--config-path: expected one argument

Fails when argument for -c (--config-path) is nonexistent path
    ${BAD_PATH}=    Set Variable    /bad_path
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -c ${BAD_PATH}
    Run Command And Check Output Contain    ${COMMAND}    70    Directory ${BAD_PATH} with configuration profiles does not exist.
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --config-path ${BAD_PATH}
    Run Command And Check Output Contain    ${COMMAND}    70    Directory ${BAD_PATH} with configuration profiles does not exist.

Warns when argument for -c (--config-path) is empty directory
    ${CONFIG_PATH}=    Set Variable    /tmp/new_empty_dir
    Create Directory    ${CONFIG_PATH}
    Directory Should Be Empty    ${CONFIG_PATH}
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -c ${CONFIG_PATH}
    Run Command And Check Output Contain    ${COMMAND}    70    The config file ${CONFIG_PATH}/basic/Elkhart_Lake/talker-noopt.json does not exist
    Directory Should Be Empty    ${CONFIG_PATH}
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --config-path ${CONFIG_PATH}
    Run Command And Check Output Contain    ${COMMAND}    70    The config file ${CONFIG_PATH}/basic/Elkhart_Lake/talker-noopt.json does not exist
    Remove Directory    ${CONFIG_PATH}    recursive=True

Fails when parameter -o (--output-file) has no argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -o
    Run Command And Check Output Contain    ${COMMAND}    2    argument -o/--output-file: expected one argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --output-file
    Run Command And Check Output Contain    ${COMMAND}    2    argument -o/--output-file: expected one argument

Fails when argument for -o (--output-file) contains nonexistent path
    ${BAD_PATH}=    Set Variable    /bad_path
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -o ${BAD_PATH}/output_file
    Run Command And Check Output Contain    ${COMMAND}    70    Path to output file (${BAD_PATH}) does not exist.
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --output-file ${BAD_PATH}/output_file
    Run Command And Check Output Contain    ${COMMAND}    70    Path to output file (${BAD_PATH}) does not exist.

Fails when parameter -e (--exec-path) has no argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -e
    Run Command And Check Output Contain    ${COMMAND}    2    argument -e/--exec-path: expected one argument
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --exec-path
    Run Command And Check Output Contain    ${COMMAND}    2    argument -e/--exec-path: expected one argument

Fails when argument for -e (--exec-path) contains nonexistent path
    ${BAD_PATH}=    Set Variable    /bad_path
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} -i ${INTERFACE} -p basic-a-noopt -e ${BAD_PATH}
    Run Command And Check Output Contain    ${COMMAND}    70    Path to directory with binary executables (${BAD_PATH}) does not exist.
    ${COMMAND}=    Set Variable    ${TCC_RUN_PREFIX} ${RUN_SAMPLE} --interface ${INTERFACE} --profile basic-a-noopt --exec-path ${BAD_PATH}
    Run Command And Check Output Contain    ${COMMAND}    70    Path to directory with binary executables (${BAD_PATH}) does not exist.
