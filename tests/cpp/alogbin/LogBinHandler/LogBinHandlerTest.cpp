#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "LogBinHandler.h"
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

// Source audit note: this suite covers LogBinHandler's deterministic parameter
// validation, empty/comment input, bad-line diagnostics, boundary bins, min and
// delta formulas, and negative-value behavior against app_alogbin. The CLI
// parser is intentionally outside these direct component tests.

// Covers parameter validation.
TEST(LogBinHandlerTest, ValidatesDeltaAndMinimumParameters)
{
  LogBinHandler handler;

  EXPECT_TRUE(handler.setDelta("0.5"));
  EXPECT_FALSE(handler.setDelta("0"));
  EXPECT_FALSE(handler.setDelta("-1"));
  EXPECT_FALSE(handler.setDelta("bad"));
  EXPECT_TRUE(handler.setMinVal("-2.5"));
  EXPECT_FALSE(handler.setMinVal("bad"));
}

// Covers input-file validation and handle-without-file behavior.
TEST(LogBinHandlerTest, RejectsMissingInputFile)
{
  TempDir temp("alogbin_file");
  LogBinHandler missing;

  EXPECT_FALSE(missing.setLogFile(temp.filePath("missing.txt")));

  LogBinHandler no_file;
  EXPECT_FALSE(no_file.handle());
}

// Covers empty readable input files.
TEST(LogBinHandlerTest, EmptyInputProducesNoBins)
{
  TempDir temp("alogbin_empty");
  const std::string input = temp.writeFile("input.txt", "");

  LogBinHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// Covers comment-only readable input files.
TEST(LogBinHandlerTest, CommentOnlyInputProducesNoBins)
{
  TempDir temp("alogbin_comments_only");
  const std::string input = temp.writeFile(
      "input.txt", WithNewlines({"% comment", "%% metadata"}));

  LogBinHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// Covers default binning, comments, and bad-line diagnostics.
TEST(LogBinHandlerTest, BinsNumericRowsAndReportsBadLines)
{
  TempDir temp("alogbin_default");
  const std::string input = temp.writeFile(
      "input.txt",
      WithNewlines({"% comment", "0.1", "0.9", "1.2", "bad"}));

  LogBinHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "Bad line: [bad]"));
  EXPECT_TRUE(Contains(output, "0.5  2"));
  EXPECT_TRUE(Contains(output, "1.5  1"));
}

// Covers current min/delta bin formula behavior.
TEST(LogBinHandlerTest, AppliesMinimumAndDeltaInBinFormula)
{
  TempDir temp("alogbin_min_delta");
  const std::string input = temp.writeFile(
      "input.txt", WithNewlines({"1.1", "1.4", "1.6"}));

  LogBinHandler handler;
  ASSERT_TRUE(handler.setDelta("0.5"));
  ASSERT_TRUE(handler.setMinVal("1.0"));
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "0.25  2"));
  EXPECT_TRUE(Contains(output, "0.75  1"));
}

// Covers exact-boundary values in the truncation-based bin formula.
TEST(LogBinHandlerTest, ExactBoundaryValuesMoveToUpperHalfBin)
{
  TempDir temp("alogbin_boundary");
  const std::string input = temp.writeFile(
      "input.txt", WithNewlines({"0", "1", "2"}));

  LogBinHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "0.5  1"));
  EXPECT_TRUE(Contains(output, "1.5  1"));
  EXPECT_TRUE(Contains(output, "2.5  1"));
}

// Covers negative values with the handler's truncation-based bin formula.
TEST(LogBinHandlerTest, BinsNegativeValuesTowardZero)
{
  TempDir temp("alogbin_negative");
  const std::string input = temp.writeFile(
      "input.txt", WithNewlines({"-0.1", "-0.9", "-1.2"}));

  LogBinHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "-0.5  1"));
  EXPECT_TRUE(Contains(output, "0.5  2"));
}

// Covers verbose mapping output before the final bin report.
TEST(LogBinHandlerTest, VerboseModePrintsValueToBinMapping)
{
  TempDir temp("alogbin_verbose");
  const std::string input = temp.writeFile("input.txt", "0.1\n");

  LogBinHandler handler;
  handler.setVerbose();
  ASSERT_TRUE(handler.setLogFile(input));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "0.1 --> 0.5"));
  EXPECT_TRUE(Contains(output, "0.5  1"));
}
