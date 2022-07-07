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
bool operator==(const settings_t& lhs, const settings_t& rhs)
{
	return ((lhs.approximation == rhs.approximation) &&
		(lhs.iterations == rhs.iterations) &&
		(lhs.deadline == rhs.deadline) &&
		(lhs.emulate_outliers == rhs.emulate_outliers) &&
		(lhs.time_unit == rhs.time_unit));
}
/* An alias for a pair where the first member is the command line arguments
   and the second is the expected settings structure for the given arguments */
using TestPair = std::pair<std::vector<std::string>, settings_t>;
class TestParser : public testing::TestWithParam<TestPair> {
protected:
	int argc;
	std::vector<char *> argv;
	settings_t received_settings;
	settings_t expected_settings;
	TestPair tmp;

	void SetUp() override {
		tmp = GetParam();
		received_settings= {
			.approximation = 0,
			.iterations = 1000,
			.deadline = 0,
			.emulate_outliers = 0,
			.time_unit = TCC_TU_CLK
		};
		expected_settings = std::move(tmp.second);
 		/* First argument is the name of the program */
		tmp.first.emplace(tmp.first.begin(), std::string("tcc_single_measurement_sample"));
		argv = std::vector<char *>(tmp.first.size());
		for (size_t i = 0; i < tmp.first.size(); i++) {
			argv[i] = tmp.first[i].data();
		}
		argc = argv.size();
	}
	void TearDown() override {
		/* Reinitialize getopt_long() by resetting optind to 0 */
		optind = 0;
	};
};
class ParserPositive : public TestParser {};
class ParserNegative : public TestParser {};

TEST_P(ParserPositive, ) {
	EXPECT_EQ(parse_args(argc, argv.data(), &received_settings), TCC_PARSE_SUCCESS);
	EXPECT_EQ(received_settings, expected_settings);
};
TEST_P(ParserNegative, ) {
	EXPECT_EQ(parse_args(argc, argv.data(), &received_settings), TCC_PARSE_ERROR);
};

std::vector<TestPair> params_pos = {
	{{"-a", "10"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 0,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_CLK
	}},
	{{"--approximation", "10"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 0,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_CLK
	}},
	{{"-a", "10", "-i 100"},
	{	.approximation = 10,
		.iterations = 100,
		.deadline = 0,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_CLK
	}},
	{{"-a", "10", "-o"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 0,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_CLK
	}},
	{{"-a", "10", "--emulate-outliers"},
 	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 0,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_CLK
	}},
	{{"-a", "10", "-d", "1,ns"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 1,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_NS
	}},
 	{{"-a", "10", "-d", " 100,us"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 100,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_US
	}},
	{{"-a", "10", "--deadline", "1,ns"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 1,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_NS
	}},
	{{"-a", "10", "--deadline", " 100,us"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 100,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_US
	}},
	{{"-a", "10", "-i 100", "-o"},
	{	.approximation = 10,
		.iterations = 100,
		.deadline = 0,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_CLK
	}},
	{{"-a", "10", "-i 100", "--emulate-outliers"},
	{	.approximation = 10,
		.iterations = 100,
		.deadline = 0,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_CLK
	}},
	{{"-a", "10", "-i 100", "--deadline", "1,ns", "-o"},
	{	.approximation = 10,
		.iterations = 100,
		.deadline = 1,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_NS
	}},
	{{"-a", "10", "-i 100", "--deadline", " 100,us", "-o"},
	{	.approximation = 10,
		.iterations = 100,
		.deadline = 100,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_US
	}},
	{{"-a", "10", "--deadline", "1,ns", "-o"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 1,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_NS
	}},
	{{"-a", "10", "--deadline", " 100,us", "-o"},
	{	.approximation = 10,
		.iterations = 1000,
		.deadline = 100,
		.emulate_outliers = 1,
		.time_unit = TCC_TU_US
	}},
	{{"--approx", "10", "--it", "500"},
	{	.approximation = 10,
		.iterations = 500,
		.deadline = 0,
		.emulate_outliers = 0,
		.time_unit = TCC_TU_CLK
	}}
};
std::vector<TestPair> params_neg = {
	{{" "}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "10", "--oa", "--iterations", "21"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "15", "-emulate-outliers"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "1", "--o1ut", "--deadline", "9999,ns", "--i", "9999"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", " ", "-d", "1,ns"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "22", "-d", "one", "hundred,us"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "12", "-it", "500"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "9999999999999999999999999999999999999999999999999999999999999"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-a", "12", "-i", "9999999999999999999999999999999999999999999999999999999999999"}, {0,0,0,0,TCC_TU_CLK}},
	{{"---a", "3", "--iteration", "743"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "--o"}, {0,0,0,0,TCC_TU_CLK}},
	{{"-approximation", "19", "-o", "-d", "555,us"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "", "--emulate-outliers", "--deadline", "21,seconds"}, {0,0,0,0,TCC_TU_CLK}},
	{{"---approximation", "-999", "--emulate-outliers"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximationr", "zero", "--emulate-outliers"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "8", "--iteration"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "e4", "--deadline", "-52,us"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--appppproximation", "30"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "20", "-d", "52^2,us", "-i", "256"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "-1", "-d", "52^2,us", "-i", "256"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "20", "-d", "-1,us", "-i", "256"}, {0,0,0,0,TCC_TU_CLK}},
	{{"--approximation", "20", "-d", "50,us", "-i", "-1"}, {0,0,0,0,TCC_TU_CLK}}
};

INSTANTIATE_TEST_CASE_P(ParserGroup, ParserPositive, testing::ValuesIn(params_pos));
INSTANTIATE_TEST_CASE_P(ParserGroup, ParserNegative, testing::ValuesIn(params_neg));
