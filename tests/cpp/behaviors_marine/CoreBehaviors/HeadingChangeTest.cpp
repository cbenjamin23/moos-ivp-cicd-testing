#include <gtest/gtest.h>

#include <memory>

#include "BHV_HeadingChange.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

namespace {

void seedHeadingChangeInfo(InfoBuffer& info,
                           double time,
                           double heading,
                           double speed,
                           double x,
                           double y)
{
  info.setCurrTime(time);
  info.setValue("NAV_HEADING", heading);
  info.setValue("NAV_SPEED", speed);
  info.setValue("DESIRED_SPEED", speed);
  info.setValue("NAV_X", x);
  info.setValue("NAV_Y", y);
}

}  // namespace

// Covers BHV heading change behavior: builds turn objective and completes when goal is reached.
TEST(BHVHeadingChangeTest, BuildsTurnObjectiveAndCompletesWhenGoalIsReached)
{
  InfoBuffer info;
  seedHeadingChangeInfo(info, 100, 350, 2.0, 0, 0);

  BHV_HeadingChange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading_delta", "20"));
  ASSERT_TRUE(behavior.setParam("turn_type", "starboard"));
  ASSERT_TRUE(behavior.setParam("turn_speed", "1.5"));
  EXPECT_FALSE(behavior.setParam("heading_delta", "0"));
  EXPECT_FALSE(behavior.setParam("turn_type", "up"));
  EXPECT_FALSE(behavior.setParam("turn_speed", "0"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  VarDataPair goal;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_GOAL", goal));
  EXPECT_DOUBLE_EQ(goal.get_ddata(), 10);

  behavior.clearMessages();
  seedHeadingChangeInfo(info, 103, 10, 1.5, 3, 0);
  std::unique_ptr<IvPFunction> completed_ipf(behavior.onRunState());
  EXPECT_EQ(completed_ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair turns;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TURNS", turns));
  EXPECT_DOUBLE_EQ(turns.get_ddata(), 1);
  VarDataPair trend;
  EXPECT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_TREND_1", trend));
}

// Covers BHV heading change behavior: port turn uses current speed and wraps goal.
TEST(BHVHeadingChangeTest, PortTurnUsesCurrentSpeedAndWrapsGoal)
{
  InfoBuffer info;
  seedHeadingChangeInfo(info, 200, 10, 3.0, 0, 0);

  BHV_HeadingChange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading_delta", "20"));
  ASSERT_TRUE(behavior.setParam("turn_type", "port"));
  ASSERT_TRUE(behavior.setParam("turn_speed", "current"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair goal;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_GOAL", goal));
  EXPECT_DOUBLE_EQ(goal.get_ddata(), 350);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 350, 3),
            evalCourseSpeedPoint(*ipf, 30, 3));
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 350, 3),
            evalCourseSpeedPoint(*ipf, 350, 1));

  behavior.clearMessages();
  seedHeadingChangeInfo(info, 204, 349.5, 2.0, -2, 0);
  std::unique_ptr<IvPFunction> done_ipf(behavior.onRunState());
  EXPECT_EQ(done_ipf, nullptr);

  VarDataPair trend;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_TREND_2", trend));
  ASSERT_TRUE(trend.is_string());
  EXPECT_NE(trend.get_sdata().find("4.00"), std::string::npos);
}

// Covers BHV heading change behavior: missing navigation inputs post warning and no objective.
TEST(BHVHeadingChangeTest, MissingNavigationInputsPostWarningAndNoObjective)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 2);
  info.setValue("NAV_X", 0);

  BHV_HeadingChange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  ASSERT_TRUE(warning.is_string());
  EXPECT_NE(warning.get_sdata().find("info_buffer"), std::string::npos);
}

// Covers BHV heading change behavior: idle transition starts new turn from current heading.
TEST(BHVHeadingChangeTest, IdleTransitionStartsNewTurnFromCurrentHeading)
{
  InfoBuffer info;
  seedHeadingChangeInfo(info, 100, 0, 2.0, 0, 0);

  BHV_HeadingChange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading_delta", "45"));
  ASSERT_TRUE(behavior.setParam("turn_type", "starboard"));
  ASSERT_TRUE(behavior.setParam("turn_speed", "current"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  VarDataPair first_goal;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_GOAL", first_goal));
  EXPECT_DOUBLE_EQ(first_goal.get_ddata(), 45);

  behavior.onIdleState();
  behavior.clearMessages();
  seedHeadingChangeInfo(info, 120, 100, 3.0, 10, 0);

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);

  VarDataPair second_goal;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_GOAL", second_goal));
  EXPECT_DOUBLE_EQ(second_goal.get_ddata(), 145);
  EXPECT_GT(evalCourseSpeedPoint(*second_ipf, 145, 2),
            evalCourseSpeedPoint(*second_ipf, 145, 0));
}

// Covers BHV heading change behavior: completion posts all trend series and resets accumulated samples.
TEST(BHVHeadingChangeTest, CompletionPostsAllTrendSeriesAndResetsAccumulatedSamples)
{
  InfoBuffer info;
  seedHeadingChangeInfo(info, 10, 90, 1.0, 0, 0);

  BHV_HeadingChange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading_delta", "30"));
  ASSERT_TRUE(behavior.setParam("turn_type", "port"));

  std::unique_ptr<IvPFunction> start_ipf(behavior.onRunState());
  ASSERT_NE(start_ipf, nullptr);

  behavior.clearMessages();
  seedHeadingChangeInfo(info, 12, 60, 1.0, -1, 1);
  std::unique_ptr<IvPFunction> complete_ipf(behavior.onRunState());
  EXPECT_EQ(complete_ipf, nullptr);

  VarDataPair trend1;
  VarDataPair trend2;
  VarDataPair pos_trend;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_TREND_1", trend1));
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_TREND_2", trend2));
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "POS_TREND", pos_trend));
  EXPECT_NE(pos_trend.get_sdata().find("-1.00,1.00"), std::string::npos);

  behavior.clearMessages();
  seedHeadingChangeInfo(info, 20, 200, 1.0, 2, 2);
  std::unique_ptr<IvPFunction> restarted_ipf(behavior.onRunState());
  ASSERT_NE(restarted_ipf, nullptr);

  VarDataPair new_goal;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HEADING_GOAL", new_goal));
  EXPECT_DOUBLE_EQ(new_goal.get_ddata(), 170);
}
