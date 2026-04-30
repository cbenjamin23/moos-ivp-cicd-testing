#include <gtest/gtest.h>

#include <cmath>
#include <list>
#include <string>
#include <vector>

#include "NumericAssertions.h"
#include "PIDEngine.h"
#include "VarDataPair.h"

namespace {

std::list<std::string> basePidConfig(bool depth_control = false)
{
  // Minimal pMarinePID-style mission config shared by surface and depth-control
  // tests. Individual tests prepend overrides with withExtra().
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

std::list<std::string> withExtra(std::list<std::string> params,
                                 const std::vector<std::string>& extras)
{
  for(std::vector<std::string>::const_reverse_iterator p = extras.rbegin();
      p != extras.rend(); ++p)
    params.push_front(*p);
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

void seedSurfaceInputs(PIDEngine& engine, double time = 1.0,
                       double desired_heading = 90.0,
                       double current_heading = 80.0,
                       double desired_speed = 2.0,
                       double current_speed = 0.0)
{
  engine.updateTime(time);
  engine.setDesHeading(desired_heading);
  engine.setDesSpeed(desired_speed);
  engine.setCurrHeading(current_heading);
  engine.setCurrSpeed(current_speed);
}

void seedDepthInputs(PIDEngine& engine, double desired_depth = 5.0,
                     double current_depth = 10.0,
                     double current_pitch = 0.0)
{
  engine.setDesDepth(desired_depth);
  engine.setCurrDepth(current_depth);
  engine.setCurrPitch(current_pitch);
}

const VarDataPair* findPosting(const std::vector<VarDataPair>& postings,
                               const std::string& var)
{
  for(const VarDataPair& posting : postings) {
    if(posting.get_var() == var)
      return &posting;
  }
  return nullptr;
}

std::vector<VarDataPair> postingsFor(const std::vector<VarDataPair>& postings,
                                     const std::string& var)
{
  std::vector<VarDataPair> matches;
  for(const VarDataPair& posting : postings) {
    if(posting.get_var() == var)
      matches.push_back(posting);
  }
  return matches;
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

std::string joined(const std::vector<std::string>& values)
{
  std::string result;
  for(const std::string& value : values)
    result += value + "\n";
  return result;
}

}  // namespace

// Covers PID engine config behavior: defaults start in override and stale without control.
TEST(PIDEngineConfigTest, DefaultsStartInOverrideAndStaleWithoutControl)
{
  PIDEngine engine;

  EXPECT_TRUE(engine.hasPIDOverride());
  EXPECT_TRUE(engine.hasPIDStale());
  EXPECT_FALSE(engine.hasControl());
  EXPECT_TRUE(engine.hasDepthControl());
  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getMaxRudder(), 100.0);
  EXPECT_DOUBLE_EQ(engine.getMaxThrust(), 100.0);
  EXPECT_DOUBLE_EQ(engine.getMaxPitch(), 15.0);
  EXPECT_DOUBLE_EQ(engine.getMaxElevator(), 13.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredElevator(), 0.0);
}

// Covers PID engine config behavior: sorts known and unhandled mission parameters.
TEST(PIDEngineConfigTest, SortsKnownAndUnhandledMissionParameters)
{
  PIDEngine engine;
  std::list<std::string> params = basePidConfig();
  params.push_back("output_suffix = _PID");
  params.push_back("unknown_token = 7");

  std::list<std::string> unhandled = engine.setConfigParams(params);
  ASSERT_EQ(unhandled.size(), 2u);
  EXPECT_EQ(unhandled.front(), "unknown_token = 7");
  EXPECT_EQ(unhandled.back(), "output_suffix = _PID");
}

// Covers PID engine config behavior: detects only yaw settings in PID heuristic.
TEST(PIDEngineConfigTest, DetectsOnlyYawSettingsInPidHeuristic)
{
  PIDEngine speed_only;
  std::list<std::string> speed_params;
  speed_params.push_back("speed_pid_kp = 20");
  speed_params.push_back("maxthrust = 100");
  EXPECT_TRUE(speed_only.setConfigParams(speed_params).empty());
  EXPECT_FALSE(speed_only.hasConfigSettingsForPID());

  PIDEngine yaw_configured;
  std::list<std::string> yaw_params;
  yaw_params.push_back("yaw_pid_kp = 1.0");
  EXPECT_TRUE(yaw_configured.setConfigParams(yaw_params).empty());
  EXPECT_TRUE(yaw_configured.hasConfigSettingsForPID());
}

// Covers PID engine config behavior: PID heuristic recognizes every yaw setting alias.
TEST(PIDEngineConfigTest, PidHeuristicRecognizesEveryYawSettingAlias)
{
  const std::vector<std::string> yaw_params = {
      "yaw_pid_kp = 1",
      "yaw_pid_kd = 1",
      "yaw_pid_ki = 1",
      "yaw_pid_integral_limit = 1",
      "yaw_pid_ki_limit = 1",
  };

  for(const std::string& param : yaw_params) {
    PIDEngine engine;
    std::list<std::string> params;
    params.push_back(param);
    EXPECT_TRUE(engine.setConfigParams(params).empty());
    EXPECT_TRUE(engine.hasConfigSettingsForPID()) << param;
  }
}

// Covers PID engine config behavior: applies surface tuning and reports summaries.
TEST(PIDEngineConfigTest, AppliesSurfaceTuningAndReportsSummaries)
{
  PIDEngine engine;
  configure(engine, basePidConfig());

  EXPECT_FALSE(engine.hasDepthControl());
  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), 20.0);
  EXPECT_DOUBLE_EQ(engine.getMaxRudder(), 100.0);
  EXPECT_DOUBLE_EQ(engine.getMaxThrust(), 100.0);

  std::vector<std::string> yaw = engine.getConfigSummary("yaw");
  ASSERT_EQ(yaw.size(), 4u);
  EXPECT_NE(yaw[0].find("YAW_PID_KP"), std::string::npos);
  EXPECT_NE(yaw[0].find("1"), std::string::npos);

  std::vector<std::string> all = engine.getConfigSummaryAll();
  EXPECT_EQ(all.size(), 16u);
}

// Covers PID engine config behavior: later mission lines override earlier duplicate settings.
TEST(PIDEngineConfigTest, LaterMissionLinesOverrideEarlierDuplicateSettings)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "yaw_pid_kp = 2.0",
      "maxrudder = 7",
      "speed_factor = 5",
      "maxthrust = 15",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 4.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 7.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredThrust(), 15.0, kGeomTol);
  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), 5.0);
  EXPECT_DOUBLE_EQ(engine.getMaxRudder(), 7.0);
  EXPECT_DOUBLE_EQ(engine.getMaxThrust(), 15.0);
}

// Covers PID engine config behavior: repeated config loads keep earlier effective settings.
TEST(PIDEngineConfigTest, RepeatedConfigLoadsKeepEarlierEffectiveSettings)
{
  PIDEngine engine;
  configure(engine, basePidConfig());

  // Re-loading config params after the handlers run stores new raw settings but
  // does not replace the already-effective PID controllers.
  std::list<std::string> updated;
  updated.push_back("yaw_pid_kp = 2.0");
  updated.push_back("maxrudder = 7");
  updated.push_back("speed_factor = 5");
  updated.push_back("maxthrust = 15");
  EXPECT_TRUE(engine.setConfigParams(updated).empty());
  EXPECT_TRUE(engine.handleYawSettings());
  EXPECT_TRUE(engine.handleSpeedSettings());
  EXPECT_TRUE(engine.handleDepthSettings());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 4.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredThrust(), 80.0, kGeomTol);
  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), 20.0);
  EXPECT_DOUBLE_EQ(engine.getMaxRudder(), 100.0);
  EXPECT_DOUBLE_EQ(engine.getMaxThrust(), 100.0);
}

// Covers PID engine config behavior: accepts case insensitive names and whitespace.
TEST(PIDEngineConfigTest, AcceptsCaseInsensitiveNamesAndWhitespace)
{
  PIDEngine engine;
  std::list<std::string> params;
  params.push_back(" TARDY_HELM_THRESHOLD = 30 ");
  params.push_back(" TARDY_NAV_THRESHOLD = 30 ");
  params.push_back(" YAW_PID_KP = 1.5 ");
  params.push_back(" YAW_PID_KD = 0.0 ");
  params.push_back(" YAW_PID_KI = 0.0 ");
  params.push_back(" YAW_PID_INTEGRAL_LIMIT = 100 ");
  params.push_back(" MAXRUDDER = 100 ");
  params.push_back(" SPEED_FACTOR = 3 ");
  params.push_back(" SPEED_PID_KP = 20 ");
  params.push_back(" SPEED_PID_KD = 0.0 ");
  params.push_back(" SPEED_PID_KI = 0.0 ");
  params.push_back(" SPEED_PID_INTEGRAL_LIMIT = 100 ");
  params.push_back(" MAXTHRUST = 100 ");
  params.push_back(" DEPTH_CONTROL = FALSE ");

  configure(engine, params);
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 4.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 15.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredThrust(), 12.0, kGeomTol);
  EXPECT_FALSE(engine.hasDepthControl());
}

// Covers PID engine config behavior: legacy yaw ki limit alias configures integral limit.
TEST(PIDEngineConfigTest, LegacyYawKiLimitAliasConfiguresIntegralLimit)
{
  PIDEngine engine;
  std::list<std::string> params;
  params.push_back("yaw_pid_kp = 1.0");
  params.push_back("yaw_pid_kd = 0.0");
  params.push_back("yaw_pid_ki = 0.0");
  params.push_back("yaw_pid_ki_limit = 7.5");
  params.push_back("maxrudder = 50");

  EXPECT_TRUE(engine.setConfigParams(params).empty());
  EXPECT_TRUE(engine.handleYawSettings());

  std::string summary = joined(engine.getConfigSummary("yaw"));
  EXPECT_NE(summary.find("YAW_PID_INTEGRAL_LIMIT = 7.5"), std::string::npos);
}

// Covers PID engine config behavior: malformed numeric settings fall back to zero gain behavior.
TEST(PIDEngineConfigTest, MalformedNumericSettingsFallBackToZeroGainBehavior)
{
  PIDEngine engine;
  // Bad numeric strings parse to zero for gains/factors, while bad max limits
  // fall back to the default actuator caps.
  std::list<std::string> params;
  params.push_back("tardy_helm_threshold = 30");
  params.push_back("tardy_nav_threshold = 30");
  params.push_back("yaw_pid_kp = bad");
  params.push_back("yaw_pid_kd = 0.0");
  params.push_back("yaw_pid_ki = 0.0");
  params.push_back("yaw_pid_integral_limit = 100");
  params.push_back("maxrudder = bad");
  params.push_back("speed_factor = 0");
  params.push_back("speed_pid_kp = bad");
  params.push_back("speed_pid_kd = 0.0");
  params.push_back("speed_pid_ki = 0.0");
  params.push_back("speed_pid_integral_limit = 100");
  params.push_back("maxthrust = bad");
  params.push_back("depth_control = false");

  EXPECT_TRUE(engine.setConfigParams(params).empty());
  EXPECT_TRUE(engine.handleYawSettings());
  EXPECT_TRUE(engine.handleSpeedSettings());
  EXPECT_TRUE(engine.handleDepthSettings());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 4.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
  EXPECT_DOUBLE_EQ(engine.getMaxRudder(), 100.0);
  EXPECT_DOUBLE_EQ(engine.getMaxThrust(), 100.0);
}

// Covers PID engine config behavior: malformed simulation value still uses staleness checks.
TEST(PIDEngineConfigTest, MalformedSimulationValueStillUsesStalenessChecks)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "simulation = maybe",
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.updateTime(3.5);
  engine.setDesiredValues();

  EXPECT_TRUE(engine.hasPIDStale());
  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_TRUE(containsTextPosting(engine.getPostings(), "PID_STALE", "Stale DesHdg"));
}

// Covers PID engine config behavior: negative speed factor preserves previous configured factor.
TEST(PIDEngineConfigTest, NegativeSpeedFactorPreservesPreviousConfiguredFactor)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {"speed_factor = -20"}));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 3.0, 1.0);

  engine.setDesiredValues();

  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), 20.0);
  EXPECT_NEAR(engine.getDesiredThrust(), 60.0, kGeomTol);
}

// Covers PID engine config behavior: missing PID settings keep default zero gains.
TEST(PIDEngineConfigTest, MissingPidSettingsKeepDefaultZeroGains)
{
  PIDEngine engine;
  std::list<std::string> params;
  params.push_back("maxrudder = 50");
  params.push_back("maxthrust = 25");
  params.push_back("depth_control = false");

  EXPECT_TRUE(engine.setConfigParams(params).empty());
  EXPECT_TRUE(engine.handleYawSettings());
  EXPECT_TRUE(engine.handleSpeedSettings());
  EXPECT_TRUE(engine.handleDepthSettings());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 3.0, 1.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
  std::string all = joined(engine.getConfigSummaryAll());
  EXPECT_NE(all.find("YAW_PID_KP             = 0"), std::string::npos);
  EXPECT_NE(all.find("SPEED_PID_KP             = 0"), std::string::npos);
}

// Covers PID engine config behavior: depth disabled skips missing depth and pitch settings.
TEST(PIDEngineConfigTest, DepthDisabledSkipsMissingDepthAndPitchSettings)
{
  PIDEngine engine;
  std::list<std::string> params;
  params.push_back("depth_control = false");

  EXPECT_TRUE(engine.setConfigParams(params).empty());
  EXPECT_TRUE(engine.handleDepthSettings());
  EXPECT_FALSE(engine.hasDepthControl());
}

// Covers PID engine config behavior: rejects malformed depth control value.
TEST(PIDEngineConfigTest, RejectsMalformedDepthControlValue)
{
  PIDEngine engine;
  std::list<std::string> params;
  params.push_back("depth_control = sometimes");

  EXPECT_TRUE(engine.setConfigParams(params).empty());
  EXPECT_FALSE(engine.handleDepthSettings());
}

// Covers PID engine config behavior: converts max pitch config from degrees to radians.
TEST(PIDEngineConfigTest, ConvertsMaxPitchConfigFromDegreesToRadians)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));

  EXPECT_TRUE(engine.hasDepthControl());
  EXPECT_NEAR(engine.getMaxPitch(), 15.0 * M_PI / 180.0, kLooseGeomTol);
  EXPECT_DOUBLE_EQ(engine.getMaxElevator(), 13.0);
}

// Covers PID engine config behavior: unknown summary type returns empty vector.
TEST(PIDEngineConfigTest, UnknownSummaryTypeReturnsEmptyVector)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));

  EXPECT_TRUE(engine.getConfigSummary("bogus").empty());
}

// Covers PID engine config behavior: missing depth controller settings leave zero gains and default limits.
TEST(PIDEngineConfigTest, MissingDepthControllerSettingsLeaveZeroGainsAndDefaultLimits)
{
  PIDEngine engine;
  std::list<std::string> params = basePidConfig(false);
  params.remove("depth_control = false");
  params.push_back("depth_control = true");

  EXPECT_TRUE(engine.setConfigParams(params).empty());
  EXPECT_TRUE(engine.handleYawSettings());
  EXPECT_TRUE(engine.handleSpeedSettings());
  EXPECT_TRUE(engine.handleDepthSettings());
  EXPECT_TRUE(engine.hasDepthControl());
  EXPECT_DOUBLE_EQ(engine.getMaxPitch(), 15.0);
  EXPECT_DOUBLE_EQ(engine.getMaxElevator(), 13.0);

  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  seedDepthInputs(engine, 5.0, 10.0, 0.0);
  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), 0.0, kGeomTol);
}

// Covers PID engine config behavior: depth config summaries expose pitch and depth controllers.
TEST(PIDEngineConfigTest, DepthConfigSummariesExposePitchAndDepthControllers)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(true), {
      "z_to_pitch_pid_kp = 0.25",
      "z_to_pitch_pid_kd = 0.5",
      "z_to_pitch_pid_ki = 0.75",
      "z_to_pitch_pid_integral_limit = 1.25",
      "pitch_pid_kp = 2.5",
      "pitch_pid_kd = 3.5",
      "pitch_pid_ki = 4.5",
      "pitch_pid_integral_limit = 5.5",
  }));

  std::string pitch = joined(engine.getConfigSummary("pitch"));
  EXPECT_NE(pitch.find("PITCH_PID_KP             = 2.5"), std::string::npos);
  EXPECT_NE(pitch.find("PITCH_PID_KD             = 3.5"), std::string::npos);
  EXPECT_NE(pitch.find("PITCH_PID_KI             = 4.5"), std::string::npos);
  EXPECT_NE(pitch.find("PITCH_PID_INTEGRAL_LIMIT = 5.5"), std::string::npos);

  std::string zpitch = joined(engine.getConfigSummary("zpitch"));
  EXPECT_NE(zpitch.find("Z_TO_PITCH_PID_KP             = 0.25"), std::string::npos);
  EXPECT_NE(zpitch.find("Z_TO_PITCH_PID_KD             = 0.5"), std::string::npos);
  EXPECT_NE(zpitch.find("Z_TO_PITCH_PID_KI             = 0.75"), std::string::npos);
  EXPECT_NE(zpitch.find("Z_TO_PITCH_PID_INTEGRAL_LIMIT = 1.25"), std::string::npos);
}

// Covers PID engine override behavior: string override transitions publish control state.
TEST(PIDEngineOverrideTest, StringOverrideTransitionsPublishControlState)
{
  PIDEngine engine;

  engine.setPIDOverride(std::string("false"));
  EXPECT_FALSE(engine.hasPIDOverride());
  ASSERT_EQ(engine.getPostings().size(), 1u);
  EXPECT_EQ(engine.getPostings()[0].get_var(), "PID_HAS_CONTROL");
  EXPECT_EQ(engine.getPostings()[0].get_sdata(), "true");

  engine.setPIDOverride(std::string("false"));
  EXPECT_FALSE(engine.hasPIDOverride());
  EXPECT_EQ(engine.getPostings().size(), 2u);

  engine.setPIDOverride(std::string("true"));
  EXPECT_TRUE(engine.hasPIDOverride());
  ASSERT_EQ(engine.getPostings().size(), 3u);
  EXPECT_EQ(engine.getPostings()[2].get_var(), "PID_HAS_CONTROL");
  EXPECT_EQ(engine.getPostings()[2].get_sdata(), "false");

  engine.setPIDOverride(std::string("nonsense"));
  EXPECT_TRUE(engine.hasPIDOverride());
  EXPECT_EQ(engine.getPostings().size(), 3u);
}

// Covers PID engine override behavior: override flag does not by itself gate engine computation.
TEST(PIDEngineOverrideTest, OverrideFlagDoesNotByItselfGateEngineComputation)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  ASSERT_TRUE(engine.hasPIDOverride());
  seedSurfaceInputs(engine);

  engine.setDesiredValues();

  EXPECT_TRUE(engine.hasPIDOverride());
  EXPECT_FALSE(engine.hasControl());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);
}

// Covers PID engine override behavior: string override accepts mixed case boolean mail.
TEST(PIDEngineOverrideTest, StringOverrideAcceptsMixedCaseBooleanMail)
{
  PIDEngine engine;

  engine.setPIDOverride(std::string("FaLsE"));
  EXPECT_FALSE(engine.hasPIDOverride());
  ASSERT_EQ(engine.getPostings().size(), 1u);
  EXPECT_EQ(engine.getPostings()[0].get_var(), "PID_HAS_CONTROL");
  EXPECT_EQ(engine.getPostings()[0].get_sdata(), "true");

  engine.setPIDOverride(std::string("TrUe"));
  EXPECT_TRUE(engine.hasPIDOverride());
  ASSERT_EQ(engine.getPostings().size(), 2u);
  EXPECT_EQ(engine.getPostings()[1].get_var(), "PID_HAS_CONTROL");
  EXPECT_EQ(engine.getPostings()[1].get_sdata(), "false");
}

// Covers PID engine override behavior: boolean override setter changes control without posting.
TEST(PIDEngineOverrideTest, BooleanOverrideSetterChangesControlWithoutPosting)
{
  PIDEngine engine;

  engine.setPIDOverride(false);
  EXPECT_FALSE(engine.hasPIDOverride());
  EXPECT_TRUE(engine.getPostings().empty());

  engine.setPIDOverride(true);
  EXPECT_TRUE(engine.hasPIDOverride());
  EXPECT_TRUE(engine.getPostings().empty());
}

// Covers PID engine override behavior: releasing override resets input timestamps until fresh mail arrives.
TEST(PIDEngineOverrideTest, ReleasingOverrideResetsInputTimestampsUntilFreshMailArrives)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(std::string("false"));
  seedSurfaceInputs(engine);
  engine.setDesiredValues();
  ASSERT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);

  engine.setPIDOverride(std::string("true"));
  engine.setPIDOverride(std::string("false"));
  engine.setDesiredValues();
  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_FALSE(engine.hasPIDStale());

  seedSurfaceInputs(engine, 2.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_FALSE(engine.hasPIDStale());
}

// Covers PID engine output behavior: runtime speed factor setter overrides configured factor.
TEST(PIDEngineOutputTest, RuntimeSpeedFactorSetterOverridesConfiguredFactor)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  engine.setSpeedFactor(7.5);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 4.0, 0.0);

  engine.setDesiredValues();

  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), 7.5);
  EXPECT_NEAR(engine.getDesiredThrust(), 30.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: inputs stamped at zero are still treated as missing.
TEST(PIDEngineOutputTest, InputsStampedAtZeroAreStillTreatedAsMissing)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 0.0, 90.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();

  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_TRUE(engine.hasPIDStale());

  seedSurfaceInputs(engine, 0.001, 90.0, 80.0, 2.0, 0.0);
  engine.setDesiredValues();
  EXPECT_FALSE(engine.hasPIDStale());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);
}

// Covers PID engine output behavior: waits for each initial surface input.
TEST(PIDEngineOutputTest, WaitsForEachInitialSurfaceInput)
{
  const std::vector<std::string> missing_inputs = {
      "desired_heading",
      "desired_speed",
      "current_heading",
      "current_speed",
  };

  for(const std::string& missing : missing_inputs) {
    SCOPED_TRACE(missing);
    PIDEngine engine;
    configure(engine, basePidConfig());
    engine.setPIDOverride(false);
    engine.updateTime(1.0);
    if(missing != "desired_heading")
      engine.setDesHeading(90.0);
    if(missing != "desired_speed")
      engine.setDesSpeed(2.0);
    if(missing != "current_heading")
      engine.setCurrHeading(80.0);
    if(missing != "current_speed")
      engine.setCurrSpeed(0.0);

    engine.setDesiredValues();

    EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
    EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
    EXPECT_TRUE(engine.hasPIDStale());
  }
}

// Covers PID engine output behavior: waits for initial required surface inputs.
TEST(PIDEngineOutputTest, WaitsForInitialRequiredSurfaceInputs)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);

  engine.updateTime(1.0);
  engine.setDesHeading(90.0);
  engine.setDesSpeed(2.0);
  engine.setCurrHeading(80.0);
  engine.setDesiredValues();

  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_TRUE(engine.hasPIDStale());
}

// Covers PID engine output behavior: waits for initial depth inputs when depth control is enabled.
TEST(PIDEngineOutputTest, WaitsForInitialDepthInputsWhenDepthControlIsEnabled)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);
  engine.setDesDepth(5.0);
  engine.setCurrDepth(10.0);

  engine.setDesiredValues();

  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredElevator(), 0.0);
  EXPECT_TRUE(engine.hasPIDStale());
}

// Covers PID engine output behavior: waits for each initial depth input when depth control is enabled.
TEST(PIDEngineOutputTest, WaitsForEachInitialDepthInputWhenDepthControlIsEnabled)
{
  const std::vector<std::string> missing_inputs = {
      "desired_depth",
      "current_depth",
      "current_pitch",
  };

  for(const std::string& missing : missing_inputs) {
    SCOPED_TRACE(missing);
    PIDEngine engine;
    configure(engine, basePidConfig(true));
    engine.setPIDOverride(false);
    seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);
    if(missing != "desired_depth")
      engine.setDesDepth(5.0);
    if(missing != "current_depth")
      engine.setCurrDepth(10.0);
    if(missing != "current_pitch")
      engine.setCurrPitch(0.0);

    engine.setDesiredValues();

    EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
    EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
    EXPECT_DOUBLE_EQ(engine.getDesiredElevator(), 0.0);
    EXPECT_TRUE(engine.hasPIDStale());
  }
}

// Covers PID engine output behavior: computes mission style rudder and speed factor thrust.
TEST(PIDEngineOutputTest, ComputesMissionStyleRudderAndSpeedFactorThrust)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine);

  engine.setDesiredValues();

  EXPECT_FALSE(engine.hasPIDStale());
  EXPECT_TRUE(engine.hasControl());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: wraps desired heading before computing rudder.
TEST(PIDEngineOutputTest, WrapsDesiredHeadingBeforeComputingRudder)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 350.0, 10.0, 2.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), -20.0, kGeomTol);
}

// Covers PID engine output behavior: normalizes desired and current heading across wrap.
TEST(PIDEngineOutputTest, NormalizesDesiredAndCurrentHeadingAcrossWrap)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 450.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);

  seedSurfaceInputs(engine, 2.0, 350.0, 370.0, 2.0, 0.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), -20.0, kGeomTol);
}

// Covers PID engine output behavior: clips rudder to configured max without debug enabled.
TEST(PIDEngineOutputTest, ClipsRudderToConfiguredMaxWithoutDebugEnabled)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "yaw_pid_kp = 3",
      "maxrudder = 12",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 12.0, kGeomTol);
  EXPECT_EQ(findPosting(engine.getPostings(), "PID_HDG_DEBUG"), nullptr);
}

// Covers PID engine output behavior: rudder bias applies sim instability sawtooth.
TEST(PIDEngineOutputTest, RudderBiasAppliesSimInstabilitySawtooth)
{
  PIDEngine engine;
  // sim_instability injects a time-based sawtooth rudder bias used by simulator
  // stress missions; yaw gain is zero here so the bias is isolated.
  configure(engine, withExtra(basePidConfig(), {
      "sim_instability = 10",
      "yaw_pid_kp = 0",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 5.0, 90.0, 90.0, 2.0, 0.0);

  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), 5.0, kGeomTol);

  seedSurfaceInputs(engine, 11.0, 90.0, 90.0, 2.0, 0.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);

  seedSurfaceInputs(engine, 16.0, 90.0, 90.0, 2.0, 0.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), -5.0, kGeomTol);
}

// Covers PID engine output behavior: rudder bias does not flIP at exact duration boundary.
TEST(PIDEngineOutputTest, RudderBiasDoesNotFlipAtExactDurationBoundary)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "sim_instability = 10",
      "yaw_pid_kp = 0",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 10.0, 90.0, 90.0, 2.0, 0.0);

  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);

  seedSurfaceInputs(engine, 10.001, 90.0, 90.0, 2.0, 0.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kLooseGeomTol);
}

// Covers PID engine output behavior: rudder bias clips negative and overlarge instability config.
TEST(PIDEngineOutputTest, RudderBiasClipsNegativeAndOverlargeInstabilityConfig)
{
  PIDEngine negative;
  configure(negative, withExtra(basePidConfig(), {
      "sim_instability = -10",
      "yaw_pid_kp = 0",
  }));
  negative.setPIDOverride(false);
  seedSurfaceInputs(negative, 5.0, 90.0, 90.0, 2.0, 0.0);

  negative.setDesiredValues();
  EXPECT_NEAR(negative.getDesiredRudder(), 0.0, kGeomTol);

  PIDEngine overlarge;
  configure(overlarge, withExtra(basePidConfig(), {
      "sim_instability = 150",
      "yaw_pid_kp = 0",
  }));
  overlarge.setPIDOverride(false);
  seedSurfaceInputs(overlarge, 5.0, 90.0, 90.0, 2.0, 0.0);

  overlarge.setDesiredValues();
  EXPECT_NEAR(overlarge.getDesiredRudder(), 50.0, kGeomTol);
}

// Covers PID engine output behavior: gates rudder and elevator when desired thrust is zero.
TEST(PIDEngineOutputTest, GatesRudderAndElevatorWhenDesiredThrustIsZero)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 0.0, 0.0);
  seedDepthInputs(engine);

  engine.setDesiredValues();

  EXPECT_FALSE(engine.hasPIDStale());
  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredElevator(), 0.0);
}

// Covers PID engine output behavior: caps speed factor thrust and clips negative speeds to zero.
TEST(PIDEngineOutputTest, CapsSpeedFactorThrustAndClipsNegativeSpeedsToZero)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 20.0, 0.0);

  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredThrust(), 100.0, kGeomTol);

  seedSurfaceInputs(engine, 2.0, 90.0, 90.0, -2.0, 0.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: tiny positive thrust below cutoff gates steering.
TEST(PIDEngineOutputTest, TinyPositiveThrustBelowCutoffGatesSteering)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {"speed_factor = 0.001"}));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 9.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: runtime negative speed factor suppresses thrust and steering.
TEST(PIDEngineOutputTest, RuntimeNegativeSpeedFactorSuppressesThrustAndSteering)
{
  PIDEngine engine;
  configure(engine, basePidConfig());
  engine.setPIDOverride(false);
  engine.setSpeedFactor(-20.0);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();

  EXPECT_DOUBLE_EQ(engine.getSpeedFactor(), -20.0);
  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: speed PID mode does not publish reverse thrust.
TEST(PIDEngineOutputTest, SpeedPidModeDoesNotPublishReverseThrust)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {"speed_factor = 0"}));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 0.0, 3.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: speed PID mode can back off accumulated thrust to zero.
TEST(PIDEngineOutputTest, SpeedPidModeCanBackOffAccumulatedThrustToZero)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {"speed_factor = 0"}));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 3.0, 1.0);
  engine.setDesiredValues();
  ASSERT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);

  seedSurfaceInputs(engine, 2.0, 90.0, 90.0, 0.0, 3.0);
  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: speed PID mode accumulates delta thrust across iterations.
TEST(PIDEngineOutputTest, SpeedPidModeAccumulatesDeltaThrustAcrossIterations)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {"speed_factor = 0"}));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 3.0, 1.0);

  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);

  seedSurfaceInputs(engine, 2.0, 90.0, 90.0, 3.0, 1.0);
  engine.setDesiredValues();
  EXPECT_NEAR(engine.getDesiredThrust(), 80.0, kGeomTol);
}

// Covers PID engine output behavior: depth control produces elevator from depth and pitch error.
TEST(PIDEngineOutputTest, DepthControlProducesElevatorFromDepthAndPitchError)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  seedDepthInputs(engine, 5.0, 10.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), -13.0, kLooseGeomTol);
}

// Covers PID engine output behavior: depth control commands positive elevator when too shallow.
TEST(PIDEngineOutputTest, DepthControlCommandsPositiveElevatorWhenTooShallow)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  seedDepthInputs(engine, 10.0, 5.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), 13.0, kLooseGeomTol);
}

// Covers PID engine output behavior: current pitch contributes directly to elevator command.
TEST(PIDEngineOutputTest, CurrentPitchContributesDirectlyToElevatorCommand)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  seedDepthInputs(engine, 5.0, 5.0, 0.1);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), 0.1 * 180.0 / M_PI, kLooseGeomTol);
}

// Covers PID engine output behavior: depth control disabled ignores depth mail and never publishes elevator.
TEST(PIDEngineOutputTest, DepthControlDisabledIgnoresDepthMailAndNeverPublishesElevator)
{
  PIDEngine engine;
  configure(engine, basePidConfig(false));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  seedDepthInputs(engine, 5.0, 100.0, -20.0);

  engine.setDesiredValues();

  EXPECT_FALSE(engine.hasDepthControl());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), 0.0, kGeomTol);
}

// Covers PID engine output behavior: runtime depth control setter bypasses depth requirements.
TEST(PIDEngineOutputTest, RuntimeDepthControlSetterBypassesDepthRequirements)
{
  PIDEngine engine;
  configure(engine, basePidConfig(true));
  engine.setPIDOverride(false);
  engine.setDepthControl(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();

  EXPECT_FALSE(engine.hasDepthControl());
  EXPECT_FALSE(engine.hasPIDStale());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredElevator(), 0.0, kGeomTol);
}

// Covers PID engine staleness behavior: reports stale surface inputs and suppresses outputs.
TEST(PIDEngineStalenessTest, ReportsStaleSurfaceInputsAndSuppressesOutputs)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.updateTime(3.5);
  engine.setDesiredValues();

  EXPECT_TRUE(engine.hasPIDStale());
  EXPECT_FALSE(engine.hasControl());
  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  std::vector<VarDataPair> postings = engine.getPostings();
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale DesHdg"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale DesSpd"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale NavHdg"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale NavSpd"));
  EXPECT_FALSE(containsTextPosting(postings, "PID_STALE", "Stale DesDepth"));
}

// Covers PID engine staleness behavior: isolates helm and nav staleness with different thresholds.
TEST(PIDEngineStalenessTest, IsolatesHelmAndNavStalenessWithDifferentThresholds)
{
  // Helm desired values and navigation feedback age independently, so each
  // threshold should report only its own stale side.
  PIDEngine helm_stale;
  configure(helm_stale, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 100",
  }));
  helm_stale.setPIDOverride(false);
  helm_stale.updateTime(1.0);
  helm_stale.setDesHeading(90.0);
  helm_stale.setDesSpeed(2.0);
  helm_stale.updateTime(3.0);
  helm_stale.setCurrHeading(80.0);
  helm_stale.setCurrSpeed(0.0);
  helm_stale.updateTime(3.5);

  helm_stale.setDesiredValues();
  std::vector<VarDataPair> helm_postings = helm_stale.getPostings();
  EXPECT_TRUE(containsTextPosting(helm_postings, "PID_STALE", "Stale DesHdg"));
  EXPECT_TRUE(containsTextPosting(helm_postings, "PID_STALE", "Stale DesSpd"));
  EXPECT_FALSE(containsTextPosting(helm_postings, "PID_STALE", "Stale NavHdg"));
  EXPECT_FALSE(containsTextPosting(helm_postings, "PID_STALE", "Stale NavSpd"));

  PIDEngine nav_stale;
  configure(nav_stale, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 100",
      "tardy_nav_threshold = 1",
  }));
  nav_stale.setPIDOverride(false);
  nav_stale.updateTime(1.0);
  nav_stale.setCurrHeading(80.0);
  nav_stale.setCurrSpeed(0.0);
  nav_stale.updateTime(3.0);
  nav_stale.setDesHeading(90.0);
  nav_stale.setDesSpeed(2.0);
  nav_stale.updateTime(3.5);

  nav_stale.setDesiredValues();
  std::vector<VarDataPair> nav_postings = nav_stale.getPostings();
  EXPECT_FALSE(containsTextPosting(nav_postings, "PID_STALE", "Stale DesHdg"));
  EXPECT_FALSE(containsTextPosting(nav_postings, "PID_STALE", "Stale DesSpd"));
  EXPECT_TRUE(containsTextPosting(nav_postings, "PID_STALE", "Stale NavHdg"));
  EXPECT_TRUE(containsTextPosting(nav_postings, "PID_STALE", "Stale NavSpd"));
}

// Covers PID engine staleness behavior: direct staleness check only posts diagnostics and control flag.
TEST(PIDEngineStalenessTest, DirectStalenessCheckOnlyPostsDiagnosticsAndControlFlag)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.updateTime(3.5);
  engine.checkForStaleness();

  EXPECT_TRUE(engine.hasPIDStale());
  EXPECT_FALSE(engine.hasControl());
  EXPECT_DOUBLE_EQ(engine.getDesiredThrust(), 0.0);
  EXPECT_DOUBLE_EQ(engine.getDesiredRudder(), 0.0);
  EXPECT_TRUE(containsTextPosting(engine.getPostings(), "PID_STALE", "Stale DesHdg"));
}

// Covers PID engine staleness behavior: repeated staleness checks append diagnostics until cleared.
TEST(PIDEngineStalenessTest, RepeatedStalenessChecksAppendDiagnosticsUntilCleared)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);
  engine.updateTime(3.5);

  engine.checkForStaleness();
  engine.checkForStaleness();

  EXPECT_EQ(postingsFor(engine.getPostings(), "PID_STALE").size(), 8u);
  engine.clearPostings();
  EXPECT_TRUE(engine.getPostings().empty());
}

// Covers PID engine staleness behavior: threshold boundary is fresh until delta exceeds limit.
TEST(PIDEngineStalenessTest, ThresholdBoundaryIsFreshUntilDeltaExceedsLimit)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.updateTime(2.0);
  engine.setDesiredValues();
  EXPECT_FALSE(engine.hasPIDStale());

  // The boundary is strict: exactly threshold seconds is fresh; any excess is stale.
  engine.updateTime(2.001);
  engine.setDesiredValues();
  EXPECT_TRUE(engine.hasPIDStale());
}

// Covers PID engine staleness behavior: negative thresholds clip to zero.
TEST(PIDEngineStalenessTest, NegativeThresholdsClipToZero)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = -5",
      "tardy_nav_threshold = -5",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();
  EXPECT_FALSE(engine.hasPIDStale());

  engine.updateTime(1.001);
  engine.setDesiredValues();
  EXPECT_TRUE(engine.hasPIDStale());
}

// Covers PID engine staleness behavior: backward time deltas clear prior stale state.
TEST(PIDEngineStalenessTest, BackwardTimeDeltasClearPriorStaleState)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);
  engine.updateTime(3.5);
  engine.checkForStaleness();
  ASSERT_TRUE(engine.hasPIDStale());

  engine.updateTime(0.5);
  engine.checkForStaleness();

  // Moving time backward clears the stale flag but leaves the earlier diagnostic
  // postings in the pending output queue.
  EXPECT_FALSE(engine.hasPIDStale());
  EXPECT_TRUE(containsTextPosting(engine.getPostings(), "PID_STALE", "Stale DesHdg"));
}

// Covers PID engine staleness behavior: reports depth specific staleness when depth control is enabled.
TEST(PIDEngineStalenessTest, ReportsDepthSpecificStalenessWhenDepthControlIsEnabled)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(true), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0);
  seedDepthInputs(engine);

  engine.updateTime(3.5);
  engine.setDesiredValues();

  std::vector<VarDataPair> postings = engine.getPostings();
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale DesDepth"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale NavDep"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_STALE", "Stale NavPitch"));
}

// Covers PID engine staleness behavior: simulation mode bypasses staleness checks.
TEST(PIDEngineStalenessTest, SimulationModeBypassesStalenessChecks)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "simulation = true",
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.updateTime(3.5);
  engine.setDesiredValues();

  // Simulation mode skips checkForStaleness(), so outputs are produced but the
  // stale flag remains at its previous value.
  EXPECT_TRUE(engine.hasPIDStale());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_NEAR(engine.getDesiredRudder(), 10.0, kGeomTol);
  EXPECT_EQ(findPosting(engine.getPostings(), "PID_STALE"), nullptr);
}

// Covers PID engine postings behavior: adds numeric and string postings and can clear them.
TEST(PIDEnginePostingsTest, AddsNumericAndStringPostingsAndCanClearThem)
{
  PIDEngine engine;

  engine.addPosting("DESIRED_RUDDER", 12.5);
  engine.addPosting("PID_HAS_CONTROL", "true");

  std::vector<VarDataPair> postings = engine.getPostings();
  ASSERT_EQ(postings.size(), 2u);
  EXPECT_EQ(postings[0].get_var(), "DESIRED_RUDDER");
  EXPECT_DOUBLE_EQ(postings[0].get_ddata(), 12.5);
  EXPECT_EQ(postings[1].get_var(), "PID_HAS_CONTROL");
  EXPECT_EQ(postings[1].get_sdata(), "true");

  engine.clearPostings();
  EXPECT_TRUE(engine.getPostings().empty());
}

// Covers PID engine staleness behavior: fresh inputs recover from prior stale state.
TEST(PIDEngineStalenessTest, FreshInputsRecoverFromPriorStaleState)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "tardy_helm_threshold = 1",
      "tardy_nav_threshold = 1",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);
  engine.updateTime(3.5);
  engine.setDesiredValues();
  ASSERT_TRUE(engine.hasPIDStale());

  engine.clearPostings();
  seedSurfaceInputs(engine, 4.0, 90.0, 80.0, 2.0, 0.0);
  engine.setDesiredValues();

  EXPECT_FALSE(engine.hasPIDStale());
  EXPECT_TRUE(engine.hasControl());
  EXPECT_NEAR(engine.getDesiredThrust(), 40.0, kGeomTol);
  EXPECT_EQ(findPosting(engine.getPostings(), "PID_STALE"), nullptr);
}

// Covers PID engine diagnostics behavior: computes frequency from set desired values calls.
TEST(PIDEngineDiagnosticsTest, ComputesFrequencyFromSetDesiredValuesCalls)
{
  PIDEngine engine;
  engine.setStartTime(10.0);
  engine.updateTime(15.0);
  engine.setDesiredValues();
  engine.setDesiredValues();

  EXPECT_NEAR(engine.getFrequency(), 0.4, kGeomTol);
}

// Covers PID engine diagnostics behavior: frequency counts calls that return before inputs arrive.
TEST(PIDEngineDiagnosticsTest, FrequencyCountsCallsThatReturnBeforeInputsArrive)
{
  PIDEngine engine;
  engine.setStartTime(0.0);
  engine.updateTime(10.0);

  engine.setDesiredValues();
  engine.setDesiredValues();
  engine.setDesiredValues();

  EXPECT_NEAR(engine.getFrequency(), 0.3, kGeomTol);
}

// Covers PID engine diagnostics behavior: frequency is zero until positive elapsed time.
TEST(PIDEngineDiagnosticsTest, FrequencyIsZeroUntilPositiveElapsedTime)
{
  PIDEngine engine;
  engine.setStartTime(10.0);
  engine.updateTime(10.0);
  engine.setDesiredValues();
  EXPECT_DOUBLE_EQ(engine.getFrequency(), 0.0);

  engine.updateTime(9.0);
  engine.setDesiredValues();
  EXPECT_DOUBLE_EQ(engine.getFrequency(), 0.0);
}

// Covers PID engine diagnostics behavior: heading debug publishes summary and saturation evidence.
TEST(PIDEngineDiagnosticsTest, HeadingDebugPublishesSummaryAndSaturationEvidence)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(), {
      "heading_debug = true",
      "yaw_pid_kp = 10",
      "maxrudder = 5",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 80.0, 2.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredRudder(), 5.0, kGeomTol);
  std::vector<VarDataPair> postings = engine.getPostings();
  EXPECT_TRUE(containsTextPosting(postings, "PID_HDG_DEBUG", "PID_COURSE"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_HDG_DEBUG", "RUDDER:5"));
  EXPECT_NE(findPosting(postings, "PID_MAX_SAT_HDG_DEBUG"), nullptr);
}

// Covers PID engine diagnostics behavior: debug modes publish summaries without max sat when inside limits.
TEST(PIDEngineDiagnosticsTest, DebugModesPublishSummariesWithoutMaxSatWhenInsideLimits)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(true), {
      "heading_debug = true",
      "speed_debug = true",
      "depth_debug = true",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 89.0, 1.0, 0.0);
  seedDepthInputs(engine, 5.0, 5.1, 0.0);

  engine.setDesiredValues();

  std::vector<VarDataPair> postings = engine.getPostings();
  EXPECT_TRUE(containsTextPosting(postings, "PID_HDG_DEBUG", "PID_COURSE"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_SPD_DEBUG", "PID_SPEED"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_DEP_DEBUG", "PID_DEPTH"));
  EXPECT_EQ(findPosting(postings, "PID_MAX_SAT_HDG_DEBUG"), nullptr);
  EXPECT_EQ(findPosting(postings, "PID_MAX_SAT_SPD_DEBUG"), nullptr);
  EXPECT_TRUE(postingsFor(postings, "PID_MAX_SAT_DEP_DEBUG").empty());
}

// Covers PID engine diagnostics behavior: speed debug publishes factor and PID mode reports.
TEST(PIDEngineDiagnosticsTest, SpeedDebugPublishesFactorAndPidModeReports)
{
  PIDEngine factor_engine;
  configure(factor_engine, withExtra(basePidConfig(), {"speed_debug = true"}));
  factor_engine.setPIDOverride(false);
  seedSurfaceInputs(factor_engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  factor_engine.setDesiredValues();
  EXPECT_TRUE(containsTextPosting(
      factor_engine.getPostings(), "PID_SPD_DEBUG", "(Fctr):20"));

  PIDEngine pid_engine;
  configure(pid_engine, withExtra(basePidConfig(), {
      "speed_factor = 0",
      "speed_debug = true",
      "speed_pid_kp = 100",
      "maxthrust = 10",
  }));
  pid_engine.setPIDOverride(false);
  seedSurfaceInputs(pid_engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  pid_engine.setDesiredValues();

  std::vector<VarDataPair> postings = pid_engine.getPostings();
  EXPECT_TRUE(containsTextPosting(postings, "PID_SPD_DEBUG", "(Delt):10"));
  EXPECT_NE(findPosting(postings, "PID_MAX_SAT_SPD_DEBUG"), nullptr);
}

// Covers PID engine diagnostics behavior: depth debug publishes summary and saturation evidence.
TEST(PIDEngineDiagnosticsTest, DepthDebugPublishesSummaryAndSaturationEvidence)
{
  PIDEngine engine;
  configure(engine, withExtra(basePidConfig(true), {
      "depth_debug = true",
      "z_to_pitch_pid_kp = 10",
      "pitch_pid_kp = 10",
      "maxpitch = 5",
      "maxelevator = 3",
  }));
  engine.setPIDOverride(false);
  seedSurfaceInputs(engine, 1.0, 90.0, 90.0, 2.0, 0.0);
  seedDepthInputs(engine, 5.0, 10.0, 0.0);

  engine.setDesiredValues();

  EXPECT_NEAR(engine.getDesiredElevator(), -3.0, kLooseGeomTol);
  std::vector<VarDataPair> postings = engine.getPostings();
  EXPECT_TRUE(containsTextPosting(postings, "PID_DEP_DEBUG", "PID_DEPTH"));
  EXPECT_TRUE(containsTextPosting(postings, "PID_DEP_DEBUG", "ELEVATOR:-3"));
  EXPECT_FALSE(postingsFor(postings, "PID_MAX_SAT_DEP_DEBUG").empty());
}
