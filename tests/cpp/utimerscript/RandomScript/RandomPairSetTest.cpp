#include <gtest/gtest.h>

#include <string>

#include "RandomPairSet.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Coverage note: this suite covers TS_MOOSApp rand_pair/randpair config
// parsing, key validation, duplicate variable protection across both pair
// variables, reset gating, and the current success-path return string quirk.
// Crash-only invalid polygon parsing is excluded from normal CTest coverage.

TEST(RandomPairSetTest, AddsQuotedPolygonPairDespiteSuccessReturnStringQuirk)
{
  RandomPairSet set;

  EXPECT_EQ(set.addRandomPair("type=poly,var1=PX,var2=PY,key=AT_POST,"
                              "poly=\"pts={0,0:10,0:10,10:0,10}\""),
            "bad poly");

  ASSERT_EQ(set.size(), 1u);
  EXPECT_TRUE(set.contains("PX"));
  EXPECT_TRUE(set.contains("PY"));
  EXPECT_EQ(set.getVarName1(0), "PX");
  EXPECT_EQ(set.getVarName2(0), "PY");
  EXPECT_EQ(set.getKeyName(0), "at_post");
  EXPECT_EQ(set.getType(0), "poly");
  EXPECT_TRUE(Contains(set.getStringSummary(0), "type=poly"));
}

TEST(RandomPairSetTest, AddsUnquotedPolygonPairBecauseBracesProtectCommas)
{
  RandomPairSet set;

  EXPECT_EQ(set.addRandomPair("type=poly, var1=PX, var2=PY, key=at_reset,"
                              " poly=pts={0,0:5,0:5,5:0,5}"),
            "bad poly");

  ASSERT_EQ(set.size(), 1u);
  EXPECT_EQ(set.getVarName1(0), "PX");
  EXPECT_EQ(set.getVarName2(0), "PY");
  EXPECT_EQ(set.getKeyName(0), "at_reset");
  set.reset("at_reset");
  EXPECT_GE(set.getValue1(0), 0.0);
  EXPECT_LE(set.getValue1(0), 5.0);
  EXPECT_GE(set.getValue2(0), 0.0);
  EXPECT_LE(set.getValue2(0), 5.0);
}

TEST(RandomPairSetTest, ResetOnlyUpdatesPairsForTheRequestedKey)
{
  RandomPairSet set;
  ASSERT_EQ(set.addRandomPairPoly("var1=AX,var2=AY,key=at_start,"
                                  "poly=\"pts={0,0:10,0:10,10:0,10}\""),
            "bad poly");
  ASSERT_EQ(set.addRandomPairPoly("var1=BX,var2=BY,key=at_post,"
                                  "poly=\"pts={20,20:30,20:30,30:20,30}\""),
            "bad poly");

  set.reset("at_start");
  EXPECT_GE(set.getValue1(0), 0.0);
  EXPECT_LE(set.getValue1(0), 10.0);
  EXPECT_EQ(set.getStringValue1(1), "");

  set.reset("at_post");
  EXPECT_GE(set.getValue1(1), 20.0);
  EXPECT_LE(set.getValue1(1), 30.0);
  EXPECT_FALSE(set.getStringValue1(1).empty());
}

TEST(RandomPairSetTest, RejectsUnknownTypeAndBadParameters)
{
  RandomPairSet set;

  EXPECT_EQ(set.addRandomPair("type=grid,var1=X,var2=Y,key=at_post"),
            "unknown random variable type: grid");
  EXPECT_EQ(set.addRandomPair("var1=X,var2=Y,key=at_post"),
            "unknown random variable type: ");
  EXPECT_EQ(set.addRandomPairPoly("var1=X,var2=Y,key=at_post,foo=bar,"
                                  "poly=\"pts={0,0:1,0:1,1:0,1}\""),
            "Bad parameter=value: foo=bar");
  EXPECT_EQ(set.size(), 0u);
}

TEST(RandomPairSetTest, RejectsMissingAndInvalidRequiredFields)
{
  RandomPairSet set;

  EXPECT_EQ(set.addRandomPairPoly("var1=X,var2=Y,poly=\"pts={0,0:1,0:1,1:0,1}\""),
            "key is not specified");
  EXPECT_EQ(set.addRandomPairPoly("var1=X,var2=Y,key=at_stop,"
                                  "poly=\"pts={0,0:1,0:1,1:0,1}\""),
            "unknown random_pair key: at_stop");
  EXPECT_EQ(set.addRandomPairPoly("var2=Y,key=at_post,"
                                  "poly=\"pts={0,0:1,0:1,1:0,1}\""),
            "Unset variable1 name");
  EXPECT_EQ(set.addRandomPairPoly("var1=X,key=at_post,"
                                  "poly=\"pts={0,0:1,0:1,1:0,1}\""),
            "Unset variable2 name");
  EXPECT_EQ(set.addRandomPairPoly("var1=X,var2=Y,key=at_post"),
            "Minimum value greater than maximum value");
}

TEST(RandomPairSetTest, RejectsDuplicateNamesAcrossEitherPairSlot)
{
  RandomPairSet set;
  ASSERT_EQ(set.addRandomPairPoly("var1=X,var2=Y,key=at_post,"
                                  "poly=\"pts={0,0:10,0:10,10:0,10}\""),
            "bad poly");

  EXPECT_EQ(set.addRandomPairPoly("var1=X,var2=Z,key=at_post,"
                                  "poly=\"pts={0,0:10,0:10,10:0,10}\""),
            "Duplicate random pair variable:X");
  EXPECT_EQ(set.addRandomPairPoly("var1=A,var2=Y,key=at_post,"
                                  "poly=\"pts={0,0:10,0:10,10:0,10}\""),
            "Duplicate random pair variable:Y");
  EXPECT_EQ(set.size(), 1u);
}

TEST(RandomPairSetTest, OutOfRangeAccessorsReturnEmptyOrZeroDefaults)
{
  RandomPairSet set;

  EXPECT_EQ(set.getVarName1(4), "");
  EXPECT_EQ(set.getVarName2(4), "");
  EXPECT_EQ(set.getKeyName(4), "");
  EXPECT_EQ(set.getType(4), "");
  EXPECT_DOUBLE_EQ(set.getValue1(4), 0);
  EXPECT_DOUBLE_EQ(set.getValue2(4), 0);
  EXPECT_EQ(set.getStringValue1(4), "");
  EXPECT_EQ(set.getStringValue2(4), "");
  EXPECT_EQ(set.getStringSummary(4), "");
  EXPECT_EQ(set.getParams(4), "");
}
