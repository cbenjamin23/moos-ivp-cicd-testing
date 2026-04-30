#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "BHV_Attractor.h"
#include "BHV_AvoidObstacle.h"
#include "BHV_AvoidObstacleV21.h"
#include "BHV_AvoidObstacleX.h"
#include "BHV_FixTurn.h"
#include "BHV_RubberBand.h"
#include "DepBehaviorTestUtils.h"
#include "IvPFunction.h"
#include "XYPoint.h"

class ExposedFixTurn : public BHV_FixTurn {
public:
  explicit ExposedFixTurn(IvPDomain domain) : BHV_FixTurn(domain) {}
  using BHV_FixTurn::getCurrFixTurn;
  using BHV_FixTurn::getCurrModHdg;
  using BHV_FixTurn::getState;
  using BHV_FixTurn::getCurrTurnDir;
  using BHV_FixTurn::getCurrTurnSpd;
  using BHV_FixTurn::buildOF;
  using BHV_FixTurn::clearTurnVisuals;
  using BHV_FixTurn::handleConfigTurnSpec;
  using BHV_FixTurn::postTurnCompleteReport;
  using BHV_FixTurn::setState;
  using BHV_FixTurn::updateOSPos;
  using BHV_FixTurn::updateOSHdg;
  using BHV_FixTurn::updateOSSpd;

  void forceState(const std::string& state) { m_state = state; }

  void seedMarkPoint(unsigned int ix, double x, double y)
  {
    if(m_mark_pts.size() != 360)
      m_mark_pts.resize(360);
    ASSERT_LT(ix, m_mark_pts.size());
    m_mark_pts[ix] = XYPoint(x, y);
  }

  void clearMarkPointsForReport() { m_mark_pts.clear(); }

  void seedRudder(double rudder) { m_mark_rudder.push_back(rudder); }
};

class ExposedAttractor : public BHV_Attractor {
public:
  explicit ExposedAttractor(IvPDomain domain) : BHV_Attractor(domain) {}
  using BHV_Attractor::getRelevance;
};

class ExposedAvoidObstacle : public BHV_AvoidObstacle {
public:
  explicit ExposedAvoidObstacle(IvPDomain domain) : BHV_AvoidObstacle(domain) {}
  using BHV_AvoidObstacle::getRelevance;

  void setPriorityGrade(const std::string& grade) { m_pwt_grade = grade; }

  void setPlatform(double x, double y, double heading)
  {
    m_osx = x;
    m_osy = y;
    m_osh = heading;
  }
};

class ExposedAvoidObstacleX : public BHV_AvoidObstacleX {
public:
  explicit ExposedAvoidObstacleX(IvPDomain domain) : BHV_AvoidObstacleX(domain) {}

  void setPriorityGrade(const std::string& grade) { m_pwt_grade = grade; }
};

class ExposedAvoidObstacleV21 : public BHV_AvoidObstacleV21 {
public:
  explicit ExposedAvoidObstacleV21(IvPDomain domain) : BHV_AvoidObstacleV21(domain) {}

  void setPriorityGrade(const std::string& grade) { m_pwt_grade = grade; }
};

// Covers dep behavior config behavior: fix turn accepts legacy turn schedule and runs against nav state.
TEST(DepBehaviorConfigTest, FixTurnAcceptsLegacyTurnScheduleAndRunsAgainstNavState)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 0, 2);
  info.setValue("DESIRED_RUDDER", 12);

  ExposedFixTurn behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);

  EXPECT_FALSE(behavior.isDeprecated().empty());
  ASSERT_TRUE(behavior.setParam("i_understand_this_behavior_is_deprecated", "true"));
  EXPECT_TRUE(behavior.isDeprecated().empty());
  ASSERT_TRUE(behavior.setParam("fix_turn", "45"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "25"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "starboard"));
  ASSERT_TRUE(behavior.setParam("speed", "auto"));
  ASSERT_TRUE(behavior.setParam("turn_spec", "clearall, spd=3, mhdg=30, fix=90, turn=port"));
  EXPECT_FALSE(behavior.setParam("turn_spec", "spd=fast,mhdg=30"));

  EXPECT_DOUBLE_EQ(behavior.getCurrTurnSpd(), 3);
  EXPECT_DOUBLE_EQ(behavior.getCurrModHdg(), 30);
  EXPECT_DOUBLE_EQ(behavior.getCurrFixTurn(), 90);
  EXPECT_EQ(behavior.getCurrTurnDir(), "port");

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(ipf->getPWT(), 0);
}

// Covers dep behavior config behavior: fix turn handles terse auto schedule clear and state reset hooks.
TEST(DepBehaviorConfigTest, FixTurnHandlesTerseAutoScheduleClearAndStateResetHooks)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 0, 2);
  info.setValue("DESIRED_RUDDER", 0);

  ExposedFixTurn behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "15"));
  ASSERT_TRUE(behavior.setParam("fix_turn", "30"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "port"));
  ASSERT_TRUE(behavior.setParam("turn_spec", "clearall,auto,auto,auto,starboard"));

  EXPECT_DOUBLE_EQ(behavior.getCurrTurnSpd(), 2);
  EXPECT_DOUBLE_EQ(behavior.getCurrModHdg(), 15);
  EXPECT_DOUBLE_EQ(behavior.getCurrFixTurn(), 30);
  EXPECT_EQ(behavior.getCurrTurnDir(), "star");

  ASSERT_TRUE(behavior.setParam("turn_spec", "clear"));
  EXPECT_EQ(behavior.getCurrTurnDir(), "star");

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_EQ(behavior.getState(), "turning");
  behavior.onRunToIdleState();
  EXPECT_EQ(behavior.getState(), "stem");
  behavior.onIdleToRunState();
  EXPECT_EQ(behavior.getState(), "stem");

  const std::string expanded = behavior.expandMacros("dist=$[TURN_DIST],time=$[TURN_TIME]");
  EXPECT_FALSE(depTextContains(expanded, "$[TURN_DIST]"));
  EXPECT_FALSE(depTextContains(expanded, "$[TURN_TIME]"));
}

// Covers dep behavior config behavior: fix turn completes after heading delta and posts turn report.
TEST(DepBehaviorConfigTest, FixTurnCompletesAfterHeadingDeltaAndPostsTurnReport)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 0, 2);
  info.setValue("DESIRED_RUDDER", 10);

  ExposedFixTurn behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("i_understand_this_behavior_is_deprecated", "true"));
  ASSERT_TRUE(behavior.setParam("fix_turn", "5"));
  ASSERT_TRUE(behavior.setParam("mod_hdg", "20"));
  ASSERT_TRUE(behavior.setParam("turn_dir", "star"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(1);
  seedDepOwnship(info, 1, 0, 10, 2);
  info.setValue("DESIRED_RUDDER", 8);

  std::unique_ptr<IvPFunction> completed_ipf(behavior.onRunState());
  EXPECT_EQ(completed_ipf, nullptr);
  EXPECT_EQ(behavior.isRunnable(), "completed");

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* report = findDepBehaviorMessage(messages, "FT_REPORT");
  ASSERT_NE(report, nullptr);
  EXPECT_TRUE(depTextContains(report->get_sdata(), "spd=1"));
  EXPECT_TRUE(depTextContains(report->get_sdata(), "mod_hdg=20"));
  EXPECT_NE(findDepBehaviorMessage(messages, "VIEW_TEXTBOX"), nullptr);
}

// Covers dep behavior config behavior: fix turn advances and repeats scheduled turns.
TEST(DepBehaviorConfigTest, FixTurnAdvancesAndRepeatsScheduledTurns)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 0, 2);
  info.setValue("DESIRED_RUDDER", 5);

  ExposedFixTurn behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("schedule", "repeat"));
  ASSERT_TRUE(behavior.setParam("perpetual", "true"));
  ASSERT_TRUE(behavior.setParam("turn_spec", "clearall,spd=2,mhdg=10,fix=5,turn=star"));
  ASSERT_TRUE(behavior.setParam("turn_spec", "spd=3,mhdg=20,fix=5,turn=port"));

  std::unique_ptr<IvPFunction> first_start(behavior.onRunState());
  ASSERT_NE(first_start, nullptr);
  EXPECT_DOUBLE_EQ(behavior.getCurrTurnSpd(), 2);
  EXPECT_DOUBLE_EQ(behavior.getCurrModHdg(), 10);
  EXPECT_EQ(behavior.getCurrTurnDir(), "star");

  info.setCurrTime(1);
  seedDepOwnship(info, 0, 0, 10, 2);
  info.setValue("DESIRED_RUDDER", 6);
  std::unique_ptr<IvPFunction> first_done(behavior.onRunState());
  EXPECT_EQ(first_done, nullptr);
  EXPECT_EQ(behavior.getState(), "stem");
  EXPECT_DOUBLE_EQ(behavior.getCurrTurnSpd(), 3);
  EXPECT_DOUBLE_EQ(behavior.getCurrModHdg(), 20);
  EXPECT_EQ(behavior.getCurrTurnDir(), "port");

  std::unique_ptr<IvPFunction> second_start(behavior.onRunState());
  ASSERT_NE(second_start, nullptr);

  info.setCurrTime(2);
  seedDepOwnship(info, 0, 0, 0, 2);
  info.setValue("DESIRED_RUDDER", 7);
  std::unique_ptr<IvPFunction> second_done(behavior.onRunState());
  EXPECT_EQ(second_done, nullptr);
  EXPECT_EQ(behavior.getState(), "stem");
  EXPECT_DOUBLE_EQ(behavior.getCurrTurnSpd(), 2);
  EXPECT_DOUBLE_EQ(behavior.getCurrModHdg(), 10);
  EXPECT_EQ(behavior.getCurrTurnDir(), "star");
}

// Covers dep behavior config behavior: fix turn rejects missing or stale navigation inputs.
TEST(DepBehaviorConfigTest, FixTurnRejectsMissingOrStaleNavigationInputs)
{
  InfoBuffer stale_info;
  stale_info.setCurrTime(0);
  stale_info.setValue("NAV_X", 0, 0);
  stale_info.setValue("NAV_Y", 0, 0);
  stale_info.setValue("NAV_HEADING", 0, 0);
  stale_info.setValue("NAV_SPEED", 1, 0);
  stale_info.setCurrTime(20);

  BHV_FixTurn stale_behavior(makeDepBehaviorDomain());
  stale_behavior.setInfoBuffer(&stale_info);
  ASSERT_TRUE(stale_behavior.setParam("stale_nav_thresh", "5"));
  std::unique_ptr<IvPFunction> stale_ipf(stale_behavior.onRunState());
  EXPECT_EQ(stale_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(stale_behavior.getMessages(), "BHV_ERROR"), nullptr);

  InfoBuffer missing_speed;
  missing_speed.setValue("NAV_X", 0);
  missing_speed.setValue("NAV_Y", 0);
  missing_speed.setValue("NAV_HEADING", 0);

  BHV_FixTurn missing_behavior(makeDepBehaviorDomain());
  missing_behavior.setInfoBuffer(&missing_speed);
  std::unique_ptr<IvPFunction> missing_ipf(missing_behavior.onRunState());
  EXPECT_EQ(missing_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_behavior.getMessages(), "BHV_ERROR"), nullptr);
}

// Covers dep behavior config behavior: fix turn pins legacy missing nav x acceptance.
TEST(DepBehaviorConfigTest, FixTurnPinsLegacyMissingNavXAcceptance)
{
  InfoBuffer info;
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_RUDDER", 1);

  ExposedFixTurn behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("fix_turn", "5"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_EQ(findDepBehaviorMessage(behavior.getMessages(), "BHV_ERROR"), nullptr);
}

// Covers dep behavior config behavior: fix turn rejects invalid config values and accepts schedule modes.
TEST(DepBehaviorConfigTest, FixTurnRejectsInvalidConfigValuesAndAcceptsScheduleModes)
{
  BHV_FixTurn behavior(makeDepBehaviorDomain());
  EXPECT_FALSE(behavior.setParam("speed", "-1"));
  EXPECT_FALSE(behavior.setParam("fix_turn", "-1"));
  EXPECT_FALSE(behavior.setParam("turn_dir", "clockwise"));
  EXPECT_FALSE(behavior.setParam("stale_nav_thresh", "0"));
  EXPECT_FALSE(behavior.setParam("radius_rep_var", "bad var"));
  EXPECT_TRUE(behavior.setParam("schedule", "repeat"));
  EXPECT_TRUE(behavior.setParam("schedule", "norepeat"));
  EXPECT_FALSE(behavior.setParam("schedule", "sometimes"));
}

// Covers dep behavior config behavior: fix turn covers direct schedule parsing and visual hint branches.
TEST(DepBehaviorConfigTest, FixTurnCoversDirectScheduleParsingAndVisualHintBranches)
{
  ExposedFixTurn behavior(makeDepBehaviorDomain());
  EXPECT_TRUE(behavior.setParam("visual_hints", "edge_color=gray70,vertex_size=4"));
  EXPECT_FALSE(behavior.setParam("visual_hints", "edge_color=not_a_color"));

  EXPECT_TRUE(behavior.handleConfigTurnSpec("clearall,3,25,90,starboard"));
  EXPECT_DOUBLE_EQ(behavior.getCurrTurnSpd(), 3);
  EXPECT_DOUBLE_EQ(behavior.getCurrModHdg(), 25);
  EXPECT_DOUBLE_EQ(behavior.getCurrFixTurn(), 90);
  EXPECT_EQ(behavior.getCurrTurnDir(), "star");

  EXPECT_TRUE(behavior.handleConfigTurnSpec("spd=auto,mhdg=auto,fix=auto,turn=starboard"));
  EXPECT_EQ(behavior.getCurrTurnDir(), "star");

  EXPECT_FALSE(behavior.handleConfigTurnSpec("bad,25,90,port"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("3,bad,90,port"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("3,25,bad,port"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("3,25,90,sideways"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("3,25,90,port,extra"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("spd=bad,mhdg=25"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("spd=3,mhdg=bad"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("spd=3,mhdg=25,fix=bad"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("spd=3,mhdg=25,fix=90,turn=sideways"));
  EXPECT_FALSE(behavior.handleConfigTurnSpec("bogus=1"));
}

// Covers dep behavior config behavior: fix turn warns for direct navigation checks and rejects bad state.
TEST(DepBehaviorConfigTest, FixTurnWarnsForDirectNavigationChecksAndRejectsBadState)
{
  InfoBuffer missing_info;
  ExposedFixTurn behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&missing_info);

  EXPECT_FALSE(behavior.updateOSPos("warn"));
  EXPECT_FALSE(behavior.updateOSHdg("warn"));
  EXPECT_FALSE(behavior.updateOSSpd("warn"));
  EXPECT_FALSE(behavior.updateOSHdg("err"));
  EXPECT_FALSE(behavior.setState("off"));

  const std::vector<VarDataPair> messages = behavior.getMessages();
  EXPECT_NE(findDepBehaviorMessage(messages, "BHV_WARNING"), nullptr);

  behavior.forceState("off");
  std::unique_ptr<IvPFunction> ipf(behavior.buildOF());
  EXPECT_EQ(ipf, nullptr);
}

// Covers dep behavior config behavior: fix turn tracks opposite heading deltas and clears radial visuals.
TEST(DepBehaviorConfigTest, FixTurnTracksOppositeHeadingDeltasAndClearsRadialVisuals)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 0, 2);
  info.setValue("DESIRED_RUDDER", 4);

  ExposedFixTurn star_behavior(makeDepBehaviorDomain());
  star_behavior.setInfoBuffer(&info);
  ASSERT_TRUE(star_behavior.setParam("fix_turn", "100"));
  ASSERT_TRUE(star_behavior.setParam("turn_dir", "star"));
  std::unique_ptr<IvPFunction> star_start(star_behavior.onRunState());
  ASSERT_NE(star_start, nullptr);

  info.setCurrTime(1);
  seedDepOwnship(info, 0.1, 0, 1.1, 2);
  std::unique_ptr<IvPFunction> star_step(star_behavior.onRunState());
  ASSERT_NE(star_step, nullptr);

  info.setCurrTime(2);
  seedDepOwnship(info, 0.2, 0, 1.2, 2);
  std::unique_ptr<IvPFunction> star_same_bucket(star_behavior.onRunState());
  ASSERT_NE(star_same_bucket, nullptr);

  InfoBuffer port_info;
  seedDepOwnship(port_info, 0, 0, 10, 2);
  port_info.setValue("DESIRED_RUDDER", 3);

  ExposedFixTurn port_behavior(makeDepBehaviorDomain());
  port_behavior.setInfoBuffer(&port_info);
  ASSERT_TRUE(port_behavior.setParam("fix_turn", "100"));
  ASSERT_TRUE(port_behavior.setParam("turn_dir", "port"));
  std::unique_ptr<IvPFunction> port_start(port_behavior.onRunState());
  ASSERT_NE(port_start, nullptr);

  port_info.setCurrTime(1);
  seedDepOwnship(port_info, 1, 0, 20, 2);
  std::unique_ptr<IvPFunction> port_wrong_way(port_behavior.onRunState());
  ASSERT_NE(port_wrong_way, nullptr);

  InfoBuffer star_wrong_way_info;
  seedDepOwnship(star_wrong_way_info, 0, 0, 20, 2);
  star_wrong_way_info.setValue("DESIRED_RUDDER", 3);

  ExposedFixTurn star_wrong_way_behavior(makeDepBehaviorDomain());
  star_wrong_way_behavior.setInfoBuffer(&star_wrong_way_info);
  ASSERT_TRUE(star_wrong_way_behavior.setParam("fix_turn", "100"));
  ASSERT_TRUE(star_wrong_way_behavior.setParam("turn_dir", "star"));
  std::unique_ptr<IvPFunction> star_wrong_way_start(star_wrong_way_behavior.onRunState());
  ASSERT_NE(star_wrong_way_start, nullptr);

  star_wrong_way_info.setCurrTime(1);
  seedDepOwnship(star_wrong_way_info, 1, 0, 10, 2);
  std::unique_ptr<IvPFunction> star_wrong_way(star_wrong_way_behavior.onRunState());
  ASSERT_NE(star_wrong_way, nullptr);

  ExposedFixTurn report_behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(report_behavior.setParam("speed", "2"));
  ASSERT_TRUE(report_behavior.setParam("mod_hdg", "15"));
  report_behavior.seedMarkPoint(0, 0, 0);
  report_behavior.seedMarkPoint(180, 10, 0);
  report_behavior.seedRudder(6);
  report_behavior.postTurnCompleteReport();
  EXPECT_NE(findDepBehaviorMessage(report_behavior.getMessages(), "FT_REPORT"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(report_behavior.getMessages(), "VIEW_SEGLIST"), nullptr);

  report_behavior.clearMessages();
  report_behavior.clearTurnVisuals();
  EXPECT_NE(findDepBehaviorMessage(report_behavior.getMessages(), "VIEW_SEGLIST"), nullptr);

  ExposedFixTurn empty_report(makeDepBehaviorDomain());
  empty_report.clearMarkPointsForReport();
  empty_report.postTurnCompleteReport();
  EXPECT_TRUE(empty_report.getMessages().empty());
}

// Covers dep behavior config behavior: rubber band posts station telemetry and pulls toward configured point.
TEST(DepBehaviorConfigTest, RubberBandPostsStationTelemetryAndPullsTowardConfiguredPoint)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 1);

  BHV_RubberBand behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("point", "30,0"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "4"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "20"));
  ASSERT_TRUE(behavior.setParam("outer_speed", "2"));
  ASSERT_TRUE(behavior.setParam("extra_speed", "3"));
  ASSERT_TRUE(behavior.setParam("stiffness", "0.2"));
  ASSERT_TRUE(behavior.setParam("center_activate", "true"));
  EXPECT_FALSE(behavior.setParam("outer_radius", "0"));
  EXPECT_FALSE(behavior.setParam("center_activate", "sometimes"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* distance = findDepBehaviorMessage(messages, "DIST_TO_STATION");
  ASSERT_NE(distance, nullptr);
  EXPECT_DOUBLE_EQ(distance->get_ddata(), 30);

  const VarDataPair* circle = findDepBehaviorMessage(messages, "STATION_CIRCLE");
  ASSERT_NE(circle, nullptr);
  EXPECT_TRUE(depTextContains(circle->get_sdata(), "30,0,20"));

  const VarDataPair* view = findDepBehaviorMessage(messages, "VIEW_POINT");
  ASSERT_NE(view, nullptr);
  EXPECT_TRUE(depTextContains(view->get_sdata(), "x=30"));
  EXPECT_TRUE(depTextContains(view->get_sdata(), "y=0"));
}

// Covers dep behavior config behavior: rubber band rejects malformed station and invalid numeric config.
TEST(DepBehaviorConfigTest, RubberBandRejectsMalformedStationAndInvalidNumericConfig)
{
  BHV_RubberBand behavior(makeDepBehaviorDomain());
  EXPECT_FALSE(behavior.setParam("point", "bad"));
  EXPECT_FALSE(behavior.setParam("point", "1,2,3"));
  EXPECT_TRUE(behavior.setParam("points", "1,2"));
  EXPECT_FALSE(behavior.setParam("inner_radius", "0"));
  EXPECT_FALSE(behavior.setParam("outer_radius", "-1"));
  EXPECT_FALSE(behavior.setParam("outer_speed", "0"));
  EXPECT_FALSE(behavior.setParam("extra_speed", "0"));
  EXPECT_FALSE(behavior.setParam("stiffness", "0"));
  EXPECT_FALSE(behavior.setParam("width", "45"));
  EXPECT_TRUE(behavior.setParam("point", "1,2"));
}

// Covers dep behavior config behavior: rubber band course utility prefers heading toward station.
TEST(DepBehaviorConfigTest, RubberBandCourseUtilityPrefersHeadingTowardStation)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 1);

  BHV_RubberBand behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("point", "30,0"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "4"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "20"));
  ASSERT_TRUE(behavior.setParam("outer_speed", "2"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  const IvPBox toward_station = makeCourseSpeedEvalBox(90, 2);
  const IvPBox away_from_station = makeCourseSpeedEvalBox(270, 2);
  EXPECT_GT(ipf->getPDMap()->evalPoint(&toward_station),
            ipf->getPDMap()->evalPoint(&away_from_station));
}

// Covers dep behavior config behavior: rubber band uses interpolated speed inside outer band.
TEST(DepBehaviorConfigTest, RubberBandUsesInterpolatedSpeedInsideOuterBand)
{
  InfoBuffer info;
  seedDepOwnship(info, 20, 0, 90, 1);

  BHV_RubberBand behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("point", "30,0"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "4"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "20"));
  ASSERT_TRUE(behavior.setParam("outer_speed", "4"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 100);
  EXPECT_NE(findDepBehaviorMessage(behavior.getMessages(), "DIST_TO_STATION"), nullptr);
}

// Covers dep behavior config behavior: rubber band reports missing nav and posts inactive trail on idle transition.
TEST(DepBehaviorConfigTest, RubberBandReportsMissingNavAndPostsInactiveTrailOnIdleTransition)
{
  InfoBuffer missing_nav;
  BHV_RubberBand missing_behavior(makeDepBehaviorDomain());
  missing_behavior.setInfoBuffer(&missing_nav);
  ASSERT_TRUE(missing_behavior.setParam("point", "30,0"));

  std::unique_ptr<IvPFunction> missing_ipf(missing_behavior.onRunState());
  EXPECT_EQ(missing_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_behavior.getMessages(), "BHV_ERROR"), nullptr);

  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 1);
  BHV_RubberBand behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("point", "30,0"));
  behavior.onSetParamComplete();
  behavior.onRunToIdleState();

  const VarDataPair* view = findDepBehaviorMessage(behavior.getMessages(), "VIEW_POINT");
  ASSERT_NE(view, nullptr);
}

// Covers dep behavior config behavior: rubber band handles present position activation and missing station.
TEST(DepBehaviorConfigTest, RubberBandHandlesPresentPositionActivationAndMissingStation)
{
  InfoBuffer info;
  seedDepOwnship(info, 12, -4, 90, 1);

  BHV_RubberBand behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("center_activate", "true"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "5"));

  behavior.onIdleState();
  std::unique_ptr<IvPFunction> inside_ipf(behavior.onRunState());
  EXPECT_EQ(inside_ipf, nullptr);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  bool found_active_center = false;
  for(const VarDataPair& message : messages) {
    if((message.get_var() == "STATION_CIRCLE") &&
       depTextContains(message.get_sdata(), "12,-4,15")) {
      found_active_center = true;
      break;
    }
  }
  EXPECT_TRUE(found_active_center);

  InfoBuffer no_station_info;
  seedDepOwnship(no_station_info, 0, 0, 90, 1);
  BHV_RubberBand no_station(makeDepBehaviorDomain());
  no_station.setInfoBuffer(&no_station_info);
  std::unique_ptr<IvPFunction> no_station_ipf(no_station.onRunState());
  EXPECT_EQ(no_station_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(no_station.getMessages(), "BHV_WARNING"), nullptr);
}

// Covers dep behavior config behavior: attractor builds objective for distant and near forward contacts.
TEST(DepBehaviorConfigTest, AttractorBuildsObjectiveForDistantAndNearForwardContacts)
{
  InfoBuffer distant_info;
  distant_info.setCurrTime(25);
  seedDepOwnship(distant_info, 0, 0, 90, 2);

  LedgerSnap distant_ledger;
  seedDepContact(distant_ledger, "alpha", 60, 0, 90, 2, 24);

  BHV_Attractor distant(makeDepBehaviorDomain());
  distant.setInfoBuffer(&distant_info);
  distant.setLedgerSnap(&distant_ledger);
  ASSERT_TRUE(distant.setParam("contact", "alpha"));
  ASSERT_TRUE(distant.setParam("dist_priority_interval", "20,80"));
  ASSERT_TRUE(distant.setParam("cpa_utility_interval", "5,50"));
  ASSERT_TRUE(distant.setParam("patience", "20"));
  std::unique_ptr<IvPFunction> distant_ipf(distant.onRunState());
  ASSERT_NE(distant_ipf, nullptr);
  EXPECT_GT(distant_ipf->getPWT(), 100);

  InfoBuffer near_info;
  near_info.setCurrTime(30);
  seedDepOwnship(near_info, 0, 0, 90, 2);

  LedgerSnap near_ledger;
  seedDepContact(near_ledger, "bravo", 10, 0, 90, 2, 29);

  BHV_Attractor near(makeDepBehaviorDomain());
  near.setInfoBuffer(&near_info);
  near.setLedgerSnap(&near_ledger);
  ASSERT_TRUE(near.setParam("contact", "bravo"));
  ASSERT_TRUE(near.setParam("dist_priority_interval", "20,80"));
  std::unique_ptr<IvPFunction> near_ipf(near.onRunState());
  ASSERT_NE(near_ipf, nullptr);
  EXPECT_LT(near_ipf->getPWT(), 100);
}

// Covers dep behavior config behavior: attractor idle state posts inactive trail point.
TEST(DepBehaviorConfigTest, AttractorIdleStatePostsInactiveTrailPoint)
{
  BHV_Attractor behavior(makeDepBehaviorDomain());
  behavior.onSetParamComplete();
  behavior.onIdleState();
  EXPECT_TRUE(behavior.getMessages().empty());
}

// Covers dep behavior config behavior: attractor uses contact ledger and range scaled relevance.
TEST(DepBehaviorConfigTest, AttractorUsesContactLedgerAndRangeScaledRelevance)
{
  InfoBuffer info;
  info.setCurrTime(50);
  seedDepOwnship(info, 0, 0, 270, 2);

  LedgerSnap ledger;
  seedDepContact(ledger, "alpha", 60, 0, 90, 2, 49);

  ExposedAttractor behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("dist_priority_interval", "20,80"));
  ASSERT_TRUE(behavior.setParam("cpa_utility_interval", "5,50"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "100"));
  ASSERT_TRUE(behavior.setParam("patience", "20"));
  ASSERT_TRUE(behavior.setParam("strength", "0.5"));
  EXPECT_FALSE(behavior.setParam("dist_priority_interval", "80,20"));
  EXPECT_FALSE(behavior.setParam("patience", "101"));

  EXPECT_DOUBLE_EQ(behavior.getRelevance(0, 0, 120, 0), 0);
  EXPECT_GT(behavior.getRelevance(0, 0, 60, 0), 1);

  behavior.clearMessages();
  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  EXPECT_NE(findDepBehaviorMessage(messages, "ATTRACTOR_CONTACT_TIME"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(messages, "ATTRACTOR_DELTA_TIME"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(messages, "ATTRACTOR_RANGE_TO_CONTACT"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(messages, "VIEW_POINT"), nullptr);
}

// Covers dep behavior config behavior: attractor reports configuration and contact data failures.
TEST(DepBehaviorConfigTest, AttractorReportsConfigurationAndContactDataFailures)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 0, 2);

  BHV_Attractor missing_contact(makeDepBehaviorDomain());
  missing_contact.setInfoBuffer(&info);
  std::unique_ptr<IvPFunction> missing_contact_ipf(missing_contact.onRunState());
  EXPECT_EQ(missing_contact_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_contact.getMessages(), "BHV_ERROR"), nullptr);

  LedgerSnap ledger;
  BHV_Attractor missing_ledger(makeDepBehaviorDomain());
  missing_ledger.setInfoBuffer(&info);
  missing_ledger.setLedgerSnap(&ledger);
  ASSERT_TRUE(missing_ledger.setParam("contact", "alpha"));
  std::unique_ptr<IvPFunction> missing_ledger_ipf(missing_ledger.onRunState());
  EXPECT_EQ(missing_ledger_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_ledger.getMessages(), "BHV_WARNING"), nullptr);
}

// Covers dep behavior config behavior: attractor rejects malformed intervals and out of range tuning.
TEST(DepBehaviorConfigTest, AttractorRejectsMalformedIntervalsAndOutOfRangeTuning)
{
  BHV_Attractor behavior(makeDepBehaviorDomain());
  EXPECT_FALSE(behavior.setParam("dist_priority_interval", "10"));
  EXPECT_FALSE(behavior.setParam("dist_priority_interval", "bad,20"));
  EXPECT_FALSE(behavior.setParam("dist_priority_interval", "-1,20"));
  EXPECT_FALSE(behavior.setParam("dist_priority_interval", "20,10"));
  EXPECT_TRUE(behavior.setParam("dist_priority_interval", "0,0"));

  EXPECT_FALSE(behavior.setParam("cpa_utility_interval", "10"));
  EXPECT_FALSE(behavior.setParam("cpa_utility_interval", "bad,20"));
  EXPECT_FALSE(behavior.setParam("cpa_utility_interval", "-1,20"));
  EXPECT_FALSE(behavior.setParam("cpa_utility_interval", "20,20"));
  EXPECT_TRUE(behavior.setParam("cpa_utility_interval", "5,20"));

  EXPECT_FALSE(behavior.setParam("giveup_range", "-1"));
  EXPECT_FALSE(behavior.setParam("giveup_range", "bad"));
  EXPECT_FALSE(behavior.setParam("patience", "-1"));
  EXPECT_FALSE(behavior.setParam("patience", "101"));
  EXPECT_FALSE(behavior.setParam("strength", "-1"));
  EXPECT_FALSE(behavior.setParam("strength", "101"));
  EXPECT_FALSE(behavior.setParam("unknown_param", "1"));
}

// Covers dep behavior config behavior: attractor relevance follows configured distance curve.
TEST(DepBehaviorConfigTest, AttractorRelevanceFollowsConfiguredDistanceCurve)
{
  ExposedAttractor behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(behavior.setParam("dist_priority_interval", "20,80"));
  ASSERT_TRUE(behavior.setParam("strength", "0.5"));

  EXPECT_DOUBLE_EQ(behavior.getRelevance(0, 0, 10, 0), 0.5);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(0, 0, 20, 0), 1.0);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(0, 0, 50, 0), 1.25);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(0, 0, 80, 0), 1.5);
}

// Covers dep behavior config behavior: attractor returns no objective beyond giveup range or with missing nav.
TEST(DepBehaviorConfigTest, AttractorReturnsNoObjectiveBeyondGiveupRangeOrWithMissingNav)
{
  InfoBuffer far_info;
  far_info.setCurrTime(10);
  seedDepOwnship(far_info, 0, 0, 0, 2);

  LedgerSnap ledger;
  seedDepContact(ledger, "alpha", 120, 0, 90, 2, 9);

  ExposedAttractor far_behavior(makeDepBehaviorDomain());
  far_behavior.setInfoBuffer(&far_info);
  far_behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(far_behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(far_behavior.setParam("dist_priority_interval", "20,80"));
  ASSERT_TRUE(far_behavior.setParam("giveup_range", "100"));
  EXPECT_DOUBLE_EQ(far_behavior.getRelevance(0, 0, 120, 0), 0);
  std::unique_ptr<IvPFunction> far_ipf(far_behavior.onRunState());
  EXPECT_EQ(far_ipf, nullptr);

  InfoBuffer missing_nav;
  missing_nav.setCurrTime(10);
  LedgerSnap near_ledger;
  seedDepContact(near_ledger, "alpha", 30, 0, 90, 2, 9);

  BHV_Attractor missing_nav_behavior(makeDepBehaviorDomain());
  missing_nav_behavior.setInfoBuffer(&missing_nav);
  missing_nav_behavior.setLedgerSnap(&near_ledger);
  ASSERT_TRUE(missing_nav_behavior.setParam("contact", "alpha"));
  std::unique_ptr<IvPFunction> missing_nav_ipf(missing_nav_behavior.onRunState());
  EXPECT_EQ(missing_nav_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_nav_behavior.getMessages(), "BHV_WARNING"), nullptr);
}

// Covers dep behavior config behavior: attractor run to idle posts inactive trail point.
TEST(DepBehaviorConfigTest, AttractorRunToIdlePostsInactiveTrailPoint)
{
  BHV_Attractor behavior(makeDepBehaviorDomain());
  behavior.onSetParamComplete();
  behavior.onRunToIdleState();

  const VarDataPair* view = findDepBehaviorMessage(behavior.getMessages(), "VIEW_POINT");
  ASSERT_NE(view, nullptr);
}

// Covers dep behavior config behavior: avoid obstacle runs classic buffered polygon behavior.
TEST(DepBehaviorConfigTest, AvoidObstacleRunsClassicBufferedPolygonBehavior)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  EXPECT_TRUE(behavior.isConstraint());
  EXPECT_FALSE(behavior.isDeprecated().empty());
  ASSERT_TRUE(behavior.setParam("i_understand_this_behavior_is_deprecated", "true"));
  EXPECT_TRUE(behavior.isDeprecated().empty());
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));
  ASSERT_TRUE(behavior.setParam("obstacle_key", "obstacle_alpha"));
  ASSERT_TRUE(behavior.setParam("visual_hints", "obstacle_edge_color=white,buffer_edge_color=gray60"));
  EXPECT_FALSE(behavior.setParam("polygon", "pts={0,0:10,0:0,10:10,10}"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(ipf->getPWT(), 0);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  EXPECT_NE(findDepBehaviorMessage(messages, "BHV_SETTINGS"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(messages, "OS_DIST_TO_POLY"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(messages, "VIEW_POLYGON"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle posts spawn alerts and completes when resolved.
TEST(DepBehaviorConfigTest, AvoidObstaclePostsSpawnAlertsAndCompletesWhenResolved)
{
  BHV_AvoidObstacle template_behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(template_behavior.setParam("updates", "OBSTACLE_UPDATES"));
  template_behavior.onHelmStart();
  const VarDataPair* alert =
    findDepBehaviorMessage(template_behavior.getMessages(), "OBM_ALERT_REQUEST");
  ASSERT_NE(alert, nullptr);

  BHV_AvoidObstacle silent_template(makeDepBehaviorDomain());
  ASSERT_TRUE(silent_template.setParam("updates", "OBSTACLE_UPDATES"));
  ASSERT_TRUE(silent_template.setParam("no_alert_request", "true"));
  silent_template.onHelmStart();
  EXPECT_EQ(findDepBehaviorMessage(silent_template.getMessages(), "OBM_ALERT_REQUEST"), nullptr);

  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacle resolved(makeDepBehaviorDomain());
  resolved.setInfoBuffer(&info);
  ASSERT_TRUE(resolved.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(resolved.setParam("buffer_dist", "8"));
  ASSERT_TRUE(resolved.setParam("completed_dist", "90"));
  ASSERT_TRUE(resolved.setParam("resolved", "true"));
  std::unique_ptr<IvPFunction> ipf(resolved.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_EQ(resolved.isRunnable(), "completed");

  resolved.onCompleteState();
  EXPECT_NE(findDepBehaviorMessage(resolved.getMessages(), "OBSTACLE_RESOLVED"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle returns no objective for aft far and unbuffered cases.
TEST(DepBehaviorConfigTest, AvoidObstacleReturnsNoObjectiveForAftFarAndUnbufferedCases)
{
  InfoBuffer aft_info;
  seedDepOwnship(aft_info, 0, 0, 270, 2);

  BHV_AvoidObstacle aft_behavior(makeDepBehaviorDomain());
  aft_behavior.setInfoBuffer(&aft_info);
  ASSERT_TRUE(aft_behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(aft_behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(aft_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(aft_behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(aft_behavior.setParam("completed_dist", "90"));
  std::unique_ptr<IvPFunction> aft_ipf(aft_behavior.onRunState());
  EXPECT_EQ(aft_ipf, nullptr);
  EXPECT_EQ(aft_behavior.isRunnable(), "running");
  EXPECT_NE(findDepBehaviorMessage(aft_behavior.getMessages(), "OS_DIST_TO_POLY"), nullptr);

  InfoBuffer far_info;
  seedDepOwnship(far_info, -100, 0, 90, 2);

  BHV_AvoidObstacle far_behavior(makeDepBehaviorDomain());
  far_behavior.setInfoBuffer(&far_info);
  ASSERT_TRUE(far_behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(far_behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(far_behavior.setParam("completed_dist", "90"));
  std::unique_ptr<IvPFunction> far_ipf(far_behavior.onRunState());
  EXPECT_EQ(far_ipf, nullptr);
  EXPECT_EQ(far_behavior.isRunnable(), "completed");

  InfoBuffer no_buffer_info;
  seedDepOwnship(no_buffer_info, 0, 0, 90, 2);

  BHV_AvoidObstacle no_buffer(makeDepBehaviorDomain());
  no_buffer.setInfoBuffer(&no_buffer_info);
  ASSERT_TRUE(no_buffer.setParam("polygon", depObstaclePolySpec()));
  std::unique_ptr<IvPFunction> no_buffer_ipf(no_buffer.onRunState());
  EXPECT_EQ(no_buffer_ipf, nullptr);
  EXPECT_EQ(no_buffer.isRunnable(), "running");
}

// Covers dep behavior config behavior: avoid obstacle rejects conflicting range configuration.
TEST(DepBehaviorConfigTest, AvoidObstacleRejectsConflictingRangeConfiguration)
{
  BHV_AvoidObstacle outer_auto_adjusts_completed(makeDepBehaviorDomain());
  EXPECT_TRUE(outer_auto_adjusts_completed.setParam("pwt_outer_dist", "80"));

  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "25"));
  EXPECT_FALSE(behavior.setParam("completed_dist", "20"));

  BHV_AvoidObstacle inner_first(makeDepBehaviorDomain());
  ASSERT_TRUE(inner_first.setParam("pwt_inner_dist", "40"));
  EXPECT_FALSE(inner_first.setParam("pwt_outer_dist", "30"));

  BHV_AvoidObstacle completed_first(makeDepBehaviorDomain());
  ASSERT_TRUE(completed_first.setParam("completed_dist", "20"));
  EXPECT_TRUE(completed_first.setParam("pwt_inner_dist", "25"));
}

// Covers dep behavior config behavior: avoid obstacle rejects invalid legacy parameter forms.
TEST(DepBehaviorConfigTest, AvoidObstacleRejectsInvalidLegacyParameterForms)
{
  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  EXPECT_TRUE(behavior.setParam("points", depObstaclePolySpec()));
  EXPECT_TRUE(behavior.setParam("poly", depObstaclePolySpec()));
  EXPECT_FALSE(behavior.setParam("allowable_ttc", "bad"));
  EXPECT_FALSE(behavior.setParam("allowable_ttc", "-1"));
  EXPECT_FALSE(behavior.setParam("buffer_dist", "-1"));
  EXPECT_FALSE(behavior.setParam("obstacle_key", "bad var"));
  EXPECT_FALSE(behavior.setParam("no_alert_request", "maybe"));
  EXPECT_FALSE(behavior.setParam("resolved", "maybe"));
  EXPECT_TRUE(behavior.setParam(
    "visual_hints", "obstacle_edge_color=white,buffer_fill_transparency=2"));
  EXPECT_FALSE(behavior.setParam("visual_hints", "obstacle_edge_color=not_a_color"));
  EXPECT_FALSE(behavior.setParam("visual_hints", "unknown_hint=white"));
}

// Covers dep behavior config behavior: avoid obstacle applies remaining visual hint colors.
TEST(DepBehaviorConfigTest, AvoidObstacleAppliesRemainingVisualHintColors)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));
  ASSERT_TRUE(behavior.setParam("build_info", "uniform_piece=discrete@course:3,speed:3"));
  ASSERT_TRUE(behavior.setParam("build_info", "uniform_grid=discrete@course:3,speed:3"));
  ASSERT_TRUE(behavior.setParam(
    "visual_hints",
    "obstacle_fill_color=orange,buffer_vertex_color=yellow,buffer_fill_color=cyan"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* obstacle = findDepBehaviorMessageByKey(messages, "VIEW_POLYGON", "orig");
  const VarDataPair* buffer = findDepBehaviorMessageByKey(messages, "VIEW_POLYGON", "buff");
  ASSERT_NE(obstacle, nullptr);
  ASSERT_NE(buffer, nullptr);
  EXPECT_TRUE(depTextContains(obstacle->get_sdata(), "fill_color=orange"));
  EXPECT_TRUE(depTextContains(buffer->get_sdata(), "vertex_color=yellow"));
  EXPECT_TRUE(depTextContains(buffer->get_sdata(), "fill_color=cyan"));
}

// Covers dep behavior config behavior: avoid obstacle applies multiple visual hints and pins vertex override.
TEST(DepBehaviorConfigTest, AvoidObstacleAppliesMultipleVisualHintsAndPinsVertexOverride)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));
  ASSERT_TRUE(behavior.setParam(
    "visual_hints",
    "obstacle_edge_color=red,obstacle_vertex_color=blue,"
    "obstacle_fill_transparency=2,buffer_edge_color=green,"
    "buffer_fill_transparency=-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* obstacle = findDepBehaviorMessageByKey(messages, "VIEW_POLYGON", "orig");
  const VarDataPair* buffer = findDepBehaviorMessageByKey(messages, "VIEW_POLYGON", "buff");
  ASSERT_NE(obstacle, nullptr);
  ASSERT_NE(buffer, nullptr);
  EXPECT_TRUE(depTextContains(obstacle->get_sdata(), "edge_color=red"));
  EXPECT_TRUE(depTextContains(obstacle->get_sdata(), "vertex_color=pink"));
  EXPECT_FALSE(depTextContains(obstacle->get_sdata(), "vertex_color=blue"));
  EXPECT_TRUE(depTextContains(obstacle->get_sdata(), "fill_transparency=1"));
  EXPECT_TRUE(depTextContains(buffer->get_sdata(), "edge_color=green"));
  EXPECT_TRUE(depTextContains(buffer->get_sdata(), "fill_transparency=0"));
}

// Covers dep behavior config behavior: avoid obstacle direct relevance covers range and aft boundaries.
TEST(DepBehaviorConfigTest, AvoidObstacleDirectRelevanceCoversRangeAndAftBoundaries)
{
  ExposedAvoidObstacle behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));

  behavior.setPlatform(0, 0, 90);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(), 0.5);

  behavior.setPlatform(30, 0, 90);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(), 1);

  behavior.setPlatform(-20, 0, 90);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(), 0);

  behavior.setPlatform(0, 0, 270);
  EXPECT_DOUBLE_EQ(behavior.getRelevance(), 0);
}

// Covers dep behavior config behavior: avoid obstacle priority grade curves transform mid range relevance.
TEST(DepBehaviorConfigTest, AvoidObstaclePriorityGradeCurvesTransformMidRangeRelevance)
{
  ExposedAvoidObstacle behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));
  behavior.setPlatform(0, 0, 90);

  EXPECT_DOUBLE_EQ(behavior.getRelevance(), 0.5);
  behavior.setPriorityGrade("quadratic");
  EXPECT_DOUBLE_EQ(behavior.getRelevance(), 0.25);
  behavior.setPriorityGrade("quasi");
  EXPECT_NEAR(behavior.getRelevance(), 0.353553, 0.000001);
}

// Covers dep behavior config behavior: avoid obstacle reports missing nav and ownship inside obstacle without objective.
TEST(DepBehaviorConfigTest, AvoidObstacleReportsMissingNavAndOwnshipInsideObstacleWithoutObjective)
{
  InfoBuffer missing_nav;
  BHV_AvoidObstacle missing_nav_behavior(makeDepBehaviorDomain());
  missing_nav_behavior.setInfoBuffer(&missing_nav);
  ASSERT_TRUE(missing_nav_behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(missing_nav_behavior.setParam("buffer_dist", "8"));
  std::unique_ptr<IvPFunction> missing_nav_ipf(missing_nav_behavior.onRunState());
  EXPECT_EQ(missing_nav_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_nav_behavior.getMessages(), "BHV_WARNING"), nullptr);

  InfoBuffer inside_info;
  seedDepOwnship(inside_info, 50, 0, 90, 2);
  BHV_AvoidObstacle inside_behavior(makeDepBehaviorDomain());
  inside_behavior.setInfoBuffer(&inside_info);
  ASSERT_TRUE(inside_behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(inside_behavior.setParam("buffer_dist", "8"));
  std::unique_ptr<IvPFunction> inside_ipf(inside_behavior.onRunState());
  EXPECT_EQ(inside_ipf, nullptr);
  EXPECT_EQ(inside_behavior.isRunnable(), "running");

  InfoBuffer missing_heading;
  missing_heading.setValue("NAV_X", 0);
  missing_heading.setValue("NAV_Y", 0);
  BHV_AvoidObstacle missing_heading_behavior(makeDepBehaviorDomain());
  missing_heading_behavior.setInfoBuffer(&missing_heading);
  ASSERT_TRUE(missing_heading_behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(missing_heading_behavior.setParam("buffer_dist", "8"));
  std::unique_ptr<IvPFunction> missing_heading_ipf(missing_heading_behavior.onRunState());
  EXPECT_EQ(missing_heading_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_heading_behavior.getMessages(), "BHV_WARNING"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle lifecycle posts spawn and inactive polygon visuals.
TEST(DepBehaviorConfigTest, AvoidObstacleLifecyclePostsSpawnAndInactivePolygonVisuals)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));

  behavior.onSpawn();
  EXPECT_NE(findDepBehaviorMessage(behavior.getMessages(), "AVD_OB_SPAWN"), nullptr);

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  behavior.clearMessages();
  behavior.onIdleState();
  const VarDataPair* idle_polygon = findDepBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON");
  ASSERT_NE(idle_polygon, nullptr);

  behavior.clearMessages();
  behavior.onInactiveState();
  const VarDataPair* inactive_polygon =
    findDepBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON");
  ASSERT_NE(inactive_polygon, nullptr);

  behavior.clearMessages();
  behavior.onIdleToRunState();
  EXPECT_NE(findDepBehaviorMessage(behavior.getMessages(), "BHV_SETTINGS"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle shrinks buffer when ownship is near obstacle.
TEST(DepBehaviorConfigTest, AvoidObstacleShrinksBufferWhenOwnshipIsNearObstacle)
{
  InfoBuffer info;
  seedDepOwnship(info, 39, 0, 90, 2);

  BHV_AvoidObstacle behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("polygon", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("buffer_dist", "8"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "60"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "90"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(behavior.getMessages(), "OS_DIST_TO_POLY"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle x and V21 use ObShipModel configuration and resolution hooks.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21UseObShipModelConfigurationAndResolutionHooks)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);
  info.setValue("OBM_RESOLVED", std::string("different_obstacle"));

  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  x_behavior.setInfoBuffer(&info);
  EXPECT_TRUE(x_behavior.isConstraint());
  EXPECT_FALSE(x_behavior.isDeprecated().empty());
  ASSERT_TRUE(x_behavior.setParam("i_understand_this_behavior_is_deprecated", "true"));
  EXPECT_TRUE(x_behavior.isDeprecated().empty());
  ASSERT_TRUE(x_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(x_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(x_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(x_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(x_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(x_behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(x_behavior.setParam("visual_hints", "buffer_max_vertex_size=2"));
  EXPECT_FALSE(x_behavior.setParam("max_util_cpa_dist", "-1"));

  x_behavior.onSetParamComplete();
  x_behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> x_ipf(x_behavior.onRunState());
  ASSERT_NE(x_ipf, nullptr);
  EXPECT_GT(x_ipf->getPWT(), 0);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("pwt_outer_dist"), 70);
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "OS_DIST_TO_POLY"), nullptr);

  InfoBuffer v21_info;
  seedDepOwnship(v21_info, 0, 0, 90, 2);

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  v21_behavior.setInfoBuffer(&v21_info);
  EXPECT_TRUE(v21_behavior.isConstraint());
  EXPECT_FALSE(v21_behavior.isDeprecated().empty());
  ASSERT_TRUE(v21_behavior.setParam("i_understand_this_behavior_is_deprecated", "true"));
  EXPECT_TRUE(v21_behavior.isDeprecated().empty());
  ASSERT_TRUE(v21_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(v21_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(v21_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(v21_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(v21_behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(v21_behavior.setParam("rng_flag", "<50 RNG_REPORT=range=$[RNG],side=$[SIDE]"));
  ASSERT_TRUE(v21_behavior.setParam("cpa_flag", "CPA_REPORT=$[CPA]"));
  ASSERT_TRUE(v21_behavior.setParam("visual_hints", "obstacle_vertex_size=2"));
  EXPECT_FALSE(v21_behavior.setParam("id", "other_obstacle"));
  EXPECT_FALSE(v21_behavior.setParam("rng_flag", "<far BAD=true"));

  v21_behavior.onSetParamComplete();
  v21_behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> v21_ipf(v21_behavior.onRunState());
  ASSERT_NE(v21_ipf, nullptr);
  EXPECT_GT(v21_ipf->getPWT(), 0);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("pwt_outer_dist"), 70);
  EXPECT_NE(findDepBehaviorMessage(v21_behavior.getMessages(), "RNG_REPORT"), nullptr);
  EXPECT_TRUE(depTextContains(v21_behavior.expandMacros("rng=$[RNG],side=$[SIDE],oid=$[OID]"),
                              "oid=obstacle_alpha"));
}

// Covers dep behavior config behavior: avoid obstacle V21 range flags respect configured thresholds.
TEST(DepBehaviorConfigTest, AvoidObstacleV21RangeFlagsRespectConfiguredThresholds)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacleV21 behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(behavior.setParam("rng_flag", "<30 NEAR_OBSTACLE=true"));
  ASSERT_TRUE(behavior.setParam("rng_flag", "<50 MID_OBSTACLE=true"));

  behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  const std::vector<VarDataPair> messages = behavior.getMessages();
  EXPECT_EQ(findDepBehaviorMessage(messages, "NEAR_OBSTACLE"), nullptr);
  EXPECT_NE(findDepBehaviorMessage(messages, "MID_OBSTACLE"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle x completes far contacts and ignores aft obstacles.
TEST(DepBehaviorConfigTest, AvoidObstacleXCompletesFarContactsAndIgnoresAftObstacles)
{
  InfoBuffer far_info;
  seedDepOwnship(far_info, -100, 0, 90, 2);

  BHV_AvoidObstacleX far_behavior(makeDepBehaviorDomain());
  far_behavior.setInfoBuffer(&far_info);
  ASSERT_TRUE(far_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(far_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(far_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(far_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(far_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(far_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(far_behavior.setParam("completed_dist", "90"));
  std::unique_ptr<IvPFunction> far_ipf(far_behavior.onRunState());
  EXPECT_EQ(far_ipf, nullptr);
  EXPECT_EQ(far_behavior.isRunnable(), "completed");

  InfoBuffer aft_info;
  seedDepOwnship(aft_info, 0, 0, 270, 2);

  BHV_AvoidObstacleX aft_behavior(makeDepBehaviorDomain());
  aft_behavior.setInfoBuffer(&aft_info);
  ASSERT_TRUE(aft_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(aft_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(aft_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(aft_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(aft_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(aft_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(aft_behavior.setParam("completed_dist", "120"));
  std::unique_ptr<IvPFunction> aft_ipf(aft_behavior.onRunState());
  EXPECT_EQ(aft_ipf, nullptr);
  EXPECT_EQ(aft_behavior.isRunnable(), "running");
  EXPECT_NE(findDepBehaviorMessage(aft_behavior.getMessages(), "OS_DIST_TO_POLY"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle x and V21 pin deprecated numeric parsing.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21PinDeprecatedNumericParsing)
{
  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  EXPECT_TRUE(x_behavior.setParam("allowable_ttc", "bad"));
  EXPECT_FALSE(x_behavior.setParam("min_util_cpa_dist", "bad"));
  EXPECT_FALSE(x_behavior.setParam("max_util_cpa_dist", "-1"));
  EXPECT_FALSE(x_behavior.setParam("visual_hints", "buffer_max_vertex_size=-1"));
  EXPECT_FALSE(x_behavior.setParam("visual_hints", "unknown_hint=white"));

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  EXPECT_TRUE(v21_behavior.setParam("allowable_ttc", "bad"));
  EXPECT_FALSE(v21_behavior.setParam("min_util_cpa_dist", "bad"));
  EXPECT_FALSE(v21_behavior.setParam("max_util_cpa_dist", "-1"));
  EXPECT_FALSE(v21_behavior.setParam("rng_flag", ""));
  EXPECT_FALSE(v21_behavior.setParam("rng_flag", "<far REPORT=true"));
  EXPECT_FALSE(v21_behavior.setParam("cpa_flag", "bad_flag"));
  EXPECT_FALSE(v21_behavior.setParam("visual_hints", "buffer_min_vertex_size=-1"));
  EXPECT_FALSE(v21_behavior.setParam("visual_hints", "unknown_hint=white"));
}

// Covers dep behavior config behavior: avoid obstacle x and V21 pin visual hint parsing quirks.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21PinVisualHintParsingQuirks)
{
  InfoBuffer x_info;
  seedDepOwnship(x_info, 0, 0, 90, 2);

  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  x_behavior.setInfoBuffer(&x_info);
  ASSERT_TRUE(x_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(x_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(x_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(x_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(x_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(x_behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(x_behavior.setParam("visual_hints", "buffer_min_fill_transparency=0.9"));

  std::unique_ptr<IvPFunction> x_ipf(x_behavior.onRunState());
  ASSERT_NE(x_ipf, nullptr);

  const std::vector<VarDataPair> x_messages = x_behavior.getMessages();
  const VarDataPair* x_min_buffer =
    findDepBehaviorMessageByKey(x_messages, "VIEW_POLYGON", "two");
  const VarDataPair* x_max_buffer =
    findDepBehaviorMessageByKey(x_messages, "VIEW_POLYGON", "three");
  ASSERT_NE(x_min_buffer, nullptr);
  ASSERT_NE(x_max_buffer, nullptr);
  EXPECT_TRUE(depTextContains(x_min_buffer->get_sdata(), "fill_transparency=0.25"));
  EXPECT_TRUE(depTextContains(x_max_buffer->get_sdata(), "fill_transparency=0.9"));

  InfoBuffer v21_info;
  seedDepOwnship(v21_info, 0, 0, 90, 2);

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  v21_behavior.setInfoBuffer(&v21_info);
  ASSERT_TRUE(v21_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(v21_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(v21_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(v21_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(v21_behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(v21_behavior.setParam(
    "visual_hints", "obstacle_edge_color=red,buffer_max_vertex_size=4"));

  v21_behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> v21_ipf(v21_behavior.onRunState());
  ASSERT_NE(v21_ipf, nullptr);

  const std::vector<VarDataPair> v21_messages = v21_behavior.getMessages();
  const VarDataPair* v21_obstacle =
    findDepBehaviorMessageByKey(v21_messages, "VIEW_POLYGON", "one");
  const VarDataPair* v21_max_buffer =
    findDepBehaviorMessageByKey(v21_messages, "VIEW_POLYGON", "three");
  ASSERT_NE(v21_obstacle, nullptr);
  ASSERT_NE(v21_max_buffer, nullptr);
  EXPECT_TRUE(depTextContains(v21_obstacle->get_sdata(), "edge_color=red"));
  EXPECT_TRUE(depTextContains(v21_max_buffer->get_sdata(), "vertex_size=1"));
  EXPECT_FALSE(depTextContains(v21_max_buffer->get_sdata(), "vertex_size=4"));
}

// Covers dep behavior config behavior: avoid obstacle x and V21 accept every single visual hint.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21AcceptEverySingleVisualHint)
{
  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "obstacle_edge_color=red"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "obstacle_vertex_color=blue"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "obstacle_vertex_size=2"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "obstacle_fill_color=green"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "obstacle_fill_transparency=0.4"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_min_edge_color=white"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_min_vertex_color=yellow"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_min_vertex_size=3"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_min_fill_color=gray50"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_max_edge_color=orange"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_max_vertex_color=purple"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_max_fill_color=cyan"));
  EXPECT_TRUE(x_behavior.setParam("visual_hints", "buffer_max_fill_transparency=0.6"));

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "obstacle_vertex_color=blue"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "obstacle_fill_color=green"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "obstacle_fill_transparency=0.4"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_min_edge_color=white"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_min_vertex_color=yellow"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_min_fill_color=gray50"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_min_fill_transparency=0.5"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_max_edge_color=orange"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_max_vertex_color=purple"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_max_vertex_size=4"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_max_fill_color=cyan"));
  EXPECT_TRUE(v21_behavior.setParam("visual_hints", "buffer_max_fill_transparency=0.6"));
}

// Covers dep behavior config behavior: avoid obstacle x and V21 double info reflects config and pose updates.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21DoubleInfoReflectsConfigAndPoseUpdates)
{
  InfoBuffer x_info;
  seedDepOwnship(x_info, 5, -2, 123, 3);

  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  x_behavior.setInfoBuffer(&x_info);
  ASSERT_TRUE(x_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(x_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(x_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(x_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(x_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(x_behavior.setParam("completed_dist", "120"));

  std::unique_ptr<IvPFunction> x_ipf(x_behavior.onRunState());
  ASSERT_NE(x_ipf, nullptr);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("osx"), 5);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("osy"), -2);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("osh"), 123);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("allowable_ttc"), 30);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("pwt_inner_dist"), 20);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("completed_dist"), 120);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("min_util_cpa"), 8);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("max_util_cpa"), 20);
  EXPECT_DOUBLE_EQ(x_behavior.getDoubleInfo("unknown"), 0);

  InfoBuffer v21_info;
  seedDepOwnship(v21_info, 7, 3, 87, 4);

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  v21_behavior.setInfoBuffer(&v21_info);
  ASSERT_TRUE(v21_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(v21_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(v21_behavior.setParam("min_util_cpa_dist", "9"));
  ASSERT_TRUE(v21_behavior.setParam("max_util_cpa_dist", "21"));
  ASSERT_TRUE(v21_behavior.setParam("allowable_ttc", "31"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_inner_dist", "19"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_outer_dist", "69"));
  ASSERT_TRUE(v21_behavior.setParam("completed_dist", "121"));
  v21_behavior.onEveryState("running");

  std::unique_ptr<IvPFunction> v21_ipf(v21_behavior.onRunState());
  ASSERT_NE(v21_ipf, nullptr);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("osx"), 7);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("osy"), 3);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("osh"), 87);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("allowable_ttc"), 31);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("pwt_outer_dist"), 69);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("completed_dist"), 121);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("min_util_cpa"), 9);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("max_util_cpa"), 21);
  EXPECT_DOUBLE_EQ(v21_behavior.getDoubleInfo("unknown"), 0);
}

// Covers dep behavior config behavior: avoid obstacle x and V21 refinery and priority grades run.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21RefineryAndPriorityGradesRun)
{
  InfoBuffer x_info;
  seedDepOwnship(x_info, 0, 0, 90, 2);

  ExposedAvoidObstacleX x_behavior(makeDepBehaviorDomain());
  x_behavior.setInfoBuffer(&x_info);
  x_behavior.setPriorityGrade("quadratic");
  ASSERT_TRUE(x_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(x_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(x_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(x_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(x_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(x_behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(x_behavior.setParam("use_refinery", "true"));
  x_behavior.onIdleToRunState();
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "BHV_SETTINGS"), nullptr);

  std::unique_ptr<IvPFunction> x_ipf(x_behavior.onRunState());
  ASSERT_NE(x_ipf, nullptr);
  EXPECT_NEAR(x_ipf->getPWT(), 36, 0.000001);

  InfoBuffer v21_info;
  seedDepOwnship(v21_info, 0, 0, 90, 2);

  ExposedAvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  v21_behavior.setInfoBuffer(&v21_info);
  v21_behavior.setPriorityGrade("quasi");
  ASSERT_TRUE(v21_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(v21_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(v21_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(v21_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(v21_behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(v21_behavior.setParam("use_refinery", "true"));
  v21_behavior.onIdleToRunState();
  EXPECT_NE(findDepBehaviorMessage(v21_behavior.getMessages(), "BHV_SETTINGS"), nullptr);

  v21_behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> v21_ipf(v21_behavior.onRunState());
  ASSERT_NE(v21_ipf, nullptr);
  EXPECT_NEAR(v21_ipf->getPWT(), 46.475800, 0.000001);
}

// Covers dep behavior config behavior: avoid obstacle x and V21 direct and range completion paths.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21DirectAndRangeCompletionPaths)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacleX directly_resolved(makeDepBehaviorDomain());
  directly_resolved.setInfoBuffer(&info);
  ASSERT_TRUE(directly_resolved.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(directly_resolved.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(directly_resolved.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(directly_resolved.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(directly_resolved.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(directly_resolved.setParam("resolved", "true"));
  EXPECT_FALSE(directly_resolved.setParam("resolved", "maybe"));
  EXPECT_FALSE(directly_resolved.setParam("use_refinery", "maybe"));
  std::unique_ptr<IvPFunction> resolved_ipf(directly_resolved.onRunState());
  EXPECT_EQ(resolved_ipf, nullptr);
  EXPECT_EQ(directly_resolved.isRunnable(), "completed");

  InfoBuffer far_info;
  seedDepOwnship(far_info, -100, 0, 90, 2);

  BHV_AvoidObstacleV21 far_v21(makeDepBehaviorDomain());
  far_v21.setInfoBuffer(&far_info);
  ASSERT_TRUE(far_v21.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(far_v21.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(far_v21.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(far_v21.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(far_v21.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(far_v21.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(far_v21.setParam("completed_dist", "90"));
  EXPECT_FALSE(far_v21.setParam("use_refinery", "maybe"));
  far_v21.onEveryState("running");
  std::unique_ptr<IvPFunction> far_ipf(far_v21.onRunState());
  EXPECT_EQ(far_ipf, nullptr);
  EXPECT_EQ(far_v21.isRunnable(), "completed");
}

// Covers dep behavior config behavior: avoid obstacle x and V21 reject invalid geometry and missing platform info.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21RejectInvalidGeometryAndMissingPlatformInfo)
{
  BHV_AvoidObstacleX invalid_x(makeDepBehaviorDomain());
  EXPECT_FALSE(invalid_x.setParam("poly", "pts={0,0:10,0:0,10:10,10}"));

  BHV_AvoidObstacleV21 invalid_v21(makeDepBehaviorDomain());
  EXPECT_FALSE(invalid_v21.setParam("poly", "pts={0,0:10,0:0,10,10:0,10}"));

  InfoBuffer missing_nav;
  BHV_AvoidObstacleX missing_x(makeDepBehaviorDomain());
  missing_x.setInfoBuffer(&missing_nav);
  ASSERT_TRUE(missing_x.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(missing_x.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(missing_x.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(missing_x.setParam("allowable_ttc", "30"));
  std::unique_ptr<IvPFunction> missing_x_ipf(missing_x.onRunState());
  EXPECT_EQ(missing_x_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_x.getMessages(), "BHV_WARNING"), nullptr);

  BHV_AvoidObstacleV21 missing_v21(makeDepBehaviorDomain());
  missing_v21.setInfoBuffer(&missing_nav);
  ASSERT_TRUE(missing_v21.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(missing_v21.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(missing_v21.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(missing_v21.setParam("allowable_ttc", "30"));
  missing_v21.onEveryState("running");
  std::unique_ptr<IvPFunction> missing_v21_ipf(missing_v21.onRunState());
  EXPECT_EQ(missing_v21_ipf, nullptr);
  EXPECT_NE(findDepBehaviorMessage(missing_v21.getMessages(), "BHV_WARNING"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle x and V21 lifecycle hooks post polygon visuals.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21LifecycleHooksPostPolygonVisuals)
{
  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(x_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(x_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(x_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("allowable_ttc", "30"));

  x_behavior.onSpawn();
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "AVD_OB_SPAWN"), nullptr);

  x_behavior.clearMessages();
  x_behavior.onIdleState();
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "VIEW_POLYGON"), nullptr);

  x_behavior.clearMessages();
  x_behavior.onCompleteState();
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "VIEW_POLYGON"), nullptr);

  x_behavior.clearMessages();
  x_behavior.onInactiveState();
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "VIEW_POLYGON"), nullptr);

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  ASSERT_TRUE(v21_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(v21_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(v21_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("allowable_ttc", "30"));

  v21_behavior.onIdleState();
  EXPECT_NE(findDepBehaviorMessage(v21_behavior.getMessages(), "VIEW_POLYGON"), nullptr);

  v21_behavior.clearMessages();
  v21_behavior.onCompleteState();
  EXPECT_NE(findDepBehaviorMessage(v21_behavior.getMessages(), "VIEW_POLYGON"), nullptr);

  v21_behavior.clearMessages();
  v21_behavior.onInactiveState();
  EXPECT_NE(findDepBehaviorMessage(v21_behavior.getMessages(), "VIEW_POLYGON"), nullptr);
}

// Covers dep behavior config behavior: avoid obstacle x and V21 spawnable templates post obstacle manager alerts.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21SpawnableTemplatesPostObstacleManagerAlerts)
{
  BHV_AvoidObstacleX x_template(makeDepBehaviorDomain());
  ASSERT_TRUE(x_template.setParam("updates", "OBX_UPDATES"));
  ASSERT_TRUE(x_template.setParam("pwt_outer_dist", "70"));
  x_template.setDynamicallySpawnable(true);
  x_template.onHelmStart();
  const VarDataPair* x_alert =
    findDepBehaviorMessage(x_template.getMessages(), "OBM_ALERT_REQUEST");
  ASSERT_NE(x_alert, nullptr);

  BHV_AvoidObstacleV21 v21_template(makeDepBehaviorDomain());
  ASSERT_TRUE(v21_template.setParam("updates", "OB21_UPDATES"));
  ASSERT_TRUE(v21_template.setParam("pwt_outer_dist", "80"));
  v21_template.setDynamicallySpawnable(true);
  v21_template.onHelmStart();
  const VarDataPair* v21_alert =
    findDepBehaviorMessage(v21_template.getMessages(), "OBM_ALERT_REQUEST");
  ASSERT_NE(v21_alert, nullptr);
}

// Covers dep behavior config behavior: avoid obstacle V21 posts CPA flags when closing range opens.
TEST(DepBehaviorConfigTest, AvoidObstacleV21PostsCPAFlagsWhenClosingRangeOpens)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacleV21 behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(behavior.setParam("cpa_flag", "CPA_REPORT=$[CPA]"));

  behavior.onEveryState("running");
  seedDepOwnship(info, 10, 0, 90, 2);
  behavior.onEveryState("running");
  seedDepOwnship(info, 20, 0, 90, 2);
  behavior.onEveryState("running");
  seedDepOwnship(info, 10, 0, 90, 2);
  behavior.onEveryState("running");

  const VarDataPair* cpa_report = findDepBehaviorMessage(behavior.getMessages(), "CPA_REPORT");
  ASSERT_NE(cpa_report, nullptr);
  EXPECT_FALSE(cpa_report->is_string());
  EXPECT_GE(cpa_report->get_ddata(), 0);
}

// Covers dep behavior config behavior: avoid obstacle V21 posts unthresholded range flags and expands macros.
TEST(DepBehaviorConfigTest, AvoidObstacleV21PostsUnthresholdedRangeFlagsAndExpandsMacros)
{
  InfoBuffer info;
  seedDepOwnship(info, 0, 0, 90, 2);

  BHV_AvoidObstacleV21 behavior(makeDepBehaviorDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "20"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "120"));
  ASSERT_TRUE(behavior.setParam("rng_flag", "RNG_ALWAYS=$[RNG]"));

  behavior.onEveryState("running");

  const VarDataPair* report = findDepBehaviorMessage(behavior.getMessages(), "RNG_ALWAYS");
  ASSERT_NE(report, nullptr);
  EXPECT_FALSE(report->is_string());

  const std::string expanded =
    behavior.expandMacros("range=$[RNG],bng=$[BNG],rbng=$[RBNG],spd=$[SPD],oid=$[OID]");
  EXPECT_FALSE(depTextContains(expanded, "$["));
  EXPECT_TRUE(depTextContains(expanded, "range=40"));
  EXPECT_TRUE(depTextContains(expanded, "oid=obstacle_alpha"));
  EXPECT_TRUE(depTextContains(expanded, "spd=0"));
}

// Covers dep behavior config behavior: avoid obstacle x and V21 honor obstacle manager resolved lists.
TEST(DepBehaviorConfigTest, AvoidObstacleXAndV21HonorObstacleManagerResolvedLists)
{
  InfoBuffer x_info;
  seedDepOwnship(x_info, 0, 0, 90, 2);
  x_info.setValue("OBM_RESOLVED", std::string("obstacle_alpha"));

  BHV_AvoidObstacleX x_behavior(makeDepBehaviorDomain());
  x_behavior.setInfoBuffer(&x_info);
  ASSERT_TRUE(x_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(x_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(x_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(x_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(x_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(x_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(x_behavior.setParam("completed_dist", "120"));
  x_behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> x_ipf(x_behavior.onRunState());
  EXPECT_EQ(x_ipf, nullptr);
  EXPECT_EQ(x_behavior.isRunnable(), "completed");
  EXPECT_NE(findDepBehaviorMessage(x_behavior.getMessages(), "NOTED_RESOLVED"), nullptr);

  InfoBuffer v21_info;
  seedDepOwnship(v21_info, 0, 0, 90, 2);
  v21_info.setValue("OBM_RESOLVED", std::string("obstacle_alpha"));

  BHV_AvoidObstacleV21 v21_behavior(makeDepBehaviorDomain());
  v21_behavior.setInfoBuffer(&v21_info);
  ASSERT_TRUE(v21_behavior.setParam("id", "obstacle_alpha"));
  ASSERT_TRUE(v21_behavior.setParam("poly", depObstaclePolySpec()));
  ASSERT_TRUE(v21_behavior.setParam("min_util_cpa_dist", "8"));
  ASSERT_TRUE(v21_behavior.setParam("max_util_cpa_dist", "20"));
  ASSERT_TRUE(v21_behavior.setParam("allowable_ttc", "30"));
  ASSERT_TRUE(v21_behavior.setParam("pwt_outer_dist", "70"));
  ASSERT_TRUE(v21_behavior.setParam("completed_dist", "120"));
  v21_behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> v21_ipf(v21_behavior.onRunState());
  EXPECT_EQ(v21_ipf, nullptr);
  EXPECT_EQ(v21_behavior.isRunnable(), "completed");
  EXPECT_NE(findDepBehaviorMessage(v21_behavior.getMessages(), "NOTED_RESOLVED"), nullptr);
}
