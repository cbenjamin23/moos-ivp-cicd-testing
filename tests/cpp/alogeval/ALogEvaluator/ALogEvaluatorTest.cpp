#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ALogEvaluator.h"
#include "TestFileUtils.h"

namespace {

class TestableALogEvaluator : public ALogEvaluator {
 public:
  using ALogEvaluator::handleALogFile;
  using ALogEvaluator::handleTestFile;
};

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers ALogEvaluator's deterministic test-file
// parsing, alog scanning, logic/vcheck integration, validation, and verbose
// sequence output against app_alogeval. CLI argument parsing is intentionally
// outside these direct component tests.

// Covers file validation and ok flags.
TEST(ALogEvaluatorTest, ValidatesAlogAndTestFileInputs)
{
  TempDir temp("alogeval_validation");
  const std::string alog = temp.writeFile("input.alog", "");
  const std::string test = temp.writeFile("checks.txt", "lead_condition=DEPLOY=true\n");

  ALogEvaluator evaluator;
  EXPECT_FALSE(evaluator.setALogFile(temp.filePath("missing.alog")));
  EXPECT_FALSE(evaluator.okALogFile());
  EXPECT_TRUE(evaluator.setALogFile(alog));
  EXPECT_TRUE(evaluator.okALogFile());

  EXPECT_FALSE(evaluator.setTestFile(temp.filePath("missing.txt")));
  EXPECT_FALSE(evaluator.okTestFile());
  EXPECT_TRUE(evaluator.setTestFile(test));
  EXPECT_TRUE(evaluator.okTestFile());
}

// Covers empty and unhandled test-file lines.
TEST(ALogEvaluatorTest, RejectsEmptyAndUnhandledTestFiles)
{
  TempDir temp("alogeval_bad_test_files");
  const std::string empty = temp.writeFile("empty.txt", "");
  const std::string unknown = temp.writeFile("unknown.txt", "unknown=value\n");

  TestableALogEvaluator empty_eval;
  ASSERT_TRUE(empty_eval.setTestFile(empty));
  testing::internal::CaptureStdout();
  EXPECT_FALSE(empty_eval.handleTestFile());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Problem with lcheck_config"));

  TestableALogEvaluator unknown_eval;
  ASSERT_TRUE(unknown_eval.setTestFile(unknown));
  testing::internal::CaptureStdout();
  EXPECT_FALSE(unknown_eval.handleTestFile());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Unhandled test file line"));
}

// Covers logic configuration validation from the parsed test file.
TEST(ALogEvaluatorTest, RejectsInvalidLogicConfiguration)
{
  TempDir temp("alogeval_bad_logic");
  const std::string test =
      temp.writeFile("checks.txt", "pass_condition=NAV_X >= 10\n");

  TestableALogEvaluator evaluator;
  ASSERT_TRUE(evaluator.setTestFile(test));

  testing::internal::CaptureStdout();
  EXPECT_FALSE(evaluator.handleTestFile());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Problem with lcheck_config"));
}

// Covers a passing logic sequence over comments, malformed rows, strings, and
// numeric alog entries. ALogEvaluator evaluates immediately when the lead
// condition is met, so pass/fail state must already be in the InfoBuffer.
TEST(ALogEvaluatorTest, HandlePassesWhenLogicSequenceIsSatisfied)
{
  TempDir temp("alogeval_pass");
  const std::string test = temp.writeFile(
      "checks.txt",
      WithNewlines({"lead_condition=DEPLOY=true",
                    "pass_condition=NAV_X >= 10",
                    "fail_condition=COLLISION=true"}));
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "not an alog row",
                    "0.500 NAV_X pNodeReporter 10",
                    "1.500 COLLISION pContactMgr false",
                    "2.000 DEPLOY pAutoPoke true"}));

  ALogEvaluator evaluator;
  ASSERT_TRUE(evaluator.setTestFile(test));
  ASSERT_TRUE(evaluator.setALogFile(alog));

  EXPECT_TRUE(evaluator.handle());
  EXPECT_TRUE(evaluator.passed());
}

// Covers fail-condition precedence once the lead condition has been met.
TEST(ALogEvaluatorTest, HandleFailsWhenFailConditionIsObserved)
{
  TempDir temp("alogeval_fail");
  const std::string test = temp.writeFile(
      "checks.txt",
      WithNewlines({"lead_condition=DEPLOY=true",
                    "pass_condition=NAV_X >= 10",
                    "fail_condition=COLLISION=true"}));
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 DEPLOY pAutoPoke true",
                    "2.000 COLLISION pContactMgr true",
                    "3.000 NAV_X pNodeReporter 10"}));

  ALogEvaluator evaluator;
  ASSERT_TRUE(evaluator.setTestFile(test));
  ASSERT_TRUE(evaluator.setALogFile(alog));

  EXPECT_TRUE(evaluator.handle());
  EXPECT_FALSE(evaluator.passed());
}

// Covers vcheck parsing and alog mail integration alongside the required logic
// sequence.
TEST(ALogEvaluatorTest, HandleProcessesVChecksWithRelativeStartTime)
{
  TempDir temp("alogeval_vcheck");
  const std::string test = temp.writeFile(
      "checks.txt",
      WithNewlines({"lead_condition=DEPLOY=true",
                    "pass_condition=MISSION_DONE=true",
                    "vcheck_start=DEPLOY=true",
                    "vcheck=var=ARRIVED,sval=true,time=2,max_tdelta=0.5"}));
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"9.000 MISSION_DONE pMissionEval true",
                    "10.000 DEPLOY pAutoPoke true",
                    "12.000 ARRIVED pMissionEval true"}));

  ALogEvaluator evaluator;
  ASSERT_TRUE(evaluator.setTestFile(test));
  ASSERT_TRUE(evaluator.setALogFile(alog));

  EXPECT_TRUE(evaluator.handle());
  EXPECT_TRUE(evaluator.passed());
}

// Covers direct alog handling validation and verbose sequence output gating.
TEST(ALogEvaluatorTest, RejectsMissingAlogAndCanPrintSequenceWhenVerbose)
{
  TempDir temp("alogeval_verbose");
  const std::string test = temp.writeFile(
      "checks.txt",
      WithNewlines({"lead_condition=DEPLOY=true",
                    "pass_condition=NAV_X >= 10"}));

  TestableALogEvaluator missing;
  ASSERT_TRUE(missing.setTestFile(test));
  ASSERT_TRUE(missing.handleTestFile());
  testing::internal::CaptureStdout();
  EXPECT_FALSE(missing.handleALogFile());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "ALog not found"));

  TestableALogEvaluator verbose;
  verbose.setVerbose();
  verbose.showSequence();
  ASSERT_TRUE(verbose.setTestFile(test));
  ASSERT_TRUE(verbose.handleTestFile());
  testing::internal::CaptureStdout();
  verbose.outputTestSequence();
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "LogicTestSequence"));
}
