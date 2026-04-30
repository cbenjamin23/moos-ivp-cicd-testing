#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "LogicCondition.h"

// Covers logic condition behavior: evaluates behavior style numeric and string conditions.
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

// Covers logic condition behavior: evaluates IvP behavior condition examples.
TEST(LogicConditionTest, EvaluatesIvPBehaviorConditionExamples)
{
  LogicCondition condition;
  ASSERT_TRUE(condition.setCondition(
      "(DEPLOY=true) and ((MODE==ACTIVE:SURVEYING) or (RETURN=true))"));

  condition.setVarVal("DEPLOY", "true");
  condition.setVarVal("MODE", "ACTIVE:SURVEYING:LEG1");
  condition.setVarVal("RETURN", "false");
  EXPECT_TRUE(condition.eval());

  condition.setVarVal("MODE", "ACTIVE:TRANSIT");
  EXPECT_FALSE(condition.eval());

  condition.setVarVal("RETURN", "true");
  EXPECT_TRUE(condition.eval());

  condition.clearVarVals();
  condition.setVarVal("DEPLOY", "false");
  condition.setVarVal("MODE", "ACTIVE:SURVEYING");
  condition.setVarVal("RETURN", "true");
  EXPECT_FALSE(condition.eval());
}

// Covers logic condition behavior: requires parens around and or operands.
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

// Covers logic condition behavior: supports field match double equals and mode entry restriction.
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

// Covers logic condition behavior: compares numbers strings and right side variables.
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

// Covers logic condition behavior: pins type overwrite and clear semantics.
TEST(LogicConditionTest, PinsTypeOverwriteAndClearSemantics)
{
  LogicCondition condition;
  ASSERT_TRUE(condition.setCondition("(NAV_X > 10) and (MODE=ACTIVE)"));

  condition.setVarVal("NAV_X", 12.0);
  condition.setVarVal("NAV_X", "not-a-number");
  condition.setVarVal("MODE", "ACTIVE");
  condition.setVarVal("MODE", 0.0);
  EXPECT_TRUE(condition.eval());

  condition.clearVarVals();
  condition.setVarVal("NAV_X", "12.0");
  condition.setVarVal("MODE", "ACTIVE");
  EXPECT_TRUE(condition.eval());

  condition.clearVarVals();
  condition.setVarVal("NAV_X", "fast");
  condition.setVarVal("MODE", "ACTIVE");
  EXPECT_FALSE(condition.eval());
}

// Covers logic condition behavior: evaluates alog test and collision detector style conditions.
TEST(LogicConditionTest, EvaluatesAlogTestAndCollisionDetectorStyleConditions)
{
  LogicCondition start_condition;
  ASSERT_TRUE(start_condition.setCondition("DEPLOY = true"));
  start_condition.setVarVal("DEPLOY", "true");
  EXPECT_TRUE(start_condition.eval());
  start_condition.setVarVal("DEPLOY", "false");
  EXPECT_FALSE(start_condition.eval());

  LogicCondition pass_condition;
  ASSERT_TRUE(pass_condition.setCondition("IVPHELM_STATE=DRIVE"));
  pass_condition.setVarVal("IVPHELM_STATE", "DRIVE");
  EXPECT_TRUE(pass_condition.eval());

  LogicCondition collision_gate;
  ASSERT_TRUE(collision_gate.setCondition(
      "(DEPLOY_ALL = true) and (CONTACT_RANGE <= 15)"));
  collision_gate.setVarVal("DEPLOY_ALL", "true");
  collision_gate.setVarVal("CONTACT_RANGE", 14.99);
  EXPECT_TRUE(collision_gate.eval());
  collision_gate.setVarVal("CONTACT_RANGE", 15.01);
  EXPECT_FALSE(collision_gate.eval());
}

// Covers logic condition behavior: copies conditions without sharing node state.
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

// Covers logic condition behavior: expands macros in parsed conditions.
TEST(LogicConditionTest, ExpandsMacrosInParsedConditions)
{
  LogicCondition condition;
  ASSERT_TRUE(condition.setCondition("CONTACT_RANGE_$[CONTACT] < 100"));
  condition.expandMacro("$[CONTACT]", "HENRY");
  condition.setVarVal("CONTACT_RANGE_HENRY", 75);
  EXPECT_TRUE(condition.eval());
}
