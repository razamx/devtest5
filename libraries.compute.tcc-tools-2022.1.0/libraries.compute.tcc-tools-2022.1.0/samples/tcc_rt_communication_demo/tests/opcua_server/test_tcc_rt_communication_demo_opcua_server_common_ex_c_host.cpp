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
} // extern "C" {

#include "pcheck.h"
#include "pconfig.h"
#include "opcua_utils.h"

#include "alloc_mock.hpp"
#include "libc_mock.hpp"
#include "json_c_mock.hpp"

#include <tuple>
#include <sys/mman.h>
#include <open62541/server_config.h>
#include <gtest/gtest.h>

#define TEST_BASE_TIME 1234567890

using ::testing::StrEq;
using ::testing::AtMost;
using ::testing::StrictMock;
using ::testing::SetErrnoAndReturn;

struct GenericPayload subscriber_incoming_data;

static PConfig *TheTestConfig = NULL;

void ptimer_atexit(void);

namespace {

class test_opcua_common_mocks : public testing::Test
{
protected:
    void SetUp() override {
        TheTestConfig = NULL;
        log_level = log_error;

        atexit(ptimer_atexit);

        m_libc_mock = new StrictMock<libc_mock>();
        m_alloc_mock = new StrictMock<alloc_mock>();
        m_json_c_mock = new StrictMock<json_c_mock>();

        EXPECT_CALL(*m_alloc_mock, free(_)).WillRepeatedly(Invoke(__real_free));
        EXPECT_CALL(*m_alloc_mock, strdup(_)).WillRepeatedly(Invoke(__real_strdup));
        EXPECT_CALL(*m_libc_mock, fclose(_)).WillRepeatedly(Invoke(__real_fclose));
        EXPECT_CALL(*m_libc_mock, close(_)).WillRepeatedly(Invoke(__real_close));
        EXPECT_CALL(*m_libc_mock, fopen(_, _)).WillRepeatedly(Invoke(__real_fopen));
        EXPECT_CALL(*m_libc_mock, read(_, _, _)).WillRepeatedly(Invoke(__real_read));
        EXPECT_CALL(*m_libc_mock, fread(_, _, _, _)).WillRepeatedly(Invoke(__real_fread));
    }

    void TearDown() override {
        delete m_json_c_mock;
        m_json_c_mock = NULL;
        delete m_alloc_mock;
        m_alloc_mock = NULL;
        delete m_libc_mock;
        m_libc_mock = NULL;

        if (m_json_object) {
            json_object_put(m_json_object);
            m_json_object = NULL;
        }
    }

public:
    static json_object *m_json_object;
    static StrictMock<libc_mock> *m_libc_mock;
    static StrictMock<alloc_mock> *m_alloc_mock;
    static StrictMock<json_c_mock> *m_json_c_mock;
};
json_object *test_opcua_common_mocks::m_json_object = NULL;
StrictMock<libc_mock> *test_opcua_common_mocks::m_libc_mock = NULL;
StrictMock<alloc_mock> *test_opcua_common_mocks::m_alloc_mock = NULL;
StrictMock<json_c_mock> *test_opcua_common_mocks::m_json_c_mock = NULL;


TEST_F(test_opcua_common_mocks, parse_json_null_argument_negative) {
    auto test = [this] {
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));

        parseJson(nullptr, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Wrong argument");
}

TEST_F(test_opcua_common_mocks, parse_json_null_argument_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));

        parseJson(nullptr, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_json_alloc_failure_negative) {
    auto test = [this] {
        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(nullptr));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        m_json_object = json_tokener_parse("{}");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Out of memory");
}

TEST_F(test_opcua_common_mocks, parse_json_alloc_failure_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(nullptr));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        m_json_object = json_tokener_parse("{}");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_json_no_publisher_interface_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        m_json_object = json_tokener_parse("{}");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'publisher_interface' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_subscriber_interface_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0"})json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'subscriber_interface' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_use_xdp_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0"})json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'use_xdp' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_polling_duration_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0", 
            "subscriber_interface": "enp2s0",
            "use_xdp": true})json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'polling_duration_ns' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_polling_duration_with_cpu_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3})json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'polling_duration_ns' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_packet_count_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0})json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'packet_count' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_cycle_time_ns_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'cycle_time_ns' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_publishers_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000 })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'publishers' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_subscribers_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {} })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'subscribers' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_no_roles_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {} })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Necessary roles of data manager are missing");
}

TEST_F(test_opcua_common_mocks, parse_json_no_roles_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {} })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_no_affinity_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {} })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'affinity' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_no_iterations_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'iterations' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_no_start_offset_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'start-offset' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_no_period_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0 } }")json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'period' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_no_tcc_settings_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'tcc_settings' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_without_latency_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'latency' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_early_offset_ns_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'early_offset_ns' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_publish_offset_ns_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'publish_offset_ns' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_socket_prio_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'socket_prio' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_xdp_queue_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'xdp_queue' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_url_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4, 
                "xdp_queue": 2 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'url' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_pub_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4,
                "xdp_queue": 2,
                "url": "opc.eth://20-20-20-20-20-21" } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'pub_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_dataset_writer_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4,
                "xdp_queue": 2,
                "url": "opc.eth://20-20-20-20-20-21",
                "pub_id": 1234 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'dataset_writer_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_writer_group_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4,
                "xdp_queue": 2,
                "url": "opc.eth://20-20-20-20-20-21",
                "pub_id": 1234,
                "dataset_writer_id": 45678 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'writer_group_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_cpu_affinity_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4,
                "xdp_queue": 2,
                "url": "opc.eth://20-20-20-20-20-21",
                "pub_id": 1234,
                "dataset_writer_id": 45678,
                "writer_group_id": 35123 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'cpu_affinity' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_publisher_no_latency_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": { "pub1": {
                "early_offset_ns": 0,
                "publish_offset_ns": 10000,
                "socket_prio": 4,
                "xdp_queue": 2,
                "url": "opc.eth://20-20-20-20-20-21",
                "pub_id": 1234,
                "dataset_writer_id": 45678,
                "writer_group_id": 35123,
                "cpu_affinity": 2 } },
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'latency' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_offset_ns_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {} },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'offset_ns' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_url_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'url' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_xdp_queue_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23" } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'xdp_queue' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_cpu_affinity_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23",
                "xdp_queue": 5 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'cpu_affinity' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_sub_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23",
                "xdp_queue": 5,
                "cpu_affinity": 1 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'sub_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_subscribed_pub_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23",
                "xdp_queue": 5,
                "cpu_affinity": 1,
                "sub_id": 4 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'subscribed_pub_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_subscribed_dataset_writer_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23",
                "xdp_queue": 5,
                "cpu_affinity": 1,
                "sub_id": 4,
                "subscribed_pub_id": 14345 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'subscribed_dataset_writer_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_subscribed_writer_group_id_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23",
                "xdp_queue": 5,
                "cpu_affinity": 1,
                "sub_id": 4,
                "subscribed_pub_id": 14345,
                "subscribed_dataset_writer_id": 73912 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
                "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'subscribed_writer_group_id' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_with_subscriber_no_latency_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": { "sub1": {
                "offset_ns": 20000,
                "url": "opc.eth://20-20-20-20-20-23",
                "xdp_queue": 5,
                "cpu_affinity": 1,
                "sub_id": 4,
                "subscribed_pub_id": 14345,
                "subscribed_dataset_writer_id": 73912,
                "subscribed_writer_group_id": 32145 } },
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "tcc_settings": { } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'latency' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_positive) {
    ServerData serverData = {};

    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000 ,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "talker": {
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": 100, "affinity": 2 } })json");

    testing::internal::CaptureStdout();
    ServerData *const outServerData = parseJson(m_json_object, 0);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(outServerData, &serverData);
    EXPECT_EQ(outServerData->tcc_latency, 100);
    EXPECT_EQ(captured.find("Parsing OPC UA* server config"), std::string::npos);
    EXPECT_EQ(captured.find("Found 0 publishers and 0 subscribers"), std::string::npos);
    EXPECT_EQ(captured.find("Found: pub if enp1s0, sub if enp2s0, cycleTimeNs 500000 pollNs 0, packet_count 2000"), std::string::npos);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_json_talker_check_info_positive) {
    ServerData serverData = {};

    log_level = log_info;
    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000 ,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "talker": {
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": 100, "affinity": 2 } })json");

    testing::internal::CaptureStdout();
    ServerData *const outServerData = parseJson(m_json_object, 0);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(outServerData, &serverData);
    EXPECT_EQ(outServerData->tcc_latency, 100);
    EXPECT_NE(captured.find("Parsing OPC UA* server config"), std::string::npos);
    EXPECT_NE(captured.find("Found 0 publishers and 0 subscribers"), std::string::npos);
    EXPECT_NE(captured.find("Found: pub if enp1s0, sub if enp2s0, cycleTimeNs 500000 pollNs 0, packet_count 2000"), std::string::npos);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_json_talker_negative_latency_positive) {
    ServerData serverData = {};

    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000 ,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "talker": {
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": -100, "affinity": 2 } })json");

    testing::internal::CaptureStdout();
    ServerData *const outServerData = parseJson(m_json_object, 0);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(captured, "");
    EXPECT_EQ(outServerData, &serverData);
    EXPECT_EQ(outServerData->tcc_latency, 25);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_json_talker_negative_latency_with_logs_positive) {
    ServerData serverData = {};
    log_level = log_info;

    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000 ,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "talker": {
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": -100, "affinity": 2 } })json");

    testing::internal::CaptureStdout();
    ServerData *const outServerData = parseJson(m_json_object, 0);
    const std::string &captured = testing::internal::GetCapturedStdout();

    EXPECT_EQ(outServerData, &serverData);
    EXPECT_EQ(outServerData->tcc_latency, 25);
    EXPECT_NE(captured.find("Detected invalid workload latency, using default latency of 25ns"), std::string::npos);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_json_talker_and_compute_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "compute": {
                "flavor": "talker",
                "output-file": "1.log",
                "affinity": 2,
                "iterations": 2000,
                "start-offset": 10,
                "period": 500000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Mutual exclusive roles on one side");
}

TEST_F(test_opcua_common_mocks, parse_json_talker_and_compute_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "talker": {
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "compute": {
                "flavor": "talker",
                "output-file": "1.log",
                "affinity": 2,
                "iterations": 2000,
                "start-offset": 10,
                "period": 500000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_json_listerner_and_compute_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "flavor": "talker",
                "output-file": "1.log",
                "affinity": 3,
                "iterations": 2000,
                "start-offset": 0,
                "period": 500000 },
            "compute": {
                "flavor": "compute",
                "output-file": "2.log",
                "affinity": 2,
                "iterations": 2000,
                "start-offset": 10,
                "period": 500000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Mutual exclusive roles on one side");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_no_affinity_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {  },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'affinity' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_no_start_offset_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'start-offset' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_no_iterations_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3,
                "start-offset": 10000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'iterations' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_no_output_file_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3,
                "start-offset": 10000,
                "iterations": 2000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'output-file' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_no_flavor_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3,
                "start-offset": 10000,
                "iterations": 2000,
                "output-file": "out.log" },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'flavor' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_wrong_flavor_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3,
                "start-offset": 10000,
                "iterations": 2000,
                "output-file": "out.log",
                "flavor": "wrong" },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Listener's flavor was not recognized");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_wrong_flavor_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3,
                "start-offset": 10000,
                "iterations": 2000,
                "output-file": "out.log",
                "flavor": "wrong" },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_no_period_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "listener": {
                "affinity": 3,
                "start-offset": 10000,
                "iterations": 2000,
                "output-file": "out.log",
                "flavor": "talker" },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'period' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_listener_positive) {
    ServerData serverData = {};

    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000 ,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "listener": {
            "flavor": "talker",
            "output-file": "1.log",
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": 100, "affinity": 2 } })json");

    ServerData *const outServerData = parseJson(m_json_object, 0);

    EXPECT_EQ(outServerData, &serverData);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_json_compute_no_affinity_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "compute": { },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'affinity' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_compute_no_output_file_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "compute": {
                "affinity": 3 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'output-file' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_compute_no_iterations_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000, 
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "compute": {
                "affinity": 3,
                "output-file": "out.log" },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'iterations' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_compute_no_start_offset_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "compute": {
                "affinity": 3,
                "output-file": "out.log",
                "iterations": 2000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'start-offset' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_compute_no_period_negative) {
    auto test = [this] {
        ServerData serverData = {};

        EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
            .WillOnce(Return(&serverData));
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, get_nprocs())
            .WillOnce(Return(3));
        m_json_object = json_tokener_parse(
            R"json({"publisher_interface": "enp1s0",
            "subscriber_interface": "enp2s0",
            "use_xdp": true,
            "cpu": 3,
            "polling_duration_ns": 0,
            "packet_count": 2000 ,
            "cycle_time_ns": 500000,
            "publishers": {},
            "subscribers": {},
            "compute": {
                "affinity": 3,
                "output-file": "out.log",
                "iterations": 2000,
                "start-offset": 20000 },
            "tcc_settings": { "latency": 100, "affinity": 2 } })json");

        parseJson(m_json_object, 0);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Key 'period' not found in object");
}

TEST_F(test_opcua_common_mocks, parse_json_compute_positive) {
    ServerData serverData = {};

    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "compute": {
            "affinity": 3,
            "output-file": "out.log",
            "iterations": 2000,
            "start-offset": 20000,
            "period": 500000 },
        "tcc_settings": { "latency": 100, "affinity": 2 } })json");

    ServerData *const outServerData = parseJson(m_json_object, 0);

    EXPECT_EQ(outServerData, &serverData);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_json_listener_and_talker_positive) {
    ServerData serverData = {};

    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    m_json_object = json_tokener_parse(
        R"json({"publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000 ,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "talker": {
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "listener": {
            "flavor": "compute",
            "output-file": "1.log",
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": 100, "affinity": 2 } })json");

    ServerData *const outServerData = parseJson(m_json_object, 0);

    EXPECT_EQ(outServerData, &serverData);
    cleanServerData(outServerData);
}

TEST_F(test_opcua_common_mocks, parse_args_no_config_file_argument_negative) {
    auto test = [this] {
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));

        std::vector<const char *> args({ "tcc_rt_communication_demo", "--log-level", "error" });
        int argc = args.size();
        char **argv = const_cast<char **>(args.data());

        parseArgs(argc, argv);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Path to json not specified");
}

TEST_F(test_opcua_common_mocks, parse_args_no_config_file_argument_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));

        std::vector<const char *> args({ "tcc_rt_communication_demo", "--log-level", "error" });
        int argc = args.size();
        char **argv = const_cast<char **>(args.data());

        parseArgs(argc, argv);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_args_no_config_file_access_negative) {
    auto test = [this] {
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, access(_, _))
            .WillOnce(SetErrnoAndReturn(EACCES, -1));

        std::vector<const char *> args({ "tcc_rt_communication_demo", "--log-level", "error", "--config-file", "/folder/file" });
        int argc = args.size();
        char **argv = const_cast<char **>(args.data());

        parseArgs(argc, argv);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "Cannot access '/folder/file' file");
}

TEST_F(test_opcua_common_mocks, parse_args_no_config_file_access_no_logs_negative) {
    auto test = [this] {
        log_level = log_undef;
        EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
            .WillRepeatedly(Invoke(__real_vfprintf));
        EXPECT_CALL(*m_libc_mock, access(_, _))
            .WillOnce(SetErrnoAndReturn(EACCES, -1));

        std::vector<const char *> args({ "tcc_rt_communication_demo", "--log-level", "error", "--config-file", "/folder/file" });
        int argc = args.size();
        char **argv = const_cast<char **>(args.data());

        parseArgs(argc, argv);
    };

    EXPECT_EXIT(test(), ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

TEST_F(test_opcua_common_mocks, parse_args_positive) {
    ServerData serverData = {};

    EXPECT_CALL(*m_libc_mock, access(_, _))
        .WillOnce(Return(0));
    EXPECT_CALL(*m_alloc_mock, calloc(1, sizeof(struct ServerData)))
        .WillOnce(Return(&serverData));
    EXPECT_CALL(*m_libc_mock, fprintf(_, _, _))
        .WillRepeatedly(Invoke(__real_vfprintf));
    EXPECT_CALL(*m_libc_mock, get_nprocs())
        .WillOnce(Return(3));
    json_object *json_object = json_tokener_parse(
        R"json({"run-apps": { "opcua_server": {
        "publisher_interface": "enp1s0",
        "subscriber_interface": "enp2s0",
        "use_xdp": true,
        "cpu": 3,
        "polling_duration_ns": 0,
        "packet_count": 2000,
        "cycle_time_ns": 500000,
        "publishers": {},
        "subscribers": {},
        "talker": {
            "affinity": 3,
            "iterations": 2000,
            "start-offset": 0,
            "period": 500000 },
        "tcc_settings": { "latency": 100, "affinity": 2 } } } })json");
    EXPECT_CALL(*m_json_c_mock, json_object_from_file(_))
        .WillOnce(Return(json_object));

    std::vector<const char *> args({ "tcc_rt_communication_demo", "--config-file", "/folder/file",
                                     "--log-level", "error", "--base-time", stringify(TEST_BASE_TIME) });
    int argc = args.size();
    char **argv = const_cast<char **>(args.data());

    ServerData *const outServerData = parseArgs(argc, argv);

    EXPECT_EQ(outServerData, &serverData);
    EXPECT_EQ(outServerData->baseTime, TEST_BASE_TIME);
    cleanServerData(outServerData);
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

void ptimer_atexit(void)
{
    bool success = true;
    if (test_opcua_common_mocks::m_libc_mock)
    {
        success = success && test_opcua_common_mocks::m_libc_mock->verify_and_clear();
        delete test_opcua_common_mocks::m_libc_mock;
        test_opcua_common_mocks::m_libc_mock = NULL;
    }
    if (test_opcua_common_mocks::m_alloc_mock)
    {
        success = success && test_opcua_common_mocks::m_alloc_mock->verify_and_clear();
        delete test_opcua_common_mocks::m_alloc_mock;
        test_opcua_common_mocks::m_alloc_mock = NULL;
    }
    if (test_opcua_common_mocks::m_json_c_mock)
    {
        success = success && test_opcua_common_mocks::m_json_c_mock->verify_and_clear();
        delete test_opcua_common_mocks::m_json_c_mock;
        test_opcua_common_mocks::m_json_c_mock = NULL;
    }
    if (test_opcua_common_mocks::m_json_object) {
        json_object_put(test_opcua_common_mocks::m_json_object);
        test_opcua_common_mocks::m_json_object = NULL;
    }

    if (!success)
    {
        // Exit with different code, if verification fails
        _exit(2);
    }
}
