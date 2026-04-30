#include <gtest/gtest.h>

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

#include "fileutil.h"
#include "stringutil.h"
#include "TestFileUtils.h"

// Covers string util behavior: tokenizes MOOS style comma and double colon payloads.
TEST(StringUtilTest, TokenizesMoosStyleCommaAndDoubleColonPayloads)
{
  EXPECT_EQ(tokenize("", ","), std::vector<std::string>{});
  EXPECT_EQ(tokenize("DEPLOY=true", ","),
            (std::vector<std::string>{"DEPLOY=true"}));
  EXPECT_EQ(tokenize("x=1,y=2,", ","),
            (std::vector<std::string>{"x=1", "y=2", ""}));
  EXPECT_EQ(tokenize(",front", ","),
            (std::vector<std::string>{"", "front"}));
  EXPECT_EQ(tokenize("src=pHelmIvP::aux=waypt_survey::iter=42", "::"),
            (std::vector<std::string>{"src=pHelmIvP",
                                      "aux=waypt_survey",
                                      "iter=42"}));
}

// Covers string util behavior: handles repeated and absent separators literally.
TEST(StringUtilTest, HandlesRepeatedAndAbsentSeparatorsLiterally)
{
  EXPECT_EQ(tokenize("a,,b", ","), (std::vector<std::string>{"a", "", "b"}));
  EXPECT_EQ(tokenize("abc", "::"), (std::vector<std::string>{"abc"}));
  EXPECT_EQ(tokenize("a::::b", "::"), (std::vector<std::string>{"a", "", "b"}));
}

// Covers string util behavior: pins overlapping and long separator behavior.
TEST(StringUtilTest, PinsOverlappingAndLongSeparatorBehavior)
{
  EXPECT_EQ(tokenize("aaaa", "aa"), (std::vector<std::string>{"", "", ""}));
  EXPECT_EQ(tokenize("ababa", "aba"), (std::vector<std::string>{"", "ba"}));
  EXPECT_EQ(tokenize("alpha", "alpha"), (std::vector<std::string>{"", ""}));
  EXPECT_EQ(tokenize("alpha", "alpha-beta"),
            (std::vector<std::string>{"alpha"}));
}

// Covers string util behavior: preserves whitespace and multi character separators for logs.
TEST(StringUtilTest, PreservesWhitespaceAndMultiCharacterSeparatorsForLogs)
{
  EXPECT_EQ(tokenize("  DEPLOY=true , RETURN=false  ", ","),
            (std::vector<std::string>{"  DEPLOY=true ", " RETURN=false  "}));
  EXPECT_EQ(tokenize("src=pHelmIvP::aux=waypt::", "::"),
            (std::vector<std::string>{"src=pHelmIvP", "aux=waypt", ""}));
  EXPECT_EQ(tokenize("::front::back", "::"),
            (std::vector<std::string>{"", "front", "back"}));
  EXPECT_EQ(tokenize("alpha<>beta<>gamma", "<>"),
            (std::vector<std::string>{"alpha", "beta", "gamma"}));
}

// Covers string util behavior: renders vectors with prefix suffix and final newline.
TEST(StringUtilTest, RendersVectorsWithPrefixSuffixAndFinalNewline)
{
  EXPECT_EQ(vect_to_string("  - ", "", {"DEPLOY", "RETURN"}),
            "  - DEPLOY\n"
            "  - RETURN\n");
  EXPECT_EQ(vect_to_string("[", "]", {"pHelmIvP", "pMarinePIDV22"}),
            "[pHelmIvP]\n"
            "[pMarinePIDV22]\n");
  EXPECT_EQ(vect_to_string("<", ">", {}), "");
}

// Covers string util behavior: renders empty and whitespace elements verbatim.
TEST(StringUtilTest, RendersEmptyAndWhitespaceElementsVerbatim)
{
  EXPECT_EQ(vect_to_string("'", "'", {"", "  ", "NAV_X=12"}),
            "''\n"
            "'  '\n"
            "'NAV_X=12'\n");
}

// Covers string util behavior: renders MOOS path and config vectors without escaping.
TEST(StringUtilTest, RendersMoosPathAndConfigVectorsWithoutEscaping)
{
  EXPECT_EQ(vect_to_string("path=", "", {"/opt/moos-ivp/lib", "/tmp/a:b"}),
            "path=/opt/moos-ivp/lib\n"
            "path=/tmp/a:b\n");
  EXPECT_EQ(vect_to_string("", ";", {"DEPLOY=true", "RETURN=false"}),
            "DEPLOY=true;\n"
            "RETURN=false;\n");
}

// Covers file util behavior: distinguishes directories regular files and missing paths.
TEST(FileUtilTest, DistinguishesDirectoriesRegularFilesAndMissingPaths)
{
  TempDir root("genutil_fileutil");
  const std::string dir = root.filePath("dir");
  const std::string file = dir + "/mission.moos";
  const std::string missing = dir + "/missing.alog";

  ASSERT_TRUE((::mkdir(dir.c_str(), 0755) == 0) || (errno == EEXIST));
  std::ofstream out(file.c_str());
  out << "ServerHost = localhost\n";
  out.close();

  EXPECT_TRUE(isDirectory(dir));
  EXPECT_FALSE(isRegularFile(dir));
  EXPECT_TRUE(isRegularFile(file));
  EXPECT_FALSE(isDirectory(file));
  EXPECT_FALSE(isDirectory(missing));
  EXPECT_FALSE(isRegularFile(missing));
}

// Covers file util behavior: treats nested mission files and trailing slash directories.
TEST(FileUtilTest, TreatsNestedMissionFilesAndTrailingSlashDirectories)
{
  TempDir root("genutil_nested");
  const std::string dir = root.filePath("root");
  const std::string child_dir = dir + "/missions";
  const std::string file = child_dir + "/alpha.bhv";

  ASSERT_TRUE((::mkdir(dir.c_str(), 0755) == 0) || (errno == EEXIST));
  ASSERT_TRUE((::mkdir(child_dir.c_str(), 0755) == 0) || (errno == EEXIST));
  std::ofstream out(file.c_str());
  out << "Behavior = BHV_Waypoint\n";
  out.close();

  EXPECT_TRUE(isDirectory(child_dir));
  EXPECT_TRUE(isDirectory(child_dir + "/"));
  EXPECT_TRUE(isRegularFile(file));
  EXPECT_FALSE(isDirectory(file + "/"));
  EXPECT_FALSE(isRegularFile(child_dir + "/"));
}

// Covers file util behavior: lists directory entries including dot entries.
TEST(FileUtilTest, ListsDirectoryEntriesIncludingDotEntries)
{
  TempDir root("genutil_listdir");
  const std::string dir = root.filePath("dir");
  ASSERT_TRUE((::mkdir(dir.c_str(), 0755) == 0) || (errno == EEXIST));
  std::ofstream(dir + "/alpha.bhv").put('\n');
  std::ofstream(dir + "/beta.moos").put('\n');

  std::vector<std::string> entries;
  EXPECT_EQ(listdir(dir, entries), 0);
  EXPECT_NE(std::find(entries.begin(), entries.end(), "."), entries.end());
  EXPECT_NE(std::find(entries.begin(), entries.end(), ".."), entries.end());
  EXPECT_NE(std::find(entries.begin(), entries.end(), "alpha.bhv"), entries.end());
  EXPECT_NE(std::find(entries.begin(), entries.end(), "beta.moos"), entries.end());
}

// Covers file util behavior: successful listdir appends without sorting or clearing.
TEST(FileUtilTest, SuccessfulListdirAppendsWithoutSortingOrClearing)
{
  TempDir root("genutil_append_listdir");
  const std::string dir = root.filePath("dir");
  ASSERT_TRUE((::mkdir(dir.c_str(), 0755) == 0) || (errno == EEXIST));
  std::ofstream(dir + "/zeta.moos").put('\n');

  std::vector<std::string> entries = {"preexisting"};
  EXPECT_EQ(listdir(dir, entries), 0);
  EXPECT_EQ(entries.front(), "preexisting");
  EXPECT_NE(std::find(entries.begin(), entries.end(), "zeta.moos"), entries.end());
}

// Covers file util behavior: detects symlinked mission files as regular files when available.
TEST(FileUtilTest, DetectsSymlinkedMissionFilesAsRegularFilesWhenAvailable)
{
  TempDir root("genutil_symlink");
  const std::string dir = root.path();
  const std::string target = dir + "/target.moos";
  const std::string link = dir + "/linked.moos";

  ::unlink(link.c_str());
  ::unlink(target.c_str());
  std::ofstream(target.c_str()) << "ProcessConfig = pHelmIvP\n";

  ASSERT_EQ(::symlink(target.c_str(), link.c_str()), 0);

  EXPECT_TRUE(isRegularFile(link));
  EXPECT_FALSE(isDirectory(link));
}

// Covers file util behavior: returns errno for missing directories without clearing output.
TEST(FileUtilTest, ReturnsErrnoForMissingDirectoriesWithoutClearingOutput)
{
  TempDir root("genutil_missing");
  std::vector<std::string> entries = {"existing"};
  int result = listdir(root.filePath("missing_genutil_dir"), entries);
  EXPECT_NE(result, 0);
  EXPECT_EQ(entries, (std::vector<std::string>{"existing"}));
}
