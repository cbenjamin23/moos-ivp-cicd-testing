#include <gtest/gtest.h>

#include <memory>

#include "BHV_AvoidObstacleV24.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"
#include "PlatModel.h"

// Covers BHV avoid obstacle V24 behavior: accepts obstacle geometry and safety configuration.
TEST(BHVAvoidObstacleV24Test, AcceptsObstacleGeometryAndSafetyConfiguration)
{
  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("poly", "pts={20,-10:40,-10:40,10:20,10}"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "100"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "150"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "5"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "50"));
  ASSERT_TRUE(behavior.setParam("id", "obstacle_1"));
  ASSERT_TRUE(behavior.setParam("rng_flag", "<100 OBSTACLE_NEAR=true"));
  ASSERT_TRUE(behavior.setParam("allstop_on_breach", "true"));
  EXPECT_FALSE(behavior.setParam("id", "obstacle_2"));
  EXPECT_FALSE(behavior.setParam("pwt_outer_dist", "-1"));
  EXPECT_FALSE(behavior.setParam("rng_flag", "<bad OBSTACLE_NEAR=true"));
}

// Covers BHV avoid obstacle V24 behavior: set param complete posts config status.
TEST(BHVAvoidObstacleV24Test, SetParamCompletePostsConfigStatus)
{
  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("poly", "pts={20,-10:40,-10:40,10:20,10}"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "100"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "150"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "5"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "50"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("id", "obstacle_1"));
  ASSERT_TRUE(behavior.setParam("visual_hints",
                                "gut_edge_color=yellow,buff_min_edge_color=green"));

  behavior.onSetParamComplete();

  VarDataPair settings;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_SETTINGS", settings));
  ASSERT_TRUE(settings.is_string());
  EXPECT_NE(settings.get_sdata().find("type=BHV_AvoidObstacleV24"), std::string::npos);
  EXPECT_NE(settings.get_sdata().find("allowable_ttc=30.00"), std::string::npos);
  EXPECT_NE(settings.get_sdata().find("pwt_outer_dist=100.00"), std::string::npos);
}

// Covers BHV avoid obstacle V24 behavior: obstacle manager resolution completes behavior.
TEST(BHVAvoidObstacleV24Test, ObstacleManagerResolutionCompletesBehavior)
{
  InfoBuffer info;
  info.setValue("OBM_RESOLVED", "obstacle_1");

  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("poly", "pts={20,-10:40,-10:40,10:20,10}"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "100"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "150"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "5"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "50"));
  ASSERT_TRUE(behavior.setParam("id", "obstacle_1"));

  behavior.onSetParamComplete();
  behavior.clearMessages();
  behavior.onEveryState("running");

  VarDataPair noted;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "NOTED_RESOLVED", noted));
  EXPECT_EQ(noted.get_sdata(), "obstacle_1");

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
}

// Covers BHV avoid obstacle V24 behavior: spawnable template requests obstacle manager alerts.
TEST(BHVAvoidObstacleV24Test, SpawnableTemplateRequestsObstacleManagerAlerts)
{
  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());

  behavior.setDynamicallySpawnable(true);
  ASSERT_TRUE(behavior.setParam("updates", "OBSTACLE_UPDATES"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "125"));

  behavior.onHelmStart();

  VarDataPair request;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OBM_ALERT_REQUEST", request));
  ASSERT_TRUE(request.is_string());
  EXPECT_NE(request.get_sdata().find("update_var=OBSTACLE_UPDATES"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("alert_range=125"), std::string::npos);
}

// Covers BHV avoid obstacle V24 behavior: range flag expands obstacle macros from platform model.
TEST(BHVAvoidObstacleV24Test, RangeFlagExpandsObstacleMacrosFromPlatformModel)
{
  PlatModel model(0, 0, 90, 2);
  model.setModelType("holo");

  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());
  behavior.setPlatModel(model);

  ASSERT_TRUE(behavior.setParam("poly",
                                "pts={20,-10:40,-10:40,10:20,10},label=radar_17"));
  ASSERT_TRUE(behavior.setParam("id", "radar_17"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "10"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "100"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "150"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "5"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "50"));
  ASSERT_TRUE(behavior.setParam("rng_flag",
                                "<25 OBSTACLE_NEAR = oid=$[OID],oidx=$[OIDX],rng=$[RNG]"));

  behavior.onSetParamComplete();
  behavior.clearMessages();
  behavior.onEveryState("running");

  VarDataPair flag;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "OBSTACLE_NEAR", flag));
  ASSERT_TRUE(flag.is_string());
  EXPECT_NE(flag.get_sdata().find("oid=radar_17"), std::string::npos);
  EXPECT_NE(flag.get_sdata().find("oidx=17"), std::string::npos);
  EXPECT_NE(flag.get_sdata().find("rng="), std::string::npos);
}

// Covers BHV avoid obstacle V24 behavior: breached obstacle posts warning when allstop disabled.
TEST(BHVAvoidObstacleV24Test, BreachedObstaclePostsWarningWhenAllstopDisabled)
{
  PlatModel model(0, 0, 90, 2);
  model.setModelType("holo");

  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());
  behavior.setPlatModel(model);

  ASSERT_TRUE(behavior.setParam("poly", "pts={-5,-5:5,-5:5,5:-5,5},label=breach"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "10"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "100"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "150"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "5"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "50"));
  ASSERT_TRUE(behavior.setParam("allstop_on_breach", "false"));

  behavior.onSetParamComplete();
  behavior.clearMessages();
  behavior.onEveryState("running");

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  bool saw_breach_warning = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "BHV_WARNING" && message.is_string())
      saw_breach_warning |=
        (message.get_sdata().find("Obstacle Breached") != std::string::npos);
  }
  EXPECT_TRUE(saw_breach_warning);
}

// Covers BHV avoid obstacle V24 behavior: idle and inactive erase gut mid and rim polygons.
TEST(BHVAvoidObstacleV24Test, IdleAndInactiveEraseGutMidAndRimPolygons)
{
  BHV_AvoidObstacleV24 behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("poly", "pts={20,-10:40,-10:40,10:20,10},label=obs"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "100"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "150"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "5"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "50"));

  behavior.onSetParamComplete();
  behavior.clearMessages();
  behavior.onInactiveState();

  int inactive_polygons = 0;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POLYGON" && message.is_string() &&
       message.get_sdata().find("active=false") != std::string::npos)
      inactive_polygons++;
  }
  EXPECT_EQ(inactive_polygons, 3);
}
