#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "IterHandler.h"
#include "TestFileUtils.h"

namespace {

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

// Source audit note: this suite covers IterHandler's stable alog parsing,
// gap/length aggregation, threshold percentages, missing-file handling, and
// report surfaces against app_alogiter.

// Covers missing input handling.
TEST(IterHandlerTest, RejectsMissingInput)
{
  TempDir temp("alogiter_missing");
  IterHandler handler;

  testing::internal::CaptureStdout();
  EXPECT_FALSE(handler.handle(temp.filePath("missing.alog")));
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "input not found"));
}

// Covers aggregation of _ITER_GAP and _ITER_LEN rows while comments and
// unrelated variables are ignored.
TEST(IterHandlerTest, AggregatesGapAndLengthRowsByAppName)
{
  TempDir temp("alogiter_aggregate");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "0.100 APP_A_ITER_GAP pApp 1.0",
                    "0.200 APP_A_ITER_GAP pApp 2.0",
                    "0.300 APP_A_ITER_LEN pApp 0.2",
                    "0.400 APP_A_ITER_LEN pApp 0.8",
                    "0.500 APP_B_ITER_GAP pApp 3.0",
                    "0.600 APP_B_ITER_LEN pApp 1.2",
                    "0.700 NAV_X pNodeReporter 10"}));

  IterHandler handler;
  ASSERT_TRUE(handler.handle(alog));

  testing::internal::CaptureStdout();
  handler.printReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "APP_A"));
  EXPECT_TRUE(Contains(output, "APP_B"));
  EXPECT_TRUE(Contains(output, "GAP"));
  EXPECT_TRUE(Contains(output, "LEN"));
  EXPECT_TRUE(Contains(output, "Mission Summmary"));
  EXPECT_TRUE(Contains(output, "Collective APP_GAP"));
}

// Covers threshold percentage reporting for gap and length buckets.
TEST(IterHandlerTest, ReportsThresholdPercentages)
{
  TempDir temp("alogiter_thresholds");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 APP_ITER_GAP pApp 1.0",
                    "2.000 APP_ITER_GAP pApp 1.3",
                    "3.000 APP_ITER_GAP pApp 1.6",
                    "4.000 APP_ITER_GAP pApp 2.1",
                    "1.100 APP_ITER_LEN pApp 0.2",
                    "2.100 APP_ITER_LEN pApp 0.3",
                    "3.100 APP_ITER_LEN pApp 0.6",
                    "4.100 APP_ITER_LEN pApp 1.1"}));

  IterHandler handler;
  ASSERT_TRUE(handler.handle(alog));

  testing::internal::CaptureStdout();
  handler.printReportGap();
  handler.printReportLen();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "0.75"));
  EXPECT_TRUE(Contains(output, "0.5"));
  EXPECT_TRUE(Contains(output, "0.25"));
}

