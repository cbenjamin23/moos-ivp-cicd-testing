#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "VCheck.h"

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

TEST(VCheckTest, RejectsMalformedChecksAndRequiresValueTimeAndDelta)
{
  EXPECT_FALSE(VCheck().setVarCheck("dval=43,time=185,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,time=185,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=185"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=abc,time=185,max_vdelta=3.4"));
  EXPECT_FALSE(VCheck().setVarCheck("var=NAV_X,dval=43,time=185,bogus=1"));
}

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
}
