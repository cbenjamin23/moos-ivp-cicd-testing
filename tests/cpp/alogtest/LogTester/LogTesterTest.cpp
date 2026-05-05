#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "ALogEntry.h"
#include "LogTest.h"
#include "LogTester.h"
#include "TestFileUtils.h"

namespace {

class TestableLogTest : public LogTest {
 public:
  using LogTest::checkEndConditions;
  using LogTest::checkFailConditions;
  using LogTest::checkPassConditions;
  using LogTest::checkStartConditions;
};

class TestableLogTester : public LogTester {
 public:
  using LogTester::parseTestFile;
};

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

ALogEntry NumericEntry(double timestamp,
                       const std::string& var,
                       double value)
{
  ALogEntry entry;
  entry.set(timestamp, var, "source", "", value);
  return entry;
}

ALogEntry StringEntry(double timestamp,
                      const std::string& var,
                      const std::string& value)
{
  ALogEntry entry;
  entry.set(timestamp, var, "source", "", value);
  return entry;
}

ALogEntry EofEntry()
{
  ALogEntry entry;
  entry.setStatus("eof");
  return entry;
}

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers LogTest and LogTester's deterministic
// condition parsing, state transitions, EOF/end handling, parser continuation
// lines, file validation, and result reporting against app_alogtest. Tests
// allocate LogTester on the heap where destructor behavior is not under test.

// Covers LogTest configuration validation and variable tracking.
TEST(LogTestTest, ValidatesTimesConditionsAndEmptyState)
{
  TestableLogTest test;
  EXPECT_TRUE(test.isEmpty());

  EXPECT_TRUE(test.setStartTime("-1"));
  EXPECT_TRUE(test.setEndTime("3.5"));
  EXPECT_FALSE(test.setStartTime("bad"));
  EXPECT_FALSE(test.setEndTime("bad"));

  EXPECT_TRUE(test.addStartCondition("DEPLOY = true"));
  EXPECT_TRUE(test.addPassCondition("NAV_X >= 10"));
  EXPECT_TRUE(test.addFailCondition("COLLISION = true"));
  EXPECT_FALSE(test.addEndCondition("(DONE = true) and BAD"));
  EXPECT_FALSE(test.isEmpty());

  const std::vector<std::string> vars = test.getVars();
  EXPECT_NE(std::find(vars.begin(), vars.end(), "DEPLOY"), vars.end());
  EXPECT_NE(std::find(vars.begin(), vars.end(), "NAV_X"), vars.end());
  EXPECT_NE(std::find(vars.begin(), vars.end(), "COLLISION"), vars.end());
}

// Covers state progression from start condition to passing to passed at EOF.
TEST(LogTestTest, PassesAtEofAfterPassConditionIsMet)
{
  TestableLogTest test;
  ASSERT_TRUE(test.setStartTime("0.5"));
  ASSERT_TRUE(test.addStartCondition("DEPLOY = true"));
  ASSERT_TRUE(test.addPassCondition("NAV_X >= 10"));

  EXPECT_TRUE(test.updateState(StringEntry(1.0, "DEPLOY", "true")));
  EXPECT_EQ(test.getState(), "pending");
  EXPECT_TRUE(test.updateState(NumericEntry(2.0, "NAV_X", 10.0)));
  EXPECT_EQ(test.getState(), "passing");
  EXPECT_TRUE(test.updateState(EofEntry()));
  EXPECT_EQ(test.getState(), "passed");

  const std::vector<std::string> events = test.getEvents();
  ASSERT_GE(events.size(), 3u);
  EXPECT_TRUE(Contains(events.back(), "eof"));
}

// Covers fail-condition precedence.
TEST(LogTestTest, FailConditionTransitionsToTerminalFailedState)
{
  TestableLogTest test;
  ASSERT_TRUE(test.setStartTime("0.5"));
  ASSERT_TRUE(test.addStartCondition("DEPLOY = true"));
  ASSERT_TRUE(test.addPassCondition("NAV_X >= 10"));
  ASSERT_TRUE(test.addFailCondition("COLLISION = true"));

  EXPECT_TRUE(test.updateState(StringEntry(1.0, "DEPLOY", "true")));
  EXPECT_TRUE(test.updateState(StringEntry(2.0, "COLLISION", "true")));
  EXPECT_EQ(test.getState(), "failed");
  EXPECT_EQ(test.getFailReason(), "met fail condition");

  EXPECT_FALSE(test.updateState(NumericEntry(3.0, "NAV_X", 10.0)));
  EXPECT_EQ(test.getState(), "failed");
}

// Covers start/end time gates. When the start-time transition itself sets the
// changed flag, the current entry is not pushed into the InfoBuffer due to the
// upstream short-circuit expression, so prior buffered values can satisfy pass.
TEST(LogTestTest, TimeGateStartCanUsePriorBufferedValues)
{
  TestableLogTest test;
  ASSERT_TRUE(test.setStartTime("2"));
  ASSERT_TRUE(test.setEndTime("4"));
  ASSERT_TRUE(test.addPassCondition("NAV_X >= 10"));

  EXPECT_FALSE(test.updateState(NumericEntry(1.0, "NAV_X", 10.0)));
  EXPECT_EQ(test.getState(), "unsat");
  EXPECT_TRUE(test.updateState(NumericEntry(2.0, "NAV_X", 5.0)));
  EXPECT_EQ(test.getState(), "passing");
  EXPECT_TRUE(test.updateState(NumericEntry(4.0, "NAV_X", 5.0)));
  EXPECT_EQ(test.getState(), "passed");
  EXPECT_EQ(test.getFailReason(), "");
}

// Covers parser support for aliases, comments, line continuation, and multiple
// named tests in one test file.
TEST(LogTesterTest, ParseTestFileAcceptsAliasesContinuationsAndMultipleTests)
{
  TempDir temp("alogtest_parse");
  const std::string test_file = temp.writeFile(
      "checks.txt",
      WithNewlines({"// comment",
                    "test_name=alpha",
                    "sc=DEPLOY = true",
                    "pc=NAV_X \\",
                    ">= 10",
                    "test_name=beta",
                    "start_time=1",
                    "end_time=3",
                    "pass_condition=MODE = DRIVE"}));

  TestableLogTester* tester = new TestableLogTester();
  testing::internal::CaptureStdout();
  EXPECT_TRUE(tester->parseTestFile(test_file));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "Beginning to parse test file"));
}

// Covers parser failure for invalid lines.
TEST(LogTesterTest, ParseTestFileRejectsInvalidLines)
{
  TempDir temp("alogtest_parse_bad");
  const std::string test_file =
      temp.writeFile("checks.txt", "pass_condition=(NAV_X >= 10) and BAD\n");

  TestableLogTester* tester = new TestableLogTester();
  testing::internal::CaptureStdout();
  EXPECT_FALSE(tester->parseTestFile(test_file));
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Problem with line"));
}

// Covers LogTester file validation and end-to-end pass reporting.
TEST(LogTesterTest, EndToEndTestReportsPassingResult)
{
  TempDir temp("alogtest_end_to_end");
  const std::string test_file = temp.writeFile(
      "checks.txt",
      WithNewlines({"test_name=mission",
                    "start_time=1",
                    "end_time=3",
                    "pass_condition=NAV_X >= 10"}));
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"0.500 NAV_X pNodeReporter 10",
                    "1.500 NAV_X pNodeReporter 10"}));

  LogTester* tester = new LogTester();
  EXPECT_FALSE(tester->addTestFile(temp.filePath("missing.txt")));
  EXPECT_TRUE(tester->addTestFile(test_file));
  EXPECT_FALSE(tester->addTestFile(test_file));
  EXPECT_FALSE(tester->setALogFile(temp.filePath("missing.alog")));
  EXPECT_TRUE(tester->setALogFile(alog));
  EXPECT_FALSE(tester->setALogFile(alog));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(tester->test());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "final_result = true"));
  EXPECT_TRUE(Contains(output, "result for test mission:true=passed="));
}

// Covers mark-file write validation and overwrite behavior.
TEST(LogTesterTest, MarkFileRequiresOverwriteForExistingFile)
{
  TempDir temp("alogtest_mark_file");
  const std::string existing = temp.writeFile("marks.txt", "stale\n");

  LogTester* tester = new LogTester();
  EXPECT_FALSE(tester->setMarkFile(existing));

  LogTester* overwrite = new LogTester();
  overwrite->setOverWrite(true);
  EXPECT_TRUE(overwrite->setMarkFile(existing));
  EXPECT_FALSE(overwrite->setMarkFile(temp.filePath("other.txt")));
  EXPECT_FALSE(overwrite->setMarkFile(temp.filePath("missing/out.txt")));
}
