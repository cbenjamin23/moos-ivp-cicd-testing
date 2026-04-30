#include <gtest/gtest.h>

#include <memory>

#include "BHV_Timer.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

class TestableTimer : public BHV_Timer {
public:
  explicit TestableTimer(IvPDomain domain) : BHV_Timer(domain) {}
  using BHV_Timer::updateStateDurations;
};

// Covers BHV timer behavior: posts idle and running duration counters with configured suffix.
TEST(BHVTimerTest, PostsIdleAndRunningDurationCountersWithConfiguredSuffix)
{
  TestableTimer timer(makeCourseSpeedDomain());

  ASSERT_TRUE(timer.setParam("var_status_idle", "IDLE_SECONDS"));
  ASSERT_TRUE(timer.setParam("var_status_running", "RUN_SECONDS"));
  ASSERT_TRUE(timer.setParam("status_suffix", "leg1"));
  EXPECT_FALSE(timer.setParam("status_suffix", "bad suffix"));
  EXPECT_FALSE(timer.setParam("unknown", "value"));

  timer.onIdleState();

  VarDataPair idle;
  VarDataPair running;
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "IDLE_SECONDS_leg1", idle));
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "RUN_SECONDS_leg1", running));
  EXPECT_DOUBLE_EQ(idle.get_ddata(), 0);
  EXPECT_DOUBLE_EQ(running.get_ddata(), 0);

  timer.clearMessages();
  std::unique_ptr<IvPFunction> ipf(timer.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(findBehaviorMessage(timer.getMessages(), "IDLE_SECONDS_leg1", idle));
  EXPECT_TRUE(findBehaviorMessage(timer.getMessages(), "RUN_SECONDS_leg1", running));
}

// Covers BHV timer behavior: uses default timer vars and accepts pre underscored suffix.
TEST(BHVTimerTest, UsesDefaultTimerVarsAndAcceptsPreUnderscoredSuffix)
{
  TestableTimer timer(makeCourseSpeedDomain());

  ASSERT_TRUE(timer.setParam("status_suffix", "_survey"));
  timer.onIdleState();

  VarDataPair idle;
  VarDataPair running;
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "TIMER_IDLE_survey", idle));
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "TIMER_RUNNING_survey", running));
  EXPECT_DOUBLE_EQ(idle.get_ddata(), 0);
  EXPECT_DOUBLE_EQ(running.get_ddata(), 0);
}

// Covers BHV timer behavior: rejects whitespace status variables without replacing previous value.
TEST(BHVTimerTest, RejectsWhitespaceStatusVariablesWithoutReplacingPreviousValue)
{
  BHV_Timer timer(makeCourseSpeedDomain());

  ASSERT_TRUE(timer.setParam("var_status_idle", "IDLE_OK"));
  ASSERT_TRUE(timer.setParam("var_status_running", "RUN_OK"));
  EXPECT_FALSE(timer.setParam("var_status_idle", "BAD VAR"));
  EXPECT_FALSE(timer.setParam("var_status_running", "BAD VAR"));

  timer.onRunState();

  VarDataPair idle;
  VarDataPair running;
  EXPECT_TRUE(findBehaviorMessage(timer.getMessages(), "IDLE_OK", idle));
  EXPECT_TRUE(findBehaviorMessage(timer.getMessages(), "RUN_OK", running));

  VarDataPair bad;
  EXPECT_FALSE(findBehaviorMessage(timer.getMessages(), "BAD VAR", bad));
}

// Covers BHV timer behavior: posts accumulated idle and running durations from behavior state tracker.
TEST(BHVTimerTest, PostsAccumulatedIdleAndRunningDurationsFromBehaviorStateTracker)
{
  InfoBuffer info;
  info.setCurrTime(10);

  TestableTimer timer(makeCourseSpeedDomain());
  timer.setInfoBuffer(&info);

  ASSERT_TRUE(timer.setParam("var_status_idle", "IDLE_SECONDS"));
  ASSERT_TRUE(timer.setParam("var_status_running", "RUN_SECONDS"));
  ASSERT_TRUE(timer.setParam("status_suffix", "survey"));

  timer.updateStateDurations("idle");
  info.setCurrTime(14);
  timer.updateStateDurations("idle");
  info.setCurrTime(19);
  timer.updateStateDurations("running");

  std::unique_ptr<IvPFunction> ipf(timer.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair idle;
  VarDataPair running;
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "IDLE_SECONDS_survey", idle));
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "RUN_SECONDS_survey", running));
  EXPECT_DOUBLE_EQ(idle.get_ddata(), 4);
  EXPECT_DOUBLE_EQ(running.get_ddata(), 5);
}

// Covers BHV timer behavior: empty suffix is rejected without clearing existing suffix.
TEST(BHVTimerTest, EmptySuffixIsRejectedWithoutClearingExistingSuffix)
{
  BHV_Timer timer(makeCourseSpeedDomain());

  ASSERT_TRUE(timer.setParam("status_suffix", "alpha"));
  EXPECT_ANY_THROW(timer.setParam("status_suffix", ""));

  timer.onRunState();

  VarDataPair idle;
  VarDataPair running;
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "TIMER_IDLE_alpha", idle));
  ASSERT_TRUE(findBehaviorMessage(timer.getMessages(), "TIMER_RUNNING_alpha", running));
}
