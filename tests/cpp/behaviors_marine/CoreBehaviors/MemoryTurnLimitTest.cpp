#include <gtest/gtest.h>

#include <memory>

#include "BHV_MemoryTurnLimit.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV memory turn limit behavior: builds course window around recent heading average.
TEST(BHVMemoryTurnLimitTest, BuildsCourseWindowAroundRecentHeadingAverage)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1.5);

  BHV_MemoryTurnLimit behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("memory_time", "10"));
  ASSERT_TRUE(behavior.setParam("turn_range", "30"));
  EXPECT_FALSE(behavior.setParam("memory_time", "-1"));
  EXPECT_FALSE(behavior.setParam("turn_range", "181"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 90), evalOneDimPoint(*ipf, 270));

  VarDataPair average;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "MEM_HDG_AVG", average));
  EXPECT_DOUBLE_EQ(average.get_ddata(), 90);
}

// Covers BHV memory turn limit behavior: prunes history by memory time across north.
TEST(BHVMemoryTurnLimitTest, PrunesHistoryByMemoryTimeAcrossNorth)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_HEADING", 350);
  info.setValue("NAV_SPEED", 1.5);

  BHV_MemoryTurnLimit behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("memory_time", "5"));
  ASSERT_TRUE(behavior.setParam("turn_range", "30"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  behavior.clearMessages();

  info.setCurrTime(1);
  info.setValue("NAV_HEADING", 10);
  std::unique_ptr<IvPFunction> wrapped_ipf(behavior.onRunState());
  ASSERT_NE(wrapped_ipf, nullptr);
  VarDataPair average;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "MEM_HDG_AVG", average));
  EXPECT_TRUE(average.get_ddata() == 0 || average.get_ddata() == 360);

  behavior.clearMessages();
  info.setCurrTime(10);
  info.setValue("NAV_HEADING", 90);
  std::unique_ptr<IvPFunction> pruned_ipf(behavior.onRunState());
  ASSERT_NE(pruned_ipf, nullptr);
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "MEM_HDG_AVG", average));
  EXPECT_DOUBLE_EQ(average.get_ddata(), 90);
}

// Covers BHV memory turn limit behavior: speed scales allowed turn window.
TEST(BHVMemoryTurnLimitTest, SpeedScalesAllowedTurnWindow)
{
  InfoBuffer slow_info;
  slow_info.setCurrTime(1);
  slow_info.setValue("NAV_HEADING", 90);
  slow_info.setValue("NAV_SPEED", 0.1);

  BHV_MemoryTurnLimit slow(makeCourseSpeedDomain());
  slow.setInfoBuffer(&slow_info);
  ASSERT_TRUE(slow.setParam("memory_time", "10"));
  ASSERT_TRUE(slow.setParam("turn_range", "30"));
  std::unique_ptr<IvPFunction> slow_ipf(slow.onRunState());
  ASSERT_NE(slow_ipf, nullptr);

  InfoBuffer fast_info;
  fast_info.setCurrTime(1);
  fast_info.setValue("NAV_HEADING", 90);
  fast_info.setValue("NAV_SPEED", 1.5);

  BHV_MemoryTurnLimit fast(makeCourseSpeedDomain());
  fast.setInfoBuffer(&fast_info);
  ASSERT_TRUE(fast.setParam("memory_time", "10"));
  ASSERT_TRUE(fast.setParam("turn_range", "30"));
  std::unique_ptr<IvPFunction> fast_ipf(fast.onRunState());
  ASSERT_NE(fast_ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*fast_ipf, 110), evalOneDimPoint(*slow_ipf, 110));
  EXPECT_DOUBLE_EQ(evalOneDimPoint(*fast_ipf, 90), evalOneDimPoint(*slow_ipf, 90));
}

// Covers BHV memory turn limit behavior: missing config or nav posts errors without objective.
TEST(BHVMemoryTurnLimitTest, MissingConfigOrNavPostsErrorsWithoutObjective)
{
  InfoBuffer info;
  info.setCurrTime(1);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1);

  BHV_MemoryTurnLimit missing_memory(makeCourseSpeedDomain());
  missing_memory.setInfoBuffer(&info);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(missing_memory.onRunState()), nullptr);
  VarDataPair message;
  ASSERT_TRUE(findBehaviorMessage(missing_memory.getMessages(), "BHV_WARNING", message));
  EXPECT_NE(message.get_sdata().find("memory_time"), std::string::npos);

  BHV_MemoryTurnLimit missing_speed(makeCourseSpeedDomain());
  InfoBuffer no_speed;
  no_speed.setCurrTime(1);
  no_speed.setValue("NAV_HEADING", 90);
  missing_speed.setInfoBuffer(&no_speed);
  ASSERT_TRUE(missing_speed.setParam("memory_time", "10"));
  ASSERT_TRUE(missing_speed.setParam("turn_range", "30"));
  EXPECT_EQ(std::unique_ptr<IvPFunction>(missing_speed.onRunState()), nullptr);
  ASSERT_TRUE(findBehaviorMessage(missing_speed.getMessages(), "BHV_ERROR", message));
  EXPECT_NE(message.get_sdata().find("NAV_SPEED"), std::string::npos);
  EXPECT_FALSE(missing_speed.stateOK());
}
