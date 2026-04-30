#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <list>
#include <string>
#include <vector>

#include "MarinePID.h"
#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "NumericAssertions.h"
#include "TestFileUtils.h"
#include "VarDataPair.h"

namespace {

std::list<std::string> basePidConfig(bool depth_control = false)
{
  std::list<std::string> params;
  params.push_back("tardy_helm_threshold = 30");
  params.push_back("tardy_nav_threshold = 30");
  params.push_back("yaw_pid_kp = 1.0");
  params.push_back("yaw_pid_kd = 0.0");
  params.push_back("yaw_pid_ki = 0.0");
  params.push_back("yaw_pid_integral_limit = 100");
  params.push_back("maxrudder = 100");
  params.push_back("speed_factor = 20");
  params.push_back("speed_pid_kp = 20");
  params.push_back("speed_pid_kd = 0.0");
  params.push_back("speed_pid_ki = 0.0");
  params.push_back("speed_pid_integral_limit = 100");
  params.push_back("maxthrust = 100");
  params.push_back(std::string("depth_control = ") + (depth_control ? "true" : "false"));
  if(depth_control) {
    params.push_back("z_to_pitch_pid_kp = 0.1");
    params.push_back("z_to_pitch_pid_kd = 0.0");
    params.push_back("z_to_pitch_pid_ki = 0.0");
    params.push_back("z_to_pitch_pid_integral_limit = 1.0");
    params.push_back("maxpitch = 15");
    params.push_back("pitch_pid_kp = 1.0");
    params.push_back("pitch_pid_kd = 0.0");
    params.push_back("pitch_pid_ki = 0.0");
    params.push_back("pitch_pid_integral_limit = 1.0");
    params.push_back("maxelevator = 13");
  }
  return params;
}

void configure(PIDEngine& engine, std::list<std::string> params)
{
  std::list<std::string> unhandled = engine.setConfigParams(params);
  EXPECT_TRUE(unhandled.empty());
  EXPECT_TRUE(engine.handleYawSettings());
  EXPECT_TRUE(engine.handleSpeedSettings());
  EXPECT_TRUE(engine.handleDepthSettings());
}

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

bool containsTextPosting(const std::vector<VarDataPair>& postings,
                         const std::string& var,
                         const std::string& needle)
{
  for(const VarDataPair& posting : postings) {
    if(posting.get_var() == var &&
       posting.get_sdata().find(needle) != std::string::npos)
      return true;
  }
  return false;
}

TempFile writeTempMission(const std::string& contents)
{
  // The MOOS mission reader is exercised with mission-like paths, so preserve
  // the .moos suffix even though the file is temporary.
  return TempFile("marinepidv22_mission", contents, ".moos");
}

class MarinePIDHarness : public MarinePID {
public:
  MarinePIDHarness()
  {
    m_curr_time = 10.0;
    m_pengine.updateTime(m_curr_time);
  }

  PIDEngine& engine() { return m_pengine; }
  const PIDEngine& engine() const { return m_pengine; }

  void setTime(double time)
  {
    m_curr_time = time;
    m_pengine.updateTime(time);
  }

  void configureEngine(bool depth_control = false)
  {
    configure(m_pengine, basePidConfig(depth_control));
  }

  void setIgnoreNavYaw(bool value) { m_pid_ignore_nav_yaw = value; }
  bool ignoreNavYaw() const { return m_pid_ignore_nav_yaw; }

  void setPidOkSkew(double value) { m_pid_ok_skew = value; }
  void setVerbose(bool value) { m_pid_verbose = value; }
  bool verbose() const { return m_pid_verbose; }
  void setTermReporting(bool value) { m_term_reporting = value; }

  void setOutputSuffix(const std::string& value) { m_output_suffix = value; }
  std::string outputSuffix() const { return m_output_suffix; }

  void setThrustCapEnabled(bool value) { m_enable_thrust_cap = value; }
  bool thrustCapEnabled() const { return m_enable_thrust_cap; }
  double thrustCap() const { return m_thrust_cap; }
  bool allstopPosted() const { return m_pid_allstop_posted; }

  void setMissionFile(const std::string& app_name, const std::string& path)
  {
    m_sAppName = app_name;
    m_sMOOSName = app_name;
    m_sMissionFile = path;
    m_MissionReader.SetAppName(app_name);
    ASSERT_TRUE(m_MissionReader.SetFile(path));
  }

  std::string reportText()
  {
    m_msgs.str("");
    m_msgs.clear();
    buildReport();
    return m_msgs.str();
  }
};

}  // namespace

// Covers marine PIDv22 mail behavior: manual override supports both spellings.
TEST(MarinePIDV22MailTest, ManualOverrideSupportsBothSpellings)
{
  MarinePIDHarness app;
  ASSERT_TRUE(app.engine().hasPIDOverride());

  MOOSMSG_LIST release = mail({smail("MOOS_MANUAL_OVERIDE", "false")});
  EXPECT_TRUE(app.OnNewMail(release));
  EXPECT_FALSE(app.engine().hasPIDOverride());

  MOOSMSG_LIST engage = mail({smail("MOOS_MANUAL_OVERRIDE", "true")});
  EXPECT_TRUE(app.OnNewMail(engage));
  EXPECT_TRUE(app.engine().hasPIDOverride());
}

// Covers marine PIDv22 startup behavior: startup applies mission config and V22 app options.
TEST(MarinePIDV22StartupTest, StartupAppliesMissionConfigAndV22AppOptions)
{
  TempFile mission = writeTempMission(
      "ServerHost = localhost\n"
      "ServerPort = 9000\n"
      "Community = alpha\n"
      "ProcessConfig = pMarinePIDV22\n"
      "{\n"
      "  AppTick = 4\n"
      "  CommsTick = 4\n"
      "  term_reporting = false\n"
      "  tardy_helm_threshold = 30\n"
      "  tardy_nav_threshold = 30\n"
      "  yaw_pid_kp = 1.5\n"
      "  yaw_pid_kd = 0\n"
      "  yaw_pid_ki = 0\n"
      "  yaw_pid_integral_limit = 100\n"
      "  maxrudder = 30\n"
      "  speed_factor = 7\n"
      "  speed_pid_kp = 20\n"
      "  speed_pid_kd = 0\n"
      "  speed_pid_ki = 0\n"
      "  speed_pid_integral_limit = 100\n"
      "  maxthrust = 80\n"
      "  depth_control = false\n"
      "  ignore_nav_yaw = true\n"
      "  pid_verbose = false\n"
      "  enable_thrust_cap = true\n"
      "  output_suffix = _pid\n"
      "}\n");

  MarinePIDHarness app;
  app.setMissionFile("pMarinePIDV22", mission.path());

  EXPECT_TRUE(app.OnStartUp());

  EXPECT_TRUE(app.ignoreNavYaw());
  EXPECT_FALSE(app.verbose());
  EXPECT_TRUE(app.thrustCapEnabled());
  EXPECT_EQ(app.outputSuffix(), "_PID");
  EXPECT_FALSE(app.engine().hasDepthControl());
  EXPECT_DOUBLE_EQ(app.engine().getSpeedFactor(), 7.0);
  EXPECT_DOUBLE_EQ(app.engine().getMaxRudder(), 30.0);
  EXPECT_DOUBLE_EQ(app.engine().getMaxThrust(), 80.0);

}

// Covers marine PIDv22 startup behavior: startup rejects malformed depth control.
TEST(MarinePIDV22StartupTest, StartupRejectsMalformedDepthControl)
{
  TempFile mission = writeTempMission(
      "ServerHost = localhost\n"
      "ServerPort = 9000\n"
      "Community = alpha\n"
      "ProcessConfig = pMarinePIDV22\n"
      "{\n"
      "  tardy_helm_threshold = 30\n"
      "  tardy_nav_threshold = 30\n"
      "  yaw_pid_kp = 1.5\n"
      "  yaw_pid_kd = 0\n"
      "  yaw_pid_ki = 0\n"
      "  yaw_pid_integral_limit = 100\n"
      "  maxrudder = 30\n"
      "  speed_factor = 7\n"
      "  speed_pid_kp = 20\n"
      "  speed_pid_kd = 0\n"
      "  speed_pid_ki = 0\n"
      "  speed_pid_integral_limit = 100\n"
      "  maxthrust = 80\n"
      "  depth_control = maybe\n"
      "}\n");

  MarinePIDHarness app;
  app.setMissionFile("pMarinePIDV22", mission.path());

  EXPECT_FALSE(app.OnStartUp());
}

// Covers marine PIDv22 mail behavior: numeric manual override mail is ignored.
TEST(MarinePIDV22MailTest, NumericManualOverrideMailIsIgnored)
{
  MarinePIDHarness app;
  ASSERT_TRUE(app.engine().hasPIDOverride());

  MOOSMSG_LIST numeric = mail({dmail("MOOS_MANUAL_OVERRIDE", 0.0)});
  EXPECT_TRUE(app.OnNewMail(numeric));

  EXPECT_TRUE(app.engine().hasPIDOverride());
  EXPECT_TRUE(app.engine().getPostings().empty());
}

// Covers marine PIDv22 mail behavior: verbose mail accepts booleans and ignores invalid values.
TEST(MarinePIDV22MailTest, VerboseMailAcceptsBooleansAndIgnoresInvalidValues)
{
  MarinePIDHarness app;
  ASSERT_TRUE(app.verbose());

  MOOSMSG_LIST off = mail({smail("PID_VERBOSE", "false")});
  EXPECT_TRUE(app.OnNewMail(off));
  EXPECT_FALSE(app.verbose());

  MOOSMSG_LIST invalid = mail({smail("PID_VERBOSE", "definitely")});
  EXPECT_TRUE(app.OnNewMail(invalid));
  EXPECT_FALSE(app.verbose());

  MOOSMSG_LIST on = mail({smail("PID_VERBOSE", "TRUE")});
  EXPECT_TRUE(app.OnNewMail(on));
  EXPECT_TRUE(app.verbose());
}

// Covers marine PIDv22 mail behavior: speed factor mail overrides configured engine factor.
TEST(MarinePIDV22MailTest, SpeedFactorMailOverridesConfiguredEngineFactor)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);

  MOOSMSG_LIST new_factor = mail({
      dmail("SPEED_FACTOR", 7.5),
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 4.0),
      dmail("NAV_HEADING", 90.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(new_factor));
  app.engine().setDesiredValues();

  EXPECT_DOUBLE_EQ(app.engine().getSpeedFactor(), 7.5);
  EXPECT_NEAR(app.engine().getDesiredThrust(), 30.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredRudder(), 0.0, kGeomTol);
}

// Covers marine PIDv22 mail behavior: surface mail feeds heading speed and navigation state.
TEST(MarinePIDV22MailTest, SurfaceMailFeedsHeadingSpeedAndNavigationState)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);

  MOOSMSG_LIST input = mail({
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_HEADING", 80.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(input));
  app.engine().setDesiredValues();

  EXPECT_FALSE(app.engine().hasPIDStale());
  EXPECT_NEAR(app.engine().getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredRudder(), 10.0, kGeomTol);
}

// Covers marine PIDv22 mail behavior: latest heading mail wins between nav heading and nav yaw.
TEST(MarinePIDV22MailTest, LatestHeadingMailWinsBetweenNavHeadingAndNavYaw)
{
  MarinePIDHarness heading_then_yaw;
  heading_then_yaw.configureEngine();
  heading_then_yaw.engine().setPIDOverride(false);
  MOOSMSG_LIST yaw_last = mail({
      dmail("DESIRED_HEADING", 100.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_HEADING", 80.0),
      dmail("NAV_YAW", -M_PI / 2.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(heading_then_yaw.OnNewMail(yaw_last));
  heading_then_yaw.engine().setDesiredValues();
  EXPECT_NEAR(heading_then_yaw.engine().getDesiredRudder(), 10.0, kLooseGeomTol);

  MarinePIDHarness yaw_then_heading;
  yaw_then_heading.configureEngine();
  yaw_then_heading.engine().setPIDOverride(false);
  MOOSMSG_LIST heading_last = mail({
      dmail("DESIRED_HEADING", 100.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_YAW", -M_PI / 2.0),
      dmail("NAV_HEADING", 80.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(yaw_then_heading.OnNewMail(heading_last));
  yaw_then_heading.engine().setDesiredValues();
  EXPECT_NEAR(yaw_then_heading.engine().getDesiredRudder(), 20.0, kGeomTol);
}

// Covers marine PIDv22 mail behavior: nav yaw mail uses radians and marine heading convention.
TEST(MarinePIDV22MailTest, NavYawMailUsesRadiansAndMarineHeadingConvention)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);

  MOOSMSG_LIST input = mail({
      dmail("DESIRED_HEADING", 100.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_YAW", -M_PI / 2.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(input));
  app.engine().setDesiredValues();

  EXPECT_NEAR(app.engine().getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredRudder(), 10.0, kLooseGeomTol);
}

// Covers marine PIDv22 mail behavior: ignore nav yaw leaves nav heading as authoritative.
TEST(MarinePIDV22MailTest, IgnoreNavYawLeavesNavHeadingAsAuthoritative)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);
  app.setIgnoreNavYaw(true);

  MOOSMSG_LIST input = mail({
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_HEADING", 80.0),
      dmail("NAV_YAW", -M_PI / 2.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(input));
  app.engine().setDesiredValues();

  EXPECT_TRUE(app.ignoreNavYaw());
  EXPECT_NEAR(app.engine().getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredRudder(), 10.0, kGeomTol);
}

// Covers marine PIDv22 mail behavior: depth mail feeds elevator path.
TEST(MarinePIDV22MailTest, DepthMailFeedsElevatorPath)
{
  MarinePIDHarness app;
  app.configureEngine(true);
  app.engine().setPIDOverride(false);

  MOOSMSG_LIST input = mail({
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("DESIRED_DEPTH", 5.0),
      dmail("NAV_HEADING", 90.0),
      dmail("NAV_SPEED", 0.0),
      dmail("NAV_DEPTH", 10.0),
      dmail("NAV_PITCH", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(input));
  app.engine().setDesiredValues();

  EXPECT_FALSE(app.engine().hasPIDStale());
  EXPECT_NEAR(app.engine().getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredElevator(), -13.0, kLooseGeomTol);
}

// Covers marine PIDv22 mail behavior: skewed mail is ignored.
TEST(MarinePIDV22MailTest, SkewedMailIsIgnored)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);
  app.setTime(1000.0);
  app.setPidOkSkew(5.0);

  MOOSMSG_LIST stale = mail({
      dmail("DESIRED_HEADING", 90.0, 10.0),
      dmail("DESIRED_SPEED", 2.0, 10.0),
      dmail("NAV_HEADING", 80.0, 10.0),
      dmail("NAV_SPEED", 0.0, 10.0),
  });

  EXPECT_TRUE(app.OnNewMail(stale));
  app.engine().setDesiredValues();

  EXPECT_DOUBLE_EQ(app.engine().getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(app.engine().getDesiredRudder(), 0.0);
  EXPECT_TRUE(app.engine().hasPIDStale());
}

// Covers marine PIDv22 mail behavior: skew filter accepts recent mail and rejects old mail.
TEST(MarinePIDV22MailTest, SkewFilterAcceptsRecentMailAndRejectsOldMail)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);
  app.setPidOkSkew(5.0);

  MOOSMSG_LIST old = mail({
      dmail("DESIRED_HEADING", 90.0, 0.0),
      dmail("DESIRED_SPEED", 2.0, 0.0),
      dmail("NAV_HEADING", 80.0, 0.0),
      dmail("NAV_SPEED", 0.0, 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(old));
  app.engine().setDesiredValues();
  EXPECT_DOUBLE_EQ(app.engine().getDesiredThrust(), 0.0);

  MOOSMSG_LIST recent = mail({
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_HEADING", 80.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(recent));
  app.engine().setDesiredValues();
  EXPECT_NEAR(app.engine().getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredRudder(), 10.0, kGeomTol);
}

// Covers marine PIDv22 mail behavior: thrust cap mail clips to configured runtime range.
TEST(MarinePIDV22MailTest, ThrustCapMailClipsToConfiguredRuntimeRange)
{
  MarinePIDHarness app;
  app.setThrustCapEnabled(true);

  MOOSMSG_LIST low = mail({smail("PID_THRUST_CAP", "5")});
  EXPECT_TRUE(app.OnNewMail(low));
  EXPECT_DOUBLE_EQ(app.thrustCap(), 20.0);

  MOOSMSG_LIST high = mail({smail("PID_THRUST_CAP", "120")});
  EXPECT_TRUE(app.OnNewMail(high));
  EXPECT_DOUBLE_EQ(app.thrustCap(), 100.0);

  MOOSMSG_LIST nominal = mail({smail("PID_THRUST_CAP", "55.5")});
  EXPECT_TRUE(app.OnNewMail(nominal));
  EXPECT_DOUBLE_EQ(app.thrustCap(), 55.5);
}

// Covers marine PIDv22 mail behavior: numeric thrust cap mail is ignored.
TEST(MarinePIDV22MailTest, NumericThrustCapMailIsIgnored)
{
  MarinePIDHarness app;
  app.setThrustCapEnabled(true);
  ASSERT_DOUBLE_EQ(app.thrustCap(), 100.0);

  MOOSMSG_LIST numeric = mail({dmail("PID_THRUST_CAP", 55.0)});
  EXPECT_TRUE(app.OnNewMail(numeric));

  EXPECT_DOUBLE_EQ(app.thrustCap(), 100.0);
}

// Covers marine PIDv22 post behavior: pengine postings are cleared after publication attempt.
TEST(MarinePIDV22PostTest, PenginePostingsAreClearedAfterPublicationAttempt)
{
  MarinePIDHarness app;
  app.engine().setPIDOverride(std::string("false"));
  ASSERT_FALSE(app.engine().getPostings().empty());

  app.postPenginePostings();

  EXPECT_TRUE(app.engine().getPostings().empty());
}

// Covers marine PIDv22 post behavior: all stop latch sets and clears across control transitions.
TEST(MarinePIDV22PostTest, AllStopLatchSetsAndClearsAcrossControlTransitions)
{
  MarinePIDHarness app;
  app.configureEngine();
  ASSERT_FALSE(app.allstopPosted());

  app.postPengineResults();
  EXPECT_TRUE(app.allstopPosted());

  app.engine().setPIDOverride(false);
  MOOSMSG_LIST input = mail({
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_HEADING", 90.0),
      dmail("NAV_SPEED", 0.0),
  });
  EXPECT_TRUE(app.OnNewMail(input));
  app.engine().setDesiredValues();
  ASSERT_TRUE(app.engine().hasControl());

  app.postPengineResults();
  EXPECT_FALSE(app.allstopPosted());

  app.engine().setPIDOverride(true);
  app.postPengineResults();
  EXPECT_TRUE(app.allstopPosted());
}

// Covers marine PIDv22 post behavior: char status honors verbose flag.
TEST(MarinePIDV22PostTest, CharStatusHonorsVerboseFlag)
{
  MarinePIDHarness app;
  app.setVerbose(true);
  testing::internal::CaptureStdout();
  app.postCharStatus();
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "*");

  app.setVerbose(false);
  testing::internal::CaptureStdout();
  app.postCharStatus();
  EXPECT_TRUE(testing::internal::GetCapturedStdout().empty());
}

// Covers marine PIDv22 iterate behavior: iterate computes outputs from fresh mail.
TEST(MarinePIDV22IterateTest, IterateComputesOutputsFromFreshMail)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.engine().setPIDOverride(false);
  app.setVerbose(false);
  app.setTermReporting(false);
  app.setTime(MOOSTime());
  MOOSMSG_LIST input = mail({
      dmail("DESIRED_HEADING", 90.0),
      dmail("DESIRED_SPEED", 2.0),
      dmail("NAV_HEADING", 80.0),
      dmail("NAV_SPEED", 0.0),
  });

  EXPECT_TRUE(app.OnNewMail(input));
  EXPECT_TRUE(app.Iterate());

  EXPECT_FALSE(app.engine().hasPIDStale());
  EXPECT_TRUE(app.engine().hasControl());
  EXPECT_NEAR(app.engine().getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(app.engine().getDesiredRudder(), 10.0, kGeomTol);
  EXPECT_FALSE(app.allstopPosted());
}

// Covers marine PIDv22 iterate behavior: iterate with override publishes all stop state once.
TEST(MarinePIDV22IterateTest, IterateWithOverridePublishesAllStopStateOnce)
{
  MarinePIDHarness app;
  app.configureEngine();
  app.setVerbose(false);
  app.setTermReporting(false);
  ASSERT_TRUE(app.engine().hasPIDOverride());

  EXPECT_TRUE(app.Iterate());

  EXPECT_TRUE(app.allstopPosted());
  EXPECT_DOUBLE_EQ(app.engine().getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(app.engine().getDesiredRudder(), 0.0);

  EXPECT_TRUE(app.Iterate());
  EXPECT_TRUE(app.allstopPosted());
}

// Covers marine PIDv22 report behavior: build report includes V22 engine and thrust cap state.
TEST(MarinePIDV22ReportTest, BuildReportIncludesV22EngineAndThrustCapState)
{
  MarinePIDHarness app;
  app.configureEngine(true);
  app.setThrustCapEnabled(true);

  std::string report = app.reportText();

  EXPECT_NE(report.find("Frequency:"), std::string::npos);
  EXPECT_NE(report.find("Speed Factor:"), std::string::npos);
  EXPECT_NE(report.find("PID has_control:"), std::string::npos);
  EXPECT_NE(report.find("Thrust Cap Enabled: true"), std::string::npos);
  EXPECT_NE(report.find("YAW_PID_KP"), std::string::npos);
  EXPECT_NE(report.find("SPEED_PID_KP"), std::string::npos);
  EXPECT_NE(report.find("PITCH_PID_KP"), std::string::npos);
  EXPECT_NE(report.find("Z_TO_PITCH_PID_KP"), std::string::npos);
}

// Covers marine PIDv22 report behavior: build report omits depth sections when depth control is disabled.
TEST(MarinePIDV22ReportTest, BuildReportOmitsDepthSectionsWhenDepthControlIsDisabled)
{
  MarinePIDHarness app;
  app.configureEngine(false);

  std::string report = app.reportText();

  EXPECT_NE(report.find("YAW_PID_KP"), std::string::npos);
  EXPECT_NE(report.find("SPEED_PID_KP"), std::string::npos);
  EXPECT_EQ(report.find("PITCH_PID_KP"), std::string::npos);
  EXPECT_EQ(report.find("Z_TO_PITCH_PID_KP"), std::string::npos);
}
