#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_OpRegion.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV op region behavior: verifies inside polygon and posts perimeter telemetry.
TEST(BHVOpRegionTest, VerifiesInsidePolygonAndPostsPerimeterTelemetry)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("NAV_DEPTH", 10);
  info.setValue("NAV_ALTITUDE", 20);

  BHV_OpRegion behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50}"));
  ASSERT_TRUE(behavior.setParam("max_depth", "30"));
  ASSERT_TRUE(behavior.setParam("min_altitude", "5"));
  ASSERT_TRUE(behavior.setParam("max_time", "60"));
  ASSERT_TRUE(behavior.setParam("time_remaining_var", "OPREG_TIME_LEFT"));
  ASSERT_TRUE(behavior.setParam("opregion_poly_var", "OPREG_POLY"));
  EXPECT_FALSE(behavior.setParam("max_depth", "-1"));
  EXPECT_FALSE(behavior.setParam("polygon", "bad"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair remaining;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_TIME_LEFT", remaining));
  EXPECT_DOUBLE_EQ(remaining.get_ddata(), 60);

  VarDataPair dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_ABSOLUTE_PERIM_DIST", dist));
  EXPECT_DOUBLE_EQ(dist.get_ddata(), 50);
}

// Covers BHV op region behavior: soft polygon breach posts manual override warning and breach flag.
TEST(BHVOpRegionTest, SoftPolygonBreachPostsManualOverrideWarningAndBreachFlag)
{
  InfoBuffer info;
  info.setCurrTime(20);
  info.setValue("NAV_X", 80);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 270);
  info.setValue("NAV_SPEED", 2);
  info.setValue("NAV_DEPTH", 10);
  info.setValue("NAV_ALTITUDE", 20);

  BHV_OpRegion behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50},label=op_region"));
  ASSERT_TRUE(behavior.setParam("soft_poly_breach", "true"));
  ASSERT_TRUE(behavior.setParam("trigger_exit_time", "0"));
  ASSERT_TRUE(behavior.setParam("breached_poly_flag", "OPREG_POLY_BREACH=true"));
  ASSERT_TRUE(behavior.setParam("visual_hints", "edge_color=yellow,vertex_size=2"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair override;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "MOOS_MANUAL_OVERRIDE", override));
  EXPECT_EQ(override.get_sdata(), "true");

  VarDataPair flag;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_POLY_BREACH", flag));
  EXPECT_EQ(flag.get_sdata(), "true");

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("Soft polygon"), std::string::npos);
}

// Covers BHV op region behavior: depth altitude and timeout breaches post dedicated flags.
TEST(BHVOpRegionTest, DepthAltitudeAndTimeoutBreachesPostDedicatedFlags)
{
  InfoBuffer info;
  info.setCurrTime(100);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("NAV_DEPTH", 10);
  info.setValue("NAV_ALTITUDE", 20);

  BHV_OpRegion behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50}"));
  ASSERT_TRUE(behavior.setParam("max_depth", "30"));
  ASSERT_TRUE(behavior.setParam("min_altitude", "5"));
  ASSERT_TRUE(behavior.setParam("max_time", "50"));
  ASSERT_TRUE(behavior.setParam("breached_depth_flag", "OPREG_DEPTH_BREACH=true"));
  ASSERT_TRUE(behavior.setParam("breached_altitude_flag", "OPREG_ALT_BREACH=true"));
  ASSERT_TRUE(behavior.setParam("breached_time_flag", "OPREG_TIME_BREACH=true"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  behavior.clearMessages();
  info.setCurrTime(151);
  info.setValue("NAV_DEPTH", 42);
  info.setValue("NAV_ALTITUDE", 2);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair depth_flag;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_DEPTH_BREACH", depth_flag));
  EXPECT_EQ(depth_flag.get_sdata(), "true");

  VarDataPair altitude_flag;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_ALT_BREACH", altitude_flag));
  EXPECT_EQ(altitude_flag.get_sdata(), "true");

  VarDataPair time_flag;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_TIME_BREACH", time_flag));
  EXPECT_EQ(time_flag.get_sdata(), "true");

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_FALSE(error.get_sdata().empty());
}

// Covers BHV op region behavior: idle erases viewer polygon and posts last computed time remaining.
TEST(BHVOpRegionTest, IdleErasesViewerPolygonAndPostsLastComputedTimeRemaining)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1);
  info.setValue("NAV_DEPTH", 5);
  info.setValue("NAV_ALTITUDE", 30);

  BHV_OpRegion behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("polygon", "pts={-50,-50:50,-50:50,50:-50,50},label=op_region"));
  ASSERT_TRUE(behavior.setParam("max_time", "60"));
  ASSERT_TRUE(behavior.setParam("time_remaining_var", "OPREG_TIME_LEFT"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  behavior.clearMessages();

  info.setCurrTime(20);

  behavior.onIdleState();

  VarDataPair time_left;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OPREG_TIME_LEFT", time_left));
  EXPECT_DOUBLE_EQ(time_left.get_ddata(), 60);

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("active=false"), std::string::npos);
}
