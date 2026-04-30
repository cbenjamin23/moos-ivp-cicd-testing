#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "VCheck.h"

// Covers v check behavior: parses numeric and string mission eval checks.
TEST(VCheckTest, ParsesNumericAndStringMissionEvalChecks)
{
  VCheck nav_x;
  ASSERT_TRUE(nav_x.setVarCheck("var=NAV_X,dval=43,time=185,max_vdelta=3.4"));
  EXPECT_EQ(nav_x.getVarName(), "NAV_X");
  EXPECT_DOUBLE_EQ(nav_x.getValDouble(), 43);
  EXPECT_EQ(nav_x.getValString(), "");
  EXPECT_DOUBLE_EQ(nav_x.getTimeStamp(), 185);
  EXPECT_DOUBLE_EQ(nav_x.getMaxValDelta(), 3.4);
  EXPECT_DOUBLE_EQ(nav_x.getMaxTimeDelta(), 0);
  EXPECT_FALSE(nav_x.isEvaluated());
  EXPECT_FALSE(nav_x.isSatisfied());

  VCheck mission_done;
  ASSERT_TRUE(mission_done.setVarCheck(
      "var=MISSION_RESULT,sval=success,time=334,max_tdelta=5"));
  EXPECT_EQ(mission_done.getVarName(), "MISSION_RESULT");
  EXPECT_EQ(mission_done.getValString(), "success");
  EXPECT_DOUBLE_EQ(mission_done.getTimeStamp(), 334);
  EXPECT_DOUBLE_EQ(mission_done.getMaxTimeDelta(), 5);
  EXPECT_DOUBLE_EQ(mission_done.getMaxValDelta(), 0);
}

// Covers v check behavior: generic val sets string and numeric when value is numeric.
TEST(VCheckTest, GenericValSetsStringAndNumericWhenValueIsNumeric)
{
  VCheck numeric;
  ASSERT_TRUE(numeric.setVarCheck("var=WPT_INDEX,val=007,time=34,max_vdelta=0"));
  EXPECT_EQ(numeric.getVarName(), "WPT_INDEX");
  EXPECT_EQ(numeric.getValString(), "007");
  EXPECT_DOUBLE_EQ(numeric.getValDouble(), 7);
  EXPECT_DOUBLE_EQ(numeric.getMaxValDelta(), 0);

  VCheck mode;
  ASSERT_TRUE(mode.setVarCheck("var=MODE,val=survey,time=12,max_tdelta=2"));
  EXPECT_EQ(mode.getValString(), "survey");
  EXPECT_DOUBLE_EQ(mode.getValDouble(), 0);
  EXPECT_DOUBLE_EQ(mode.getMaxTimeDelta(), 2);
}

// Covers v check behavior: pins mission eval case and negative numeric tolerance semantics.
TEST(VCheckTest, PinsMissionEvalCaseAndNegativeNumericToleranceSemantics)
{
  VCheck check;
  ASSERT_TRUE(check.setVarCheck(
      "VAR=NAV_HEADING,DVAL=-12.5,TIME=0,MAX_VDELTA=-0.25"));

  EXPECT_EQ(check.getVarName(), "NAV_HEADING");
  EXPECT_DOUBLE_EQ(check.getValDouble(), -12.5);
  EXPECT_DOUBLE_EQ(check.getTimeStamp(), 0);
  EXPECT_DOUBLE_EQ(check.getMaxValDelta(), -0.25);
  EXPECT_DOUBLE_EQ(check.getMaxTimeDelta(), 0);
}

// Covers v check behavior: pins duplicate and mixed value fields as last parsed state.
TEST(VCheckTest, PinsDuplicateAndMixedValueFieldsAsLastParsedState)
{
  VCheck check;
  ASSERT_TRUE(check.setVarCheck(
      "var=MODE,sval=transit,val=survey,dval=42,time=10,"
      "max_tdelta=5,max_vdelta=2"));

  EXPECT_EQ(check.getVarName(), "MODE");
  EXPECT_EQ(check.getValString(), "survey");
  EXPECT_DOUBLE_EQ(check.getValDouble(), 42);
  EXPECT_DOUBLE_EQ(check.getTimeStamp(), 10);
  EXPECT_DOUBLE_EQ(check.getMaxTimeDelta(), 5);
  EXPECT_DOUBLE_EQ(check.getMaxValDelta(), 2);
}

// Covers v check behavior: setters record evaluation result and actual observation fields.
TEST(VCheckTest, SettersRecordEvaluationResultAndActualObservationFields)
{
  VCheck check;
  check.setVarName("NAV_X");
  check.setValDouble(100);
  check.setTimeStamp(22.5);
  check.setMaxValDelta(1.5);
  check.setEvaluated();
  check.setSatisfied(true);
  check.setActualDelta(0.75);
  check.setActualTime(22.7);
  check.setActualValDbl(100.75);
  check.setActualValStr("100.75");

  EXPECT_EQ(check.getVarName(), "NAV_X");
  EXPECT_DOUBLE_EQ(check.getValDouble(), 100);
  EXPECT_DOUBLE_EQ(check.getTimeStamp(), 22.5);
  EXPECT_DOUBLE_EQ(check.getMaxValDelta(), 1.5);
  EXPECT_TRUE(check.isEvaluated());
  EXPECT_TRUE(check.isSatisfied());
  EXPECT_DOUBLE_EQ(check.getActualDelta(), 0.75);
  EXPECT_DOUBLE_EQ(check.getActualTime(), 22.7);
  EXPECT_DOUBLE_EQ(check.getActualValDbl(), 100.75);
  EXPECT_EQ(check.getActualValStr(), "100.75");

  check.setEvaluated(false);
  check.setSatisfied(false);
  EXPECT_FALSE(check.isEvaluated());
  EXPECT_FALSE(check.isSatisfied());
}

// Covers v check behavior: rejects malformed checks and requires value time and delta.
TEST(VCheckTest, RejectsMalformedChecksAndRequiresValueTimeAndDelta)
{
  EXPECT_FALSE(VCheck().setVarCheck("dval=43,time=185,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,time=185,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=185"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=abc,time=185,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=185,bogus=1"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=abc,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=185,max_vdelta=bad"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=185,max_tdelta=bad"));
}

// Covers v check behavior: pins failed parse can leave earlier parsed fields.
TEST(VCheckTest, PinsFailedParseCanLeaveEarlierParsedFields)
{
  VCheck check;
  ASSERT_TRUE(check.setVarCheck("var=NAV_X,dval=43,time=185,max_vdelta=3.4"));

  EXPECT_FALSE(check.setVarCheck("var=NAV_Y,dval=bad,time=200,max_vdelta=1"));
  EXPECT_EQ(check.getVarName(), "NAV_Y");
  EXPECT_DOUBLE_EQ(check.getValDouble(), 43);
  EXPECT_DOUBLE_EQ(check.getTimeStamp(), 185);
  EXPECT_DOUBLE_EQ(check.getMaxValDelta(), 3.4);
}

// Covers v check behavior: pins trailing comma as ignored empty field.
TEST(VCheckTest, PinsTrailingCommaAsIgnoredEmptyField)
{
  VCheck check;
  EXPECT_TRUE(check.setVarCheck("var=NAV_X,dval=43,time=185,max_vdelta=3.4,"));
  EXPECT_EQ(check.getVarName(), "NAV_X");
  EXPECT_DOUBLE_EQ(check.getValDouble(), 43);
  EXPECT_DOUBLE_EQ(check.getTimeStamp(), 185);
  EXPECT_DOUBLE_EQ(check.getMaxValDelta(), 3.4);
}

// Covers v check behavior: sorts earlier target times before later times in eval sequences.
TEST(VCheckTest, SortsEarlierTargetTimesBeforeLaterTimesInEvalSequences)
{
  VCheck early;
  ASSERT_TRUE(early.setVarCheck("var=A,dval=1,time=10,max_vdelta=0.1"));
  VCheck late;
  ASSERT_TRUE(late.setVarCheck("var=B,dval=2,time=20,max_vdelta=0.1"));

  std::vector<VCheck> checks{late, early};
  std::sort(checks.begin(), checks.end());
  std::reverse(checks.begin(), checks.end());

  EXPECT_EQ(checks[0].getVarName(), "A");
  EXPECT_EQ(checks[1].getVarName(), "B");

  EXPECT_TRUE(late < early);
  EXPECT_TRUE(early > late);
}
