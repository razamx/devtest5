/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

extern "C" {
#include "opcua_common.c"
} // extern "C"

#include "pconfig.h"
#include "opcua_utils.h"

#include <tuple>
#include <sys/mman.h>
#include <open62541/server_config.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::StrEq;
using ::testing::MatchesRegex;
using ::testing::AtMost;

#define OPCUA_COMMON_TEST_URL "test-url"
#define OPCUA_COMMON_TEST_FILENAME "filename.txt"
#define OPCUA_COMMON_TEST_FILENAME_2 "new-filename.txt"
#define OPCUA_COMMON_TEST_PUB_INTERFACE "publisher-interface"
#define OPCUA_COMMON_TEST_SUB_INTERFACE "subscriber-interface"

struct GenericPayload subscriber_incoming_data;

static PConfig *TheTestConfig = NULL;

namespace {

/**
 * Test fixture class for basic UT for OPC UA* Common tools
 */
class test_opcua_common_c : public testing::Test
{
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

/**
 * Test fixture class for testing the log level conversion
 */
class test_opcua_common_debug_argument :
    public testing::TestWithParam<std::tuple<const char *, int> >
{
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

/**
 * The test fixture class for parsing CLI arguments
 */
class test_opcua_common_c_parseArgs :
    public testing::TestWithParam<std::tuple<std::vector<const char *>, std::string>>
{
protected:

    void SetUp() override {
    }

    void TearDown() override {
    }

public:
    static const std::string usage_string;
};

const std::string test_opcua_common_c_parseArgs::usage_string =
    "\nUsage: tcc_rt_communication_demo \\[OPTIONS\\]\n\n"
    "\t-c, --config-file=<configuration file>\n"
    "\t\tMandatory option\\.\n"
    "\t\tDefine name of file in JSON format with configuration for tcc_rt_communication_demo\\.\n\n"
    "\t-b, --base-time=<requested-UTC-base-time>\n"
    "\t\tOptional argument to define the UTC start time for network communications\n\n"
    "\t-l, --log-level=<log level>\n"
    "\t\tDefine log level which will be used for output from tcc_rt_communication_demo\\.\n"
    "\t\tPossible log levels are: debug, info, error\\.\n"
    "\t\tLog level error is set as per default\\.\n\n"
    "\t-h, --help\n"
    "\t\tShow this help message\\.\n\n"
    "Example: tcc_rt_communication_demo --config-file=opcua_server_config\\.json"
    " --log-level=info\n\n";

/**
 * The test fixture class for testing time conversion
 */
class test_opcua_common_as_nanoseconds :
    public testing::TestWithParam<std::tuple<timespec, uint64_t> >
{
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_P(test_opcua_common_c_parseArgs, parseArgs_prints_help_msg_and_exits_mostly_negative)
{
    std::vector<const char *> vector = std::get<0>(GetParam());
    int argc = vector.size();
    char **argv = const_cast<char **>(vector.data());
    const std::string &output = std::get<1>(GetParam());

    // Checking...
    EXPECT_EXIT(
        parseArgs(argc, argv),
        ::testing::ExitedWithCode(EXIT_FAILURE),
        output
    );
}

TEST_P(test_opcua_common_c_parseArgs, parseArgs_prints_help_msg_and_exits_negative)
{
    EXPECT_EXIT(
        parseJson(nullptr, 0),
        ::testing::ExitedWithCode(EXIT_FAILURE),
        ""
    );
}

INSTANTIATE_TEST_CASE_P(
    parseArgs_prints_usage_scenarios,
    test_opcua_common_c_parseArgs,
    ::testing::Values(
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo" }),
            test_opcua_common_c_parseArgs::usage_string
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "-h" }),
            test_opcua_common_c_parseArgs::usage_string
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--help" }),
            test_opcua_common_c_parseArgs::usage_string
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "-c" }),
            (
                std::string("tcc_rt_communication_demo: option requires an argument -- 'c'\n") +
                test_opcua_common_c_parseArgs::usage_string
            )
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--config-file" }),
            (
                std::string("tcc_rt_communication_demo: option '--config-file' requires an argument\n") +
                test_opcua_common_c_parseArgs::usage_string
            )
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "-l" }),
            (
                std::string("tcc_rt_communication_demo: option requires an argument -- 'l'\n") +
                test_opcua_common_c_parseArgs::usage_string
            )
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--log-level" }),
            (
                std::string("tcc_rt_communication_demo: option '--log-level' requires an argument\n") +
                test_opcua_common_c_parseArgs::usage_string
            )
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--log-level", "debug", "-h" }),
            test_opcua_common_c_parseArgs::usage_string
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--log-level", "info", "-h" }),
            test_opcua_common_c_parseArgs::usage_string
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--log-level", "error", "-h" }),
            test_opcua_common_c_parseArgs::usage_string
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--log-level", "unknown", "-h" }),
            std::string("Provided unknown log level name 'unknown'. Type \\[-h\\] to see usage information")
        ),
        std::make_tuple(
            std::vector<const char *>({ "tcc_rt_communication_demo", "--config-file", "/folder/file", "--help" }),
            test_opcua_common_c_parseArgs::usage_string
        )
    )
);

TEST_F(test_opcua_common_c, clean_pub_data_positive)
{
    PublisherData data = {};
    data.url = strdup(OPCUA_COMMON_TEST_URL);

    cleanPubData(&data);
}

TEST_F(test_opcua_common_c, clean_pub_data_null_data_positive)
{
    cleanPubData(NULL);
}

TEST_F(test_opcua_common_c, clean_sub_data_data_positive)
{
    SubscriberData data = {};
    data.url = strdup(OPCUA_COMMON_TEST_URL);
    data.targetVars = malloc(128);
    data.pMetaData_fields = malloc(64);

    cleanSubData(&data);
}

TEST_F(test_opcua_common_c, clean_sub_data_null_data_positive)
{
    cleanSubData(NULL);
}

TEST_F(test_opcua_common_c, clean_data_manager_data_positive)
{
    listener_config.pLogFile = strdup(OPCUA_COMMON_TEST_FILENAME);
    compute_config.pLogFile = strdup(OPCUA_COMMON_TEST_FILENAME_2);

    cleanDataMgrData();
}

TEST_F(test_opcua_common_c, clean_data_manager_data_null_positive)
{
    compute_config.pLogFile = NULL;
    listener_config.pLogFile = NULL;

    cleanDataMgrData();
}

TEST_F(test_opcua_common_c, clean_server_data_with_publisher_and_subscriber_positive)
{
    ServerData data = {};
    data.pubInterface = strdup(OPCUA_COMMON_TEST_PUB_INTERFACE);
    data.subInterface = strdup(OPCUA_COMMON_TEST_SUB_INTERFACE);
    // Add publisher data
    PublisherData *pubData = reinterpret_cast<PublisherData *>(malloc(sizeof (PublisherData)));
    pubData->url = strdup(OPCUA_COMMON_TEST_URL);
    data.pubCount = 1;
    data.pubData = pubData;
    // Add subscriber data
    SubscriberData *subData = reinterpret_cast<SubscriberData *>(malloc(sizeof (SubscriberData)));
    subData->targetVars = malloc(128);
    subData->pMetaData_fields = malloc(64);
    subData->url = strdup(OPCUA_COMMON_TEST_URL);
    data.subCount = 1;
    data.subData = subData;

    cleanServerData(&data);
}

TEST_F(test_opcua_common_c, clean_server_data_positive)
{
    ServerData data = {};
    data.pubInterface = strdup(OPCUA_COMMON_TEST_PUB_INTERFACE);
    data.subInterface = strdup(OPCUA_COMMON_TEST_SUB_INTERFACE);

    cleanServerData(&data);
}

TEST_F(test_opcua_common_c, clean_server_data_null_positive)
{
    cleanServerData(NULL);
}

INSTANTIATE_TEST_CASE_P(
    test_as_nanoseconds,
    test_opcua_common_as_nanoseconds,
    ::testing::Values(
        std::make_tuple(timespec({ 0, 0 }), 0),
        std::make_tuple(timespec({ 0, 1 }), 1),
        std::make_tuple(timespec({ 1, 0 }), 1000000000),
        std::make_tuple(timespec({ 0x12345678, 0x4abcdef0 }), UINT64_C(305419897253891824))
    )
);

TEST_P(test_opcua_common_as_nanoseconds, as_nanoseconds_positive)
{
    const timespec &argument = std::get<0>(GetParam());
    const uint64_t expected = std::get<1>(GetParam());

    const uint64_t actual = as_nanoseconds(&argument);

    EXPECT_EQ(actual, expected);
}

INSTANTIATE_TEST_CASE_P(
    parse_debug_arguments,
    test_opcua_common_debug_argument,
    ::testing::Values(
        std::make_tuple("debug", log_debug),
        std::make_tuple("info", log_info),
        std::make_tuple("error", log_error),
        std::make_tuple("undef", log_undef),
        std::make_tuple("unknown", log_undef)
    )
);

TEST_P(test_opcua_common_debug_argument, test)
{
    const char *const log_level = std::get<0>(GetParam());
    const int log_level_expected = std::get<1>(GetParam());

    const log_level_t actual_log_level = get_log_level_by_name(log_level);

    EXPECT_EQ(actual_log_level, log_level_expected);
}

} // namespace {

void create_producer(PConfig* config)
{
    TheTestConfig = config;
}

void create_consumer(PConfig* config)
{
    TheTestConfig = config;
}

void create_compute(PConfig* config)
{
    TheTestConfig = config;
}
