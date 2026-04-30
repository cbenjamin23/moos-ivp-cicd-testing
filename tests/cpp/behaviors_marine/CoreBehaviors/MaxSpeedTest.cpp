#include <gtest/gtest.h>

#include <memory>

#include "BHV_MaxSpeed.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV max speed behavior: builds constraint and reports speed slack.
TEST(BHVMaxSpeedTest, BuildsConstraintAndReportsSpeedSlack)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 3.5);

  BHV_MaxSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  EXPECT_TRUE(behavior.isConstraint());
  ASSERT_TRUE(behavior.setParam("max_speed", "2"));
  ASSERT_TRUE(behavior.setParam("tolerance", "1"));
  ASSERT_TRUE(behavior.setParam("speed_slack_var", "SPD_SLACK"));
  EXPECT_FALSE(behavior.setParam("speed_slack_var", "BAD VAR"));
  EXPECT_FALSE(behavior.setParam("max_speed", "fast"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 2), evalOneDimPoint(*ipf, 5));

  VarDataPair slack;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SPD_SLACK", slack));
  EXPECT_DOUBLE_EQ(slack.get_ddata(), -1.5);
}

// Covers BHV max speed behavior: clips negative max speed and tolerance from mission config.
TEST(BHVMaxSpeedTest, ClipsNegativeMaxSpeedAndToleranceFromMissionConfig)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 0.25);

  BHV_MaxSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("max_speed", "-3"));
  ASSERT_TRUE(behavior.setParam("basewidth", "-4"));
  ASSERT_TRUE(behavior.setParam("tolerance", "-5"));
  ASSERT_TRUE(behavior.setParam("speed_slack_var", "SPD_SLACK"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GE(evalOneDimPoint(*ipf, 0), evalOneDimPoint(*ipf, 3));

  VarDataPair slack;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SPD_SLACK", slack));
  EXPECT_DOUBLE_EQ(slack.get_ddata(), -0.25);
}

// Covers BHV max speed behavior: warns on missing nav speed but still builds constraint.
TEST(BHVMaxSpeedTest, WarnsOnMissingNavSpeedButStillBuildsConstraint)
{
  BHV_MaxSpeed behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("max_speed", "1.5"));
  ASSERT_TRUE(behavior.setParam("speed_slack_var", "SPD_SLACK"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("No ownship SPEED"), std::string::npos);

  VarDataPair slack;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "SPD_SLACK", slack));
}

// Covers BHV max speed behavior: fails without speed dimension in helm domain.
TEST(BHVMaxSpeedTest, FailsWithoutSpeedDimensionInHelmDomain)
{
  BHV_MaxSpeed behavior(makeDepthDomain());

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("No 'speed' variable"), std::string::npos);
}

// Covers BHV max speed behavior: mission style pursuit max speed config applies priority.
TEST(BHVMaxSpeedTest, MissionStylePursuitMaxSpeedConfigAppliesPriority)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 4.1);

  BHV_MaxSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  // Mirrors ivp/missions/m15_pursuit/meta_chaser.bhv max speed block.
  ASSERT_TRUE(behavior.setParam("pwt", "500"));
  ASSERT_TRUE(behavior.setParam("updates", "MAX_SPEED_UPDATES"));
  ASSERT_TRUE(behavior.setParam("max_speed", "3.6"));
  ASSERT_TRUE(behavior.setParam("tolerance", "0.2"));
  ASSERT_TRUE(behavior.setParam("speed_slack_var", "SPD_SLACK"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_DOUBLE_EQ(ipf->getPWT(), 500);
  EXPECT_GT(evalOneDimPoint(*ipf, 3), evalOneDimPoint(*ipf, 5));

  VarDataPair slack;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SPD_SLACK", slack));
  EXPECT_NEAR(slack.get_ddata(), -0.5, 1e-6);
}
