#include <gtest/gtest.h>

#include <string>

#include "EnumVariable.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Coverage note: this translation unit intentionally includes only
// EnumVariable.h because upstream EnumVariable.h and RandomVariable.h currently
// use the same include guard. The runtime tests below cover the app-facing enum
// selection, key, timestamp, and zero-weight behavior that uTimerScript uses.

TEST(EnumVariableTest, DefaultsRepresentAnEmptyUntimedVariable)
{
  EnumVariable variable;

  EXPECT_EQ(variable.getVarName(), "random_var");
  EXPECT_EQ(variable.getKeyName(), "");
  EXPECT_EQ(variable.getValue(), "empty");
  EXPECT_EQ(variable.reset("at_start"), "Error: Index Out of Range");
  EXPECT_DOUBLE_EQ(variable.getAge(4.5), 4.5);
}

TEST(EnumVariableTest, SettersAndSummaryExposeConfiguredElementsAndWeights)
{
  EnumVariable variable;
  variable.setVarName("MODE");
  variable.setKeyName("at_post");
  variable.addElement("survey", 2);
  variable.addElement("return", 3.5);

  const std::string summary = variable.getStringSummary();

  EXPECT_EQ(variable.getVarName(), "MODE");
  EXPECT_EQ(variable.getKeyName(), "at_post");
  EXPECT_TRUE(Contains(summary, "varname=MODE"));
  EXPECT_TRUE(Contains(summary, "keyname=at_post"));
  EXPECT_TRUE(Contains(summary, "survey[2]"));
  EXPECT_TRUE(Contains(summary, "return[3.5]"));
}

TEST(EnumVariableTest, ResetOnlyChangesStateWhenKeyMatches)
{
  EnumVariable variable;
  variable.setKeyName("at_reset");
  variable.addElement("alpha", 1);
  variable.addElement("bravo", 0);

  EXPECT_EQ(variable.reset("at_post", 10), "alpha");
  EXPECT_DOUBLE_EQ(variable.getAge(12), 12);

  EXPECT_EQ(variable.reset("at_reset", 20), "bravo");
  EXPECT_EQ(variable.getValue(), "bravo");
  EXPECT_DOUBLE_EQ(variable.getAge(23.25), 3.25);
}

TEST(EnumVariableTest, NegativeWeightsAreClampedToZero)
{
  EnumVariable variable;
  variable.setKeyName("at_start");
  variable.addElement("never", -10);
  variable.addElement("selected", 1);

  EXPECT_EQ(variable.reset("at_start", 5), "selected");
  EXPECT_EQ(variable.getValue(), "selected");
}

TEST(EnumVariableTest, ZeroWeightElementAfterPositiveWeightMatchesCurrentNoBreakBehavior)
{
  EnumVariable variable;
  variable.setKeyName("at_post");
  variable.addElement("weighted", 1);
  variable.addElement("zero_tail", 0);

  // Upstream keeps scanning after a match, so a zero-weight tail element can
  // replace the positive-weight element. This protects the current behavior
  // until uTimerScript intentionally changes the selection loop.
  EXPECT_EQ(variable.reset("at_post", 1), "zero_tail");
  EXPECT_EQ(variable.getValue(), "zero_tail");
}

TEST(EnumVariableTest, NegativeTimestampDisablesFutureAgeAccumulation)
{
  EnumVariable variable;
  variable.setKeyName("at_reset");
  variable.addElement("armed", 1);

  EXPECT_EQ(variable.reset("at_reset", -1), "armed");
  EXPECT_DOUBLE_EQ(variable.getAge(100), 0);

  EXPECT_EQ(variable.reset("at_reset", 50), "armed");
  EXPECT_DOUBLE_EQ(variable.getAge(100), 0);
}
