#!/usr/bin/env robot

*** Variables ***
${PLOT_SCRIPTS_PATH}    ${EMPTY}
${RESOURCES}            ${CURDIR}
${PYTHON_VERSION}       3

${TEST_DATA}                         ${CURDIR}/data/plot_scripts_tests/tcc_signal_analyzer_plotter
${PLOT_SCRIPTS_RESULTS_DIR}          plot_scripts_results
${SAMPLE}                            tcc_signal_analyzer_plotter.py

*** Settings ***
Library          Process
Library          OperatingSystem
Library          String
Resource         ${RESOURCES}/robot/util/system_util.robot
Resource         ${RESOURCES}/robot/util/extensions.robot
Suite Setup      Run Keywords
...              Check Host Environment    AND
...              Set Common Pathes         AND
...              Prepare Input Files
Force Tags       TGPIO    PLOT_SCRIPT
Default Tags     HOST

*** Keywords ***

Prepare Input Files
    Copy Files    ${TEST_DATA}/*.csv    /tmp
    Set Suite Variable    ${ONE_CH_INPUT_1}    /tmp/input_file_one_ch_1.csv
    Set Suite Variable    ${ONE_CH_INPUT_2}    /tmp/input_file_one_ch_2.csv
    Set Suite Variable    ${TWO_CH_INPUT_1}    /tmp/input_file_two_ch_1.csv
    Set Suite Variable    ${TWO_CH_INPUT_2}    /tmp/input_file_two_ch_2.csv

*** Test Cases ***

#################### Negative cases ############################

Shows usage when called without arguments
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE}
    Run Command And Check Output Contain    ${COMMAND}    1    usage

Shows usage for -h (--help) option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} -h
    Run Command And Check Output Contain    ${COMMAND}    0    usage
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --help
    Run Command And Check Output Contain    ${COMMAND}    0    usage

Fails without required parameters --period or --shift option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} ${ONE_CH_INPUT_1}
    Run Command And Check Output Contain    ${COMMAND}    2    one of the arguments --period --shift is required

Fails when no file is provided with --period option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: log_file

Fails when no file is provided with --shift option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift
    Run Command And Check Output Contain    ${COMMAND}    2    the following arguments are required: log_file

Fails when nonexistent file is provided with --period option
    ${BAD_FILE}=    Set Variable    /no/file/here
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    1    File ${BAD_FILE} not found

Fails when nonexistent file is provided with --shift option
    ${BAD_FILE}=    Set Variable    /no/file/here
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    1    File ${BAD_FILE} not found

Fails when empty file is provided with --period option
    ${BAD_FILE}=    Set Variable    /tmp/empty_file
    Create File    ${BAD_FILE}
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    255    File '${BAD_FILE}' is Empty
    Remove File    ${BAD_FILE}

Fails when empty file is provided with --shift option
    ${BAD_FILE}=    Set Variable    /tmp/empty_file
    Create File    ${BAD_FILE}
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${BAD_FILE}
    Run Command And Check Output Contain    ${COMMAND}    255    File '${BAD_FILE}' is Empty
    Remove File    ${BAD_FILE}

Fails when two channels file is provided with --period option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${TWO_CH_INPUT_1}
    Run Command And Check Output Contain    ${COMMAND}    255    --period mode requires one channel in file

Fails when one channel file is provided with --shift option
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${ONE_CH_INPUT_1}
    Run Command And Check Output Contain    ${COMMAND}    255    --shift mode requires two channels in file

Fails when parameter --output has no argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --output
    Run Command And Check Output Contain    ${COMMAND}    2    argument --output: expected one argument

Fails when parameter --min has no argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --min
    Run Command And Check Output Contain    ${COMMAND}    2    argument --min: expected one argument

Fails when argument for parameter --min is not integer
    ${BAD_ARG}=    Set Variable    string
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --min ${BAD_ARG}
    Run Command And Check Output Contain    ${COMMAND}    2    argument --min: invalid int value: '${BAD_ARG}'

Fails when parameter --max has no argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --max
    Run Command And Check Output Contain    ${COMMAND}    2    argument --max: expected one argument

Fails when argument for parameter --max is not integer
    ${BAD_ARG}=    Set Variable    string
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --max ${BAD_ARG}
    Run Command And Check Output Contain    ${COMMAND}    2    argument --max: invalid int value: '${BAD_ARG}'

Fails when parameter --units has no argument
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --units
    Run Command And Check Output Contain    ${COMMAND}    2    argument --units: expected one argument

Fails when parameter --units has an unsupported argument
    ${BAD_ARG}=    Set Variable    xxx
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --units ${BAD_ARG}
    Run Command And Check Output Contain    ${COMMAND}    2    argument --units: invalid choice: '${BAD_ARG}' (choose from 's', 'ms', 'us', 'ns')

########### Positive cases with --shift option #################

Shows graph without --output option (--shift option is set)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${TWO_CH_INPUT_1} ${TWO_CH_INPUT_2}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    498491.000000 ns
    Should Contain    ${res.stdout}    499494.681546 ns
    Should Contain    ${res.stdout}    508854.000000 ns
    Should Contain    ${res.stdout}    892.810253 ns
    Should Contain    ${res.stdout}    10363.000000 ns

    Should Contain    ${res.stdout}    500014.000000 ns
    Should Contain    ${res.stdout}    500014.602265 ns
    Should Contain    ${res.stdout}    500016.000000 ns
    Should Contain    ${res.stdout}    0.917500 ns
    Should Contain    ${res.stdout}    2.000000 ns

Shows graph without --output option and with custom --units (--shift option is set)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${TWO_CH_INPUT_1} ${TWO_CH_INPUT_2} --units s
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    0.000498 s
    Should Contain    ${res.stdout}    0.000499 s
    Should Contain    ${res.stdout}    0.000509 s
    Should Contain    ${res.stdout}    0.000001 s
    Should Contain    ${res.stdout}    0.000010 s

    Should Contain    ${res.stdout}    0.000500 s
    Should Contain    ${res.stdout}    0.000500 s
    Should Contain    ${res.stdout}    0.000500 s
    Should Contain    ${res.stdout}    0.000000 s
    Should Contain    ${res.stdout}    0.000000 s

Dumps graph to file with --output option (--shift option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--shift.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${TWO_CH_INPUT_1} ${TWO_CH_INPUT_2} --output ${output_file}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    498491.000000 ns
    Should Contain    ${res.stdout}    499494.681546 ns
    Should Contain    ${res.stdout}    508854.000000 ns
    Should Contain    ${res.stdout}    892.810253 ns
    Should Contain    ${res.stdout}    10363.000000 ns

    Should Contain    ${res.stdout}    500014.000000 ns
    Should Contain    ${res.stdout}    500014.602265 ns
    Should Contain    ${res.stdout}    500016.000000 ns
    Should Contain    ${res.stdout}    0.917500 ns
    Should Contain    ${res.stdout}    2.000000 ns

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist    ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--shift.png
    [Tags]    TCCSDK-9031

Dumps graph to file with --output option and with custom --units (--shift option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--shift_--units_s.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${TWO_CH_INPUT_1} ${TWO_CH_INPUT_2} --output ${output_file} --units s
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    0.000498 s
    Should Contain    ${res.stdout}    0.000499 s
    Should Contain    ${res.stdout}    0.000509 s
    Should Contain    ${res.stdout}    0.000001 s
    Should Contain    ${res.stdout}    0.000010 s

    Should Contain    ${res.stdout}    0.000500 s
    Should Contain    ${res.stdout}    0.000500 s
    Should Contain    ${res.stdout}    0.000500 s
    Should Contain    ${res.stdout}    0.000000 s
    Should Contain    ${res.stdout}    0.000000 s

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist         ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--shift_--units_s.png
    [Tags]    TCCSDK-9031

Shows graph without --output option and with custom plot bounds (--shift option is set)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${TWO_CH_INPUT_1} ${TWO_CH_INPUT_2} --min 499000 --max 500000
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    498491.000000 ns
    Should Contain    ${res.stdout}    499494.681546 ns
    Should Contain    ${res.stdout}    508854.000000 ns
    Should Contain    ${res.stdout}    892.810253 ns
    Should Contain    ${res.stdout}    10363.000000 ns

    Should Contain    ${res.stdout}    500014.000000 ns
    Should Contain    ${res.stdout}    500014.602265 ns
    Should Contain    ${res.stdout}    500016.000000 ns
    Should Contain    ${res.stdout}    0.917500 ns
    Should Contain    ${res.stdout}    2.000000 ns

Dumps graph to file with --output option and with custom plot bounds (--shift option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--shift_min_max.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --shift ${TWO_CH_INPUT_1} ${TWO_CH_INPUT_2} --output ${output_file} --min 499000 --max 500000
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    498491.000000 ns
    Should Contain    ${res.stdout}    499494.681546 ns
    Should Contain    ${res.stdout}    508854.000000 ns
    Should Contain    ${res.stdout}    892.810253 ns
    Should Contain    ${res.stdout}    10363.000000 ns

    Should Contain    ${res.stdout}    500014.000000 ns
    Should Contain    ${res.stdout}    500014.602265 ns
    Should Contain    ${res.stdout}    500016.000000 ns
    Should Contain    ${res.stdout}    0.917500 ns
    Should Contain    ${res.stdout}    2.000000 ns

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist         ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--shift_min_max.png
    [Tags]    TCCSDK-9031

########### Positive cases with --period option ################

Shows graph without --output option (--period option is set)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    963309.999999 ns
    Should Contain    ${res.stdout}    1000033.426972 ns
    Should Contain    ${res.stdout}    1037724.000001 ns
    Should Contain    ${res.stdout}    1898.775173 ns
    Should Contain    ${res.stdout}    74414.000002 ns

Shows graph without --output option and with custom --units (--period option is set)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1} --units s
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    0.000963 s
    Should Contain    ${res.stdout}    0.001000 s
    Should Contain    ${res.stdout}    0.001038 s
    Should Contain    ${res.stdout}    0.000002 s
    Should Contain    ${res.stdout}    0.000074 s

Dumps graph to file with --output option (--period option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--period_one_input.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1} --output ${output_file}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    963309.999999 ns
    Should Contain    ${res.stdout}    1000033.426972 ns
    Should Contain    ${res.stdout}    1037724.000001 ns
    Should Contain    ${res.stdout}    1898.775173 ns
    Should Contain    ${res.stdout}    74414.000002 ns

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist         ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--period_one_input.png
    [Tags]    TCCSDK-9031

Dumps graph to file with --output option and with custom --units (--period option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--period_--units_s_one_input.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1} --output ${output_file} --units s
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    0.000963 s
    Should Contain    ${res.stdout}    0.001000 s
    Should Contain    ${res.stdout}    0.001038 s
    Should Contain    ${res.stdout}    0.000002 s
    Should Contain    ${res.stdout}    0.000074 s

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist         ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--period_--units_s_one_input.png
    [Tags]    TCCSDK-9031

Dumps graph to file with --output option and two input files (--period option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--period_two_input.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1} ${ONE_CH_INPUT_2} --output ${output_file}
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    963309.999999 ns
    Should Contain    ${res.stdout}    1000033.426972 ns
    Should Contain    ${res.stdout}    1037724.000001 ns
    Should Contain    ${res.stdout}    1898.775173 ns
    Should Contain    ${res.stdout}    74414.000002 ns

    Should Contain    ${res.stdout}    1000029.999998 ns
    Should Contain    ${res.stdout}    1000033.305704 ns
    Should Contain    ${res.stdout}    1000036.000002 ns
    Should Contain    ${res.stdout}    2.001689 ns
    Should Contain    ${res.stdout}    6.000003 ns

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist         ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--period_two_input.png
    [Tags]    TCCSDK-9031

Shows graph without --output option and with custom plot bounds (--period option is set)
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1} --min 970000 --max 1000000
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    963309.999999 ns
    Should Contain    ${res.stdout}    1000033.426972 ns
    Should Contain    ${res.stdout}    1037724.000001 ns
    Should Contain    ${res.stdout}    1898.775173 ns
    Should Contain    ${res.stdout}    74414.000002 ns

Dumps graph to file with --output option and with custom plot bounds (--period option is set)
    Create Directory    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}
    Set Test Variable    ${output_file}    ${OUTPUT DIR}/${PLOT_SCRIPTS_RESULTS_DIR}/signal_analyzer_plot_--period_one_input_min_max.png
    ${COMMAND}=    Set Variable    python${PYTHON_VERSION} ${PLOT_SCRIPTS_PATH}/${SAMPLE} --period ${ONE_CH_INPUT_1} --output ${output_file} --min 970000 --max 1000000
    ${res}=    Run Command And Compare Status    ${COMMAND}    0

    Should Contain    ${res.stdout}    963309.999999 ns
    Should Contain    ${res.stdout}    1000033.426972 ns
    Should Contain    ${res.stdout}    1037724.000001 ns
    Should Contain    ${res.stdout}    1898.775173 ns
    Should Contain    ${res.stdout}    74414.000002 ns

    Should Contain    ${res.stdout}    Image saved to ${output_file}

    File Should Exist         ${output_file}
    Should Be Equal By Md5    ${output_file}    ${TEST_DATA}/signal_analyzer_plot_--period_one_input_min_max.png
    [Tags]    TCCSDK-9031

