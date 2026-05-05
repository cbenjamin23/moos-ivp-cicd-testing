#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "LoadReporter.h"
#include "TestFileUtils.h"

namespace {

class TestableLoadReporter : public LoadReporter {
 public:
  using LoadReporter::breachCount;
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

// Source audit note: this suite covers LoadReporter's deterministic file
// validation, duplicate handling, max breach counting, near-breach mode, terse
// mode, and report behavior against app_alogload.

// Covers file validation and duplicate rejection.
TEST(LoadReporterTest, AddsOnlyReadableUniqueAlogFiles)
{
  TempDir temp("alogload_files");
  const std::string input = temp.writeFile("input.alog", "");

  LoadReporter reporter;
  reporter.setVerbose();

  EXPECT_FALSE(reporter.addALogFile(temp.filePath("missing.alog")));
  EXPECT_TRUE(reporter.addALogFile(input));
  EXPECT_FALSE(reporter.addALogFile(input));
}

// Covers breach counting by maximum observed value, not final value.
TEST(LoadReporterTest, CountsMaximumBreachValuePerFile)
{
  TempDir temp("alogload_max");
  const std::string first = temp.writeFile(
      "first.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "1.000 ULW_BREACH_COUNT uLoadWatch 1",
                    "2.000 ULW_BREACH_COUNT uLoadWatch 5",
                    "3.000 ULW_BREACH_COUNT uLoadWatch 2"}));
  const std::string second = temp.writeFile(
      "second.alog", "1.000 ULW_BREACH_COUNT uLoadWatch 3\n");

  LoadReporter reporter;
  ASSERT_TRUE(reporter.addALogFile(first));
  ASSERT_TRUE(reporter.addALogFile(second));

  testing::internal::CaptureStdout();
  reporter.report();
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Final Total Breaches: 8"));
}

// Covers near-breach mode and non-numeric values being ignored.
TEST(LoadReporterTest, NearModeCountsNearBreachVariableOnly)
{
  TempDir temp("alogload_near");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 ULW_BREACH_COUNT uLoadWatch 9",
                    "2.000 ULW_NEAR_BREACH_COUNT uLoadWatch bad",
                    "3.000 ULW_NEAR_BREACH_COUNT uLoadWatch 4"}));

  LoadReporter reporter;
  reporter.setNearMode();
  ASSERT_TRUE(reporter.addALogFile(input));

  testing::internal::CaptureStdout();
  reporter.report();
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Final Total Near Breaches: 4"));
}

// Covers terse output and direct breachCount validation.
TEST(LoadReporterTest, TerseModePrintsOnlyCountAndRejectsMissingDirectCount)
{
  TempDir temp("alogload_terse");
  const std::string input =
      temp.writeFile("input.alog", "1.000 ULW_BREACH_COUNT uLoadWatch 2\n");

  TestableLoadReporter missing;
  EXPECT_FALSE(missing.breachCount(""));
  EXPECT_FALSE(missing.breachCount(temp.filePath("missing.alog")));

  LoadReporter reporter;
  reporter.setVerbose();
  reporter.setTerse();
  ASSERT_TRUE(reporter.addALogFile(input));

  testing::internal::CaptureStdout();
  reporter.report();
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "2\n");
}

