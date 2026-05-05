#include <gtest/gtest.h>

#include <string>

#include "RandPairPoly.h"
#include "RandomPair.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Coverage note: these tests cover the random pair value object and polygon
// sampler directly. Invalid non-convex polygon specs are intentionally left out
// because upstream RandPairPoly exits the process instead of returning false.

TEST(RandomPairTest, DefaultsAndSettersAppearInSummary)
{
  RandomPair pair;
  pair.setVarName1("PX");
  pair.setVarName2("PY");
  pair.setKeyName("at_post");
  pair.setType("poly");

  const std::string summary = pair.getStringSummary();

  EXPECT_EQ(pair.getVarName1(), "PX");
  EXPECT_EQ(pair.getVarName2(), "PY");
  EXPECT_EQ(pair.getKeyName(), "at_post");
  EXPECT_EQ(pair.getType(), "poly");
  EXPECT_DOUBLE_EQ(pair.getValue1(), 0);
  EXPECT_DOUBLE_EQ(pair.getValue2(), 0);
  EXPECT_TRUE(Contains(summary, "varname1=PX,varname2=PY"));
  EXPECT_TRUE(Contains(summary, "keyname=at_post"));
}

TEST(RandomPairTest, BaseParameterHooksRejectNumericAndStringValues)
{
  RandomPair pair;

  EXPECT_FALSE(pair.setParam("poly", "pts={0,0:1,0:1,1:0,1}"));
  EXPECT_FALSE(pair.setParam("snap", 1.0));
  EXPECT_EQ(pair.getStringValue1(), "");
  EXPECT_EQ(pair.getStringValue2(), "");
}

TEST(RandPairPolyTest, RejectsUnknownParameterWithoutChangingDefaults)
{
  RandPairPoly pair;

  EXPECT_FALSE(pair.setParam("unknown", "value"));
  EXPECT_DOUBLE_EQ(pair.getValue1(), 0);
  EXPECT_DOUBLE_EQ(pair.getValue2(), 0);
  EXPECT_TRUE(Contains(pair.getStringSummary(), "type=poly"));
}

TEST(RandPairPolyTest, ResetBeforePolygonIsConfiguredLeavesDefaultValues)
{
  RandPairPoly pair;

  pair.reset();

  EXPECT_DOUBLE_EQ(pair.getValue1(), 0);
  EXPECT_DOUBLE_EQ(pair.getValue2(), 0);
  EXPECT_EQ(pair.getStringValue1(), "");
  EXPECT_EQ(pair.getStringValue2(), "");
}

TEST(RandPairPolyTest, SamplesPointInsideConfiguredConvexPolygon)
{
  RandPairPoly pair;
  ASSERT_TRUE(pair.setParam("poly", "pts={0,0:10,0:10,10:0,10}"));

  for(int i = 0; i < 20; ++i) {
    pair.reset();
    EXPECT_GE(pair.getValue1(), 0.0);
    EXPECT_LE(pair.getValue1(), 10.0);
    EXPECT_GE(pair.getValue2(), 0.0);
    EXPECT_LE(pair.getValue2(), 10.0);
    EXPECT_FALSE(pair.getStringValue1().empty());
    EXPECT_FALSE(pair.getStringValue2().empty());
  }
}
