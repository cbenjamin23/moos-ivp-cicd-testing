#include <gtest/gtest.h>

#include <string>

#include "ScopeEntry.h"

// Covers scope entry behavior: defaults all fields to empty strings.
TEST(ScopeEntryTest, DefaultsAllFieldsToEmptyStrings)
{
  ScopeEntry entry;

  EXPECT_EQ(entry.getValue(), "");
  EXPECT_EQ(entry.getType(), "");
  EXPECT_EQ(entry.getSource(), "");
  EXPECT_EQ(entry.getSrcAux(), "");
  EXPECT_EQ(entry.getTime(), "");
  EXPECT_EQ(entry.getCommunity(), "");
  EXPECT_EQ(entry.getIteration(), "");
  EXPECT_EQ(entry.getNameBHV(), "");
  EXPECT_TRUE(entry.isEqual(ScopeEntry()));
}

// Covers scope entry behavior: stores fields used by xms and helm scope.
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

// Covers scope entry behavior: stores numeric scope values as strings without normalization.
TEST(ScopeEntryTest, StoresNumericScopeValuesAsStringsWithoutNormalization)
{
  ScopeEntry entry;
  entry.setValue("  0042.500 ");
  entry.setType("double");
  entry.setSource("pNodeReporter");
  entry.setTime("000123.4500");
  entry.setCommunity("alpha shoreside");

  EXPECT_EQ(entry.getValue(), "  0042.500 ");
  EXPECT_EQ(entry.getTime(), "000123.4500");
  EXPECT_EQ(entry.getCommunity(), "alpha shoreside");
}

// Covers scope entry behavior: compares every stored field for equality.
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

  rhs = lhs;
  rhs.setValue("45.3");
  EXPECT_FALSE(lhs.isEqual(rhs));

  rhs = lhs;
  rhs.setType("string");
  EXPECT_FALSE(lhs.isEqual(rhs));

  rhs = lhs;
  rhs.setSource("uXMS");
  EXPECT_FALSE(lhs.isEqual(rhs));

  rhs = lhs;
  rhs.setSrcAux("different_aux");
  EXPECT_FALSE(lhs.isEqual(rhs));

  rhs = lhs;
  rhs.setCommunity("abe");
  EXPECT_FALSE(lhs.isEqual(rhs));

  rhs = lhs;
  rhs.setIteration("100");
  EXPECT_FALSE(lhs.isEqual(rhs));
}

// Covers scope entry behavior: pins helm scope behavior aux fields as plain strings.
TEST(ScopeEntryTest, PinsHelmScopeBehaviorAuxFieldsAsPlainStrings)
{
  ScopeEntry entry;
  entry.setValue("no_ipf");
  entry.setType("string");
  entry.setSource("pHelmIvP");
  entry.setSrcAux("pHelmIvP:45:wpt_survey");
  entry.setTime("123.456");
  entry.setCommunity("abe");
  entry.setIteration("45");
  entry.setNameBHV("wpt_survey");

  EXPECT_EQ(entry.getSrcAux(), "pHelmIvP:45:wpt_survey");
  EXPECT_EQ(entry.getIteration(), "45");
  EXPECT_EQ(entry.getNameBHV(), "wpt_survey");

  ScopeEntry different_aux = entry;
  different_aux.setSrcAux("45:wpt_survey");
  EXPECT_FALSE(entry.isEqual(different_aux));
  EXPECT_EQ(different_aux.getIteration(), "45");
  EXPECT_EQ(different_aux.getNameBHV(), "wpt_survey");
}

// Covers scope entry behavior: treats empty and whitespace scope fields as significant.
TEST(ScopeEntryTest, TreatsEmptyAndWhitespaceScopeFieldsAsSignificant)
{
  ScopeEntry unset;

  ScopeEntry blanked;
  blanked.setValue("");
  blanked.setType("");
  blanked.setSource("");
  blanked.setSrcAux("");
  blanked.setTime("");
  blanked.setCommunity("");
  blanked.setIteration("");
  blanked.setNameBHV("");
  EXPECT_TRUE(unset.isEqual(blanked));

  ScopeEntry spaced = blanked;
  spaced.setValue(" ");
  EXPECT_FALSE(blanked.isEqual(spaced));

  spaced = blanked;
  spaced.setCommunity("abe ");
  EXPECT_FALSE(blanked.isEqual(spaced));

  spaced = blanked;
  spaced.setNameBHV("WPT_SURVEY");
  EXPECT_FALSE(blanked.isEqual(spaced));
}
