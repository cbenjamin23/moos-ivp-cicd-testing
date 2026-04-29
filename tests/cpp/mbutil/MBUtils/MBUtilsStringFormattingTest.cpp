#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "MBUtils.h"

TEST(MBUtilsStringFormattingTest, ConvertsAndCompactsNumbersForSpecs)
{
  EXPECT_EQ(boolToString(true), "true");
  EXPECT_EQ(boolToString(false), "false");
  EXPECT_EQ(uintToString(42, 5), "00042");
  EXPECT_EQ(uintToCommaString(1234567), "1,234,567");
  EXPECT_EQ(ulintToCommaString(1234567890UL), "1,234,567,890");
  EXPECT_EQ(doubleToString(3.14159, 2), "3.14");
  EXPECT_EQ(doubleToStringX(3.1000, 4), "3.1");
  EXPECT_EQ(doubleToStringX(-0.00001, 3), "0");
}

TEST(MBUtilsStringFormattingTest, TruncatesPadsAndCompactsStrings)
{
  EXPECT_EQ(truncString("abcdefghijkl", 5), "abcde");
  EXPECT_EQ(truncString("abcdefghijkl", 5, "back"), "hijkl");
  EXPECT_EQ(truncString("abcdefghijkl", 6, "middle"), "ab..kl");
  EXPECT_EQ(truncString("abcdefghijkl", 3, "middle"), "abc");

  EXPECT_EQ(padString("7", 3), "  7");
  EXPECT_EQ(padString("7", 3, false), "7  ");
  EXPECT_EQ(compactConsecutive("Too         Bad", ' '), "Too Bad");
  EXPECT_EQ(stripComment("value=10 // mission note", "//"), "value=10 ");
}

TEST(MBUtilsCollectionFormattingTest, SortsMergesAndSerializesContainers)
{
  std::vector<std::string> sorted = sortStrings({"bravo", "alpha", "charlie"});
  ASSERT_EQ(sorted.size(), 3u);
  EXPECT_EQ(sorted[0], "alpha");
  EXPECT_EQ(sorted[1], "bravo");
  EXPECT_EQ(sorted[2], "charlie");

  std::vector<std::string> merged = mergeVectors({"a", "b"}, {"b", "c"});
  ASSERT_EQ(merged.size(), 4u);
  std::vector<std::string> unique = removeDuplicates(merged);
  ASSERT_EQ(unique.size(), 3u);
  EXPECT_EQ(svectorToString(unique, ':'), "a:b:c");

  std::set<std::string> set1 = {"alpha", "charlie"};
  std::set<std::string> set2 = {"bravo", "charlie"};
  EXPECT_EQ(setToString(mergeSets(set1, set2)), "alpha,bravo,charlie");
}

TEST(MBUtilsCollectionFormattingTest, PadsVectorsAndChecksMembership)
{
  std::vector<std::string> padded = padVector({"x", "long"}, true);
  ASSERT_EQ(padded.size(), 2u);
  EXPECT_EQ(padded[0], "   x");
  EXPECT_EQ(padded[1], "long");

  EXPECT_TRUE(vectorContains({"Alpha", "Bravo"}, "alpha", false));
  EXPECT_FALSE(vectorContains({"Alpha", "Bravo"}, "alpha", true));

  std::list<std::string> names = {"abe", "ben"};
  EXPECT_TRUE(listContains(names, "ABE", false));
  EXPECT_FALSE(listContains(names, "ABE", true));
}

TEST(MBUtilsStringFormattingTest, HandlesCaseAndPrefixSuffixChecks)
{
  EXPECT_EQ(tolower("Alpha_123"), "alpha_123");
  EXPECT_EQ(toupper("Alpha_123"), "ALPHA_123");
  EXPECT_TRUE(strBegins("BHV_Waypoint", "BHV"));
  EXPECT_TRUE(strBegins("BHV_Waypoint", "bhv", false));
  EXPECT_FALSE(strBegins("BHV_Waypoint", "bhv", true));
  EXPECT_TRUE(strEnds("VIEW_POLYGON", "polygon", false));
  EXPECT_TRUE(strContains("VIEW_POLYGON", "POLY"));
  EXPECT_TRUE(strContainsWhite("NAV X"));
  EXPECT_TRUE(strBeginsWhite("\tNAV_X"));
}

TEST(MBUtilsStringFormattingTest, IncrementsEmbeddedIntegerStrings)
{
  EXPECT_EQ(incIntString("foo23"), "24");
  EXPECT_EQ(incIntString("foo23bar45"), "2346");
  EXPECT_EQ(incIntString("foo23", 1, true), "24");
  EXPECT_EQ(incIntString("27foo", 4, true), "31");
}
