#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_Waypoint.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV waypoint behavior: tracks mission waypoints and posts waypoint telemetry.
TEST(BHVWaypointTest, TracksMissionWaypointsAndPostsWaypointTelemetry)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Waypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("points", "pts={0,100:100,100}"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("radius", "5"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "20"));
  ASSERT_TRUE(behavior.setParam("wpt_status_var", "WPT_STAT_TEST"));
  ASSERT_TRUE(behavior.setParam("wpt_index_var", "WPT_INDEX_TEST"));
  ASSERT_TRUE(behavior.setParam("wpt_dist_to_next", "WPT_DIST_NEXT"));
  EXPECT_FALSE(behavior.setParam("points", "bad"));
  EXPECT_FALSE(behavior.setParam("speed", "-1"));
  EXPECT_FALSE(behavior.setParam("radius", "0"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 90, 2));

  VarDataPair stat;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_STAT_TEST", stat));
  ASSERT_TRUE(stat.is_string());
  const std::string stat_spec = stat.get_sdata();
  EXPECT_NE(stat_spec.find("index=0"), std::string::npos);
  EXPECT_NE(stat_spec.find("dist=100"), std::string::npos);

  VarDataPair index;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_INDEX_TEST", index));
  EXPECT_FALSE(index.is_string());
  EXPECT_DOUBLE_EQ(index.get_ddata(), 0);

  VarDataPair dist_next;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_DIST_NEXT", dist_next));
  EXPECT_DOUBLE_EQ(dist_next.get_ddata(), 100);

  VarDataPair view_path;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", view_path));
  EXPECT_NE(view_path.get_sdata().find("pts={0,100:100,100}"), std::string::npos);
}

// Covers BHV waypoint behavior: advances waypoint posts flags and steers to next point.
TEST(BHVWaypointTest, AdvancesWaypointPostsFlagsAndSteersToNextPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 95);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Waypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("points", "pts={0,100:100,100}"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("radius", "10"));
  ASSERT_TRUE(behavior.setParam("wptflag", "WPT_HIT=true"));
  ASSERT_TRUE(behavior.setParam("wptflag_on_start", "true"));
  ASSERT_TRUE(behavior.setParam("eager_prev_index_flag", "true"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair hit;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_HIT", hit));
  EXPECT_EQ(hit.get_sdata(), "true");

  VarDataPair index;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_INDEX", index));
  EXPECT_DOUBLE_EQ(index.get_ddata(), 1);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 90, 2),
            evalCourseSpeedPoint(*ipf, 270, 2));
}

// Covers BHV waypoint behavior: run to idle erases viewer state and posts inactive distances.
TEST(BHVWaypointTest, RunToIdleErasesViewerStateAndPostsInactiveDistances)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Waypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("points", "pts={0,50:50,50}"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("wpt_dist_to_prev", "WPT_DIST_PREV"));
  ASSERT_TRUE(behavior.setParam("wpt_dist_to_next", "WPT_DIST_NEXT"));
  ASSERT_TRUE(behavior.setParam("reset_on_idle", "true"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  behavior.clearMessages();
  behavior.onRunToIdleState();

  VarDataPair dist_prev;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_DIST_PREV", dist_prev));
  EXPECT_DOUBLE_EQ(dist_prev.get_ddata(), -1);

  VarDataPair dist_next;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_DIST_NEXT", dist_next));
  EXPECT_DOUBLE_EQ(dist_next.get_ddata(), -1);

  bool erased_view_point = false;
  bool erased_view_segl = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      erased_view_point |=
        (message.get_sdata().find("active=false") != std::string::npos);
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string())
      erased_view_segl |=
        (message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(erased_view_point);
  EXPECT_TRUE(erased_view_segl);
}

// Covers BHV waypoint behavior: x points update preserves current waypoint index.
TEST(BHVWaypointTest, XPointsUpdatePreservesCurrentWaypointIndex)
{
  // Runtime xpoints updates replace the route geometry but should not rewind
  // progress to the first waypoint.
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 95);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Waypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("points", "pts={0,100:100,100}"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("radius", "10"));
  behavior.onSetParamComplete();

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  VarDataPair index;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPT_INDEX", index));
  EXPECT_DOUBLE_EQ(index.get_ddata(), 1);

  ASSERT_TRUE(behavior.setParam("xpoints", "pts={0,100:200,100}"));
  EXPECT_FALSE(behavior.setParam("xpoints", "pts={0,100:200,100:300,100}"));

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);

  VarDataPair next_point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", next_point));
  ASSERT_TRUE(next_point.is_string());
  EXPECT_NE(next_point.get_sdata().find("x=200"), std::string::npos);
  EXPECT_GT(evalCourseSpeedPoint(*second_ipf, 90, 2),
            evalCourseSpeedPoint(*second_ipf, 270, 2));
}

// Covers BHV waypoint behavior: point start is initialized from navigation on every state.
TEST(BHVWaypointTest, PointStartIsInitializedFromNavigationOnEveryState)
{
  InfoBuffer info;
  info.setValue("NAV_X", 12);
  info.setValue("NAV_Y", -8);
  info.setValue("NAV_HEADING", 45);
  info.setValue("NAV_SPEED", 1);

  BHV_Waypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "start"));
  ASSERT_TRUE(behavior.setParam("speed", "1"));
  behavior.onSetParamComplete();
  behavior.onEveryState("idle");

  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", -8);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair view_point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", view_point));
  ASSERT_TRUE(view_point.is_string());
  EXPECT_NE(view_point.get_sdata().find("x=12"), std::string::npos);
  EXPECT_NE(view_point.get_sdata().find("y=-8"), std::string::npos);
}
