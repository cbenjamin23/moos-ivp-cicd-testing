#include <gtest/gtest.h>

#include <initializer_list>
#include <list>
#include <string>

#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "NumericAssertions.h"
#include "USM_MOOSApp.h"

namespace {

class TestableUSMMOOSApp : public USM_MOOSApp {
 public:
  using USM_MOOSApp::handleMailBuoyancyControl;
  using USM_MOOSApp::handleMailTrimControl;

  TestableUSMMOOSApp()
  {
    m_curr_time = 10;
  }

  USM_Model& model() { return m_model; }
  bool enabled() const { return m_enabled; }
  bool obstacleHit() const { return m_obstacle_hit; }
  bool buoyancyRequested() const { return m_buoyancy_requested; }
  bool trimRequested() const { return m_trim_requested; }
};

MOOSMSG_LIST mail(std::initializer_list<CMOOSMsg> messages)
{
  MOOSMSG_LIST result;
  for(const CMOOSMsg& msg : messages)
    result.push_back(msg);
  return result;
}

CMOOSMsg dmail(const std::string& key, double value, double time = MOOSTime())
{
  return CMOOSMsg(MOOS_NOTIFY, key, value, time);
}

CMOOSMsg smail(const std::string& key, const std::string& value, double time = MOOSTime())
{
  return CMOOSMsg(MOOS_NOTIFY, key, value, time);
}

}  // namespace

// Covers uSimMarine app mail dispatch: actuator and mode mail is routed into the simulator model.
TEST(USMMOOSAppTest, OnNewMailRoutesActuatorAndModeInputsToModel)
{
  TestableUSMMOOSApp app;

  MOOSMSG_LIST normal_mail = mail({
      dmail("DESIRED_THRUST", 65),
      dmail("DESIRED_RUDDER", 20),
      dmail("DESIRED_ELEVATOR", -5),
      smail("THRUST_MODE_REVERSE", "true")});
  ASSERT_TRUE(app.OnNewMail(normal_mail));

  EXPECT_DOUBLE_EQ(app.model().getThrust(), 65);
  EXPECT_TRUE(app.model().getThrustModeReverse());

  MOOSMSG_LIST diff_mail = mail({
      smail("THRUST_MODE_DIFFERENTIAL", "true"),
      dmail("DESIRED_THRUST_L", 125),
      dmail("DESIRED_THRUST_R", -125)});
  ASSERT_TRUE(app.OnNewMail(diff_mail));

  EXPECT_EQ(app.model().getThrustModeDiff(), "differential");
  EXPECT_DOUBLE_EQ(app.model().getThrustLeft(), 100);
  EXPECT_DOUBLE_EQ(app.model().getThrustRight(), -100);
}

// Covers uSimMarine app mail dispatch: environmental, pause, reset, and obstacle mail update model/app state.
TEST(USMMOOSAppTest, OnNewMailRoutesEnvironmentResetPauseAndObstacleInputs)
{
  TestableUSMMOOSApp app;

  MOOSMSG_LIST env_mail = mail({
      dmail("CURRENT_X", 0.25),
      dmail("CURRENT_Y", -0.5),
      smail("DRIFT_VECTOR_ADD", "90,1"),
      dmail("DRIFT_VECTOR_MULT", 0.5),
      dmail("WATER_DEPTH", 30),
      dmail("BUOYANCY_RATE", -0.1),
      dmail("ROTATE_SPEED", 3),
      smail("USM_SIM_PAUSED", "true"),
      smail("OBSTACLE_HIT", "true")});
  ASSERT_TRUE(app.OnNewMail(env_mail));

  EXPECT_NEAR(app.model().getDriftX(), 0.625, kLooseGeomTol);
  EXPECT_NEAR(app.model().getDriftY(), -0.25, kLooseGeomTol);
  EXPECT_DOUBLE_EQ(app.model().getWaterDepth(), 30);
  EXPECT_DOUBLE_EQ(app.model().getBuoyancyRate(), -0.1);
  EXPECT_DOUBLE_EQ(app.model().getRotateSpd(), 3);
  EXPECT_TRUE(app.obstacleHit());

  MOOSMSG_LIST reset_mail = mail({smail("USM_RESET", "x=4,y=-3,heading=270,speed=1,depth=2")});
  ASSERT_TRUE(app.OnNewMail(reset_mail));

  EXPECT_EQ(app.model().getResetCount(), 1u);
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getX(), 4);
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getY(), -3);
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getHeading(), 270);
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getDepth(), 2);
}

// Covers uSimMarine app disabled-sim behavior: incoming NAV mail informs the model for later re-enable.
TEST(USMMOOSAppTest, DisabledSimUsesIncomingNavMailToRefreshModelPose)
{
  TestableUSMMOOSApp app;

  MOOSMSG_LIST disabled_mail = mail({
      smail("USM_ENABLED", "false"),
      dmail("NAV_X", 12),
      dmail("NAV_Y", -8),
      dmail("NAV_HEADING", 405)});
  ASSERT_TRUE(app.OnNewMail(disabled_mail));

  EXPECT_FALSE(app.enabled());
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getX(), 12);
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getY(), -8);
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getHeading(), 405);
}

// Covers uSimMarine app mail dispatch: reset-nav snaps the reported nav state to ground truth.
TEST(USMMOOSAppTest, ResetNavMailCopiesGroundTruthIntoNavigationState)
{
  TestableUSMMOOSApp app;
  ASSERT_TRUE(app.model().initPosition("x=0,y=0,heading=90,speed=0,depth=0"));
  ASSERT_TRUE(app.model().setDualState("true"));
  app.model().resetTime(0);
  app.model().setDriftX(2, "current");
  ASSERT_TRUE(app.model().propagate(1));
  ASSERT_NE(app.model().getNodeRecord().getX(), app.model().getNodeRecordGT().getX());

  MOOSMSG_LIST reset_nav_mail = mail({smail("USM_RESET_NAV", "true")});
  ASSERT_TRUE(app.OnNewMail(reset_nav_mail));

  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getX(), app.model().getNodeRecordGT().getX());
  EXPECT_DOUBLE_EQ(app.model().getNodeRecord().getY(), app.model().getNodeRecordGT().getY());
}

// Covers uSimMarine app control helpers: buoyancy and trim requests set request state and model inputs.
TEST(USMMOOSAppTest, BuoyancyAndTrimControlHandlersAcceptTrueOnly)
{
  TestableUSMMOOSApp app;
  app.model().setParam("buoyancy_rate", 0.2);

  app.handleMailBuoyancyControl("false");
  app.handleMailTrimControl("false");
  EXPECT_FALSE(app.buoyancyRequested());
  EXPECT_FALSE(app.trimRequested());
  EXPECT_DOUBLE_EQ(app.model().getBuoyancyRate(), 0.2);

  app.handleMailBuoyancyControl("true");
  app.handleMailTrimControl("true");
  EXPECT_TRUE(app.buoyancyRequested());
  EXPECT_TRUE(app.trimRequested());
  EXPECT_DOUBLE_EQ(app.model().getBuoyancyRate(), 0.0);
}
