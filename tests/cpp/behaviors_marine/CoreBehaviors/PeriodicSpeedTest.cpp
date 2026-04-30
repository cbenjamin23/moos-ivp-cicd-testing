#include <gtest/gtest.h>

#include <memory>

#include "BHV_PeriodicSpeed.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV periodic speed behavior: alternates from lazy to busy and builds speed objective.
TEST(BHVPeriodicSpeedTest, AlternatesFromLazyToBusyAndBuildsSpeedObjective)
{
  InfoBuffer info;
  info.setCurrTime(0);

  BHV_PeriodicSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period_lazy", "5"));
  ASSERT_TRUE(behavior.setParam("period_busy", "3"));
  ASSERT_TRUE(behavior.setParam("period_speed", "2"));
  ASSERT_TRUE(behavior.setParam("basewidth", "1"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "0"));
  EXPECT_FALSE(behavior.setParam("period_lazy", "0"));
  EXPECT_FALSE(behavior.setParam("period_speed", "-1"));

  behavior.onSetParamComplete();
  VarDataPair busy_count;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PS_BUSY_COUNT", busy_count));
  EXPECT_DOUBLE_EQ(busy_count.get_ddata(), 0);

  behavior.clearMessages();
  behavior.onIdleToRunState();
  std::unique_ptr<IvPFunction> lazy_ipf(behavior.onRunState());
  EXPECT_EQ(lazy_ipf, nullptr);

  VarDataPair pending_busy;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PS_PENDING_ACTIVE", pending_busy));
  EXPECT_DOUBLE_EQ(pending_busy.get_ddata(), 5);

  behavior.clearMessages();
  info.setCurrTime(6);
  std::unique_ptr<IvPFunction> busy_ipf(behavior.onRunState());
  ASSERT_NE(busy_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*busy_ipf, 2), evalOneDimPoint(*busy_ipf, 0));

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PS_BUSY_COUNT", busy_count));
  EXPECT_DOUBLE_EQ(busy_count.get_ddata(), 1);
}

// Covers BHV periodic speed behavior: initially busy uses deprecated ZAIC aliases and returns to lazy.
TEST(BHVPeriodicSpeedTest, InitiallyBusyUsesDeprecatedZaicAliasesAndReturnsToLazy)
{
  InfoBuffer info;
  info.setCurrTime(100);

  BHV_PeriodicSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period_lazy", "8"));
  ASSERT_TRUE(behavior.setParam("period_busy", "2"));
  ASSERT_TRUE(behavior.setParam("period_speed", "3"));
  ASSERT_TRUE(behavior.setParam("period_basewidth", "1"));
  ASSERT_TRUE(behavior.setParam("zaic_peakwidth", "0"));
  ASSERT_TRUE(behavior.setParam("zaic_summit_delta", "35"));
  ASSERT_TRUE(behavior.setParam("initially_busy", "true"));
  EXPECT_FALSE(behavior.setParam("initially_busy", "maybe"));

  std::unique_ptr<IvPFunction> busy_ipf(behavior.onRunState());
  ASSERT_NE(busy_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*busy_ipf, 3), evalOneDimPoint(*busy_ipf, 0));

  behavior.clearMessages();
  info.setCurrTime(103);
  std::unique_ptr<IvPFunction> lazy_ipf(behavior.onRunState());
  EXPECT_EQ(lazy_ipf, nullptr);

  VarDataPair pending_lazy;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PS_PENDING_INACTIVE", pending_lazy));
  EXPECT_DOUBLE_EQ(pending_lazy.get_ddata(), 0);
}

// Covers BHV periodic speed behavior: idle reset policy controls whether timer restarts.
TEST(BHVPeriodicSpeedTest, IdleResetPolicyControlsWhetherTimerRestarts)
{
  InfoBuffer info;
  info.setCurrTime(0);

  BHV_PeriodicSpeed resetter(makeCourseSpeedDomain());
  resetter.setInfoBuffer(&info);
  ASSERT_TRUE(resetter.setParam("period_lazy", "10"));
  ASSERT_TRUE(resetter.setParam("period_busy", "5"));
  ASSERT_TRUE(resetter.setParam("period_speed", "2"));

  resetter.onIdleState();
  VarDataPair reset_pending;
  ASSERT_TRUE(findBehaviorMessage(resetter.getMessages(), "PS_PENDING_ACTIVE", reset_pending));
  EXPECT_DOUBLE_EQ(reset_pending.get_ddata(), 10);

  BHV_PeriodicSpeed continuous(makeCourseSpeedDomain());
  continuous.setInfoBuffer(&info);
  ASSERT_TRUE(continuous.setParam("period_lazy", "10"));
  ASSERT_TRUE(continuous.setParam("period_busy", "5"));
  ASSERT_TRUE(continuous.setParam("period_speed", "2"));
  ASSERT_TRUE(continuous.setParam("reset_upon_running", "false"));

  std::unique_ptr<IvPFunction> first_ipf(continuous.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  continuous.clearMessages();
  info.setCurrTime(6);
  continuous.onIdleState();

  VarDataPair continuous_pending;
  ASSERT_TRUE(findBehaviorMessage(continuous.getMessages(), "PS_PENDING_ACTIVE",
                                  continuous_pending));
  EXPECT_DOUBLE_EQ(continuous_pending.get_ddata(), 4);
}

// Covers BHV periodic speed behavior: zero speed is allowed and priority param is not delegated.
TEST(BHVPeriodicSpeedTest, ZeroSpeedIsAllowedAndPriorityParamIsNotDelegated)
{
  InfoBuffer info;
  info.setCurrTime(0);

  BHV_PeriodicSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period_lazy", "1"));
  ASSERT_TRUE(behavior.setParam("period_busy", "5"));
  ASSERT_TRUE(behavior.setParam("period_speed", "0"));
  EXPECT_FALSE(behavior.setParam("priority", "35"));

  std::unique_ptr<IvPFunction> waiting_ipf(behavior.onRunState());
  EXPECT_EQ(waiting_ipf, nullptr);

  info.setCurrTime(2);
  std::unique_ptr<IvPFunction> busy_ipf(behavior.onRunState());
  ASSERT_NE(busy_ipf, nullptr);
  EXPECT_DOUBLE_EQ(busy_ipf->getPWT(), 100);
  EXPECT_GT(evalOneDimPoint(*busy_ipf, 0), evalOneDimPoint(*busy_ipf, 2));
}

// Covers BHV periodic speed behavior: multiple lazy busy cycles increment busy count.
TEST(BHVPeriodicSpeedTest, MultipleLazyBusyCyclesIncrementBusyCount)
{
  InfoBuffer info;
  info.setCurrTime(0);

  BHV_PeriodicSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period_lazy", "2"));
  ASSERT_TRUE(behavior.setParam("period_busy", "1"));
  ASSERT_TRUE(behavior.setParam("period_speed", "2"));

  std::unique_ptr<IvPFunction> lazy0(behavior.onRunState());
  EXPECT_EQ(lazy0, nullptr);

  info.setCurrTime(3);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> busy1(behavior.onRunState());
  ASSERT_NE(busy1, nullptr);

  VarDataPair busy_count;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PS_BUSY_COUNT", busy_count));
  EXPECT_DOUBLE_EQ(busy_count.get_ddata(), 1);

  info.setCurrTime(4.5);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> lazy1(behavior.onRunState());
  EXPECT_EQ(lazy1, nullptr);

  info.setCurrTime(7);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> busy2(behavior.onRunState());
  ASSERT_NE(busy2, nullptr);

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PS_BUSY_COUNT", busy_count));
  EXPECT_DOUBLE_EQ(busy_count.get_ddata(), 2);
}
