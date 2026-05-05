#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "TGrepHandler.h"
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

// Source audit note: this suite covers TGrepHandler's deterministic mark,
// key, de-duplication, min-gap, test-format, and validation paths against
// app_alogtm. The thin CLI parser is intentionally outside these tests.

// Covers input file validation and handle preconditions.
TEST(TGrepHandlerTest, ValidatesInputFileAndRequiredMark)
{
  TempDir temp("alogtm_validation");
  const std::string input = temp.writeFile("input.alog", "");

  TGrepHandler missing;
  EXPECT_FALSE(missing.setLogFile(temp.filePath("missing.alog")));
  EXPECT_FALSE(missing.okALogFile());
  EXPECT_FALSE(missing.handle());

  TGrepHandler no_mark;
  EXPECT_TRUE(no_mark.setLogFile(input));
  EXPECT_TRUE(no_mark.okALogFile());
  EXPECT_FALSE(no_mark.handle());

  TGrepHandler empty_mark;
  EXPECT_FALSE(empty_mark.setTimeMark(""));
}

// Covers numeric parameter validation.
TEST(TGrepHandlerTest, ValidatesNumericConfiguration)
{
  TGrepHandler handler;

  EXPECT_TRUE(handler.setMinGap("0"));
  EXPECT_TRUE(handler.setMinGap("2.5"));
  EXPECT_FALSE(handler.setMinGap("-1"));
  EXPECT_FALSE(handler.setMinGap("bad"));

  EXPECT_TRUE(handler.setMaxTDelta("3"));
  EXPECT_FALSE(handler.setMaxTDelta("-0.1"));
  EXPECT_FALSE(handler.setMaxTDelta("nan-ish"));

  EXPECT_TRUE(handler.setMaxVDelta("4"));
  EXPECT_FALSE(handler.setMaxVDelta("-2"));
  EXPECT_FALSE(handler.setMaxVDelta("bad"));

  EXPECT_TRUE(handler.setSigDigits("0"));
  EXPECT_TRUE(handler.setSigDigits("6"));
  EXPECT_FALSE(handler.setSigDigits("-1"));
  EXPECT_FALSE(handler.setSigDigits("bad"));
}

// Covers key insertion de-duplication.
TEST(TGrepHandlerTest, RejectsDuplicateKeys)
{
  TGrepHandler handler;

  EXPECT_TRUE(handler.addKey("NAV_X"));
  EXPECT_FALSE(handler.addKey("NAV_X"));
  EXPECT_TRUE(handler.addKey("NAV_Y"));
}

// Covers mark-relative output: comments, malformed rows, pre-mark rows, and
// unrequested variables are ignored; requested rows are shifted by mark time.
TEST(TGrepHandlerTest, EmitsRequestedVariablesAfterMarkWithRelativeTime)
{
  TempDir temp("alogtm_relative");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "not an alog row",
                    "9.000 NAV_X pNodeReporter 1",
                    "10.000 DEPLOY pAutoPoke true",
                    "11.500 NAV_X pNodeReporter 2",
                    "12.000 NAV_Y pNodeReporter 3",
                    "13.000 NAV_X pNodeReporter 4"}));

  TGrepHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.setTimeMark("DEPLOY=true"));
  ASSERT_TRUE(handler.addKey("NAV_X"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1.5"));
  EXPECT_TRUE(Contains(output, "NAV_X"));
  EXPECT_TRUE(Contains(output, "2"));
  EXPECT_TRUE(Contains(output, "3"));
  EXPECT_TRUE(Contains(output, "4"));
  EXPECT_FALSE(Contains(output, "NAV_Y"));
  EXPECT_FALSE(Contains(output, "9.000"));
}

// Covers duplicate suppression using the shifted timestamp, variable, and value.
TEST(TGrepHandlerTest, SuppressesDuplicateShiftedPosts)
{
  TempDir temp("alogtm_duplicates");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"10.000 DEPLOY pAutoPoke true",
                    "11.000 NAV_X pNodeReporter 7",
                    "11.000 NAV_X pNodeReporter 7",
                    "11.000 NAV_X pNodeReporter 8"}));

  TGrepHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.setTimeMark("DEPLOY=true"));
  ASSERT_TRUE(handler.addKey("NAV_X"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output.find("7"), output.rfind("7"));
  EXPECT_TRUE(Contains(output, "8"));
}

// Covers min-gap suppression per variable after the mark has been made.
TEST(TGrepHandlerTest, MinGapSuppressesTooFrequentReportsPerVariable)
{
  TempDir temp("alogtm_mingap");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"10.000 DEPLOY pAutoPoke true",
                    "11.000 NAV_X pNodeReporter 1",
                    "11.500 NAV_X pNodeReporter 2",
                    "12.000 NAV_Y pNodeReporter 3",
                    "13.100 NAV_X pNodeReporter 4"}));

  TGrepHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.setTimeMark("DEPLOY=true"));
  ASSERT_TRUE(handler.setMinGap("2"));
  ASSERT_TRUE(handler.addKey("NAV_X"));
  ASSERT_TRUE(handler.addKey("NAV_Y"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1"));
  EXPECT_FALSE(Contains(output, "NAV_X             2"));
  EXPECT_TRUE(Contains(output, "3"));
  EXPECT_TRUE(Contains(output, "4"));
}

// Covers mission-eval vcheck output format, significant digits, and optional
// max delta fields.
TEST(TGrepHandlerTest, TestFormatEmitsVCheckRowsWithConfiguredDeltas)
{
  TempDir temp("alogtm_test_format");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"5.000 START pAutoPoke true",
                    "7.000 NAV_X pNodeReporter 12.34567",
                    "8.000 MODE pHelmIvP DRIVE"}));

  TGrepHandler handler;
  handler.setTestFormat();
  ASSERT_TRUE(handler.setSigDigits("2"));
  ASSERT_TRUE(handler.setMaxTDelta("3"));
  ASSERT_TRUE(handler.setMaxVDelta("0.5"));
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.setTimeMark("START=true"));
  ASSERT_TRUE(handler.addKey("NAV_X"));
  ASSERT_TRUE(handler.addKey("MODE"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "vcheck = var=NAV_X, val=12.35, time=2"));
  EXPECT_TRUE(Contains(output, "max_tdelta=3"));
  EXPECT_TRUE(Contains(output, "max_vdelta=0.5"));
  EXPECT_TRUE(Contains(output, "vcheck = var=MODE, val=DRIVE, time=3"));
}
