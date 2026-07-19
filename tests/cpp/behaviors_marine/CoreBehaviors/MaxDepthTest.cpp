#include <gtest/gtest.h>

#include <memory>

#include "BHV_MaxDepth.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV max depth behavior: builds constraint and reports depth slack.
TEST(BHVMaxDepthTest, BuildsConstraintAndReportsDepthSlack)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 32.0);

  BHV_MaxDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  EXPECT_TRUE(behavior.isConstraint());
  ASSERT_TRUE(behavior.setParam("max_depth", "25"));
  ASSERT_TRUE(behavior.setParam("tolerance", "5"));
  ASSERT_TRUE(behavior.setParam("depth_slack_var", "DEPTH_SLACK"));
  EXPECT_FALSE(behavior.setParam("depth_slack_var", "BAD VAR"));
  EXPECT_FALSE(behavior.setParam("max_depth", "deep"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 25), evalOneDimPoint(*ipf, 40));

  VarDataPair slack;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DEPTH_SLACK", slack));
  EXPECT_DOUBLE_EQ(slack.get_ddata(), -7.0);
}

// Covers BHV max depth behavior: tolerance sets the exact utility transition.
TEST(BHVMaxDepthTest, ToleranceSetsExactObjectiveTransition)
{
  IvPDomain half_meter_domain;
  half_meter_domain.addDomain("depth", 0, 50, 101);

  InfoBuffer info;
  info.setValue("NAV_DEPTH", 12.0);

  BHV_MaxDepth behavior(half_meter_domain);
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("max_depth", "12"));
  ASSERT_TRUE(behavior.setParam("tolerance", "1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_NEAR(evalOneDimPoint(*ipf, 24), 100, 1e-3);
  EXPECT_NEAR(evalOneDimPoint(*ipf, 25), 50, 1e-3);
  EXPECT_NEAR(evalOneDimPoint(*ipf, 26), 0, 1e-3);
}

// Covers BHV max depth behavior: zero tolerance makes an immediate utility step.
TEST(BHVMaxDepthTest, ZeroToleranceDropsAtFirstDeeperSample)
{
  IvPDomain half_meter_domain;
  half_meter_domain.addDomain("depth", 0, 50, 101);

  InfoBuffer info;
  info.setValue("NAV_DEPTH", 12.0);

  BHV_MaxDepth behavior(half_meter_domain);
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("max_depth", "12"));
  ASSERT_TRUE(behavior.setParam("tolerance", "0"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_NEAR(evalOneDimPoint(*ipf, 24), 100, 1e-3);
  EXPECT_NEAR(evalOneDimPoint(*ipf, 25), 0, 1e-3);
}

// Covers BHV max depth behavior: basewidth is an exact alias for tolerance.
TEST(BHVMaxDepthTest, BasewidthAliasMatchesToleranceAtEveryDepth)
{
  IvPDomain half_meter_domain;
  half_meter_domain.addDomain("depth", 0, 50, 101);

  InfoBuffer info;
  info.setValue("NAV_DEPTH", 12.0);

  BHV_MaxDepth tolerance_behavior(half_meter_domain);
  tolerance_behavior.setInfoBuffer(&info);
  ASSERT_TRUE(tolerance_behavior.setParam("max_depth", "12"));
  ASSERT_TRUE(tolerance_behavior.setParam("tolerance", "1"));

  BHV_MaxDepth basewidth_behavior(half_meter_domain);
  basewidth_behavior.setInfoBuffer(&info);
  ASSERT_TRUE(basewidth_behavior.setParam("max_depth", "12"));
  ASSERT_TRUE(basewidth_behavior.setParam("basewidth", "1"));

  std::unique_ptr<IvPFunction> tolerance_ipf(tolerance_behavior.onRunState());
  std::unique_ptr<IvPFunction> basewidth_ipf(basewidth_behavior.onRunState());
  ASSERT_NE(tolerance_ipf, nullptr);
  ASSERT_NE(basewidth_ipf, nullptr);

  for(unsigned int point = 0; point <= 100; ++point) {
    EXPECT_DOUBLE_EQ(evalOneDimPoint(*basewidth_ipf, point),
                     evalOneDimPoint(*tolerance_ipf, point));
  }
}

// Covers BHV max depth behavior: clips negative max depth and tolerance from mission config.
TEST(BHVMaxDepthTest, ClipsNegativeMaxDepthAndToleranceFromMissionConfig)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 2.5);

  BHV_MaxDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("max_depth", "-3"));
  ASSERT_TRUE(behavior.setParam("basewidth", "-4"));
  ASSERT_TRUE(behavior.setParam("tolerance", "-5"));
  ASSERT_TRUE(behavior.setParam("depth_slack_var", "DEPTH_SLACK"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GE(evalOneDimPoint(*ipf, 0), evalOneDimPoint(*ipf, 30));

  VarDataPair slack;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DEPTH_SLACK", slack));
  EXPECT_DOUBLE_EQ(slack.get_ddata(), -2.5);
}

// Covers BHV max depth behavior: warns on missing nav depth but still builds constraint.
TEST(BHVMaxDepthTest, WarnsOnMissingNavDepthButStillBuildsConstraint)
{
  BHV_MaxDepth behavior(makeDepthDomain());

  ASSERT_TRUE(behavior.setParam("max_depth", "18"));
  ASSERT_TRUE(behavior.setParam("depth_slack_var", "DEPTH_SLACK"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("No ownship DEPTH"), std::string::npos);

  VarDataPair slack;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "DEPTH_SLACK", slack));
}

// Covers BHV max depth behavior: fails without depth dimension in helm domain.
TEST(BHVMaxDepthTest, FailsWithoutDepthDimensionInHelmDomain)
{
  BHV_MaxDepth behavior(makeCourseSpeedDomain());

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("No 'depth' variable"), std::string::npos);
}

// Covers BHV max depth behavior: mission style delta max depth pins unsupported ascent fields.
TEST(BHVMaxDepthTest, MissionStyleDeltaMaxDepthPinsUnsupportedAscentFields)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 8);

  BHV_MaxDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  // Mirrors ivp/missions/s4_delta/delta.bhv; ascent_* are not handled by
  // the current BHV_MaxDepth implementation.
  ASSERT_TRUE(behavior.setParam("pwt", "200"));
  ASSERT_TRUE(behavior.setParam("max_depth", "7"));
  EXPECT_FALSE(behavior.setParam("ascent_speed", "1.0"));
  EXPECT_FALSE(behavior.setParam("ascent_grade", "fullspeed"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_DOUBLE_EQ(ipf->getPWT(), 200);
  EXPECT_GT(evalOneDimPoint(*ipf, 7), evalOneDimPoint(*ipf, 20));
}
