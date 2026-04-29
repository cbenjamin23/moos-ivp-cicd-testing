#include <gtest/gtest.h>

#include "ScopeEntry.h"

TEST(ScopeEntryTest, StoresFieldsUsedByXmsAndHelmScope)
{
  ScopeEntry entry;
  entry.setValue("deploy=true");
  entry.setType("string");
  entry.setSource("pHelmIvP");
  entry.setSrcAux("23:wpt_survey");
  entry.setTime("123.45");
  entry.setCommunity("abe");
  entry.setIteration("23");
  entry.setNameBHV("wpt_survey");

  EXPECT_EQ(entry.getValue(), "deploy=true");
  EXPECT_EQ(entry.getType(), "string");
  EXPECT_EQ(entry.getSource(), "pHelmIvP");
  EXPECT_EQ(entry.getSrcAux(), "23:wpt_survey");
  EXPECT_EQ(entry.getTime(), "123.45");
  EXPECT_EQ(entry.getCommunity(), "abe");
  EXPECT_EQ(entry.getIteration(), "23");
  EXPECT_EQ(entry.getNameBHV(), "wpt_survey");
}

TEST(ScopeEntryTest, ComparesEveryStoredFieldForEquality)
{
  ScopeEntry lhs;
  lhs.setValue("45.2");
  lhs.setType("double");
  lhs.setSource("pNodeReporter");
  lhs.setSrcAux("src_aux");
  lhs.setTime("10.5");
  lhs.setCommunity("shoreside");
  lhs.setIteration("99");
  lhs.setNameBHV("loiter");

  ScopeEntry rhs = lhs;
  EXPECT_TRUE(lhs.isEqual(rhs));

  rhs.setNameBHV("station_keep");
  EXPECT_FALSE(lhs.isEqual(rhs));

  rhs = lhs;
  rhs.setTime("11.0");
  EXPECT_FALSE(lhs.isEqual(rhs));
}
