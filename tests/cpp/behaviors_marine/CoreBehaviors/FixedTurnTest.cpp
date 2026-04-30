#include <gtest/gtest.h>

#include <memory>

#include "BHV_FixedTurn.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV fixed turn behavior: starts turn with configured direction heading and speed.
TEST(BHVFixedTurnTest, StartsTurnWithConfiguredDirectionHeadingAndSpeed)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_RUDDER", 5);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("fix_turn", "90"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "20"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "star"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("timeout", "30"));
  EXPECT_FALSE(behavior.setParam("fix_turn", "-1"));
  EXPECT_FALSE(behavior.setParam("turn_dir", "up"));
  EXPECT_FALSE(behavior.setParam("speed", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 20, 2),
            evalCourseSpeedPoint(*ipf, 200, 2));
}

// Covers BHV fixed turn behavior: port turn wraps heading and auto speed uses stem speed.
TEST(BHVFixedTurnTest, PortTurnWrapsHeadingAndAutoSpeedUsesStemSpeed)
{
  InfoBuffer info;
  info.setCurrTime(5);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 5);
  info.setValue("NAV_SPEED", 3);
  info.setValue("DESIRED_RUDDERX", -7);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("fix_turn", "180"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "20"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "port"));
  ASSERT_TRUE(behavior.setParam("speed", "auto"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 345, 3),
            evalCourseSpeedPoint(*ipf, 25, 3));
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 345, 3),
            evalCourseSpeedPoint(*ipf, 345, 0));
}

// Covers BHV fixed turn behavior: turn delay holds stem heading before beginning turn.
TEST(BHVFixedTurnTest, TurnDelayHoldsStemHeadingBeforeBeginningTurn)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 45);
  info.setValue("NAV_SPEED", 2);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("fix_turn", "90"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "30"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "star"));
  ASSERT_TRUE(behavior.setParam("turn_delay", "5"));
  ASSERT_TRUE(behavior.setParam("stale_nav_thresh", "100"));

  std::unique_ptr<IvPFunction> delay_ipf(behavior.onRunState());
  ASSERT_NE(delay_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*delay_ipf, 45, 2),
            evalCourseSpeedPoint(*delay_ipf, 75, 2));

  info.setCurrTime(16);
  info.setValue("NAV_HEADING", 45);
  info.setValue("NAV_SPEED", 2);
  std::unique_ptr<IvPFunction> turn_ipf(behavior.onRunState());
  ASSERT_NE(turn_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*turn_ipf, 75, 2),
            evalCourseSpeedPoint(*turn_ipf, 45, 2));
}

// Covers BHV fixed turn behavior: heading delta completes turn and posts report.
TEST(BHVFixedTurnTest, HeadingDeltaCompletesTurnAndPostsReport)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_RUDDER", 4);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("fix_turn", "5"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "20"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "star"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("timeout", "100"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(2);
  info.setValue("NAV_X", 2);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 10);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_RUDDER", 6);

  std::unique_ptr<IvPFunction> complete_ipf(behavior.onRunState());
  EXPECT_EQ(complete_ipf, nullptr);

  VarDataPair report;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FT_REPORT", report));
  ASSERT_TRUE(report.is_string());
  EXPECT_NE(report.get_sdata().find("spd=2"), std::string::npos);
  EXPECT_NE(report.get_sdata().find("mod_hdg=20"), std::string::npos);
  EXPECT_NE(report.get_sdata().find("rudder_avg=6"), std::string::npos);
}

// Covers BHV fixed turn behavior: scheduled turn spec overrides defaults and posts completion flag.
TEST(BHVFixedTurnTest, ScheduledTurnSpecOverridesDefaultsAndPostsCompletionFlag)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);
  info.setValue("DESIRED_RUDDER", 2);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("fix_turn", "180"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "20"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "port"));
  ASSERT_TRUE(behavior.setParam("speed", "1"));
  ASSERT_TRUE(behavior.setParam("turn_spec",
                                "key=survey_exit,turn=star,fix=20,mhdg=40,spd=4,timeout=30,flag=FT_DONE=true"));
  EXPECT_FALSE(behavior.setParam("turn_spec", "key=bad,turn=up"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*first_ipf, 40, 4),
            evalCourseSpeedPoint(*first_ipf, 340, 4));
  EXPECT_GT(evalCourseSpeedPoint(*first_ipf, 40, 4),
            evalCourseSpeedPoint(*first_ipf, 40, 1));

  behavior.clearMessages();
  info.setCurrTime(1);
  info.setValue("NAV_X", 1);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 25);
  info.setValue("NAV_SPEED", 4);
  info.setValue("DESIRED_RUDDER", 8);

  std::unique_ptr<IvPFunction> complete_ipf(behavior.onRunState());
  EXPECT_EQ(complete_ipf, nullptr);

  VarDataPair done;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FT_DONE", done));
  EXPECT_EQ(done.get_sdata(), "true");

  VarDataPair report;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FT_REPORT", report));
  EXPECT_NE(report.get_sdata().find("spd=4"), std::string::npos);
  EXPECT_NE(report.get_sdata().find("mod_hdg=40"), std::string::npos);
}

// Covers BHV fixed turn behavior: timeout completes even without required heading change.
TEST(BHVFixedTurnTest, TimeoutCompletesEvenWithoutRequiredHeadingChange)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 2);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("fix_turn", "270"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "30"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "star"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("timeout", "1"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(2);
  info.setValue("NAV_X", 2);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);

  std::unique_ptr<IvPFunction> settling_ipf(behavior.onRunState());
  ASSERT_NE(settling_ipf, nullptr);

  info.setCurrTime(4);
  info.setValue("NAV_X", 4);
  std::unique_ptr<IvPFunction> timeout_ipf(behavior.onRunState());
  EXPECT_EQ(timeout_ipf, nullptr);

  VarDataPair report;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FT_REPORT", report));
  EXPECT_NE(report.get_sdata().find("spd=2"), std::string::npos);
  EXPECT_NE(report.get_sdata().find("mod_hdg=30"), std::string::npos);
}

// Covers BHV fixed turn behavior: perpetual scheduled turns repeat after completion.
TEST(BHVFixedTurnTest, PerpetualScheduledTurnsRepeatAfterCompletion)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_FixedTurn behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("perpetual", "true"));
  ASSERT_TRUE(behavior.setParam("schedule_repeat", "true"));
  ASSERT_TRUE(behavior.setParam("turn_spec",
                                "key=a,turn=star,fix=10,mhdg=20,spd=2,flag=FT_CYCLE=a"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*first_ipf, 20, 2),
            evalCourseSpeedPoint(*first_ipf, 340, 2));

  behavior.clearMessages();
  info.setCurrTime(1);
  info.setValue("NAV_X", 1);
  info.setValue("NAV_HEADING", 15);
  std::unique_ptr<IvPFunction> first_complete(behavior.onRunState());
  EXPECT_EQ(first_complete, nullptr);

  VarDataPair first_cycle;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FT_CYCLE", first_cycle));
  EXPECT_EQ(first_cycle.get_sdata(), "a");

  behavior.clearMessages();
  info.setCurrTime(2);
  info.setValue("NAV_X", 2);
  info.setValue("NAV_HEADING", 15);
  std::unique_ptr<IvPFunction> repeated_ipf(behavior.onRunState());
  ASSERT_NE(repeated_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*repeated_ipf, 35, 2),
            evalCourseSpeedPoint(*repeated_ipf, 355, 2));
}
