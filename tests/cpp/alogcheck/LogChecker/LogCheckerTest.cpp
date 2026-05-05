#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "ALogEntry.h"
#include "LogChecker.h"
#include "TestFileUtils.h"

namespace {

class TestableLogChecker : public LogChecker {
 public:
  unsigned int startFlagCount() const { return m_start_flags.size(); }
  unsigned int endFlagCount() const { return m_end_flags.size(); }
  unsigned int passFlagCount() const { return m_pass_flags.size(); }
  unsigned int failFlagCount() const { return m_fail_flags.size(); }
  const std::vector<std::string>& trackedVars() const { return m_vars; }
};

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

std::string ReadFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  EXPECT_TRUE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
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

bool Contains(const std::vector<std::string>& values, const std::string& value)
{
  for(const std::string& candidate : values) {
    if(candidate == value)
      return true;
  }
  return false;
}

}  // namespace

// Coverage note: this suite exercises LogChecker through deterministic flag
// parsing, InfoBuffer updates, and synthetic .alog scans. The CLI argument
// parser is thin and intentionally left to ordinary executable smoke coverage.

// Covers direct flag parsing: valid conditions are accepted and tracked
// variables are deduplicated.
TEST(LogCheckerTest, AddsValidFlagsAndTracksVariables)
{
  TestableLogChecker checker;

  EXPECT_TRUE(checker.addStartFlag("DEPLOY = true"));
  EXPECT_TRUE(checker.addPassFlag("(NAV_X >= 10) and (MODE = DRIVE)"));

  EXPECT_EQ(checker.startFlagCount(), 1u);
  EXPECT_EQ(checker.passFlagCount(), 1u);
  EXPECT_TRUE(Contains(checker.trackedVars(), "DEPLOY"));
  EXPECT_TRUE(Contains(checker.trackedVars(), "NAV_X"));
  EXPECT_TRUE(Contains(checker.trackedVars(), "MODE"));
}

// Covers invalid direct flag parsing.
TEST(LogCheckerTest, RejectsInvalidDirectFlags)
{
  TestableLogChecker checker;

  testing::internal::CaptureStdout();
  EXPECT_FALSE(checker.addFailFlag(""));
  EXPECT_FALSE(checker.addEndFlag("(DONE = true) and BAD"));
  EXPECT_NE(testing::internal::GetCapturedStdout().find("Bad Condition"),
            std::string::npos);

  EXPECT_EQ(checker.failFlagCount(), 0u);
  EXPECT_EQ(checker.endFlagCount(), 0u);
}

// Covers input-file parsing: blank lines, comments, tabs, quoted flags, and
// mixed-case directive names are normalized; unknown directives are ignored.
TEST(LogCheckerTest, ParseInputFileLoadsValidFlagDirectives)
{
  TempDir temp("alogcheck_input_file");
  const std::string input_path = temp.writeFile(
      "checks.txt",
      WithNewlines({"",
                    "# full-line comment",
                    "START\t\"DEPLOY = true\"  # inline comment",
                    "Pass NAV_X >= 10 // trailing comment",
                    "fail MISSION_RESULT = bad",
                    "END DONE = true",
                    "unknown NAV_Y > 3"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.parseInputFile(input_path));

  EXPECT_EQ(checker.startFlagCount(), 1u);
  EXPECT_EQ(checker.passFlagCount(), 1u);
  EXPECT_EQ(checker.failFlagCount(), 1u);
  EXPECT_EQ(checker.endFlagCount(), 1u);
}

// Covers parsed check files driving the full log scan.
TEST(LogCheckerTest, ParsedInputFileFlagsDriveCheckOutcome)
{
  TempDir temp("alogcheck_input_file_scan");
  const std::string input_path = temp.writeFile(
      "checks.txt",
      WithNewlines({"start DEPLOY = true",
                    "pass NAV_X >= 10",
                    "fail MISSION_RESULT = bad",
                    "end DONE = true"}));
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 DEPLOY pAutoPoke true",
                    "2.000 NAV_X pNodeReporter 10",
                    "3.000 DONE pMissionEval true",
                    "4.000 MISSION_RESULT pMissionEval bad"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.parseInputFile(input_path));

  EXPECT_TRUE(checker.check(alog_path));
}

// Covers input-file parse failure modes.
TEST(LogCheckerTest, ParseInputFileRejectsEmptyMissingAndInvalidOnlyFiles)
{
  TempDir temp("alogcheck_bad_input_file");
  const std::string invalid_path = temp.writeFile(
      "invalid.txt", WithNewlines({"# comment", "unknown NAV_X > 3"}));

  testing::internal::CaptureStdout();
  TestableLogChecker checker;
  EXPECT_FALSE(checker.parseInputFile(""));
  EXPECT_FALSE(checker.parseInputFile(temp.filePath("missing.txt")));
  EXPECT_FALSE(checker.parseInputFile(invalid_path));
  testing::internal::GetCapturedStdout();
}

// Covers InfoBuffer updates: only variables referenced by flags are tracked,
// and both string and numeric alog entries can satisfy conditions.
TEST(LogCheckerTest, UpdateInfoBufferTracksReferencedStringAndNumericVars)
{
  TestableLogChecker checker;
  ASSERT_TRUE(checker.addPassFlag("(MODE = DRIVE) and (NAV_X >= 10)"));

  EXPECT_FALSE(checker.updateInfoBuffer(NumericEntry(1.0, "NAV_Y", 20.0)));
  EXPECT_FALSE(checker.checkPassFlags());

  EXPECT_TRUE(checker.updateInfoBuffer(StringEntry(2.0, "MODE", "DRIVE")));
  EXPECT_FALSE(checker.checkPassFlags());

  EXPECT_TRUE(checker.updateInfoBuffer(NumericEntry(3.0, "NAV_X", 10.5)));
  EXPECT_TRUE(checker.checkPassFlags());
}

// Covers timestamp conditions: ALOG_TIMESTAMP is updated when a tracked entry
// is accepted into the InfoBuffer.
TEST(LogCheckerTest, TimestampConditionsUseTrackedEntryTime)
{
  TestableLogChecker checker;
  ASSERT_TRUE(checker.addPassFlag("(NAV_X = 10) and (ALOG_TIMESTAMP >= 2)"));

  EXPECT_TRUE(checker.updateInfoBuffer(NumericEntry(1.0, "NAV_X", 10.0)));
  EXPECT_FALSE(checker.checkPassFlags());

  EXPECT_TRUE(checker.updateInfoBuffer(NumericEntry(2.5, "NAV_X", 10.0)));
  EXPECT_TRUE(checker.checkPassFlags());
}

// Covers check validation for empty input, missing input, and identical input
// and output paths.
TEST(LogCheckerTest, CheckRejectsEmptyMissingAndSameOutputPath)
{
  TempDir temp("alogcheck_validation");
  const std::string alog_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");

  TestableLogChecker empty_input;
  testing::internal::CaptureStdout();
  EXPECT_FALSE(empty_input.check(""));
  EXPECT_NE(testing::internal::GetCapturedStdout().find(
                "Input filename cannot be empty"),
            std::string::npos);

  TestableLogChecker missing_input;
  testing::internal::CaptureStdout();
  EXPECT_FALSE(missing_input.check(temp.filePath("missing.alog")));
  EXPECT_NE(testing::internal::GetCapturedStdout().find("Input file not found"),
            std::string::npos);

  TestableLogChecker same_output;
  testing::internal::CaptureStdout();
  EXPECT_FALSE(same_output.check(alog_path, alog_path));
  EXPECT_NE(testing::internal::GetCapturedStdout().find(
                "cannot be the same"),
            std::string::npos);
}

// Covers basic pass behavior over a scanned .alog file.
TEST(LogCheckerTest, CheckPassesWhenPassConditionIsObserved)
{
  TempDir temp("alogcheck_pass");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 9",
                    "2.000 NAV_X pNodeReporter 10"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));

  EXPECT_TRUE(checker.check(alog_path));
}

// Covers string-valued alog conditions.
TEST(LogCheckerTest, CheckPassesOnStringCondition)
{
  TempDir temp("alogcheck_string_pass");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 IVPHELM_STATE pHelmIvP PARK",
                    "2.000 IVPHELM_STATE pHelmIvP DRIVE"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addPassFlag("IVPHELM_STATE = DRIVE"));

  EXPECT_TRUE(checker.check(alog_path));
}

// Covers multiple pass flags: all pass conditions must be satisfied.
TEST(LogCheckerTest, MultiplePassFlagsRequireAllConditions)
{
  TempDir temp("alogcheck_multi_pass");
  const std::string partial_path =
      temp.writeFile("partial.alog", "1.000 NAV_X pNodeReporter 10\n");
  const std::string full_path = temp.writeFile(
      "full.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "2.000 MODE pHelmIvP DRIVE"}));

  TestableLogChecker partial;
  ASSERT_TRUE(partial.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(partial.addPassFlag("MODE = DRIVE"));
  EXPECT_FALSE(partial.check(partial_path));

  TestableLogChecker full;
  ASSERT_TRUE(full.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(full.addPassFlag("MODE = DRIVE"));
  EXPECT_TRUE(full.check(full_path));
}

// Covers the required pass condition: no pass flag means the check fails even
// when no fail flag is satisfied.
TEST(LogCheckerTest, CheckFailsWhenNoPassConditionIsSatisfied)
{
  TempDir temp("alogcheck_no_pass");
  const std::string alog_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");

  TestableLogChecker no_flags;
  EXPECT_FALSE(no_flags.check(alog_path));

  TestableLogChecker fail_only;
  ASSERT_TRUE(fail_only.addFailFlag("NAV_X < 0"));
  EXPECT_FALSE(fail_only.check(alog_path));
}

// Covers fail precedence: a satisfied fail flag makes the check fail even if a
// pass flag is also satisfied.
TEST(LogCheckerTest, FailConditionOverridesSatisfiedPass)
{
  TempDir temp("alogcheck_fail_override");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "2.000 MISSION_RESULT pMissionEval bad"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(checker.addFailFlag("MISSION_RESULT = bad"));

  EXPECT_FALSE(checker.check(alog_path));
}

// Covers multiple fail flags: fail conditions are also evaluated with AND
// semantics, so a single satisfied fail flag does not fail the check by itself.
TEST(LogCheckerTest, MultipleFailFlagsRequireAllConditions)
{
  TempDir temp("alogcheck_multi_fail");
  const std::string one_fail_path = temp.writeFile(
      "one_fail.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "2.000 MISSION_RESULT pMissionEval good"}));
  const std::string all_fail_path = temp.writeFile(
      "all_fail.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "2.000 MISSION_RESULT pMissionEval bad"}));

  TestableLogChecker one_fail;
  ASSERT_TRUE(one_fail.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(one_fail.addFailFlag("NAV_X >= 10"));
  ASSERT_TRUE(one_fail.addFailFlag("MISSION_RESULT = bad"));
  EXPECT_TRUE(one_fail.check(one_fail_path));

  TestableLogChecker all_fail;
  ASSERT_TRUE(all_fail.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(all_fail.addFailFlag("NAV_X >= 10"));
  ASSERT_TRUE(all_fail.addFailFlag("MISSION_RESULT = bad"));
  EXPECT_FALSE(all_fail.check(all_fail_path));
}

// Covers current start search semantics: entries before the start condition
// still populate the InfoBuffer, so a pre-start fail value can fail the check.
TEST(LogCheckerTest, StartSearchPopulatesInfoBufferBeforeEvaluation)
{
  TempDir temp("alogcheck_start_gate");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "1.500 MISSION_RESULT pMissionEval bad",
                    "2.000 DEPLOY pAutoPoke true",
                    "3.000 NAV_X pNodeReporter 10"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addStartFlag("DEPLOY = true"));
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(checker.addFailFlag("MISSION_RESULT = bad"));

  EXPECT_FALSE(checker.check(alog_path));
}

// Covers missing start condition: reaching EOF before start satisfaction fails
// the check immediately.
TEST(LogCheckerTest, MissingStartConditionFailsAtEof)
{
  TempDir temp("alogcheck_missing_start");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "2.000 NAV_X pNodeReporter 11"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addStartFlag("DEPLOY = true"));
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));

  EXPECT_FALSE(checker.check(alog_path));
}

// Covers the phase boundary between start search and pass/fail evaluation:
// the entry satisfying start is not itself evaluated as a pass entry.
TEST(LogCheckerTest, StartEntryDoesNotImmediatelySatisfyPassCondition)
{
  TempDir temp("alogcheck_start_entry_pass");
  const std::string alog_path =
      temp.writeFile("input.alog", "1.000 DEPLOY pAutoPoke true\n");

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addStartFlag("DEPLOY = true"));
  ASSERT_TRUE(checker.addPassFlag("DEPLOY = true"));

  EXPECT_FALSE(checker.check(alog_path));
}

// Covers end-gated evaluation: entries after an end condition are ignored.
TEST(LogCheckerTest, EndFlagStopsScanBeforeLaterPassOrFail)
{
  TempDir temp("alogcheck_end_gate");
  const std::string pass_after_end_path = temp.writeFile(
      "pass_after_end.alog",
      WithNewlines({"1.000 DONE pMissionEval true",
                    "2.000 NAV_X pNodeReporter 10"}));

  TestableLogChecker pass_after_end;
  ASSERT_TRUE(pass_after_end.addEndFlag("DONE = true"));
  ASSERT_TRUE(pass_after_end.addPassFlag("NAV_X >= 10"));
  EXPECT_FALSE(pass_after_end.check(pass_after_end_path));

  const std::string fail_after_end_path = temp.writeFile(
      "fail_after_end.alog",
      WithNewlines({"1.000 NAV_X pNodeReporter 10",
                    "2.000 DONE pMissionEval true",
                    "3.000 MISSION_RESULT pMissionEval bad"}));

  TestableLogChecker fail_after_end;
  ASSERT_TRUE(fail_after_end.addEndFlag("DONE = true"));
  ASSERT_TRUE(fail_after_end.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(fail_after_end.addFailFlag("MISSION_RESULT = bad"));
  EXPECT_TRUE(fail_after_end.check(fail_after_end_path));
}

// Covers multiple end flags: the scan does not stop until all end conditions
// are satisfied.
TEST(LogCheckerTest, MultipleEndFlagsRequireAllConditionsBeforeStopping)
{
  TempDir temp("alogcheck_multi_end");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 DONE pMissionEval true",
                    "2.000 NAV_X pNodeReporter 10",
                    "3.000 MODE pHelmIvP FINISHED",
                    "4.000 MISSION_RESULT pMissionEval bad"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addEndFlag("DONE = true"));
  ASSERT_TRUE(checker.addEndFlag("MODE = FINISHED"));
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));
  ASSERT_TRUE(checker.addFailFlag("MISSION_RESULT = bad"));

  EXPECT_TRUE(checker.check(alog_path));
}

// Covers the scan step where an end-condition entry can also satisfy the pass
// condition before the loop terminates.
TEST(LogCheckerTest, EndEntryCanAlsoSatisfyPassCondition)
{
  TempDir temp("alogcheck_end_is_pass");
  const std::string alog_path =
      temp.writeFile("input.alog", "1.000 DONE pMissionEval true\n");

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addEndFlag("DONE = true"));
  ASSERT_TRUE(checker.addPassFlag("DONE = true"));

  EXPECT_TRUE(checker.check(alog_path));
}

// Covers verbose output to a caller-specified file.
TEST(LogCheckerTest, VerboseModeWritesStatusToOutputFile)
{
  TempDir temp("alogcheck_verbose_file");
  const std::string alog_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");
  const std::string output_path = temp.filePath("report.txt");

  {
    TestableLogChecker checker;
    checker.setVerbose(true);
    ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));

    ASSERT_TRUE(checker.check(alog_path, output_path));
  }

  EXPECT_NE(ReadFile(output_path).find("No start conditions specified"),
            std::string::npos);
}

// Covers verbose output for an explicitly satisfied start condition.
TEST(LogCheckerTest, VerboseModeReportsSatisfiedStartCondition)
{
  TempDir temp("alogcheck_verbose_start");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 DEPLOY pAutoPoke true",
                    "2.000 NAV_X pNodeReporter 10"}));
  const std::string output_path = temp.filePath("report.txt");

  {
    TestableLogChecker checker;
    checker.setVerbose(true);
    ASSERT_TRUE(checker.addStartFlag("DEPLOY = true"));
    ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));

    ASSERT_TRUE(checker.check(alog_path, output_path));
  }

  EXPECT_NE(ReadFile(output_path).find(
                "The start consitions have been satisfied"),
            std::string::npos);
}

// Covers output open failure: the checker reports the issue to stdout and
// continues using stdout.
TEST(LogCheckerTest, OutputOpenFailureFallsBackToStdout)
{
  TempDir temp("alogcheck_output_fallback");
  const std::string alog_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");
  const std::string bad_output = temp.filePath("missing_dir/report.txt");

  TestableLogChecker checker;
  checker.setVerbose(true);
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));

  testing::internal::CaptureStdout();
  const bool ok = checker.check(alog_path, bad_output);
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(ok);
  EXPECT_NE(output.find("Output could not be created"), std::string::npos);
  EXPECT_NE(output.find("No start conditions specified"), std::string::npos);
}

// Covers malformed/comment log lines during scanning: invalid entries are
// ignored and do not prevent later valid entries from satisfying flags.
TEST(LogCheckerTest, CheckIgnoresCommentsAndMalformedALogEntries)
{
  TempDir temp("alogcheck_malformed_entries");
  const std::string alog_path = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "not an alog entry",
                    "1.000 NAV_X pNodeReporter 10"}));

  TestableLogChecker checker;
  ASSERT_TRUE(checker.addPassFlag("NAV_X >= 10"));

  EXPECT_TRUE(checker.check(alog_path));
}
