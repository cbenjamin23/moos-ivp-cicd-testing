#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "LogicCondition.h"
#include "LogicUtils.h"

TEST(LogicUtilsTest, ClassifiesParenthesesAndPrunesGlobalParens)
{
  EXPECT_TRUE(legalParens("(MODE=ACTIVE) and (NAV_X>10)"));
  EXPECT_FALSE(legalParens("(MODE=ACTIVE"));

  EXPECT_TRUE(globalParens("(MODE=ACTIVE)"));
  EXPECT_FALSE(globalParens("(MODE=ACTIVE) and (NAV_X>10)"));
  EXPECT_TRUE(globalParens("((MODE=ACTIVE) and (NAV_X>10))"));
  EXPECT_TRUE(globalNotParens("!(MODE=ACTIVE)"));
  EXPECT_FALSE(globalNotParens("!MODE=ACTIVE"));

  EXPECT_EQ(pruneParens("((MODE=ACTIVE))"), "MODE=ACTIVE");
  EXPECT_EQ(pruneParens("((MODE=ACTIVE) or (MODE=RETURN))"),
            "(MODE=ACTIVE) or (MODE=RETURN)");
}

TEST(LogicUtilsTest, ValidatesVariablesLiteralsAndRightSideVariables)
{
  EXPECT_TRUE(isValidVariable("NAV_X"));
  EXPECT_TRUE(isValidVariable("CONTACT_1"));
  EXPECT_FALSE(isValidVariable("NAV>X"));
  EXPECT_FALSE(isValidVariable("AND"));

  EXPECT_TRUE(isValidLiteral("ACTIVE"));
  EXPECT_TRUE(isValidLiteral("\"AND\""));
  EXPECT_FALSE(isValidLiteral("ACTIVE)"));
  EXPECT_FALSE(isValidLiteral("OR"));

  EXPECT_TRUE(isValidRightVariable("$(DESIRED_MODE)"));
  EXPECT_FALSE(isValidRightVariable("$DESIRED_MODE"));
  EXPECT_FALSE(isValidRightVariable("$(BAD>VAR)"));
}

TEST(LogicUtilsTest, MatchesColonSeparatedModeFields)
{
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING", "SURVEYING"));
  EXPECT_TRUE(strFieldMatch("SURVEYING", "ACTIVE:SURVEYING"));
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "SURVEYING:LEG1"));
  EXPECT_FALSE(strFieldMatch("ACTIVE:SURVEYING", "VEY"));
  EXPECT_FALSE(strFieldMatch("ACTIVE:SURVEYING", "RETURNING"));
}

TEST(LogicUtilsTest, BuildsConditionVectorsAndExtractsUniqueVariables)
{
  std::vector<LogicCondition> conditions;
  EXPECT_TRUE(setLogicConditionOnString(conditions, "MODE=ACTIVE"));
  EXPECT_TRUE(setLogicConditionOnString(conditions, "NAV_X > 10"));
  EXPECT_TRUE(setLogicConditionOnString(conditions, "MARK_DELTA < 5"));
  EXPECT_FALSE(setLogicConditionOnString(conditions, "(MODE=ACTIVE) and NAV_X>10"));

  std::vector<std::string> unique = getUniqueVars(conditions);
  EXPECT_EQ(std::set<std::string>(unique.begin(), unique.end()),
            (std::set<std::string>{"MARK_DELTA", "MODE", "NAV_X"}));

  std::set<std::string> logic_vars = getLogicVars(conditions);
  EXPECT_EQ(logic_vars,
            (std::set<std::string>{"MARK", "MARK_DELTA", "MODE", "NAV_X"}));
}
