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
