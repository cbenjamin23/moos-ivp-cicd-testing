#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "TagHandler.h"
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

// Source audit note: this suite covers TagHandler's deterministic file/tag
// validation, tag activation, comment and blank filtering, optional tag-line
// echoing, header prefixing, and first-line mode against app_tagrep. CLI
// argument parsing is intentionally outside these direct component tests.

// Covers input-file validation and single-assignment behavior.
TEST(TagHandlerTest, ValidatesInputFileAndRejectsDuplicateInput)
{
  TempDir temp("tagrep_input");
  const std::string input = temp.writeFile("input.txt", "<tag>alpha\nvalue");
  const std::string other = temp.writeFile("other.txt", "<tag>alpha\nother\n");

  TagHandler missing;
  EXPECT_FALSE(missing.setInputFile(temp.filePath("missing.txt")));
  EXPECT_FALSE(missing.isInputFileSet());

  TagHandler handler;
  EXPECT_TRUE(handler.setInputFile(input));
  EXPECT_TRUE(handler.isInputFileSet());
  EXPECT_FALSE(handler.setInputFile(other));
}

// Covers tag/header single-assignment and chevron/blank normalization.
TEST(TagHandlerTest, NormalizesTagAndRejectsDuplicateTagAndHeader)
{
  TempDir temp("tagrep_tag");
  const std::string input = temp.writeFile("input.txt", "<tag>alpha\nvalue");

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  EXPECT_TRUE(handler.setTag(" <alpha> "));
  EXPECT_FALSE(handler.setTag("beta"));
  EXPECT_TRUE(handler.setHeader("same_as_tag"));
  EXPECT_FALSE(handler.setHeader("prefix"));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "alpha=value\n");
}

// Covers required tag and empty/unreadable content handling.
TEST(TagHandlerTest, RejectsMissingTagAndEmptyFile)
{
  TempDir temp("tagrep_empty");
  const std::string empty = temp.writeFile("empty.txt", "");

  TagHandler no_file;
  ASSERT_TRUE(no_file.setTag("alpha"));
  EXPECT_FALSE(no_file.handle());

  TagHandler no_tag;
  ASSERT_TRUE(no_tag.setInputFile(empty));
  EXPECT_FALSE(no_tag.handle());

  TagHandler empty_handler;
  ASSERT_TRUE(empty_handler.setInputFile(empty));
  ASSERT_TRUE(empty_handler.setTag("alpha"));
  EXPECT_FALSE(empty_handler.handle());
}

// Covers active-region extraction, nonmatching tag deactivation, comment
// suppression, and blank-line preservation.
TEST(TagHandlerTest, ExtractsOnlyActiveMatchingRegion)
{
  TempDir temp("tagrep_region");
  const std::string input = temp.writeFile(
      "input.txt",
      WithNewlines({"before",
                    "<tag>alpha",
                    "alpha one",
                    "",
                    "# skipped comment",
                    "alpha two",
                    "<tag>beta",
                    "beta hidden",
                    "#<tag>alpha",
                    "alpha three"}));

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  ASSERT_TRUE(handler.setTag("alpha"));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "alpha one\n\nalpha two\n"));
  EXPECT_TRUE(Contains(output, "alpha three\n"));
  EXPECT_FALSE(Contains(output, "before"));
  EXPECT_FALSE(Contains(output, "beta hidden"));
  EXPECT_FALSE(Contains(output, "skipped comment"));
}

// Covers optional blank suppression.
TEST(TagHandlerTest, CanSuppressBlankLines)
{
  TempDir temp("tagrep_no_blanks");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"<tag>alpha", "one", "", "two"}));

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  ASSERT_TRUE(handler.setTag("alpha"));
  handler.setNoBlankLines();

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "one\ntwo\n");
}

// Covers first-line mode and optional original tag-line echoing.
TEST(TagHandlerTest, CanEchoTagLineAndStopAfterFirstDataLine)
{
  TempDir temp("tagrep_first");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"  <tag>alpha", "one", "two"}));

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  ASSERT_TRUE(handler.setTag("alpha"));
  handler.setKeepTagLine();
  handler.setFirstLineOnly();

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "  <tag>alpha\none\n");
}

// Covers custom header prefixing each time the matching tag is activated.
TEST(TagHandlerTest, AppliesCustomHeaderToEachMatchingRegion)
{
  TempDir temp("tagrep_header");
  const std::string input = temp.writeFile(
      "input.txt",
      WithNewlines({"<tag>alpha", "one", "<tag>beta", "hidden", "<tag>alpha", "two"}));

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  ASSERT_TRUE(handler.setTag("alpha"));
  ASSERT_TRUE(handler.setHeader("prefix"));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "prefix=one\nprefix=two\n\n");
}

// Covers header output when blank active lines are preserved.
TEST(TagHandlerTest, HeaderCanPrefixBlankActiveLine)
{
  TempDir temp("tagrep_header_blank");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"<tag>alpha", "", "one"}));

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  ASSERT_TRUE(handler.setTag("alpha"));
  ASSERT_TRUE(handler.setHeader("prefix"));

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "prefix=\none\n\n");
}

// Covers unmatched tags returning false after processing nonempty input.
TEST(TagHandlerTest, ReportsFalseWhenTagNeverActivated)
{
  TempDir temp("tagrep_no_match");
  const std::string input = temp.writeFile("input.txt", "<tag>beta\nvalue\n");

  TagHandler handler;
  ASSERT_TRUE(handler.setInputFile(input));
  ASSERT_TRUE(handler.setTag("alpha"));

  testing::internal::CaptureStdout();
  EXPECT_FALSE(handler.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}
