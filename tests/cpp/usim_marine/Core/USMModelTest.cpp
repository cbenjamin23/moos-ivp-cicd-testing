#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "NumericAssertions.h"
#include "USM_Model.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Covers uSimMarine model config: numeric parameters set state, clamp where appropriate, and reject invalid values.
TEST(USMModelTest, NumericParametersSetStateAndClampInvalidValues)
{
  USM_Model model;

  EXPECT_TRUE(model.setParam("start_x", 10));
  EXPECT_TRUE(model.setParam("start_y", -5));
  EXPECT_TRUE(model.setParam("start_heading", 370));
  EXPECT_TRUE(model.setParam("start_speed", 2.5));
  EXPECT_FALSE(model.setParam("start_depth", -4));
  EXPECT_TRUE(model.setParam("turn_rate", 150));
  EXPECT_FALSE(model.setParam("max_acceleration", -1));
  EXPECT_FALSE(model.setParam("max_deceleration", -2));
  EXPECT_TRUE(model.setParam("water_depth", 20));
  EXPECT_FALSE(model.setParam("water_depth", -1));
  EXPECT_FALSE(model.setParam("unknown", 1));

  NodeRecord record = model.getNodeRecord();
  EXPECT_DOUBLE_EQ(record.getX(), 10);
  EXPECT_DOUBLE_EQ(record.getY(), -5);
  EXPECT_DOUBLE_EQ(record.getHeading(), 370);
  EXPECT_DOUBLE_EQ(record.getSpeed(), 2.5);
  EXPECT_DOUBLE_EQ(record.getDepth(), 0);
  EXPECT_DOUBLE_EQ(model.getTurnRate(), 100);
  EXPECT_DOUBLE_EQ(model.getMaxAcceleration(), 0);
  EXPECT_DOUBLE_EQ(model.getMaxDeceleration(), 0);
  EXPECT_DOUBLE_EQ(model.getWaterDepth(), 20);
}

// Covers uSimMarine model config: string toggles accept aliases and reject malformed values.
TEST(USMModelTest, ModeTogglesAcceptAliasesAndRejectMalformedValues)
{
  USM_Model model;

  EXPECT_TRUE(model.setDualState("true"));
  EXPECT_TRUE(model.usingDualState());
  EXPECT_TRUE(model.setDualState("false"));
  EXPECT_FALSE(model.usingDualState());
  EXPECT_FALSE(model.setDualState("maybe"));

  EXPECT_TRUE(model.setThrustModeReverse("reverse"));
  EXPECT_TRUE(model.getThrustModeReverse());
  EXPECT_TRUE(model.setThrustModeReverse("false"));
  EXPECT_FALSE(model.getThrustModeReverse());
  EXPECT_FALSE(model.setThrustModeReverse("backward"));

  EXPECT_TRUE(model.setThrustModeDiff("differential"));
  EXPECT_EQ(model.getThrustModeDiff(), "differential");
  EXPECT_TRUE(model.setThrustModeDiff("false"));
  EXPECT_EQ(model.getThrustModeDiff(), "normal");
  EXPECT_FALSE(model.setThrustModeDiff("sideways"));
}

// Covers uSimMarine model behavior: normal and differential actuators ignore commands in the wrong mode.
TEST(USMModelTest, ActuatorCommandsRespectThrustMode)
{
  USM_Model model;

  model.setRudder(40);
  model.setThrust(60);
  EXPECT_DOUBLE_EQ(model.getThrust(), 60);

  ASSERT_TRUE(model.setThrustModeDiff("true"));
  model.setRudder(10);
  model.setThrust(20);
  EXPECT_DOUBLE_EQ(model.getThrust(), 60);

  model.setThrustLeft(150);
  model.setThrustRight(-150);
  EXPECT_DOUBLE_EQ(model.getThrustLeft(), 100);
  EXPECT_DOUBLE_EQ(model.getThrustRight(), -100);

  ASSERT_TRUE(model.setThrustModeReverse("true"));
  model.setThrustLeft(40);
  model.setThrustRight(20);
  EXPECT_DOUBLE_EQ(model.getThrustLeft(), -20);
  EXPECT_DOUBLE_EQ(model.getThrustRight(), -40);
}

// Covers uSimMarine model behavior: rudder slew rate limits timestamped rudder changes.
TEST(USMModelTest, TimestampedRudderHonorsMaxDegreesPerSecond)
{
  USM_Model model;
  ASSERT_TRUE(model.setMaxRudderDegreesPerSec(10));
  EXPECT_FALSE(model.setMaxRudderDegreesPerSec(-1));

  model.setRudder(100, 1);
  model.setParam("start_x", 0);
  model.setParam("start_y", 0);
  model.setParam("start_heading", 0);
  model.setParam("start_speed", 0);
  model.resetTime(1);
  model.setThrust(100);
  model.propagate(2);

  EXPECT_LT(model.getNodeRecord().getHeading(), 30);
}

// Covers uSimMarine model drift behavior: vector setters, additive vectors, source tracking, and magnification.
TEST(USMModelTest, DriftVectorSetAddMagnitudeAndSources)
{
  USM_Model model;

  EXPECT_TRUE(model.setDriftVector("0,2", "wind"));
  EXPECT_NEAR(model.getDriftX(), 0, kLooseGeomTol);
  EXPECT_NEAR(model.getDriftY(), 2, kLooseGeomTol);
  EXPECT_TRUE(model.isDriftFresh());
  EXPECT_TRUE(Contains(model.getDriftSources(), "wind"));

  EXPECT_TRUE(model.setDriftVector("90,1", "current", true));
  EXPECT_NEAR(model.getDriftX(), 1, kLooseGeomTol);
  EXPECT_NEAR(model.getDriftY(), 2, kLooseGeomTol);
  EXPECT_TRUE(Contains(model.getDriftSources(), "current"));
  EXPECT_FALSE(model.setDriftVector("bad,1", "bad"));

  model.magDriftVector(0.5, "scale");
  EXPECT_NEAR(model.getDriftMag(), std::sqrt(5.0) * 0.5, kLooseGeomTol);
  EXPECT_TRUE(Contains(model.getDriftSummary(), "xmag="));
}

// Covers uSimMarine model behavior: initPosition supports keyed and legacy positional forms.
TEST(USMModelTest, InitPositionSupportsKeyedAndLegacyForms)
{
  USM_Model model;

  EXPECT_TRUE(model.initPosition("x=10,y=-5,heading=90,speed=2,depth=3"));
  EXPECT_EQ(model.getResetCount(), 1u);
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getX(), 10);
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getY(), -5);
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getHeading(), 90);
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getSpeed(), 2);
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getDepth(), 3);

  EXPECT_TRUE(model.initPosition("1,2,180,4,5"));
  EXPECT_EQ(model.getResetCount(), 2u);
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getHeading(), 180);
  EXPECT_FALSE(model.initPosition("x=1,bogus=2"));
  EXPECT_EQ(model.getResetCount(), 3u);
}

// Covers uSimMarine model behavior: propagation updates nav, altitude, and dual-state ground truth separation.
TEST(USMModelTest, PropagationUpdatesNavAltitudeAndDualStateGroundTruth)
{
  USM_Model model;
  ASSERT_TRUE(model.initPosition("x=0,y=0,heading=90,speed=0,depth=5"));
  model.resetTime(0);
  model.setParam("water_depth", 20);
  model.setThrustFactor(20);
  model.setThrust(40);
  model.setDriftX(1, "current");
  model.setDriftY(0, "current");
  ASSERT_TRUE(model.setDualState("true"));

  ASSERT_TRUE(model.propagate(1));
  NodeRecord nav = model.getNodeRecord();
  NodeRecord truth = model.getNodeRecordGT();

  EXPECT_LT(nav.getX(), truth.getX());
  EXPECT_GT(nav.getSpeed(), 0);
  EXPECT_GT(truth.getSpeed(), 0);
  EXPECT_NEAR(nav.getAltitude(), 20 - nav.getDepth(), kGeomTol);

  model.resetNavGroundTruth();
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getX(), model.getNodeRecordGT().getX());
}

// Covers uSimMarine model behavior: pause and obstacle-hit states suppress propagation.
TEST(USMModelTest, PauseAndObstacleHitSuppressPropagation)
{
  USM_Model model;
  ASSERT_TRUE(model.initPosition("x=0,y=0,heading=90,speed=0"));
  model.resetTime(0);
  model.setThrust(100);

  ASSERT_TRUE(model.setPaused("true"));
  ASSERT_TRUE(model.propagate(10));
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getX(), 0);
  ASSERT_TRUE(model.setPaused("false"));

  model.setObstacleHit(true);
  ASSERT_TRUE(model.propagate(10));
  EXPECT_DOUBLE_EQ(model.getNodeRecord().getX(), 0);
}

// Covers uSimMarine model behavior: thrust mapping, reflection, and mode summaries are exposed.
TEST(USMModelTest, ThrustMappingReflectionAndSummary)
{
  USM_Model model;
  EXPECT_TRUE(model.usingThrustFactor());
  EXPECT_DOUBLE_EQ(model.getThrustFactor(), 20);

  EXPECT_TRUE(model.handleFullThrustMapping("20:1,60:3"));
  EXPECT_FALSE(model.handleFullThrustMapping("20:bad"));
  EXPECT_FALSE(model.handleFullThrustMapping(""));
  EXPECT_FALSE(model.usingThrustFactor());
  EXPECT_TRUE(Contains(model.getThrustMapPos(), "20,1"));

  EXPECT_TRUE(model.setThrustReflect("true"));
  EXPECT_EQ(model.getThrustMapNeg(), "Positive thrust-map reflected");
  EXPECT_FALSE(model.setThrustReflect("maybe"));
}

// Covers uSimMarine model config: geodesy updates lat/lon and cached origin fields.
TEST(USMModelTest, GeodesyUpdatesGlobalPositionAndStartingDatum)
{
  CMOOSGeodesy geodesy;
  ASSERT_TRUE(geodesy.Initialise(43.8253, -70.3304));

  USM_Model model;
  model.setGeodesy(geodesy);
  ASSERT_TRUE(model.geoOK());
  ASSERT_TRUE(model.initPosition("x=0,y=0,heading=90,speed=0,depth=0"));
  model.resetTime(0);
  model.setThrust(40);
  ASSERT_TRUE(model.propagate(1));
  model.cacheStartingInfo();

  EXPECT_NE(model.getNodeRecord().getLat(), 0);
  EXPECT_NE(model.getNodeRecord().getLon(), 0);
  EXPECT_EQ(model.getStartDatumLat(), "43.8253");
  EXPECT_EQ(model.getStartDatumLon(), "-70.3304");
}

// Covers uSimMarine model config: wind and polar-plot parameters enable sailing speed limits.
TEST(USMModelTest, WindAndPolarPlotEnableSailingSpeedLimit)
{
  USM_Model model;
  ASSERT_TRUE(model.setParam("wind_conditions", "spd=3, dir=180"));
  ASSERT_TRUE(model.setParam("polar_plot", "0,0:20,40:45,65:90,80:110,90:135,83:150,83:165,60:180,50"));
  EXPECT_FALSE(model.setParam("wind_conditions", "not-a-wind-spec"));

  ASSERT_TRUE(model.initPosition("x=0,y=0,heading=90,speed=0,depth=0"));
  model.resetTime(0);
  model.setThrustFactor(20);
  model.setThrust(100);
  ASSERT_TRUE(model.propagate(1));

  EXPECT_TRUE(model.sailingEnabled());
  EXPECT_FALSE(model.getWindModelSpec().empty());
  EXPECT_FALSE(model.getPolarPlotSpec().empty());
  EXPECT_LT(model.getNodeRecord().getSpeed(), 5);
}

// Covers uSimMarine model behavior: starting-info cache captures nav, drift, water-depth, and origin fields.
TEST(USMModelTest, CacheStartingInfoCapturesFormattedState)
{
  USM_Model model;
  ASSERT_TRUE(model.initPosition("x=1,y=2,heading=45,speed=3,depth=4"));
  model.setParam("water_depth", 10);
  model.setDriftX(0.25, "current");
  model.setDriftY(0.5, "current");
  model.cacheStartingInfo();

  EXPECT_EQ(model.getStartNavX(), "1");
  EXPECT_EQ(model.getStartNavY(), "2");
  EXPECT_EQ(model.getStartNavHdg(), "45");
  EXPECT_EQ(model.getStartNavSpd(), "3");
  EXPECT_EQ(model.getStartNavDep(), "4");
  EXPECT_EQ(model.getStartNavAlt(), "6");
  EXPECT_EQ(model.getStartDriftX(), "0.25");
  EXPECT_EQ(model.getStartDriftY(), "0.5");
  EXPECT_EQ(model.getStartDatumLat(), "?");
  EXPECT_EQ(model.getStartDatumLon(), "?");
}
