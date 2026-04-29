#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "LogicCondition.h"

TEST(LogicConditionTest, EvaluatesBehaviorStyleNumericAndStringConditions)
{
  LogicCondition condition;
  ASSERT_TRUE(condition.setCondition("(MODE=ACTIVE) and (NAV_X > 10)"));

  condition.setVarVal("MODE", "ACTIVE");
  condition.setVarVal("NAV_X", 12.5);
  EXPECT_TRUE(condition.eval());

  condition.setVarVal("NAV_X", 9.5);
  EXPECT_FALSE(condition.eval());

  condition.setVarVal("NAV_X", 12.5);
  condition.setVarVal("MODE", "INACTIVE");
  EXPECT_FALSE(condition.eval());
}

TEST(LogicConditionTest, RequiresParensAroundAndOrOperands)
{
  LogicCondition valid;
  EXPECT_TRUE(valid.setCondition("(MODE=ACTIVE) or !(MODE=INACTIVE)"));

  LogicCondition invalid;
  testing::internal::CaptureStdout();
  EXPECT_FALSE(invalid.setCondition("(MODE=ACTIVE) and NAV_X>10"));
  EXPECT_NE(testing::internal::GetCapturedStdout().find("Bad Condition"),
            std::string::npos);
}

TEST(LogicConditionTest, SupportsFieldMatchDoubleEqualsAndModeEntryRestriction)
{
  LogicCondition field_match;
  ASSERT_TRUE(field_match.setCondition("MODE == SURVEYING"));
  field_match.setVarVal("MODE", "ACTIVE:SURVEYING");
  EXPECT_TRUE(field_match.eval());
  field_match.setVarVal("MODE", "ACTIVE:RETURNING");
  EXPECT_FALSE(field_match.eval());

  LogicCondition mode_entry_style;
  mode_entry_style.setAllowDoubleEquals(false);
  testing::internal::CaptureStdout();
  EXPECT_FALSE(mode_entry_style.setCondition("MODE == SURVEYING"));
  EXPECT_NE(testing::internal::GetCapturedStdout().find("Bad Condition"),
            std::string::npos);
}

TEST(LogicConditionTest, ComparesNumbersStringsAndRightSideVariables)
{
  LogicCondition numeric_string;
  ASSERT_TRUE(numeric_string.setCondition("SPEED >= 2.5"));
  numeric_string.setVarVal("SPEED", "2.75");
  EXPECT_TRUE(numeric_string.eval());
  numeric_string.setVarVal("SPEED", "fast");
  EXPECT_FALSE(numeric_string.eval());

  LogicCondition var_to_var;
  ASSERT_TRUE(var_to_var.setCondition("MODE=$(DESIRED_MODE)"));
  var_to_var.setVarVal("MODE", "SURVEY");
  var_to_var.setVarVal("DESIRED_MODE", "SURVEY");
  EXPECT_TRUE(var_to_var.eval());
  var_to_var.setVarVal("DESIRED_MODE", "RETURN");
  EXPECT_FALSE(var_to_var.eval());
}

TEST(LogicConditionTest, CopiesConditionsWithoutSharingNodeState)
{
  LogicCondition original;
  ASSERT_TRUE(original.setCondition("DEPLOY=true"));
  original.setVarVal("DEPLOY", "true");

  LogicCondition copy(original);
  EXPECT_TRUE(copy.eval());

  original.setVarVal("DEPLOY", "false");
  EXPECT_FALSE(original.eval());
  EXPECT_TRUE(copy.eval());
}

TEST(LogicConditionTest, ExpandsMacrosInParsedConditions)
{
  LogicCondition condition;
  ASSERT_TRUE(condition.setCondition("CONTACT_RANGE_$[CONTACT] < 100"));
  condition.expandMacro("$[CONTACT]", "HENRY");
  condition.setVarVal("CONTACT_RANGE_HENRY", 75);
  EXPECT_TRUE(condition.eval());
}
