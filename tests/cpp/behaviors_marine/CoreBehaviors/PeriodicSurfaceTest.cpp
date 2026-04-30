#include <gtest/gtest.h>

#include <memory>

#include "BHV_PeriodicSurface.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV periodic surface behavior: waits for period then commands ascent to surface.
TEST(BHVPeriodicSurfaceTest, WaitsForPeriodThenCommandsAscentToSurface)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 20);
  info.setValue("NAV_SPEED", 2);

  BHV_PeriodicSurface behavior(makeSpeedDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period", "5"));
  ASSERT_TRUE(behavior.setParam("zero_speed_depth", "1"));
  ASSERT_TRUE(behavior.setParam("ascent_speed", "2"));
  ASSERT_TRUE(behavior.setParam("max_time_at_surface", "3"));
  EXPECT_FALSE(behavior.setParam("period", "0"));
  EXPECT_FALSE(behavior.setParam("ascent_speed", "-1"));
  EXPECT_FALSE(behavior.setParam("ascent_grade", "steep"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(6);
  std::unique_ptr<IvPFunction> ascent_ipf(behavior.onRunState());
  ASSERT_NE(ascent_ipf, nullptr);

  VarDataPair ascend;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PERIODIC_ASCEND", ascend));
  EXPECT_EQ(ascend.get_sdata(), "true");
  EXPECT_GT(evalTwoDimPoint(*ascent_ipf, 2, 0),
            evalTwoDimPoint(*ascent_ipf, 0, 20));

  behavior.clearMessages();
  info.setCurrTime(7);
  info.setValue("NAV_DEPTH", 0.5);
  std::unique_ptr<IvPFunction> surface_ipf(behavior.onRunState());
  ASSERT_NE(surface_ipf, nullptr);

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PERIODIC_ASCEND", ascend));
  EXPECT_EQ(ascend.get_sdata(), "false");
  VarDataPair at_surface;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TIME_AT_SURFACE", at_surface));
  EXPECT_FALSE(at_surface.is_string());
  EXPECT_DOUBLE_EQ(at_surface.get_ddata(), 0);
}

// Covers BHV periodic surface behavior: mark variable change restarts surface period.
TEST(BHVPeriodicSurfaceTest, MarkVariableChangeRestartsSurfacePeriod)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 12);
  info.setValue("NAV_SPEED", 1.5);
  info.setValue("GPS_UPDATE_RECEIVED", "initial");

  BHV_PeriodicSurface behavior(makeSpeedDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period", "5"));
  ASSERT_TRUE(behavior.setParam("ascent_speed", "1"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(6);
  info.setValue("GPS_UPDATE_RECEIVED", "fresh-gps-fix");

  std::unique_ptr<IvPFunction> marked_ipf(behavior.onRunState());
  EXPECT_EQ(marked_ipf, nullptr);

  VarDataPair pending;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PENDING_SURFACE", pending));
  EXPECT_FALSE(pending.is_string());

  behavior.clearMessages();
  info.setCurrTime(12);

  std::unique_ptr<IvPFunction> ascent_ipf(behavior.onRunState());
  ASSERT_NE(ascent_ipf, nullptr);

  VarDataPair ascend;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PERIODIC_ASCEND", ascend));
  EXPECT_EQ(ascend.get_sdata(), "true");
}

// Covers BHV periodic surface behavior: acomms mark extends pending surface window.
TEST(BHVPeriodicSurfaceTest, AcommsMarkExtendsPendingSurfaceWindow)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 20);
  info.setValue("NAV_SPEED", 2);

  BHV_PeriodicSurface behavior(makeSpeedDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period", "5"));
  ASSERT_TRUE(behavior.setParam("acomms_mark_variable", "ACOMMS_UPDATE,2,4"));
  ASSERT_TRUE(behavior.setParam("ascent_speed", "current"));
  EXPECT_FALSE(behavior.setParam("acomms_mark_variable", "BAD VAR,2,4"));
  EXPECT_FALSE(behavior.setParam("acomms_mark_variable", "ACOMMS_UPDATE,5,4"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(4);
  info.setValue("ACOMMS_UPDATE", "ping-1");
  std::unique_ptr<IvPFunction> extended_ipf(behavior.onRunState());
  EXPECT_EQ(extended_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(6);
  std::unique_ptr<IvPFunction> still_waiting_ipf(behavior.onRunState());
  EXPECT_EQ(still_waiting_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(8);
  std::unique_ptr<IvPFunction> ascent_ipf(behavior.onRunState());
  ASSERT_NE(ascent_ipf, nullptr);

  EXPECT_GT(evalTwoDimPoint(*ascent_ipf, 2, 0),
            evalTwoDimPoint(*ascent_ipf, 0, 20));
}

// Covers BHV periodic surface behavior: missing depth or speed posts error status.
TEST(BHVPeriodicSurfaceTest, MissingDepthOrSpeedPostsErrorStatus)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 5);

  BHV_PeriodicSurface behavior(makeSpeedDepthDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair pending;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PENDING_SURFACE", pending));
  ASSERT_TRUE(pending.is_string());
  EXPECT_EQ(pending.get_sdata(), "ERROR");
}

// Covers BHV periodic surface behavior: surface timeout resets period and posts timeout notice.
TEST(BHVPeriodicSurfaceTest, SurfaceTimeoutResetsPeriodAndPostsTimeoutNotice)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 10);
  info.setValue("NAV_SPEED", 1);

  BHV_PeriodicSurface behavior(makeSpeedDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period", "2"));
  ASSERT_TRUE(behavior.setParam("zero_speed_depth", "1"));
  ASSERT_TRUE(behavior.setParam("ascent_speed", "1"));
  ASSERT_TRUE(behavior.setParam("max_time_at_surface", "3"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  info.setCurrTime(3);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ascent_ipf(behavior.onRunState());
  ASSERT_NE(ascent_ipf, nullptr);

  info.setCurrTime(4);
  info.setValue("NAV_DEPTH", 0);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> surface_ipf(behavior.onRunState());
  ASSERT_NE(surface_ipf, nullptr);

  info.setCurrTime(8);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> timeout_ipf(behavior.onRunState());
  EXPECT_EQ(timeout_ipf, nullptr);

  VarDataPair timeout;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PSF_AT_SURFACE_TIMEOUT", timeout));
  ASSERT_TRUE(timeout.is_string());
  EXPECT_NE(timeout.get_sdata().find("Timestamp=8.00"), std::string::npos);
}

// Covers BHV periodic surface behavior: custom status variables and numeric mark reset pending window.
TEST(BHVPeriodicSurfaceTest, CustomStatusVariablesAndNumericMarkResetPendingWindow)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 15);
  info.setValue("NAV_SPEED", 1.5);
  info.setValue("GPS_FIX_COUNT", 1.0);

  BHV_PeriodicSurface behavior(makeSpeedDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("period", "5"));
  ASSERT_TRUE(behavior.setParam("mark_variable", "GPS_FIX_COUNT"));
  ASSERT_TRUE(behavior.setParam("pending_status_var", "SURFACE_WAIT"));
  ASSERT_TRUE(behavior.setParam("atsurface_status_var", "SURFACE_TIME"));
  EXPECT_FALSE(behavior.setParam("mark_variable", ""));
  EXPECT_FALSE(behavior.setParam("pending_status_var", ""));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  info.setCurrTime(4);
  info.setValue("GPS_FIX_COUNT", 2.0);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> marked_ipf(behavior.onRunState());
  EXPECT_EQ(marked_ipf, nullptr);

  VarDataPair pending;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SURFACE_WAIT", pending));
  EXPECT_DOUBLE_EQ(pending.get_ddata(), 1);

  info.setCurrTime(5);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> still_waiting_ipf(behavior.onRunState());
  EXPECT_EQ(still_waiting_ipf, nullptr);

  VarDataPair surface_time;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SURFACE_TIME", surface_time));
  EXPECT_DOUBLE_EQ(surface_time.get_ddata(), 0);
}

// Covers BHV periodic surface behavior: ascent grade controls speed ramp.
TEST(BHVPeriodicSurfaceTest, AscentGradeControlsSpeedRamp)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 20);
  info.setValue("NAV_SPEED", 2);

  BHV_PeriodicSurface linear(makeSpeedDepthDomain());
  linear.setInfoBuffer(&info);
  ASSERT_TRUE(linear.setParam("period", "1"));
  ASSERT_TRUE(linear.setParam("zero_speed_depth", "0"));
  ASSERT_TRUE(linear.setParam("ascent_speed", "4"));
  ASSERT_TRUE(linear.setParam("ascent_grade", "linear"));
  std::unique_ptr<IvPFunction> linear_wait(linear.onRunState());
  EXPECT_EQ(linear_wait, nullptr);

  info.setCurrTime(2);
  std::unique_ptr<IvPFunction> linear_start(linear.onRunState());
  ASSERT_NE(linear_start, nullptr);

  info.setCurrTime(3);
  info.setValue("NAV_DEPTH", 10);
  std::unique_ptr<IvPFunction> linear_mid(linear.onRunState());
  ASSERT_NE(linear_mid, nullptr);
  EXPECT_GT(evalTwoDimPoint(*linear_mid, 2, 0),
            evalTwoDimPoint(*linear_mid, 4, 0));

  InfoBuffer fullspeed_info;
  fullspeed_info.setCurrTime(0);
  fullspeed_info.setValue("NAV_DEPTH", 20);
  fullspeed_info.setValue("NAV_SPEED", 2);

  BHV_PeriodicSurface fullspeed(makeSpeedDepthDomain());
  fullspeed.setInfoBuffer(&fullspeed_info);
  ASSERT_TRUE(fullspeed.setParam("period", "1"));
  ASSERT_TRUE(fullspeed.setParam("zero_speed_depth", "0"));
  ASSERT_TRUE(fullspeed.setParam("ascent_speed", "4"));
  ASSERT_TRUE(fullspeed.setParam("ascent_grade", "fullspeed"));
  std::unique_ptr<IvPFunction> full_wait(fullspeed.onRunState());
  EXPECT_EQ(full_wait, nullptr);

  fullspeed_info.setCurrTime(2);
  std::unique_ptr<IvPFunction> full_start(fullspeed.onRunState());
  ASSERT_NE(full_start, nullptr);

  fullspeed_info.setCurrTime(3);
  fullspeed_info.setValue("NAV_DEPTH", 10);
  std::unique_ptr<IvPFunction> full_mid(fullspeed.onRunState());
  ASSERT_NE(full_mid, nullptr);
  EXPECT_GT(evalTwoDimPoint(*full_mid, 4, 0),
            evalTwoDimPoint(*full_mid, 2, 0));
}
