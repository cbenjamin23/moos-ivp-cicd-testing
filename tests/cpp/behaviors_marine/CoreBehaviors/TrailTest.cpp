#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "BHV_Trail.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"
#include "XYFormatUtilsPoint.h"

class TestableBHVTrail : public BHV_Trail {
public:
  explicit TestableBHVTrail(IvPDomain domain) : BHV_Trail(domain) {}
  using IvPBehavior::setBehaviorName;
};

namespace {

XYPoint findViewPoint(const std::vector<VarDataPair>& messages)
{
  VarDataPair point;
  if(!findBehaviorMessage(messages, "VIEW_POINT", point) || !point.is_string())
    return XYPoint();
  return string2Point(point.get_sdata());
}

std::string findStringMessage(const std::vector<VarDataPair>& messages,
                              const std::string& var)
{
  VarDataPair message;
  if(!findBehaviorMessage(messages, var, message) || !message.is_string())
    return "";
  return message.get_sdata();
}

double findDoubleMessage(const std::vector<VarDataPair>& messages,
                         const std::string& var)
{
  VarDataPair message;
  if(!findBehaviorMessage(messages, var, message) || message.is_string())
    return -9999;
  return message.get_ddata();
}

}  // namespace

// Covers BHV trail behavior: computes trail point and pursuit state for ledger contact.
TEST(BHVTrailTest, ComputesTrailPointAndPursuitStateForLedgerContact)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 40, 0, 2, 0, 100, 0, 2);

  BHV_Trail behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("trail_range", "50"));
  ASSERT_TRUE(behavior.setParam("trail_angle", "180"));
  ASSERT_TRUE(behavior.setParam("trail_angle_type", "relative"));
  ASSERT_TRUE(behavior.setParam("radius", "5"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "20"));
  EXPECT_FALSE(behavior.setParam("trail_angle_type", "sideways"));
  EXPECT_FALSE(behavior.setParam("mod_trail_range_pct", "0"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair pursuit;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PURSUIT", pursuit));
  EXPECT_DOUBLE_EQ(pursuit.get_ddata(), 1);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("trailpoint"), std::string::npos);
}

// Covers BHV trail behavior: outer distance suppresses pursuit and idle can post trail distance.
TEST(BHVTrailTest, OuterDistanceSuppressesPursuitAndIdleCanPostTrailDistance)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 0, 2);

  BHV_Trail behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("trail_range", "50"));
  ASSERT_TRUE(behavior.setParam("trail_angle", "180"));
  ASSERT_TRUE(behavior.setParam("trail_angle_type", "absolute"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "50"));
  ASSERT_TRUE(behavior.setParam("post_trail_dist_on_idle", "true"));
  ASSERT_TRUE(behavior.setParam("mod_trail_range", "-100"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair relevance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TRAIL_RELEVANCE", relevance));
  EXPECT_DOUBLE_EQ(relevance.get_ddata(), 0);

  VarDataPair pursuit;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PURSUIT", pursuit));
  EXPECT_DOUBLE_EQ(pursuit.get_ddata(), 0);

  behavior.clearMessages();
  behavior.onIdleState();

  VarDataPair distance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TRAIL_DISTANCE", distance));
  EXPECT_FALSE(distance.is_string());

  behavior.clearMessages();
  behavior.onRunToIdleState();
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "PURSUIT", pursuit));
  EXPECT_DOUBLE_EQ(pursuit.get_ddata(), 0);
}

// Covers BHV trail behavior: ownship at trail point uses inside radius region.
TEST(BHVTrailTest, OwnshipAtTrailPointUsesInsideRadiusRegion)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 50, 0, 2, 0, 100, 0, 2);

  BHV_Trail behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("trail_range", "50"));
  ASSERT_TRUE(behavior.setParam("trail_angle", "180"));
  ASSERT_TRUE(behavior.setParam("trail_angle_type", "relative"));
  ASSERT_TRUE(behavior.setParam("radius", "5"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "20"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair region;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "REGION", region));
  EXPECT_EQ(region.get_sdata(), "Inside radius");

  VarDataPair distance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "TRAIL_DISTANCE", distance));
  EXPECT_DOUBLE_EQ(distance.get_ddata(), 0);
}

// Covers BHV trail behavior: spawnable template can request contact manager alerts.
TEST(BHVTrailTest, SpawnableTemplateCanRequestContactManagerAlerts)
{
  BHV_Trail behavior(makeCourseSpeedDomain());

  behavior.setDynamicallySpawnable(true);
  ASSERT_TRUE(behavior.setParam("updates", "TRAIL_UPDATES"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "120"));
  ASSERT_TRUE(behavior.setParam("no_alert_request", "false"));

  behavior.onHelmStart();

  VarDataPair request;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BCM_ALERT_REQUEST", request));
  EXPECT_NE(request.get_sdata().find("id="), std::string::npos);
  EXPECT_NE(request.get_sdata().find("on_flag=TRAIL_UPDATES=name=$[VNAME] # contact=$[VNAME]"),
            std::string::npos);
  EXPECT_NE(request.get_sdata().find("alert_range=120"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("cpa_range=125"), std::string::npos);
}

// Covers the exact world-frame trail point for absolute and contact-relative
// angle configurations, including a repeated angle update on one behavior.
TEST(BHVTrailTest, AngleParametersPlaceExactTrailPoint)
{
  auto point_for = [](const std::string& angle,
                      const std::string& angle_type) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                           100, 200, 90, 2);

    BHV_Trail behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    EXPECT_TRUE(behavior.setParam("trail_range", "50"));
    EXPECT_TRUE(behavior.setParam("trail_angle", angle));
    EXPECT_TRUE(behavior.setParam("trail_angle_type", angle_type));

    behavior.onEveryState("running");
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    return findViewPoint(behavior.getMessages());
  };

  const XYPoint absolute_west = point_for("270", "absolute");
  ASSERT_TRUE(absolute_west.valid());
  EXPECT_NEAR(absolute_west.get_vx(), 50, 0.1);
  EXPECT_NEAR(absolute_west.get_vy(), 200, 0.1);

  const XYPoint relative_port = point_for("135", "relative");
  ASSERT_TRUE(relative_port.valid());
  EXPECT_NEAR(relative_port.get_vx(), 64.64, 0.1);
  EXPECT_NEAR(relative_port.get_vy(), 164.64, 0.1);

  const XYPoint relative_starboard = point_for("-135", "relative");
  ASSERT_TRUE(relative_starboard.valid());
  EXPECT_NEAR(relative_starboard.get_vx(), 64.64, 0.1);
  EXPECT_NEAR(relative_starboard.get_vy(), 235.36, 0.1);

  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                         100, 200, 90, 2);
  BHV_Trail updated(makeCourseSpeedDomain());
  updated.setInfoBuffer(&info);
  updated.setLedgerSnap(&ledger);
  ASSERT_TRUE(updated.setParam("contact", "alpha"));
  ASSERT_TRUE(updated.setParam("trail_range", "50"));
  ASSERT_TRUE(updated.setParam("trail_angle_type", "relative"));
  ASSERT_TRUE(updated.setParam("trail_angle", "180"));
  updated.onEveryState("running");
  updated.clearMessages();
  std::unique_ptr<IvPFunction> before(updated.onRunState());
  ASSERT_NE(before, nullptr);
  const XYPoint astern = findViewPoint(updated.getMessages());
  ASSERT_TRUE(astern.valid());
  EXPECT_NEAR(astern.get_vx(), 50, 0.1);
  EXPECT_NEAR(astern.get_vy(), 200, 0.1);

  updated.clearMessages();
  ASSERT_TRUE(updated.setParam("trail_angle", "135"));
  std::unique_ptr<IvPFunction> after(updated.onRunState());
  ASSERT_NE(after, nullptr);
  const XYPoint port_after_update = findViewPoint(updated.getMessages());
  ASSERT_TRUE(port_after_update.valid());
  EXPECT_NEAR(port_after_update.get_vx(), 64.64, 0.1);
  EXPECT_NEAR(port_after_update.get_vy(), 164.64, 0.1);
}

// Covers direct, additive, percentage, and runtime-style desired-range
// changes by comparing the resulting trail-point distance, including the
// ten-meter minimum clamp.
TEST(BHVTrailTest, RangeParametersSetExactEffectiveTrailDistance)
{
  auto point_for = [](const std::vector<std::pair<std::string, std::string>>& params) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                           100, 200, 90, 2);

    BHV_Trail behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    EXPECT_TRUE(behavior.setParam("trail_angle", "180"));
    EXPECT_TRUE(behavior.setParam("trail_angle_type", "relative"));
    for(const auto& param : params)
      EXPECT_TRUE(behavior.setParam(param.first, param.second));

    behavior.onEveryState("running");
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    return findViewPoint(behavior.getMessages());
  };

  const std::vector<std::pair<
    std::vector<std::pair<std::string, std::string>>, double>> cases = {
    {{{"trail_range", "28"}}, 72},
    {{{"trail_range", "70"}}, 30},
    {{{"trail_range", "45"}, {"mod_trail_range", "15"}}, 40},
    {{{"trail_range", "50"}, {"mod_trail_range_pct", "50"}}, 75},
    {{{"trail_range", "12"}, {"mod_trail_range", "-50"}}, 90},
    {{{"trail_range", "50"}, {"mod_trail_range_pct", "10"}}, 90},
  };
  for(const auto& test_case : cases) {
    const XYPoint point = point_for(test_case.first);
    ASSERT_TRUE(point.valid());
    EXPECT_NEAR(point.get_vx(), test_case.second, 0.1);
    EXPECT_NEAR(point.get_vy(), 200, 0.1);
  }

  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                         100, 200, 90, 2);
  BHV_Trail updated(makeCourseSpeedDomain());
  updated.setInfoBuffer(&info);
  updated.setLedgerSnap(&ledger);
  ASSERT_TRUE(updated.setParam("contact", "alpha"));
  ASSERT_TRUE(updated.setParam("trail_angle", "180"));
  ASSERT_TRUE(updated.setParam("trail_angle_type", "relative"));
  ASSERT_TRUE(updated.setParam("trail_range", "45"));
  updated.onEveryState("running");
  updated.clearMessages();
  std::unique_ptr<IvPFunction> before(updated.onRunState());
  ASSERT_NE(before, nullptr);
  EXPECT_NEAR(findViewPoint(updated.getMessages()).get_vx(), 55, 0.1);

  updated.clearMessages();
  ASSERT_TRUE(updated.setParam("trail_range", "70"));
  std::unique_ptr<IvPFunction> after(updated.onRunState());
  ASSERT_NE(after, nullptr);
  EXPECT_NEAR(findViewPoint(updated.getMessages()).get_vx(), 30, 0.1);
}

// Covers the exact closed/open boundaries for the inner radius and near-mode
// radius using fixed ownship/contact geometry.
TEST(BHVTrailTest, RadiusBoundariesSelectExactPursuitRegion)
{
  auto region_at_y = [](double ownship_y) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, ownship_y, 0, 1,
                           0, 100, 0, 2);

    BHV_Trail behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    EXPECT_TRUE(behavior.setParam("trail_range", "50"));
    EXPECT_TRUE(behavior.setParam("trail_angle", "180"));
    EXPECT_TRUE(behavior.setParam("trail_angle_type", "relative"));
    EXPECT_TRUE(behavior.setParam("radius", "5"));
    EXPECT_TRUE(behavior.setParam("nm_radius", "20"));

    behavior.onEveryState("running");
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    return findStringMessage(behavior.getMessages(), "REGION");
  };

  EXPECT_EQ(region_at_y(55), "Inside radius");
  EXPECT_EQ(region_at_y(56), "Inside nm_radius");
  EXPECT_EQ(region_at_y(70), "Inside nm_radius");
  EXPECT_EQ(region_at_y(71), "Outside nm_radius");
}

// Covers the strict outer-distance relevance comparison without simulator
// motion or an unrestricted control configuration.
TEST(BHVTrailTest, OuterDistanceIsRelevantOnlyInsideStrictBoundary)
{
  auto run_at_range = [](double range) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                           range, 0, 90, 2);

    BHV_Trail behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    EXPECT_TRUE(behavior.setParam("pwt_outer_dist", "120"));
    behavior.onEveryState("running");
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    return std::make_pair(std::move(ipf),
                          findDoubleMessage(behavior.getMessages(),
                                            "TRAIL_RELEVANCE"));
  };

  auto inside = run_at_range(119);
  ASSERT_NE(inside.first, nullptr);
  EXPECT_DOUBLE_EQ(inside.second, 1);

  auto boundary = run_at_range(120);
  EXPECT_EQ(boundary.first, nullptr);
  EXPECT_DOUBLE_EQ(boundary.second, 0);
}

// Covers Trail's own extrapolate flag branch: with ownship already at the
// trail point, extrapolation enabled matches contact speed and disabled
// explicitly commands zero speed.
TEST(BHVTrailTest, ExtrapolateFlagSelectsInsideRadiusSpeed)
{
  auto trail_speed = [](bool extrapolate) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, 50, 0, 1,
                           0, 100, 0, 2);

    BHV_Trail behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    EXPECT_TRUE(behavior.setParam("trail_range", "50"));
    EXPECT_TRUE(behavior.setParam("trail_angle", "180"));
    EXPECT_TRUE(behavior.setParam("trail_angle_type", "relative"));
    EXPECT_TRUE(behavior.setParam("radius", "5"));
    EXPECT_TRUE(behavior.setParam("extrapolate",
                                  extrapolate ? "true" : "false"));
    behavior.onEveryState("running");
    behavior.clearMessages();
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_NE(ipf, nullptr);
    return findDoubleMessage(behavior.getMessages(), "TRAIL_SPEED");
  };

  EXPECT_DOUBLE_EQ(trail_speed(true), 2);
  EXPECT_DOUBLE_EQ(trail_speed(false), 0);
}

// Covers the complete spawn alert request and each condition that suppresses
// it: explicit opt-out, missing updates variable, or a nonspawnable behavior.
TEST(BHVTrailTest, AlertRequestRequiresSpawnUpdateAndOptIn)
{
  auto request_for = [](bool spawnable, bool set_updates, bool opt_in) {
    TestableBHVTrail behavior(makeCourseSpeedDomain());
    EXPECT_TRUE(behavior.setBehaviorName("trail_contact"));
    behavior.setDynamicallySpawnable(spawnable);
    if(set_updates)
      EXPECT_TRUE(behavior.setParam("updates", "TRAIL_UPDATES"));
    EXPECT_TRUE(behavior.setParam("pwt_outer_dist", "120"));
    EXPECT_TRUE(behavior.setParam("no_alert_request",
                                  opt_in ? "false" : "true"));
    behavior.onHelmStart();
    return findStringMessage(behavior.getMessages(), "BCM_ALERT_REQUEST");
  };

  EXPECT_EQ(request_for(true, true, false), "");
  EXPECT_EQ(request_for(true, false, true), "");
  EXPECT_EQ(request_for(false, true, true), "");
  EXPECT_EQ(request_for(true, true, true),
            "id=trail_contact, on_flag=TRAIL_UPDATES=name=$[VNAME] # "
            "contact=$[VNAME],alert_range=120, cpa_range=125");
}

// Covers warning, error, and omitted-contact policies with their exact
// behavior-owned diagnostic payloads.
TEST(BHVTrailTest, MissingContactPoliciesPostExactDiagnostics)
{
  auto messages_for = [](bool set_contact, bool missing_ok) {
    InfoBuffer info;
    info.setCurrTime(100);
    info.setValue("NAV_X", 0);
    info.setValue("NAV_Y", 0);
    info.setValue("NAV_HEADING", 90);
    info.setValue("NAV_SPEED", 1);
    LedgerSnap ledger;

    TestableBHVTrail behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setBehaviorName("trail_contact"));
    if(set_contact)
      EXPECT_TRUE(behavior.setParam("contact", "ghost"));
    EXPECT_TRUE(behavior.setParam("on_no_contact_ok",
                                  missing_ok ? "true" : "false"));
    behavior.onEveryState("running");
    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_EQ(ipf, nullptr);
    return behavior.getMessages();
  };

  const auto warning_messages = messages_for(true, true);
  EXPECT_EQ(findStringMessage(warning_messages, "BHV_WARNING"),
            "trail_contact: ghost x/y/heading/speed info not found");
  EXPECT_EQ(findStringMessage(warning_messages, "BHV_ERROR"), "");

  const auto error_messages = messages_for(true, false);
  EXPECT_EQ(findStringMessage(error_messages, "BHV_ERROR"),
            "trail_contact: ghost x/y/heading/speed info not found");

  const auto omitted_messages = messages_for(false, false);
  EXPECT_EQ(findStringMessage(omitted_messages, "BHV_ERROR"),
            "trail_contact: contact IDX not set.");
}

// Covers every malformed startup value retained by the Trail harness,
// including parameters inherited from IvPContactBehavior.
TEST(BHVTrailTest, RejectsEveryMalformedHarnessParameter)
{
  BHV_Trail behavior(makeCourseSpeedDomain());

  EXPECT_FALSE(behavior.setParam("trail_angle_type", "diagonal"));
  EXPECT_FALSE(behavior.setParam("trail_angle", "west"));
  EXPECT_FALSE(behavior.setParam("trail_range", "-1"));
  EXPECT_FALSE(behavior.setParam("mod_trail_range_pct", "0"));
  EXPECT_FALSE(behavior.setParam("radius", "-1"));
  EXPECT_FALSE(behavior.setParam("nm_radius", "-1"));
  EXPECT_FALSE(behavior.setParam("pwt_outer_dist", "-1"));
  EXPECT_FALSE(behavior.setParam("decay", "60,30"));
  EXPECT_FALSE(behavior.setParam("time_on_leg", "-1"));
}
