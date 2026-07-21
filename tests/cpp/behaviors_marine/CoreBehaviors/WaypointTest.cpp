#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "BHV_Waypoint.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

namespace {

class TestableBHVWaypoint : public BHV_Waypoint {
 public:
  explicit TestableBHVWaypoint(const IvPDomain& domain) : BHV_Waypoint(domain) {}

  WaypointEngine& engine() { return m_waypoint_engine; }
  const WaypointEngine& engine() const { return m_waypoint_engine; }
  const XYPoint& trackPoint() const { return m_trackpt; }
  const XYPoint& nextPoint() const { return m_nextpt; }
  const std::string& ipfType() const { return m_ipf_type; }
};

IvPDomain makeFineCourseSpeedDomain()
{
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 3, 31);
  return domain;
}

void seedWaypointInfo(InfoBuffer& info, double x, double y,
                      double heading = 90, double speed = 2,
                      double time = 100)
{
  info.setCurrTime(time);
  info.setValue("NAV_X", x);
  info.setValue("NAV_Y", y);
  info.setValue("NAV_HEADING", heading);
  info.setValue("NAV_SPEED", speed);
}

double requireDoubleMessage(const std::vector<VarDataPair>& messages,
                            const std::string& variable)
{
  VarDataPair message;
  EXPECT_TRUE(findBehaviorMessage(messages, variable, message));
  EXPECT_FALSE(message.is_string());
  return message.get_ddata();
}

std::string requireStringMessage(const std::vector<VarDataPair>& messages,
                                 const std::string& variable)
{
  VarDataPair message;
  EXPECT_TRUE(findBehaviorMessage(messages, variable, message));
  EXPECT_TRUE(message.is_string());
  return message.get_sdata();
}

std::string requireStringMessageContaining(
  const std::vector<VarDataPair>& messages,
  const std::string& variable,
  const std::string& marker)
{
  for(const VarDataPair& message : messages) {
    if((message.get_var() == variable) && message.is_string() &&
       (message.get_sdata().find(marker) != std::string::npos))
      return message.get_sdata();
  }
  ADD_FAILURE() << "Missing " << variable << " containing " << marker;
  return "";
}

}  // namespace

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

// Distinguishes each capture mechanism with fixed geometry instead of
// inferring it from eventual simulator arrival.
TEST(BHVWaypointTest, CaptureModesAdvanceOnlyThroughTheirConfiguredMechanism)
{
  auto large_radius_completes = [](double radius) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 0);
    TestableBHVWaypoint behavior(makeFineCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("point", "30,0"));
    EXPECT_TRUE(behavior.setParam("speed", "2"));
    EXPECT_TRUE(behavior.setParam("capture_line", "false"));
    EXPECT_TRUE(behavior.setParam("capture_radius", std::to_string(radius)));
    EXPECT_TRUE(behavior.setParam("slip_radius", "12"));
    behavior.onSetParamComplete();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    return std::make_pair(ipf == nullptr, behavior.engine().getCaptureHits());
  };

  const auto radius_40 = large_radius_completes(40);
  EXPECT_TRUE(radius_40.first);
  EXPECT_EQ(radius_40.second, 1u);
  const auto radius_20 = large_radius_completes(20);
  EXPECT_FALSE(radius_20.first);
  EXPECT_EQ(radius_20.second, 0u);

  auto crosses_absolute_line = [](bool enabled) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 0);
    TestableBHVWaypoint behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("point", "10,0"));
    EXPECT_TRUE(behavior.setParam("speed", "2"));
    EXPECT_TRUE(behavior.setParam("capture_line", "absolute"));
    if(!enabled)
      EXPECT_TRUE(behavior.setParam("capture_line", "false"));
    behavior.onSetParamComplete();
    std::unique_ptr<IvPFunction> before(behavior.onRunState());
    EXPECT_NE(before, nullptr);
    seedWaypointInfo(info, 11, 0);
    std::unique_ptr<IvPFunction> after(behavior.onRunState());
    return after == nullptr;
  };

  EXPECT_TRUE(crosses_absolute_line(true));
  EXPECT_FALSE(crosses_absolute_line(false));

  InfoBuffer slip_info;
  seedWaypointInfo(slip_info, 4, 0);
  TestableBHVWaypoint slip(makeCourseSpeedDomain());
  slip.setInfoBuffer(&slip_info);
  ASSERT_TRUE(slip.setParam("point", "10,0"));
  ASSERT_TRUE(slip.setParam("speed", "2"));
  ASSERT_TRUE(slip.setParam("capture_line", "false"));
  ASSERT_TRUE(slip.setParam("capture_radius", "0.1"));
  ASSERT_TRUE(slip.setParam("slip_radius", "8"));
  slip.onSetParamComplete();
  EXPECT_NE(std::unique_ptr<IvPFunction>(slip.onRunState()), nullptr);
  seedWaypointInfo(slip_info, 8, 0);
  EXPECT_NE(std::unique_ptr<IvPFunction>(slip.onRunState()), nullptr);
  seedWaypointInfo(slip_info, 14, 0);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(slip.onRunState()), nullptr);
  EXPECT_EQ(slip.engine().getCaptureHits(), 0u);
  EXPECT_EQ(slip.engine().getNonmonoHits(), 1u);
}

// Requires the configured route transform itself rather than accepting any
// order that eventually visits all points.
TEST(BHVWaypointTest, RouteOrderCurrentIndexAndGreedyTourSelectExactFirstPoint)
{
  auto first_x = [](const std::string& parameter,
                    const std::string& value, bool run) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 0);
    TestableBHVWaypoint behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("points", "pts={30,0:10,0:20,0}"));
    EXPECT_TRUE(behavior.setParam(parameter, value));
    behavior.onSetParamComplete();
    if(run) {
      std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
      EXPECT_NE(ipf, nullptr);
    }
    return behavior.engine().getPointX();
  };

  EXPECT_DOUBLE_EQ(first_x("order", "reverse", false), 20);
  EXPECT_DOUBLE_EQ(first_x("currix", "1", false), 10);
  EXPECT_DOUBLE_EQ(first_x("greedy_tour", "true", true), 10);
}

// Pins the actual steering point used by lead-to-start and by a runtime lead
// condition, avoiding path-shape guesses from completed missions.
TEST(BHVWaypointTest, LeadControlsSelectExactTrackPoint)
{
  auto start_track = [](bool lead_to_start) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 10);
    TestableBHVWaypoint behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("point", "100,0"));
    EXPECT_TRUE(behavior.setParam("speed", "2"));
    EXPECT_TRUE(behavior.setParam("lead", "10"));
    EXPECT_TRUE(behavior.setParam("lead_damper", "1"));
    EXPECT_TRUE(behavior.setParam("lead_to_start",
                                  lead_to_start ? "true" : "false"));
    behavior.onSetParamComplete();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    return behavior.trackPoint();
  };

  const XYPoint led_start = start_track(true);
  EXPECT_NEAR(led_start.x(), 29.8511, 0.01);
  EXPECT_NEAR(led_start.y(), 7.01489, 0.01);
  const XYPoint direct_start = start_track(false);
  EXPECT_NEAR(direct_start.x(), 100, 0.01);
  EXPECT_NEAR(direct_start.y(), 0, 0.01);

  InfoBuffer info;
  seedWaypointInfo(info, 0, 0);
  info.setValue("ALLOW_LEAD", "false");
  TestableBHVWaypoint conditional(makeCourseSpeedDomain());
  conditional.setInfoBuffer(&info);
  ASSERT_TRUE(conditional.setParam("points", "pts={0,0:100,0}"));
  ASSERT_TRUE(conditional.setParam("speed", "2"));
  ASSERT_TRUE(conditional.setParam("radius", "5"));
  ASSERT_TRUE(conditional.setParam("lead", "10"));
  ASSERT_TRUE(conditional.setParam("lead_damper", "1"));
  ASSERT_TRUE(conditional.setParam("lead_condition", "ALLOW_LEAD=true"));
  conditional.onSetParamComplete();
  std::unique_ptr<IvPFunction> blocked(conditional.onRunState());
  ASSERT_NE(blocked, nullptr);
  EXPECT_NEAR(conditional.trackPoint().x(), 100, 0.01);

  seedWaypointInfo(info, 20, 10);
  info.setValue("ALLOW_LEAD", "true");
  std::unique_ptr<IvPFunction> allowed(conditional.onRunState());
  ASSERT_NE(allowed, nullptr);
  EXPECT_NEAR(conditional.trackPoint().x(), 30, 0.01);
  EXPECT_NEAR(conditional.trackPoint().y(), 0, 0.01);
}

// Requires the alternate-speed switch, terminal slowdown, stop distance, and
// both alternate objective builders to affect the generated objective.
TEST(BHVWaypointTest, SpeedAndObjectiveSettingsSelectExactDirectBranches)
{
  auto speed_ipf = [](bool use_alt) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 0);
    TestableBHVWaypoint behavior(makeFineCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("point", "100,0"));
    EXPECT_TRUE(behavior.setParam("speed", "0.35"));
    EXPECT_TRUE(behavior.setParam("speed_alt", "2.2"));
    EXPECT_TRUE(behavior.setParam("use_alt_speed",
                                  use_alt ? "true" : "false"));
    behavior.onSetParamComplete();
    return std::unique_ptr<IvPFunction>(behavior.onRunState());
  };

  const auto normal = speed_ipf(false);
  ASSERT_NE(normal, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*normal, 90, 4),
            evalCourseSpeedPoint(*normal, 90, 22));
  const auto alternate = speed_ipf(true);
  ASSERT_NE(alternate, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*alternate, 90, 22),
            evalCourseSpeedPoint(*alternate, 90, 4));

  auto slowed_ipf = [](double ownship_x) {
    InfoBuffer info;
    seedWaypointInfo(info, ownship_x, 0);
    TestableBHVWaypoint behavior(makeFineCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("point", "100,0"));
    EXPECT_TRUE(behavior.setParam("speed", "2"));
    EXPECT_TRUE(behavior.setParam("capture_line", "false"));
    EXPECT_TRUE(behavior.setParam("capture_radius", "0.1"));
    EXPECT_TRUE(behavior.setParam("slip_radius", "0"));
    EXPECT_TRUE(behavior.setParam("end_spd",
                                  "slow_dist=25,stop_dist=1"));
    behavior.onSetParamComplete();
    return std::unique_ptr<IvPFunction>(behavior.onRunState());
  };

  const auto taper = slowed_ipf(80);
  ASSERT_NE(taper, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*taper, 90, 16),
            evalCourseSpeedPoint(*taper, 90, 20));
  const auto stop = slowed_ipf(99.5);
  ASSERT_NE(stop, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*stop, 90, 0),
            evalCourseSpeedPoint(*stop, 90, 20));

  for(const std::string& method : {std::string("roc"),
                                   std::string("zaic_spd")}) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 0);
    TestableBHVWaypoint behavior(makeFineCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    ASSERT_TRUE(behavior.setParam("point", "100,0"));
    ASSERT_TRUE(behavior.setParam("speed", "2"));
    ASSERT_TRUE(behavior.setParam("ipf_type", method));
    behavior.onSetParamComplete();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    EXPECT_EQ(behavior.ipfType(), method);
  }
}

// Proves that reset_on_idle rewinds traversal state and that the default does
// not, rather than merely allowing both missions to finish later.
TEST(BHVWaypointTest, ResetOnIdleRewindsOnlyWhenEnabled)
{
  auto index_after_idle = [](bool reset) {
    InfoBuffer info;
    seedWaypointInfo(info, 0, 0);
    TestableBHVWaypoint behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    EXPECT_TRUE(behavior.setParam("points", "pts={0,0:50,0}"));
    EXPECT_TRUE(behavior.setParam("speed", "2"));
    EXPECT_TRUE(behavior.setParam("radius", "5"));
    EXPECT_TRUE(behavior.setParam("reset_on_idle",
                                  reset ? "true" : "false"));
    behavior.onSetParamComplete();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    EXPECT_EQ(behavior.engine().getCurrIndex(), 1);
    behavior.onRunToIdleState();
    return behavior.engine().getCurrIndex();
  };

  EXPECT_EQ(index_after_idle(true), 0);
  EXPECT_EQ(index_after_idle(false), 1);
}

// Requires every suffixed status output and its exact route-derived values,
// including the cycle index that is not published until a cycle completes.
TEST(BHVWaypointTest, CustomStatusVariablesCarryExactSuffixedValues)
{
  InfoBuffer info;
  seedWaypointInfo(info, 0, 0);
  TestableBHVWaypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("points", "pts={0,0:10,0}"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("radius", "1"));
  ASSERT_TRUE(behavior.setParam("repeat", "1"));
  ASSERT_TRUE(behavior.setParam("post_suffix", "CUSTOM"));
  ASSERT_TRUE(behavior.setParam("wpt_status", "WPT_STATUS"));
  ASSERT_TRUE(behavior.setParam("wpt_index", "WPT_INDEX"));
  ASSERT_TRUE(behavior.setParam("cycle_index_var", "CYCLE_INDEX"));
  ASSERT_TRUE(behavior.setParam("wpt_dist_to_prev", "CUSTOM_DIST_TO_PREV"));
  ASSERT_TRUE(behavior.setParam("wpt_dist_to_next", "CUSTOM_DIST_TO_NEXT"));
  behavior.onSetParamComplete();

  std::unique_ptr<IvPFunction> first(behavior.onRunState());
  ASSERT_NE(first, nullptr);
  EXPECT_DOUBLE_EQ(requireDoubleMessage(behavior.getMessages(),
                                       "WPT_INDEX_CUSTOM"), 1);
  EXPECT_DOUBLE_EQ(requireDoubleMessage(behavior.getMessages(),
                                       "CUSTOM_DIST_TO_NEXT"), 10);

  behavior.clearMessages();
  seedWaypointInfo(info, 10, 0);
  std::unique_ptr<IvPFunction> cycle(behavior.onRunState());
  ASSERT_NE(cycle, nullptr);
  EXPECT_DOUBLE_EQ(requireDoubleMessage(behavior.getMessages(),
                                       "CYCLE_INDEX_CUSTOM"), 1);
  EXPECT_DOUBLE_EQ(requireDoubleMessage(behavior.getMessages(),
                                       "WPT_INDEX_CUSTOM"), 0);
  EXPECT_DOUBLE_EQ(requireDoubleMessage(behavior.getMessages(),
                                       "CUSTOM_DIST_TO_PREV"), 10);
  EXPECT_DOUBLE_EQ(requireDoubleMessage(behavior.getMessages(),
                                       "CUSTOM_DIST_TO_NEXT"), 10);
  const std::string status =
    requireStringMessageContaining(behavior.getMessages(),
                                   "WPT_STATUS_CUSTOM", "index=");
  EXPECT_NE(status.find("index=0"), std::string::npos);
  EXPECT_NE(status.find("hits=2/2"), std::string::npos);
  EXPECT_NE(status.find("cycles=1"), std::string::npos);
  EXPECT_NE(status.find("dist=10"), std::string::npos);
  EXPECT_NE(status.find("eta=5"), std::string::npos);
}

// Drives one deterministic ten-meter leg in six seconds so both efficiency
// formulas can be checked against their exact inputs.
TEST(BHVWaypointTest, EfficiencyValuesMatchDeterministicDistanceAndTime)
{
  InfoBuffer info;
  seedWaypointInfo(info, 0, 0, 90, 2, 0);
  TestableBHVWaypoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("points", "pts={0,0:10,0}"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("capture_line", "false"));
  ASSERT_TRUE(behavior.setParam("capture_radius", "1"));
  ASSERT_TRUE(behavior.setParam("slip_radius", "0"));
  ASSERT_TRUE(behavior.setParam("efficiency_measure", "all"));
  behavior.onSetParamComplete();

  std::unique_ptr<IvPFunction> first(behavior.onRunState());
  ASSERT_NE(first, nullptr);
  behavior.clearMessages();
  seedWaypointInfo(info, 6, 0, 90, 2, 3);
  std::unique_ptr<IvPFunction> transit(behavior.onRunState());
  ASSERT_NE(transit, nullptr);
  behavior.clearMessages();
  seedWaypointInfo(info, 10, 0, 90, 2, 6);
  std::unique_ptr<IvPFunction> completed(behavior.onRunState());
  EXPECT_EQ(completed, nullptr);

  EXPECT_NEAR(requireDoubleMessage(behavior.getMessages(),
                                  "WPT_EFF_DIST_ALL"), 1.0, 1e-6);
  EXPECT_NEAR(requireDoubleMessage(behavior.getMessages(),
                                  "WPT_EFF_TIME_ALL"), 5.0 / 6.0, 1e-3);
  const std::string summary =
    requireStringMessage(behavior.getMessages(), "WPT_EFF_SUMM_ALL");
  EXPECT_NE(summary.find("linear_dist=10"), std::string::npos);
  EXPECT_NE(summary.find("odo_dist=10"), std::string::npos);
  EXPECT_NE(summary.find("linear_time=5"), std::string::npos);
  EXPECT_NE(summary.find("odo_time=6"), std::string::npos);
}

// Keeps parameter identity in the direct layer while the live harness proves
// that each rejected startup fixture reaches the helm's MALCONFIG state.
TEST(BHVWaypointTest, RejectsEveryMalformedHarnessParameterAndRouteUpdate)
{
  TestableBHVWaypoint behavior(makeCourseSpeedDomain());
  ASSERT_TRUE(behavior.setParam("points", "pts={10,0:20,0}"));

  EXPECT_FALSE(behavior.setParam("points", "not_a_point"));
  EXPECT_FALSE(behavior.setParam("xpoints", "pts={30,0}"));
  EXPECT_FALSE(behavior.setParam("speed", "-1"));
  EXPECT_FALSE(behavior.setParam("capture_line", "diagonal"));
  EXPECT_FALSE(behavior.setParam("capture_radius", "-1"));
  EXPECT_FALSE(behavior.setParam("slip_radius", "-1"));
  EXPECT_FALSE(behavior.setParam("order", "sideways"));
  EXPECT_FALSE(behavior.setParam("repeat", "-1"));
  EXPECT_FALSE(behavior.setParam("lead_damper", "0"));
  EXPECT_FALSE(behavior.setParam("lead_condition", "ALLOW_LEAD >"));
  EXPECT_FALSE(behavior.setParam("end_spd",
                                 "slow_dist=10,stop_dist=20"));
  EXPECT_FALSE(behavior.setParam("patience", "0"));
  EXPECT_FALSE(behavior.setParam("efficiency_measure", "summary"));

  EXPECT_EQ(behavior.engine().size(), 2u);
  EXPECT_DOUBLE_EQ(behavior.engine().getPointX(0), 10);
  EXPECT_DOUBLE_EQ(behavior.engine().getPointX(1), 20);
}
