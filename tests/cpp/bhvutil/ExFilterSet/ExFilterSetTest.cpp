#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ExFilterSet.h"
#include "GeometryTestUtils.h"
#include "NodeRecord.h"
#include "XYFormatUtilsPoly.h"

namespace {

NodeRecord contact(const std::string& name,
                   const std::string& group,
                   const std::string& type,
                   double x,
                   double y)
{
  NodeRecord record(name, type);
  record.setGroup(group);
  record.setX(x);
  record.setY(y);
  return record;
}

}  // namespace

// Covers ex filter set behavior: applies name group type and region filters for contact behaviors.
TEST(ExFilterSetTest, AppliesNameGroupTypeAndRegionFiltersForContactBehaviors)
{
  ExFilterSet filters;
  ASSERT_TRUE(filters.addMatchGroup("alpha"));
  ASSERT_TRUE(filters.addMatchType("uuv"));
  ASSERT_TRUE(filters.addIgnoreName("ben"));
  ASSERT_TRUE(filters.addIgnoreRegion("pts={40,40:60,40:60,60:40,60}"));

  EXPECT_TRUE(filters.filterCheck(contact("abe", "alpha", "uuv", 10, 10)));
  EXPECT_FALSE(filters.filterCheck(contact("cal", "bravo", "uuv", 10, 10)));
  EXPECT_FALSE(filters.filterCheck(contact("deb", "alpha", "usv", 10, 10)));
  EXPECT_FALSE(filters.filterCheck(contact("ben", "alpha", "uuv", 10, 10)));
  EXPECT_FALSE(filters.filterCheck(contact("eve", "alpha", "uuv", 50, 50)));
}

// Covers ex filter set behavior: strict ignore controls empty field rejection.
TEST(ExFilterSetTest, StrictIgnoreControlsEmptyFieldRejection)
{
  ExFilterSet filters;
  ASSERT_TRUE(filters.addIgnoreGroup("red"));

  EXPECT_FALSE(filters.filterCheckGroup(""));
  EXPECT_FALSE(filters.filterCheckGroup("red"));
  EXPECT_TRUE(filters.filterCheckGroup("blue"));

  ASSERT_TRUE(filters.setStrictIgnore(std::string("false")));
  EXPECT_TRUE(filters.filterCheckGroup(""));
  EXPECT_FALSE(filters.filterCheckGroup("red"));
}

// Covers ex filter set behavior: rejects conflicting name filters and supports removal.
TEST(ExFilterSetTest, RejectsConflictingNameFiltersAndSupportsRemoval)
{
  ExFilterSet filters;
  ASSERT_TRUE(filters.addMatchName("{abe,cal}"));

  EXPECT_TRUE(filters.filterCheckVName("abe"));
  EXPECT_FALSE(filters.filterCheckVName("ben"));
  EXPECT_FALSE(filters.addIgnoreName("abe"));

  EXPECT_TRUE(filters.removeMatchName("abe"));
  EXPECT_FALSE(filters.filterCheckVName("abe"));
  EXPECT_TRUE(filters.addIgnoreName("abe"));
  EXPECT_FALSE(filters.filterCheckVName("abe"));

  EXPECT_TRUE(filters.removeIgnoreName("abe"));
  EXPECT_TRUE(filters.addMatchName("abe"));
  EXPECT_TRUE(filters.filterCheckVName("abe"));
}

// Covers ex filter set behavior: config filter applies supported fields and reports unhandled tokens.
TEST(ExFilterSetTest, ConfigFilterAppliesSupportedFieldsAndReportsUnhandledTokens)
{
  ExFilterSet filters;
  std::string unhandled = filters.configFilter(
      "match_group=alpha,ignore_type=kayak,strict_ignore=false,bogus=value");

  EXPECT_EQ(unhandled, "bogus=value");
  EXPECT_TRUE(filters.filterCheckGroup("alpha"));
  EXPECT_FALSE(filters.filterCheckGroup("bravo"));
  EXPECT_FALSE(filters.filterCheckVType("kayak"));
  EXPECT_TRUE(filters.filterCheckVType(""));
}

// Covers ex filter set behavior: serializes summary in deterministic filter order.
TEST(ExFilterSetTest, SerializesSummaryInDeterministicFilterOrder)
{
  ExFilterSet filters;
  ASSERT_TRUE(filters.addIgnoreGroup("red"));
  ASSERT_TRUE(filters.addMatchGroup("blue"));
  ASSERT_TRUE(filters.addIgnoreType("kayak"));
  ASSERT_TRUE(filters.addMatchType("uuv"));
  ASSERT_TRUE(filters.addIgnoreRegion(makeSquarePoly(0, 0, 10, 10)));

  std::vector<std::string> summary = filters.getSummaryX();
  ASSERT_GE(summary.size(), 5u);
  EXPECT_EQ(summary[0], "ignore_group=red");
  EXPECT_EQ(summary[1], "match_group=blue");
  EXPECT_EQ(summary[2], "ignore_type=kayak");
  EXPECT_EQ(summary[3], "match_type=uuv");
  bool found_ignore_region = false;
  for(const std::string& item : summary)
    found_ignore_region = found_ignore_region || stringContains(item, "ignore_region=pts={");
  EXPECT_TRUE(found_ignore_region);
  EXPECT_EQ(summary.back(), "strict_ignore=true");
}

// Covers ex filter set behavior: combines match and ignore regions for operating area filters.
TEST(ExFilterSetTest, CombinesMatchAndIgnoreRegionsForOperatingAreaFilters)
{
  ExFilterSet filters;
  ASSERT_TRUE(filters.addMatchRegion("pts={0,0:100,0:100,100:0,100}"));
  ASSERT_TRUE(filters.addIgnoreRegion("pts={40,40:60,40:60,60:40,60}"));
  EXPECT_FALSE(filters.addMatchRegion("pts={0,0:10,10:0,10:10,0}"));

  EXPECT_TRUE(filters.filterCheckRegion(10, 10));
  EXPECT_FALSE(filters.filterCheckRegion(50, 50));
  EXPECT_FALSE(filters.filterCheckRegion(150, 50));

  EXPECT_TRUE(filters.filterCheck(contact("abe", "", "", 10, 10)));
  EXPECT_FALSE(filters.filterCheck(contact("ben", "", "", 50, 50)));
}

// Covers ex filter set behavior: direct checks expect already normalized case for names groups and types.
TEST(ExFilterSetTest, DirectChecksExpectAlreadyNormalizedCaseForNamesGroupsAndTypes)
{
  ExFilterSet filters;
  ASSERT_TRUE(filters.addMatchName("ABE"));
  ASSERT_TRUE(filters.addMatchGroup("ALPHA"));
  ASSERT_TRUE(filters.addMatchType("UUV"));

  EXPECT_TRUE(filters.filterCheckVName("abe"));
  EXPECT_FALSE(filters.filterCheckVName("ABE"));
  EXPECT_TRUE(filters.filterCheckGroup("alpha"));
  EXPECT_FALSE(filters.filterCheckGroup("ALPHA"));
  EXPECT_TRUE(filters.filterCheckVType("uuv"));
  EXPECT_FALSE(filters.filterCheckVType("UUV"));

  EXPECT_TRUE(filters.filterCheck(contact("ABE", "alpha", "uuv", 0, 0)));
  EXPECT_FALSE(filters.filterCheck(contact("ABE", "ALPHA", "uuv", 0, 0)));
  EXPECT_FALSE(filters.filterCheck(contact("ABE", "alpha", "UUV", 0, 0)));
}
