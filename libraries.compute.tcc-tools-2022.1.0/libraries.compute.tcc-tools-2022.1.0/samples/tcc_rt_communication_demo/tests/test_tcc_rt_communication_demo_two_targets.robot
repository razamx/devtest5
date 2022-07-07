#!/usr/bin/env robot

*** Variables ***
${RESOURCES}                ${CURDIR}


*** Settings ***
Library           String
Library           SSHLibrary
Library           OperatingSystem
Resource          ${RESOURCES}/robot/util/ssh_util.robot
Test Teardown     Test Teardown
Suite Setup       Suite Setup
Suite Teardown    Close All Connections
Force Tags        RTC_DEMO    TWO_TARGETS
Default Tags      NO_PRE_COMMIT  NO_DAILY  EHL  TGL-U  TGL-H


*** Test Cases ***
Test noopt basic profile in RTC Sample
    [Timeout]  2 minutes
    # Start the remote launch request for 'setup' mode
    Switch Connection  Board A Connection
    ${results} =  Run RTC Sample basic profile and get statistics  basic-a-noopt  setup
    Log  Setup log: ${results}
    Sleep  10
    ${Remote log} =  Run RTC Sample basic profile and get statistics  basic-a-noopt  run
    Log  Run log: ${Remote log}
    ${result} =  Parse RTC Sample basic profile results  ${Remote log}

    # Check the results
    Log  Results: ${result}
    Should Be True  ${result.min} > 0 and ${result.min} < 500
    Should Be True  ${result.avg} > 0 and ${result.avg} < 500
    Should Be True  ${result.max} > 0 and ${result.max} < 1000
    Should Be True  ${result.packets} > 1950 and ${result.packets} <= 2000


Test noopt SISO Single profile in RTC Sample
    [Timeout]  2 minutes
    # Start the remote launch request for 'setup' mode
    Switch Connection  Board A Connection
    ${results} =  Run RTC Sample SISO single profile and get statistics  siso-single-a-noopt  setup
    Sleep  10
    ${Remote log} =  Run RTC Sample SISO single profile and get statistics  siso-single-a-noopt  run
    ${result} =  Parse RTC Sample basic profile results  ${Remote log}

    # Check the results
    Log  Results: ${result}
    Should Be True  ${result.min} > 0 and ${result.min} < 2000
    Should Be True  ${result.avg} > 0 and ${result.avg} < 2000
    Should Be True  ${result.max} > 0 and ${result.max} < 2000
    Should Be True  ${result.packets} > 1950 and ${result.packets} <= 2000


Test opt basic profile in RTC Sample
    [Timeout]  3 minutes
    # Start the remote launch request for 'setup' mode
    Switch Connection  Board A Connection
    ${results} =  Run RTC Sample basic profile and get statistics  basic-a-opt  setup
    Log  Setup log: ${results}
    Sleep  10
    ${Remote log} =  Run RTC Sample basic profile and get statistics  basic-a-opt  run
    Log  Run log: ${Remote log}
    ${result} =  Parse RTC Sample basic profile results  ${Remote log}

    # Check the results
    Log  Results: ${result}
    Should Be True  ${result.min} > 0 and ${result.min} < 500
    Should Be True  ${result.avg} > 0 and ${result.avg} < 500
    Should Be True  ${result.max} > 0 and ${result.max} < 1500
    Should Be True  ${result.packets} > 1900 and ${result.packets} <= 2000


Test opt SISO Single profile in RTC Sample
    [Timeout]  5 minutes
    # Start the remote launch request for 'setup' mode
    Switch Connection  Board A Connection
    ${results} =  Run RTC Sample SISO single profile and get statistics  siso-single-a-opt  setup
    Log  Setup log: ${results}
    Sleep  10
    ${Remote log} =  Run RTC Sample SISO single profile and get statistics  siso-single-a-opt  run
    Log  Run log: ${Remote log}
    ${result} =  Parse RTC Sample basic profile results  ${Remote log}

    # Check the results
    Log  Results: ${result}
    Should Be True  ${result.min} > 0 and ${result.min} < 2000
    Should Be True  ${result.avg} > 0 and ${result.avg} < 2000
    Should Be True  ${result.max} > 0 and ${result.max} < 2000
    Should Be True  ${result.packets} > 1500 and ${result.packets} <= 2000


*** Keywords ***
Establish connections
    # Establish connections to the 2 targets used in the test case
    Open Connection     ${TARGET A HOSTNAME}  alias=Board A Connection
    Login               ${TARGET A USERNAME}  ${TARGET A PASSWORD}
    Open Connection     ${TARGET B HOSTNAME}  alias=Board B Connection
    Login               ${TARGET B USERNAME}  ${TARGET B PASSWORD}


Get target TSN interface names
    ${interface A} =  Get TSN interface name  Board A Connection
    Log  TSN network interface name on TARGET A: ${interface A}.
    Should Not Be Empty  ${interface A}
    Set Suite Variable  \${TARGET A INTERFACE}  ${interface A}
    ${interface B} =  Get TSN interface name  Board B Connection
    Log  TSN network interface name on TARGET B: ${interface B}.
    Should Not Be Empty  ${interface B}
    Set Suite Variable  \${TARGET B INTERFACE}  ${interface B}


Get TSN interface name
    [Arguments]  ${connection}
    Switch Connection  ${connection}
    ${res} =  Run Remote Command And Compare Status  ls /sys/devices/pci0000\:00/0000\:00\:1e.4/net  0
    ${interface} =  Set variable  ${res.stdout}
    Log  Got network interface, it is ${interface}
    Return From Keyword If  '${interface} != ""'  ${interface}
    Return from keyword  eth0


Run RTC Sample basic profile and get statistics
    [Arguments]  ${profile}  ${mode}
    # Clear the output directory on Target A
    Execute Command  rm -r /tmp/tcc_rt_communication_demo_output
    ${results} =  Run Remote Command And Compare Status
    ...  cd /tmp && tcc_rt_communication_demo -p ${profile} -i ${TARGET A INTERFACE} -r ${TARGET B INTERFACE} -a ${TARGET B HOSTNAME} -m ${mode}
    ...  0
    Log  RTC Sample, basic profile: ${profile}, mode: ${mode}, output: ${results.stdout}
    ${results} =  Run Remote Command And Compare Status  cat /tmp/tcc_rt_communication_demo_output/remote-log-${mode}.txt  0
    Return from keyword  ${results.stdout}


Run RTC Sample SISO single profile and get statistics
    [Arguments]  ${profile}  ${mode}
    ${results} =  Run Remote Command And Compare Status
    ...  cd /tmp && tcc_rt_communication_demo -p ${profile} -i ${TARGET A INTERFACE} -r ${TARGET B INTERFACE} -a ${TARGET B HOSTNAME} -m ${mode}
    ...  0
    Log  RTC Sample, SISO single profile: ${profile}, mode: ${mode}, output: ${results.stdout}
    Return from keyword  ${results.stdout}


Parse RTC Sample basic profile results
    [Arguments]  ${log}
    # Extract string parameters
    ${Param 1} =  Get Regexp Matches  ${log}  Received ([0-9]+) packets, the last sequence number: ([0-9]+)  1  2
    ${Param 2} =  Get Regexp Matches  ${log}  Minimum packet latency: ([0-9]+) us  1
    ${Param 3} =  Get Regexp Matches  ${log}  Maximum packet latency: ([0-9]+) us  1
    ${Param 4} =  Get Regexp Matches  ${log}  Average packet latency: ([0-9]+) us  1

    # Extract numeric parameters
    ${min} =  Convert To Integer  ${Param 2[0]}
    ${max} =  Convert To Integer  ${Param 3[0]}
    ${avg} =  Convert To Integer  ${Param 4[0]}
    ${packets count} =  Convert To Integer  ${Param 1[0][0]}
    ${last sequence} =  Convert To Integer  ${Param 1[0][1]}

    # Create the object (map) with the parsed statistics
    &{result} =  Create Dictionary  min=${min}  max=${max}  avg=${avg}  packets=${packets count}  last=${last sequence}
    Return from keyword  ${result}


Get Target A kernel version
    Switch Connection  Board A Connection
    ${results} =  Run Remote Command And Compare Status  uname -r  0  False
    ${full kernel version} =  Set variable  ${results.stdout}
    @{version components} =  split string  ${full kernel version}  .
    ${kernel version} =  Set variable  ${version components[0]}.${version components[1]}
    Log  Kernel version: ${kernel version}
    Return from keyword  ${kernel version}


Generate PKI key pair on target
    Switch Connection  Board A Connection
    ${rc} =  Execute Command  ls /home/root/.ssh/id_rsa.pub >/dev/null 2>&1  return_stdout=False  return_stderr=False  return_rc=True
    ${code} =  Convert To Integer  ${rc}
    Run Keyword If  ${code} == ${0}  Return from keyword
    ${rc} =  Execute Command  ssh-keygen -N "" -q -f /home/root/.ssh/id_rsa 2>&1  return_stdout=False  return_stderr=False  return_rc=True
    ${code} =  Convert To Integer  ${rc}
    Run Keyword If  ${code} != ${0}  Fail  Failed to create PKI key pair
    Log  Created PKI key pair on target A


Copy PKI ID from board A to board B
    Switch Connection  Board A Connection
    ${res} =  Run Remote Command And Compare Status  cat /home/root/.ssh/id_rsa.pub  0
    ${public key} =  Set variable  ${res.stdout}
    Switch Connection  Board B Connection
    Run Remote Command And Compare Status  mkdir -p /home/root/.ssh  0
    ${rc} =  Execute Command  grep -q '${public key}' /home/root/.ssh/authorized_keys 2>/dev/null  return_stdout=False  return_stderr=False  return_rc=True
    ${code} =  Convert To Integer  ${rc}
    Run Keyword If  ${code} == ${0}  Return from keyword
    Log  Updating authorized_keys on target B
    Run Remote Command And Compare Status  echo ${public key} >> /home/root/.ssh/authorized_keys  0
    Switch Connection  Board A Connection
    Run Remote Command And Compare Status  ssh -o StrictHostKeyChecking=no 10.125.233.146 uname -r  0
    Log  Created passwordless connection from target A to target B


Suite Setup
    Get Targets Information
    Establish connections
    Generate PKI key pair on target
    Copy PKI ID from board A to board B
    Get target TSN interface names
    Setup TCC Settings on target  Board A Connection
    Setup TCC Settings on target  Board B Connection


Test Teardown
    # Make sure the opcua_server instances are finished
    Switch Connection  Board A Connection
    Run Remote Command  pkill opcua_server
    Switch Connection  Board B Connection
    Run Remote Command  pkill opcua_server


Setup TCC Settings on target
    [Arguments]  ${target connection}
    # Check if the TCC config file already exists
    Switch Connection  ${target connection}
    ${rc} =  Execute Command  ls /home/root/.tcc.config 2>/dev/null 1>&2  return_stdout=False  return_stderr=False  return_rc=True
    ${code} =  Convert To Integer  ${rc}
    Run Keyword If  ${code} == ${0}  Return from keyword
    Run Remote Command And Compare Status  cp /usr/share/tcc_tools/config/.tcc.config /home/root/.tcc.config  0
    Run Remote Command And Compare Status  sed -i 's/98304/131072/' /home/root/.tcc.config  0


Get Targets Information
    ${TARGET A HOSTNAME} =  Get Environment Variable  TARGET_A_HOSTNAME
    ${TARGET A PORT} =      Get Environment Variable  TARGET_A_PORT      default=22
    ${TARGET A USERNAME} =  Get Environment Variable  TARGET_A_USERNAME  default=root
    ${TARGET A PASSWORD} =  Get Environment Variable  TARGET_A_PASSWORD  default=fake_pass
    ${TARGET B HOSTNAME} =  Get Environment Variable  TARGET_B_HOSTNAME
    ${TARGET B PORT} =      Get Environment Variable  TARGET_B_PORT      default=22
    ${TARGET B USERNAME} =  Get Environment Variable  TARGET_B_USERNAME  default=root
    ${TARGET B PASSWORD} =  Get Environment Variable  TARGET_B_PASSWORD  default=fake_pass

    Set Suite Variable  ${TARGET A HOSTNAME}
    Set Suite Variable  ${TARGET A PORT}
    Set Suite Variable  ${TARGET A USERNAME}
    Set Suite Variable  ${TARGET A PASSWORD}
    Set Suite Variable  ${TARGET B HOSTNAME}
    Set Suite Variable  ${TARGET B PORT}
    Set Suite Variable  ${TARGET B USERNAME}
    Set Suite Variable  ${TARGET B PASSWORD}
