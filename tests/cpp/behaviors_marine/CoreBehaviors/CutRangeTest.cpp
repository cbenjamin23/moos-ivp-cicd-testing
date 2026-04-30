#include <gtest/gtest.h>

#include <memory>

#include "BHV_CutRange.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV cut range behavior: pursues contact inside giveup range and builds CPA objective.
TEST(BHVCutRangeTest, PursuesContactInsideGiveupRangeAndBuildsCPAObjective)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 180, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "150"));
  ASSERT_TRUE(behavior.setParam("pursueflag", "CUT_PURSUIT=true"));
  ASSERT_TRUE(behavior.setParam("patience", "50"));
  EXPECT_FALSE(behavior.setParam("patience", "101"));
  EXPECT_FALSE(behavior.setParam("giveup_range", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair pursue;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "CUT_PURSUIT", pursue));
  EXPECT_EQ(pursue.get_sdata(), "true");
}

// Covers BHV cut range behavior: giveup range drops objective after pursuit was active.
TEST(BHVCutRangeTest, GiveupRangeDropsObjectiveAfterPursuitWasActive)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 180, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "150"));
  ASSERT_TRUE(behavior.setParam("giveupflag", "CUT_GIVEUP=true"));
  ASSERT_TRUE(behavior.setParam("max_patience", "40"));
  ASSERT_TRUE(behavior.setParam("patience", "80"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> pursuit_ipf(behavior.onRunState());
  ASSERT_NE(pursuit_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(101);
  ledger.setX("alpha", 0);
  ledger.setY("alpha", 180);
  ledger.setHdg("alpha", 180);
  ledger.setSpd("alpha", 1);
  ledger.setUTC("alpha", info.getCurrTime());

  std::unique_ptr<IvPFunction> giveup_ipf(behavior.onRunState());
  EXPECT_EQ(giveup_ipf, nullptr);

  VarDataPair giveup;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "CUT_GIVEUP", giveup));
  EXPECT_EQ(giveup.get_sdata(), "true");
}

// Covers BHV cut range behavior: relevance scales priority weight between inner and outer ranges.
TEST(BHVCutRangeTest, RelevanceScalesPriorityWeightBetweenInnerAndOuterRanges)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 180, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "200"));
  ASSERT_TRUE(behavior.setParam("priority", "80"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 40);
}

// Covers BHV cut range behavior: spawnable template posts contact manager alert request.
TEST(BHVCutRangeTest, SpawnableTemplatePostsContactManagerAlertRequest)
{
  BHV_CutRange behavior(makeCourseSpeedDomain());

  behavior.setDynamicallySpawnable(true);
  ASSERT_TRUE(behavior.setParam("updates", "CUT_UPDATES"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "175"));

  behavior.onHelmStart();

  VarDataPair request;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BCM_ALERT_REQUEST", request));
  EXPECT_NE(request.get_sdata().find("id="), std::string::npos);
  EXPECT_NE(request.get_sdata().find("on_flag=CUT_UPDATES=name=$[VNAME] # contact=$[VNAME]"),
            std::string::npos);
  EXPECT_NE(request.get_sdata().find("alert_range=175"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("cpa_range=176"), std::string::npos);
}
