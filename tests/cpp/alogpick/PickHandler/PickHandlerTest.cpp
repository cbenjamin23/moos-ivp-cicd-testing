#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "PickHandler.h"
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

// Source audit note: this suite covers PickHandler's stable field validation,
// file validation, comment skipping, token parsing, duplicate fields, aligned
// and unaligned output paths against app_alogpick. The CLI parser is
// intentionally outside these direct component tests.

// Covers field validation.
TEST(PickHandlerTest, AddsOnlyWhitespaceFreeFields)
{
  PickHandler handler;

  EXPECT_TRUE(handler.addField("x"));
  EXPECT_TRUE(handler.addField("spd"));
  EXPECT_FALSE(handler.addField("bad field"));
}

// Covers input file validation.
TEST(PickHandlerTest, AcceptsOnlyOneReadableLogFile)
{
  TempDir temp("alogpick_file");
  const std::string input = temp.writeFile("input.alog", "");
  const std::string second = temp.writeFile("second.alog", "");

  PickHandler missing;
  EXPECT_FALSE(missing.setLogFile(temp.filePath("missing.alog")));

  PickHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));
  EXPECT_FALSE(handler.setLogFile(second));
}

// Covers handle validation for missing file or fields.
TEST(PickHandlerTest, HandleRejectsMissingFileOrFields)
{
  TempDir temp("alogpick_handle_validation");
  const std::string input =
      temp.writeFile("input.alog", "1.000 NODE_REPORT pNodeReporter x=1\n");

  PickHandler no_file;
  ASSERT_TRUE(no_file.addField("x"));
  EXPECT_FALSE(no_file.handle());

  PickHandler no_fields;
  ASSERT_TRUE(no_fields.setLogFile(input));
  testing::internal::CaptureStdout();
  EXPECT_FALSE(no_fields.handle());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "No fields identified"));
}

// Covers unaligned field extraction with missing field placeholders.
TEST(PickHandlerTest, PicksFieldsFromAlogRowsUnaligned)
{
  TempDir temp("alogpick_unaligned");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "1.000 NODE_REPORT pNodeReporter name=abe,x=1,y=2,spd=3",
                    "2.000 NODE_REPORT pNodeReporter name=ben,x=4,y=5"}));

  PickHandler handler;
  handler.setFormatAligned(false);
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.addField("x"));
  ASSERT_TRUE(handler.addField("spd"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, WithNewlines({"1 3", "4 -"}));
}

// Covers comment skipping before payload parsing.
TEST(PickHandlerTest, SkipsCommentLines)
{
  TempDir temp("alogpick_comments");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "1.000 NODE_REPORT pNodeReporter name=abe,x=1"}));

  PickHandler handler;
  handler.setFormatAligned(false);
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.addField("x"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "1\n");
}

// Covers whitespace trimming around key/value payload tokens.
TEST(PickHandlerTest, StripsWhitespaceAroundTokenNamesAndValues)
{
  TempDir temp("alogpick_whitespace");
  const std::string input = temp.writeFile(
      "input.alog",
      "1.000 NODE_REPORT pNodeReporter name=abe, x = 1 , spd = 3 \n");

  PickHandler handler;
  handler.setFormatAligned(false);
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.addField("x"));
  ASSERT_TRUE(handler.addField("spd"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "1 3\n");
}

// Covers duplicate selected fields being emitted independently.
TEST(PickHandlerTest, DuplicateFieldsProduceDuplicateColumns)
{
  TempDir temp("alogpick_duplicates");
  const std::string input = temp.writeFile(
      "input.alog", "1.000 NODE_REPORT pNodeReporter name=abe,x=1\n");

  PickHandler handler;
  handler.setFormatAligned(false);
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.addField("x"));
  ASSERT_TRUE(handler.addField("x"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "1 1\n");
}

// Covers aligned output path without depending on exact padding.
TEST(PickHandlerTest, AlignedOutputContainsPickedValues)
{
  TempDir temp("alogpick_aligned");
  const std::string input = temp.writeFile(
      "input.alog", "1.000 NODE_REPORT pNodeReporter name=abe,x=1,y=2\n");

  PickHandler handler;
  ASSERT_TRUE(handler.setLogFile(input));
  ASSERT_TRUE(handler.addField("x"));
  ASSERT_TRUE(handler.addField("y"));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1"));
  EXPECT_TRUE(Contains(output, "2"));
}
