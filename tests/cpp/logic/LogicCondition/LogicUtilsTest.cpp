#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "LogicCondition.h"
#include "LogicUtils.h"

namespace {

void expectVectorEq(const std::vector<std::string>& actual,
                    const std::vector<std::string>& expected)
{
  ASSERT_EQ(actual.size(), expected.size());
  for(std::size_t i = 0; i < expected.size(); ++i)
    EXPECT_EQ(actual[i], expected[i]) << "at index " << i;
}

}  // namespace

// Covers logic utils behavior: classifies parentheses and prunes global parens.
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

// Covers logic utils behavior: parses behavior condition relations at top level only.
TEST(LogicUtilsTest, ParsesBehaviorConditionRelationsAtTopLevelOnly)
{
  expectVectorEq(parseRelation("(MODE=ACTIVE) and (NAV_X>10)"),
                 {"and", "(MODE=ACTIVE)", "(NAV_X>10)"});
  expectVectorEq(parseRelation("!(DEPLOY=true)"), {"not", " (DEPLOY=true)"});
  expectVectorEq(parseRelation("MODE == ACTIVE:SURVEYING"),
                 {"==", "MODE", "ACTIVE:SURVEYING"});
  expectVectorEq(parseRelation("CONTACT_RANGE_$[CONTACT] < 100"),
                 {"<", "CONTACT_RANGE_$[CONTACT]", "100"});
  expectVectorEq(parseRelation("(MODE=ACTIVE"), {});
}

// Covers logic utils behavior: validates variables literals and right side variables.
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

// Covers logic utils behavior: validates quoted literals and conditional param strings.
TEST(LogicUtilsTest, ValidatesQuotedLiteralsAndConditionalParamStrings)
{
  EXPECT_TRUE(isValidLiteral("\"or\""));
  EXPECT_TRUE(isValidLiteral("\"AND\""));
  EXPECT_FALSE(isValidLiteral("\"bad\"quote\""));
  EXPECT_FALSE(isValidVariable(""));
  EXPECT_FALSE(isValidVariable("MODE)"));

  EXPECT_TRUE(isConditionalParamString("speed = 2.0 [DEPLOY=true]"));
  EXPECT_TRUE(isConditionalParamString("speed = 2.0 [DEPLOY=true] // comment"));
  EXPECT_TRUE(isConditionalParamString("speed[0] = 2.0 [DEPLOY=true]"));
  EXPECT_FALSE(isConditionalParamString("speed = 2.0 [DEPLOY=true] extra"));
  EXPECT_FALSE(isConditionalParamString("speed = 2.0 DEPLOY=true]"));
}

// Covers logic utils behavior: matches colon separated mode fields.
TEST(LogicUtilsTest, MatchesColonSeparatedModeFields)
{
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING", "SURVEYING"));
  EXPECT_TRUE(strFieldMatch("SURVEYING", "ACTIVE:SURVEYING"));
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "SURVEYING:LEG1"));
  EXPECT_FALSE(strFieldMatch("ACTIVE:SURVEYING", "VEY"));
  EXPECT_FALSE(strFieldMatch("ACTIVE:SURVEYING", "RETURNING"));
}

// Covers logic utils behavior: matches mode paths on whole colon delimited fields.
TEST(LogicUtilsTest, MatchesModePathsOnWholeColonDelimitedFields)
{
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "ACTIVE:SURVEYING"));
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "SURVEYING"));
  EXPECT_TRUE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "LEG1"));
  EXPECT_FALSE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "ACTIVE:LEG1"));
  EXPECT_FALSE(strFieldMatch("ACTIVE:SURVEYING:LEG1", "SURVEYING:LEG2"));
  EXPECT_TRUE(strFieldMatch("ACTIVE, SURVEYING, LEG1", "SURVEYING", ','));
}

// Covers logic utils behavior: builds condition vectors and extracts unique variables.
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
