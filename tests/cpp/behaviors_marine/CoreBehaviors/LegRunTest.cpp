#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_LegRun.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV leg run behavior: initializes fixed leg and builds waypoint objective.
TEST(BHVLegRunTest, InitializesFixedLegAndBuildsWaypointObjective)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_LegRun behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("p1", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("p2", "x=100,y=100"));
  ASSERT_TRUE(behavior.setParam("turn_rad", "20"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("capture_radius", "5"));
  ASSERT_TRUE(behavior.setParam("init_leg_mode", "fixed"));
  EXPECT_FALSE(behavior.setParam("p1", "bad"));
  EXPECT_FALSE(behavior.setParam("speed", "-1"));
  EXPECT_FALSE(behavior.setParam("init_leg_mode", "sideways"));

  behavior.onSetParamComplete();
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 90, 2));

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "leg1");
}

// Covers BHV leg run behavior: close turn initialization chooses nearest leg endpoint.
TEST(BHVLegRunTest, CloseTurnInitializationChoosesNearestLegEndpoint)
{
  InfoBuffer info;
  info.setCurrTime(100);
  info.setValue("NAV_X", 90);
  info.setValue("NAV_Y", 100);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 2);

  BHV_LegRun behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("p1", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("p2", "x=100,y=100"));
  ASSERT_TRUE(behavior.setParam("turn_rad", "20"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("init_leg_mode", "close_turn"));
  ASSERT_TRUE(behavior.setParam("visual_hints", "turn_edge_color=yellow,legpt_vertex_size=9"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "leg2");

  VarDataPair report;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_MODE_REPORT", report));
  ASSERT_TRUE(report.is_string());
  EXPECT_NE(report.get_sdata().find("mode=leg2"), std::string::npos);

  VarDataPair now_speed;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_NOW_SPD", now_speed));
  EXPECT_DOUBLE_EQ(now_speed.get_ddata(), 2);
}

// Covers BHV leg run behavior: missing navigation or invalid leg suppresses objective.
TEST(BHVLegRunTest, MissingNavigationOrInvalidLegSuppressesObjective)
{
  InfoBuffer missing_nav;
  missing_nav.setValue("NAV_X", 0);
  missing_nav.setValue("NAV_Y", 0);

  BHV_LegRun no_nav(makeCourseSpeedDomain());
  no_nav.setInfoBuffer(&missing_nav);
  ASSERT_TRUE(no_nav.setParam("p1", "x=0,y=100"));
  ASSERT_TRUE(no_nav.setParam("p2", "x=100,y=100"));
  ASSERT_TRUE(no_nav.setParam("turn_rad", "20"));

  no_nav.onSetParamComplete();
  std::unique_ptr<IvPFunction> no_nav_ipf(no_nav.onRunState());
  EXPECT_EQ(no_nav_ipf, nullptr);
  EXPECT_FALSE(no_nav.stateOK());

  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_LegRun invalid_leg(makeCourseSpeedDomain());
  invalid_leg.setInfoBuffer(&info);
  ASSERT_TRUE(invalid_leg.setParam("p1", "x=0,y=100"));
  EXPECT_FALSE(invalid_leg.setParam("turn_rad", "-1"));

  invalid_leg.onSetParamComplete();
  std::unique_ptr<IvPFunction> invalid_ipf(invalid_leg.onRunState());
  EXPECT_EQ(invalid_ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(invalid_leg.getMessages(), "BHV_WARNING", warning));
  ASSERT_TRUE(warning.is_string());
  EXPECT_NE(warning.get_sdata().find("Invalid or unset legrun"), std::string::npos);
}

// Covers BHV leg run behavior: mission style deprecated vx config posts preview and settings.
TEST(BHVLegRunTest, MissionStyleDeprecatedVxConfigPostsPreviewAndSettings)
{
  // Legacy .bhv files can define a leg with vx1/vx2 instead of a modern points
  // string; this pins the deprecated config path and its visualization posts.
  InfoBuffer info;
  info.setValue("NAV_X", 70);
  info.setValue("NAV_Y", -80);
  info.setValue("NAV_HEADING", 180);
  info.setValue("NAV_SPEED", 4);

  BHV_LegRun behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("vx1", "70,-35"));
  ASSERT_TRUE(behavior.setParam("vx2", "-35,-85"));
  ASSERT_TRUE(behavior.setParam("lead", "5"));
  ASSERT_TRUE(behavior.setParam("lead_damper", "1"));
  ASSERT_TRUE(behavior.setParam("capture_line", "true"));
  ASSERT_TRUE(behavior.setParam("capture_radius", "2"));
  ASSERT_TRUE(behavior.setParam("slip_radius", "15"));
  ASSERT_TRUE(behavior.setParam("speed", "4.0"));
  ASSERT_TRUE(behavior.setParam("repeat", "1"));
  ASSERT_TRUE(behavior.setParam("turn1_dir", "port"));
  ASSERT_TRUE(behavior.setParam("turn2_dir", "star"));
  ASSERT_TRUE(behavior.setParam("turn1_rad", "20"));
  ASSERT_TRUE(behavior.setParam("turn2_rad", "20"));
  ASSERT_TRUE(behavior.setParam("turn1_bias", "100"));
  ASSERT_TRUE(behavior.setParam("turn2_bias", "100"));
  ASSERT_TRUE(behavior.setParam("visual_hints",
                                "nextpt_vertex_size=8,vertex_color=dodger_blue,"
                                "edge_color=white,turn_edge_color=white"));

  behavior.onSetParamComplete();

  VarDataPair settings;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_SETTINGS", settings));
  ASSERT_TRUE(settings.is_string());
  EXPECT_NE(settings.get_sdata().find("type=BHV_LegRun"), std::string::npos);
  EXPECT_NE(settings.get_sdata().find("name=legrun"), std::string::npos);
  EXPECT_NE(settings.get_sdata().find("speed=4"), std::string::npos);
  EXPECT_NE(settings.get_sdata().find("capture_line=true"), std::string::npos);

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_MODE", mode));
  EXPECT_EQ(mode.get_sdata(), "leg1");

  bool saw_preview = false;
  bool saw_leg_endpoint = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string())
      saw_preview |=
        (message.get_sdata().find("pview") != std::string::npos);
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      saw_leg_endpoint |=
        (message.get_sdata().find("_p1") != std::string::npos ||
         message.get_sdata().find("_p2") != std::string::npos);
  }
  EXPECT_TRUE(saw_preview);
  EXPECT_TRUE(saw_leg_endpoint);

  VarDataPair speed;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_NOW_SPD", speed));
  EXPECT_DOUBLE_EQ(speed.get_ddata(), 4);
}

// Covers BHV leg run behavior: leg speed schedule rejects bad entries and can be cleared.
TEST(BHVLegRunTest, LegSpeedScheduleRejectsBadEntriesAndCanBeCleared)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1);

  BHV_LegRun behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("p1", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("p2", "x=100,y=100"));
  ASSERT_TRUE(behavior.setParam("turn_rad", "20"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  EXPECT_TRUE(behavior.setParam("leg_spds", "replace,2:1.5,3"));
  EXPECT_FALSE(behavior.setParam("leg_spds", "2:fast"));
  EXPECT_FALSE(behavior.setParam("leg_spds", "-1:2"));
  EXPECT_FALSE(behavior.setParam("leg_spds", "6"));
  EXPECT_TRUE(behavior.setParam("leg_spds", "reset"));
  EXPECT_TRUE(behavior.setParam("leg_spds", "clear"));

  behavior.onSetParamComplete();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair speed;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LR_NOW_SPD", speed));
  EXPECT_DOUBLE_EQ(speed.get_ddata(), 2);
}
