#!/usr/bin/env robot

*** Variables ***
${RESOURCES}           ${CURDIR}
${SCRIPT}              tcc_setup.py
${TARGET_ARCHIVE}      tcc_tools_target*.tar.gz
${SETUP_DIRECTORY}     /tmp/test_setup
${VERSION_FILE}        share/tcc_tools/version.txt

*** Settings ***
Library           OperatingSystem
Library           String
Library           ${RESOURCES}/robot/lib/save_env.py
Resource          ${RESOURCES}/robot/util/extensions.robot

Suite Setup       Run Keywords
...               Save Environment Variables    AND
...               Get Target Path               AND
...               Change Home Path
Suite Teardown    Restore Environment Variables

Test Setup        Create Setup Directory
Test Teardown     Remove Setup Directory

Force Tags        TARGET_INSTALLATION_SCRIPT    HOST

*** Keywords ***

Get Target Path
    Check Host Environment
    Set Common Pathes
    Set Suite Variable    ${TCC_TARGET_PATH}    ${TCC_ROOT}/target

Change Home Path
    Set Environment Variable    HOME    ${SETUP_DIRECTORY}

Create Setup Directory
    Create Directory    ${SETUP_DIRECTORY}

Remove Setup Directory
    Remove Directory    ${SETUP_DIRECTORY}    recursive=True

Set Installed Files To Path
    ${PATH}=    Get Environment Variable    PATH
    ${PATH}=    Replace String    ${PATH}    ${TCC_ROOT}/bin    ${SETUP_DIRECTORY}/bin
    Set Environment Variable    PATH    ${PATH}
    ${CPATH}=    Get Environment Variable    CPATH
    ${CPATH}=    Replace String    ${CPATH}    ${TCC_ROOT}/include    ${SETUP_DIRECTORY}/include
    Set Environment Variable    CPATH    ${CPATH}
    ${LIBRARY_PATH}=    Get Environment Variable    LIBRARY_PATH
    ${LIBRARY_PATH}=    Replace String    ${LIBRARY_PATH}    ${TCC_ROOT}/lib64    ${SETUP_DIRECTORY}/lib64
    Set Environment Variable    LIBRARY_PATH    ${LIBRARY_PATH}
    ${LD_LIBRARY_PATH}=    Get Environment Variable    LD_LIBRARY_PATH
    ${LD_LIBRARY_PATH}=    Replace String    ${LD_LIBRARY_PATH}    ${TCC_ROOT}/lib64    ${SETUP_DIRECTORY}/lib64
    Set Environment Variable    LD_LIBRARY_PATH    ${LD_LIBRARY_PATH}

*** Test Cases ***
Call Help
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -h    0

Install Files To Custom Directory
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -i ${TCC_TARGET_PATH}/${TARGET_ARCHIVE} -r ${SETUP_DIRECTORY}    0
    Directory Should Exist    ${SETUP_DIRECTORY}/bin
    Directory Should Exist    ${SETUP_DIRECTORY}/lib64
    Directory Should Exist    ${SETUP_DIRECTORY}/share
    Directory Should Exist    ${SETUP_DIRECTORY}/include

Uninstall From Custom Directory
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -i ${TCC_TARGET_PATH}/${TARGET_ARCHIVE} -r ${SETUP_DIRECTORY}    0
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -u    0
    Directory Should Not Exist    ${SETUP_DIRECTORY}/bin
    Directory Should Not Exist    ${SETUP_DIRECTORY}/lib64
    Directory Should Not Exist    ${SETUP_DIRECTORY}/share
    Directory Should Not Exist    ${SETUP_DIRECTORY}/include

Uninstall With Forse Option
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -i ${TCC_TARGET_PATH}/${TARGET_ARCHIVE} -r ${SETUP_DIRECTORY}    0
    Create File    ${SETUP_DIRECTORY}/share/tcc_tools/user_file.txt    "Some user file"
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -u -f    0
    File Should Not Exist    ${SETUP_DIRECTORY}/share/tcc_tools/user_file.txt
    Directory Should Not Exist    ${SETUP_DIRECTORY}/bin
    Directory Should Not Exist    ${SETUP_DIRECTORY}/lib64
    Directory Should Not Exist    ${SETUP_DIRECTORY}/share
    Directory Should Not Exist    ${SETUP_DIRECTORY}/include

Script Can Reinstall Binaries
    [Documentation]    Check is product will be reinstalled. Removed file as indicator
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -i ${TCC_TARGET_PATH}/${TARGET_ARCHIVE} -r ${SETUP_DIRECTORY}    0
    # TODO: Print something to file. Check that reinstalled files doesn't contain it.
    Remove File    ${SETUP_DIRECTORY}/${VERSION_FILE}
    File Should Not Exist    ${SETUP_DIRECTORY}/${VERSION_FILE}
    Run Command And Compare Status    echo -n 'y' | ${TCC_TARGET_PATH}/${SCRIPT} -i ${TCC_TARGET_PATH}/${TARGET_ARCHIVE} -r ${SETUP_DIRECTORY}    0
    File Should Exist    ${SETUP_DIRECTORY}/${VERSION_FILE}

Install Symlinks Correct
    Run Command And Compare Status    ${TCC_TARGET_PATH}/${SCRIPT} -i ${TCC_TARGET_PATH}/${TARGET_ARCHIVE} -r ${SETUP_DIRECTORY}    0
    Save Environment Variables
    Set Installed Files To Path
    Run Command And Compare Status    echo -n 'y' | tcc_rt_checker    0
    Restore Environment Variables
