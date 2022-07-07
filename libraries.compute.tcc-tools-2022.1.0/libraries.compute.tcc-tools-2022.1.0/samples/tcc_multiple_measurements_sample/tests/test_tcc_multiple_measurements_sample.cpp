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
#include <gtest/gtest.h>
#include <string>
#include <vector>
#define main main_for_testing
extern "C" {
	#include "../src/main.c"
}

class TestParser : public testing::TestWithParam<std::vector<std::string>> {
protected:
	int argc;
	struct settings_t sample_settings;
	std::vector<char *> argv;
	ParamType tmp = GetParam();
	void SetUp() override {
	sample_settings= {
		.approximation = 0,
		.multiplication = 0,
		.iterations = 1000
	};
	tmp.emplace(tmp.begin(), std::string("tcc_multiple_measurements_sample"));
	argv = std::vector<char *>(tmp.size());
	for (size_t i = 0; i < tmp.size(); i++) {
		argv[i] = tmp[i].data();
	}
	argc = argv.size();
	}
	void TearDown() override {
		optind = 0;
	};
};
class ParserPositive : public TestParser {};
class ParserNegative : public TestParser {};

TEST_P(ParserPositive, ) {
	EXPECT_EQ(parse_args(argc, argv.data(), &sample_settings), TCC_PARSE_SUCCESS);
};
TEST_P(ParserNegative, ) {
	EXPECT_EQ(parse_args(argc, argv.data(), &sample_settings), TCC_PARSE_ERROR);
};

std::vector<std::vector<std::string>> params_pos = {
	std::vector<std::string>({"-a", "5", "-m", "5", "-i", "10"}),
	std::vector<std::string>({"-a", "100", "-m", "100"}),
	std::vector<std::string>({"--approximation", "10", "-m", "100"}),
	std::vector<std::string>({"--approximation", "10", "-m", "100", "-i", "10"}),
	std::vector<std::string>({"--approximation", "10", "-m", "100", "--iterations", "10"}),
	std::vector<std::string>({"-a", "10", "--multiplication", "100"}),
	std::vector<std::string>({"-a", "10", "--multiplication", "100", "-i", "10"}),
	std::vector<std::string>({"-a", "10", "--multiplication", "100", "--iterations", "10"}),
	std::vector<std::string>({"--approximation", "10", "--multiplication", "100"}),
	std::vector<std::string>({"--approximation", "10", "--multiplication", "100", "-i", "10"}),
	std::vector<std::string>({"--approximation", "10", "--multiplication", "100", "--iterations", "10"})
};
std::vector<std::vector<std::string>> params_neg = {
	std::vector<std::string>({" "}),
	std::vector<std::string>({"-a", "10"}),
	std::vector<std::string>({"-m", "10"}),
	std::vector<std::string>({"-i", "10"}),
	std::vector<std::string>({"---a", "10", "-m", "10"}),
	std::vector<std::string>({"-ami", "10"}),
	std::vector<std::string>({"-m"}),
	std::vector<std::string>({"-a", "e10", "-m", "10"}),
	std::vector<std::string>({"-a", "e", "-m", "10"}),
	std::vector<std::string>({"--approximatioN", "10", "-m", "10"}),
	std::vector<std::string>({"--approximationn", "10", "-m", "10"}),
	std::vector<std::string>({"-a", "-10", "-m", "10", "-i", "1" }),
	std::vector<std::string>({"-a", "10", "-m", "-10", "-i", "1" }),
	std::vector<std::string>({"-a", "10", "-m", "10", "-i", "-1" })
};

INSTANTIATE_TEST_CASE_P(ParserGroup, ParserPositive, testing::ValuesIn(params_pos));
INSTANTIATE_TEST_CASE_P(ParserGroup, ParserNegative, testing::ValuesIn(params_neg));
