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
#include <functional>
#include "tcc_log.h"

#ifdef TCC_COVERAGE_ENABLED
extern "C" {
	void __gcov_flush();
}
#endif

template <typename TestType>
class SubprocessTest : public TestType
{
public:
	SubprocessTest();
	virtual ~SubprocessTest();
protected:
	bool is_subprocess();
	void create_subprocess();
private:
	std::function<bool()> m_has_failure;
	bool m_is_subprocess;
	bool m_is_subprocess_created;
};

template <typename TestType>
SubprocessTest<TestType>::SubprocessTest() : m_has_failure{[&]() { return ::testing::Test::HasFailure(); }},
											 m_is_subprocess{false},
											 m_is_subprocess_created{false}
{
}

template <typename TestType>
SubprocessTest<TestType>::~SubprocessTest()
{
	int exit_status = 0;
	if (!is_subprocess())
	{
		#ifdef TCC_COVERAGE_ENABLED
		__gcov_flush();
		#endif
		return;
	}
	if (::testing::Test::HasFailure())
	{
		TCC_LOG_TRACE("test FAILURE");
		exit_status = 1;
	}
	TCC_LOG_TRACE("test SUCCESS");
	#ifdef TCC_COVERAGE_ENABLED
	__gcov_flush();
	#endif
	exit(exit_status);
}

template <typename TestType>
bool SubprocessTest<TestType>::is_subprocess()
{
	return m_is_subprocess;
}

template <typename TestType>
void SubprocessTest<TestType>::create_subprocess()
{
	if (m_is_subprocess_created) {
		TCC_LOG_ERROR("Subprocess already created");
		return;
	}
	m_is_subprocess_created = true;
	#ifdef TCC_COVERAGE_ENABLED
	__gcov_flush();
	#endif
	TCC_LOG_TRACE("starting process");
	pid_t child_test_pid = fork();
	TCC_LOG_TRACE("pid: %d", child_test_pid);
	ASSERT_NE(-1, child_test_pid) << "Failed to fork test case: " << strerror(errno);
	if (child_test_pid != 0)
	{
		int retval;
		ASSERT_NE(-1, waitpid(child_test_pid, &retval, 0)) << "waitpid failed: " << strerror(errno);
		TCC_LOG_TRACE("Child closed");
		ASSERT_EQ(0, WEXITSTATUS(retval)) << "Test failed, see child process output";
		return;
	}
	m_is_subprocess = true;
}

#define SUBPROCESS_FIXTURE_NAME(fixture, test_case) fixture ## ___ ## test_case  
/// Run test in separate process for this fixture
/// Test create a separate process after constructor during SetUp step.
/// WARNING: If you configure mocks or same in cunstructor step expect that you can have
/// issues than test with test PASS than main thread finished.
#define SUBPROCESS_TEST_F(fixture, test_case) \
class SUBPROCESS_FIXTURE_NAME(fixture, test_case): public fixture \
{ \
public: \
	void SetUp() override \
	{ \
		create_subprocess(); \
		if (!is_subprocess()) { \
			return; \
		} \
		fixture::SetUp(); \
	} \
	void TearDown() override \
	{ \
		if (!is_subprocess()) { \
			return; \
		} \
		fixture::TearDown(); \
	} \
	void SubprocessTestBody(); \
}; \
TEST_F(SUBPROCESS_FIXTURE_NAME(fixture, test_case), SubprocessTest) \
{ \
	if(!is_subprocess()) { \
		return; \
	} \
	SUBPROCESS_FIXTURE_NAME(fixture, test_case)::SubprocessTestBody(); \
} \
void SUBPROCESS_FIXTURE_NAME(fixture, test_case)::SubprocessTestBody()
