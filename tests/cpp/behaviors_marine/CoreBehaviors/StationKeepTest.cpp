#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_StationKeep.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV station keep behavior: seeks configured station and posts range telemetry.
TEST(BHVStationKeepTest, SeeksConfiguredStationAndPostsRangeTelemetry)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1);

  BHV_StationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "5"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "20"));
  ASSERT_TRUE(behavior.setParam("outer_speed", "1"));
  ASSERT_TRUE(behavior.setParam("transit_speed", "2"));
  EXPECT_FALSE(behavior.setParam("point", "bad"));
  EXPECT_FALSE(behavior.setParam("inner_radius", "0"));
  EXPECT_FALSE(behavior.setParam("transit_speed", "-1"));

  std::unique_ptr<IvPFunction> latch_ipf(behavior.onRunState());
  ASSERT_NE(latch_ipf, nullptr);

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 90, 0));

  VarDataPair dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DIST_TO_STATION", dist));
  EXPECT_DOUBLE_EQ(dist.get_ddata(), 100);

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PSKEEP_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "SEEKING_STATION");

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  ASSERT_TRUE(polygon.is_string());
  EXPECT_NE(polygon.get_sdata().find("station-keep-out"), std::string::npos);
  EXPECT_NE(polygon.get_sdata().find("radius=20"), std::string::npos);
}

// Covers BHV station keep behavior: hibernates inside configured hibernation radius.
TEST(BHVStationKeepTest, HibernatesInsideConfiguredHibernationRadius)
{
  InfoBuffer info;
  info.setValue("NAV_X", 20);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 270);
  info.setValue("NAV_SPEED", 1);

  BHV_StationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "0,0"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "5"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "10"));
  ASSERT_TRUE(behavior.setParam("hibernation_radius", "50"));

  std::unique_ptr<IvPFunction> latch_ipf(behavior.onRunState());
  ASSERT_NE(latch_ipf, nullptr);

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 270, 0),
            evalCourseSpeedPoint(*ipf, 270, 2));

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PSKEEP_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "HIBERNATING");

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("station-keep-out"), std::string::npos);
}

// Covers BHV station keep behavior: center activate uses current ownship position.
TEST(BHVStationKeepTest, CenterActivateUsesCurrentOwnshipPosition)
{
  InfoBuffer info;
  info.setCurrTime(50);
  info.setValue("NAV_X", 12);
  info.setValue("NAV_Y", -8);
  info.setValue("NAV_HEADING", 45);
  info.setValue("NAV_SPEED", 0.5);

  BHV_StationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("center_activate", "true"));
  ASSERT_TRUE(behavior.setParam("swing_time", "120"));
  ASSERT_TRUE(behavior.setParam("visual_hints",
                                "edge_color=yellow,vertex_color=blue,edge_size=2"));
  EXPECT_FALSE(behavior.setParam("center_activate", "maybe"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 45, 0),
            evalCourseSpeedPoint(*ipf, 45, 2));

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  ASSERT_TRUE(polygon.is_string());
  EXPECT_NE(polygon.get_sdata().find("x=12.0"), std::string::npos);
  EXPECT_NE(polygon.get_sdata().find("y=-8.0"), std::string::npos);
  EXPECT_NE(polygon.get_sdata().find("edge_color=yellow"), std::string::npos);
}

// Covers BHV station keep behavior: hibernation progress history returns to sleep after closing then opening.
TEST(BHVStationKeepTest, HibernationProgressHistoryReturnsToSleepAfterClosingThenOpening)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 80);
  info.setValue("NAV_HEADING", 180);
  info.setValue("NAV_SPEED", 2);

  BHV_StationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=0"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "5"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "15"));
  ASSERT_TRUE(behavior.setParam("hibernation_radius", "50"));
  ASSERT_TRUE(behavior.setParam("outer_speed", "1"));
  ASSERT_TRUE(behavior.setParam("extra_speed", "3"));

  std::unique_ptr<IvPFunction> latch_ipf(behavior.onRunState());
  ASSERT_NE(latch_ipf, nullptr);

  info.setCurrTime(1);
  info.setValue("NAV_Y", 80);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> seeking_ipf(behavior.onRunState());
  ASSERT_NE(seeking_ipf, nullptr);

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PSKEEP_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "SEEKING_STATION");
  EXPECT_GT(evalCourseSpeedPoint(*seeking_ipf, 180, 3),
            evalCourseSpeedPoint(*seeking_ipf, 180, 0));

  const std::vector<std::pair<double, double>> inbound = {
    {2, 45}, {4, 40}, {6, 35}, {8, 30}
  };
  for(const auto& sample : inbound) {
    info.setCurrTime(sample.first);
    info.setValue("NAV_Y", sample.second);
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    ASSERT_NE(ipf, nullptr);
    ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PSKEEP_MODE", mode));
    EXPECT_EQ(mode.get_sdata(), "SEEKING_STATION");
  }

  const std::vector<std::pair<double, double>> outbound = {
    {10, 31}, {12, 34}, {14, 38}, {16, 42}
  };
  for(const auto& sample : outbound) {
    info.setCurrTime(sample.first);
    info.setValue("NAV_Y", sample.second);
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    ASSERT_NE(ipf, nullptr);
  }

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PSKEEP_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "HIBERNATING");
}

// Covers BHV station keep behavior: run to idle erases rings and center activate recenters on next run.
TEST(BHVStationKeepTest, RunToIdleErasesRingsAndCenterActivateRecentersOnNextRun)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 5);
  info.setValue("NAV_Y", 6);
  info.setValue("NAV_HEADING", 45);
  info.setValue("NAV_SPEED", 0.5);

  BHV_StationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("center_activate", "true"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "12"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "3"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  behavior.clearMessages();
  behavior.onRunToIdleState();

  int inactive_rings = 0;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POLYGON" && message.is_string() &&
       message.get_sdata().find("active=false") != std::string::npos)
      inactive_rings++;
  }
  EXPECT_EQ(inactive_rings, 3);

  info.setCurrTime(20);
  info.setValue("NAV_X", -10);
  info.setValue("NAV_Y", 30);
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("x=-10.0"), std::string::npos);
  EXPECT_NE(polygon.get_sdata().find("y=30.0"), std::string::npos);
}
