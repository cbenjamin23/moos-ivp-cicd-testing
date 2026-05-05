#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "AvgHandler.h"
#include "ExpEntry.h"
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

// Source audit note: this suite covers ExpEntry and AvgHandler's deterministic
// statistics, lazy recomputation, grouping, formatting, validation, and
// empty-input behavior against app_alogavg. The CLI parser is intentionally
// outside these direct component tests.

// Covers ExpEntry aggregate statistics.
TEST(AvgHandlerTest, ExpEntryComputesAverageRangeAndStdDev)
{
  ExpEntry entry;
  entry.setXVal(1.5);
  entry.addYVal(2);
  entry.addYVal(4);
  entry.addYVal(6);

  EXPECT_EQ(entry.getXVal(), 1.5);
  EXPECT_EQ(entry.size(), 3u);
  EXPECT_DOUBLE_EQ(entry.getYAvg(), 4.0);
  EXPECT_DOUBLE_EQ(entry.getYMin(), 2.0);
  EXPECT_DOUBLE_EQ(entry.getYMax(), 6.0);
  EXPECT_NEAR(entry.getYStd(), std::sqrt(8.0 / 3.0), 1e-9);
}

// Covers lazy recomputation after a processed entry receives more data.
TEST(AvgHandlerTest, ExpEntryRecomputesAfterAdditionalValues)
{
  ExpEntry entry;
  entry.addYVal(2);
  EXPECT_DOUBLE_EQ(entry.getYAvg(), 2.0);

  entry.addYVal(6);
  EXPECT_DOUBLE_EQ(entry.getYAvg(), 4.0);
  EXPECT_DOUBLE_EQ(entry.getYMax(), 6.0);
}

// Covers default ExpEntry statistics before any samples have been added.
TEST(AvgHandlerTest, EmptyExpEntryReturnsZeroStatistics)
{
  ExpEntry entry;
  entry.setXVal(5.0);

  EXPECT_EQ(entry.size(), 0u);
  EXPECT_DOUBLE_EQ(entry.getXVal(), 5.0);
  EXPECT_DOUBLE_EQ(entry.getYAvg(), 0.0);
  EXPECT_DOUBLE_EQ(entry.getYMin(), 0.0);
  EXPECT_DOUBLE_EQ(entry.getYMax(), 0.0);
  EXPECT_DOUBLE_EQ(entry.getYStd(), 0.0);
}

// Covers input file validation.
TEST(AvgHandlerTest, AcceptsOnlyOneReadableLogFile)
{
  TempDir temp("alogavg_file");
  const std::string input = temp.writeFile("input.txt", "1 2\n");
  const std::string second = temp.writeFile("second.txt", "1 3\n");

  AvgHandler missing;
  EXPECT_FALSE(missing.setLogFile(temp.filePath("missing.txt")));

  AvgHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));
  EXPECT_FALSE(handler.setLogFile(second));
}

// Covers handle validation when no file is open.
TEST(AvgHandlerTest, HandleRejectsMissingInputFile)
{
  AvgHandler handler;

  EXPECT_FALSE(handler.handle());
}

// Covers empty readable files producing an empty report.
TEST(AvgHandlerTest, EmptyInputProducesNoRows)
{
  TempDir temp("alogavg_empty");
  const std::string input = temp.writeFile("input.txt", "");

  AvgHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// Covers unaligned average/min/max/std output grouped by x value.
TEST(AvgHandlerTest, AveragesRowsGroupedByXValue)
{
  TempDir temp("alogavg_grouped");
  const std::string input = temp.writeFile(
      "input.txt",
      WithNewlines({"% comment",
                    "1 2",
                    "1 4",
                    "2 10"}));

  AvgHandler handler;
  handler.setFormatAligned(false);
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1 3 2 4 1"));
  EXPECT_TRUE(Contains(output, "2 10 10 10 0"));
}

// Covers negative/positive delta output mode.
TEST(AvgHandlerTest, NegPosModeReportsAverageDeltas)
{
  TempDir temp("alogavg_negpos");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"1 2", "1 4"}));

  AvgHandler handler;
  handler.setFormatAligned(false);
  handler.setFormatNegPos();
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1 3 1 1 1"));
}

// Covers negative y values in range and neg/pos delta calculations.
TEST(AvgHandlerTest, NegPosModeHandlesNegativeValues)
{
  TempDir temp("alogavg_negative");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"1 -4", "1 2"}));

  AvgHandler handler;
  handler.setFormatAligned(false);
  handler.setFormatNegPos();
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1 -1 3 3 3"));
}

// Covers verbose diagnostics for malformed rows.
TEST(AvgHandlerTest, VerboseModeReportsBadLines)
{
  TempDir temp("alogavg_verbose_bad_line");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"bad row", "1 2"}));

  AvgHandler handler;
  handler.setVerbose();
  handler.setFormatAligned(false);
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "bad line: [bad row]"));
  EXPECT_TRUE(Contains(output, "1 2 2 2 0"));
}

// Covers aligned output path without depending on exact table padding.
TEST(AvgHandlerTest, AlignedOutputContainsComputedValues)
{
  TempDir temp("alogavg_aligned");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"1 2", "1 4"}));

  AvgHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1"));
  EXPECT_TRUE(Contains(output, "3"));
  EXPECT_TRUE(Contains(output, "4"));
}
