#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Plucker.h"
#include "TestFileUtils.h"

namespace {

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

}  // namespace

// Source audit note: this suite covers Plucker's deterministic file validation,
// counted-line selection, positional token extraction, grep/comment filtering,
// field parsing, and separator validation against app_pluck. The CLI wrapper is
// intentionally outside these direct component tests.

// Covers file validation and no-file/zero-byte file return text.
TEST(PluckerTest, ValidatesReadableFileAndEmptyInputs)
{
  TempDir temp("pluck_file");
  const std::string empty = temp.writeFile("empty.txt", "");

  Plucker missing;
  EXPECT_FALSE(missing.setFile(temp.filePath("missing.txt")));

  Plucker no_file;
  EXPECT_EQ(no_file.pluck(), "empty file");

  // fileBuffer() returns one blank row for a readable zero-byte file, so the
  // loop completes without selecting a counted line.
  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(empty));
  EXPECT_EQ(plucker.pluck(), "");
}

// Covers unsigned numeric setters and rejection of malformed input.
TEST(PluckerTest, ValidatesLineNumberAndIndex)
{
  Plucker plucker;

  EXPECT_TRUE(plucker.setLNum("1"));
  EXPECT_TRUE(plucker.setIndex("2"));
  EXPECT_FALSE(plucker.setLNum("-1"));
  EXPECT_FALSE(plucker.setLNum("bad"));
  EXPECT_FALSE(plucker.setIndex("-2"));
  EXPECT_FALSE(plucker.setIndex("bad"));
}

// Covers positional token extraction from the selected counted line.
TEST(PluckerTest, PlucksPositionalTokenFromSelectedLine)
{
  TempDir temp("pluck_position");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"alpha beta gamma",
                                                "delta epsilon zeta"}));

  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(input));
  ASSERT_TRUE(plucker.setLNum("2"));
  ASSERT_TRUE(plucker.setIndex("2"));

  EXPECT_EQ(plucker.pluck(), "epsilon");
}

// Covers default zero line/index selection semantics.
TEST(PluckerTest, ReturnsEmptyForUnsetLineOrZeroIndex)
{
  TempDir temp("pluck_zero");
  const std::string input = temp.writeFile("input.txt", "alpha beta\n");

  Plucker no_line;
  ASSERT_TRUE(no_line.setFile(input));
  ASSERT_TRUE(no_line.setIndex("1"));
  EXPECT_EQ(no_line.pluck(), "");

  Plucker zero_index;
  ASSERT_TRUE(zero_index.setFile(input));
  ASSERT_TRUE(zero_index.setLNum("1"));
  EXPECT_EQ(zero_index.pluck(), "");
}

// Covers line/index selections beyond available counted content.
TEST(PluckerTest, ReturnsEmptyWhenSelectionIsOutOfRange)
{
  TempDir temp("pluck_out_of_range");
  const std::string input = temp.writeFile("input.txt", "alpha beta\n");

  Plucker late_line;
  ASSERT_TRUE(late_line.setFile(input));
  ASSERT_TRUE(late_line.setLNum("2"));
  ASSERT_TRUE(late_line.setIndex("1"));
  EXPECT_EQ(late_line.pluck(), "");

  Plucker late_index;
  ASSERT_TRUE(late_index.setFile(input));
  ASSERT_TRUE(late_index.setLNum("1"));
  ASSERT_TRUE(late_index.setIndex("3"));
  EXPECT_EQ(late_index.pluck(), "");
}

// Covers blank/comment skipping and grep filtering before counted-line
// selection.
TEST(PluckerTest, CountsOnlyNonblankNoncommentGrepMatches)
{
  TempDir temp("pluck_filters");
  const std::string input = temp.writeFile(
      "input.txt",
      WithNewlines({"",
                    "# ignored",
                    "keep first value",
                    "drop second value",
                    "keep third value"}));

  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(input));
  ASSERT_TRUE(plucker.setLNum("2"));
  ASSERT_TRUE(plucker.setIndex("3"));
  ASSERT_TRUE(plucker.setGrep("keep"));

  EXPECT_EQ(plucker.pluck(), "value");
}

// Covers configurable comment prefix.
TEST(PluckerTest, HonorsCustomCommentPrefix)
{
  TempDir temp("pluck_comment");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"// ignored", "# retained line"}));

  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(input));
  ASSERT_TRUE(plucker.setComment("//"));
  ASSERT_TRUE(plucker.setLNum("1"));
  ASSERT_TRUE(plucker.setIndex("2"));

  EXPECT_EQ(plucker.pluck(), "retained");
}

// Covers field-name extraction using the default group and local separators.
TEST(PluckerTest, PlucksNamedFieldWithDefaultSeparators)
{
  TempDir temp("pluck_field");
  const std::string input =
      temp.writeFile("input.txt", "x=4,y=5,name=abe\nx=8,y=9,name=ben\n");

  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(input));
  ASSERT_TRUE(plucker.setLNum("2"));
  ASSERT_TRUE(plucker.setFld("name"));

  EXPECT_EQ(plucker.pluck(), "ben");
}

// Covers field filtering before counted-line selection.
TEST(PluckerTest, CountsOnlyLinesContainingRequestedField)
{
  TempDir temp("pluck_field_count");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"x=1,y=2", "name=abe,x=3", "name=ben,x=4"}));

  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(input));
  ASSERT_TRUE(plucker.setLNum("2"));
  ASSERT_TRUE(plucker.setFld("name"));

  EXPECT_EQ(plucker.pluck(), "ben");
}

// Covers grep misses and field-substring false positives returning no value.
TEST(PluckerTest, ReturnsEmptyForNoGrepMatchAndFieldSubstringOnly)
{
  TempDir temp("pluck_no_match");
  const std::string input =
      temp.writeFile("input.txt", WithNewlines({"username=abe,x=1", "speed=2"}));

  Plucker grep_miss;
  ASSERT_TRUE(grep_miss.setFile(input));
  ASSERT_TRUE(grep_miss.setLNum("1"));
  ASSERT_TRUE(grep_miss.setIndex("1"));
  ASSERT_TRUE(grep_miss.setGrep("missing"));
  EXPECT_EQ(grep_miss.pluck(), "");

  Plucker field_substring;
  ASSERT_TRUE(field_substring.setFile(input));
  ASSERT_TRUE(field_substring.setLNum("1"));
  ASSERT_TRUE(field_substring.setFld("name"));
  EXPECT_EQ(field_substring.pluck(), "");
}

// Covers separator length validation.
TEST(PluckerTest, RejectsMultiCharacterSeparators)
{
  Plucker plucker;

  EXPECT_TRUE(plucker.setGSep(";"));
  EXPECT_TRUE(plucker.setLSep(":"));
  EXPECT_FALSE(plucker.setGSep("::"));
  EXPECT_FALSE(plucker.setLSep(""));
}

// Covers current upstream behavior: setLSep() assigns the group separator
// member, so alternate local separators are not applied as callers would expect.
TEST(PluckerTest, LocalSeparatorSetterDoesNotEnableAlternateLocalSeparator)
{
  TempDir temp("pluck_lsep");
  const std::string input = temp.writeFile("input.txt", "name:abe;speed:2\n");

  Plucker plucker;
  ASSERT_TRUE(plucker.setFile(input));
  ASSERT_TRUE(plucker.setLNum("1"));
  ASSERT_TRUE(plucker.setFld("name"));
  ASSERT_TRUE(plucker.setGSep(";"));
  ASSERT_TRUE(plucker.setLSep(":"));

  EXPECT_EQ(plucker.pluck(), "");
}
