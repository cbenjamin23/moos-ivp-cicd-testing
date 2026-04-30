#include <gtest/gtest.h>

#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "NumericAssertions.h"
#include "ScalarPID.h"
#include "TestFileUtils.h"

namespace {

std::vector<std::string> listFiles(const std::string& dir)
{
  std::vector<std::string> files;
  DIR* handle = opendir(dir.c_str());
  if(handle == nullptr)
    return files;

  while(dirent* entry = readdir(handle)) {
    std::string name = entry->d_name;
    if(name != "." && name != "..")
      files.push_back(name);
  }
  closedir(handle);
  return files;
}

std::string readFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

// Covers scalar PID behavior: default and configured controllers expose gains and limits.
TEST(ScalarPIDTest, DefaultAndConfiguredControllersExposeGainsAndLimits)
{
  ScalarPID unset;
  EXPECT_DOUBLE_EQ(unset.getKP(), 0.0);
  EXPECT_DOUBLE_EQ(unset.getKD(), 0.0);
  EXPECT_DOUBLE_EQ(unset.getKI(), 0.0);
  EXPECT_DOUBLE_EQ(unset.getIL(), 0.0);
  EXPECT_FALSE(unset.getDebug());
  EXPECT_FALSE(unset.getMaxSat());

  ScalarPID pid(1.2, 0.3, 0.4, 5.5, 20.0);
  EXPECT_DOUBLE_EQ(pid.getKP(), 1.2);
  EXPECT_DOUBLE_EQ(pid.getKD(), 0.3);
  EXPECT_DOUBLE_EQ(pid.getKI(), 0.4);
  EXPECT_DOUBLE_EQ(pid.getIL(), 5.5);

  pid.SetGains(2.0, 3.0, 4.0);
  pid.SetLimits(6.0, 7.0);
  EXPECT_DOUBLE_EQ(pid.getKP(), 2.0);
  EXPECT_DOUBLE_EQ(pid.getKD(), 3.0);
  EXPECT_DOUBLE_EQ(pid.getKI(), 4.0);
  EXPECT_DOUBLE_EQ(pid.getIL(), 6.0);
}

// Covers scalar PID behavior: first run uses proportional only and stores output.
TEST(ScalarPIDTest, FirstRunUsesProportionalOnlyAndStoresOutput)
{
  ScalarPID pid(2.0, 10.0, 4.0, 100.0, 100.0);
  double output = -1.0;

  ASSERT_TRUE(pid.Run(3.0, 10.0, output));
  EXPECT_NEAR(output, 6.0, kGeomTol);
  EXPECT_FALSE(pid.getMaxSat());

  ASSERT_TRUE(pid.Run(99.0, 10.0, output));
  EXPECT_NEAR(output, 6.0, kGeomTol);
  EXPECT_FALSE(pid.getMaxSat());
}

// Covers scalar PID behavior: rejects negative time step without replacing last output.
TEST(ScalarPIDTest, RejectsNegativeTimeStepWithoutReplacingLastOutput)
{
  ScalarPID pid(2.0, 0.0, 0.0, 100.0, 100.0);
  double output = 0.0;
  ASSERT_TRUE(pid.Run(4.0, 10.0, output));
  EXPECT_NEAR(output, 8.0, kGeomTol);

  output = 123.0;
  EXPECT_FALSE(pid.Run(8.0, 9.5, output));
  EXPECT_NEAR(output, 123.0, kGeomTol);
}

// Covers scalar PID behavior: averages derivative history across positive time steps.
TEST(ScalarPIDTest, AveragesDerivativeHistoryAcrossPositiveTimeSteps)
{
  ScalarPID pid(0.0, 2.0, 0.0, 100.0, 100.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(0.0, 0.0, output));
  EXPECT_NEAR(output, 0.0, kGeomTol);

  ASSERT_TRUE(pid.Run(10.0, 2.0, output));
  EXPECT_NEAR(output, 10.0, kGeomTol);

  ASSERT_TRUE(pid.Run(14.0, 4.0, output));
  EXPECT_NEAR(output, 7.0, kGeomTol);

  ASSERT_TRUE(pid.Run(20.0, 5.0, output));
  EXPECT_NEAR(output, 8.6666666667, kLooseGeomTol);
}

// Covers scalar PID behavior: keeps nine derivative samples in current history window.
TEST(ScalarPIDTest, KeepsNineDerivativeSamplesInCurrentHistoryWindow)
{
  ScalarPID pid(0.0, 1.0, 0.0, 100.0, 100.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(0.0, 0.0, output));
  for(int i = 1; i <= 10; ++i)
    ASSERT_TRUE(pid.Run(static_cast<double>(i * i), static_cast<double>(i), output));

  // The implementation pops when size reaches 10, so after ten positive
  // samples the average covers the last nine slopes: 3,5,7,...,19.
  EXPECT_NEAR(output, 11.0, kGeomTol);
}

// Covers scalar PID behavior: integrates and clips positive and negative accumulated error.
TEST(ScalarPIDTest, IntegratesAndClipsPositiveAndNegativeAccumulatedError)
{
  ScalarPID pid(0.0, 0.0, 2.0, 5.0, 100.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(10.0, 0.0, output));
  EXPECT_NEAR(output, 0.0, kGeomTol);

  ASSERT_TRUE(pid.Run(10.0, 1.0, output));
  EXPECT_NEAR(output, 5.0, kGeomTol);

  ASSERT_TRUE(pid.Run(-10.0, 2.0, output));
  EXPECT_NEAR(output, -5.0, kGeomTol);
}

// Covers scalar PID behavior: disables integral memory when ki is zero.
TEST(ScalarPIDTest, DisablesIntegralMemoryWhenKiIsZero)
{
  ScalarPID pid(1.0, 0.0, 2.0, 100.0, 100.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(5.0, 0.0, output));
  ASSERT_TRUE(pid.Run(5.0, 1.0, output));
  EXPECT_NEAR(output, 15.0, kGeomTol);

  pid.SetGains(1.0, 0.0, 0.0);
  ASSERT_TRUE(pid.Run(5.0, 2.0, output));
  EXPECT_NEAR(output, 5.0, kGeomTol);
}

// Covers scalar PID behavior: negative ki also clears integral memory.
TEST(ScalarPIDTest, NegativeKiAlsoClearsIntegralMemory)
{
  ScalarPID pid(1.0, 0.0, 2.0, 100.0, 100.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(5.0, 0.0, output));
  ASSERT_TRUE(pid.Run(5.0, 1.0, output));
  EXPECT_NEAR(output, 15.0, kGeomTol);

  pid.SetGains(1.0, 0.0, -1.0);
  ASSERT_TRUE(pid.Run(5.0, 2.0, output));
  EXPECT_NEAR(output, 5.0, kGeomTol);
}

// Covers scalar PID behavior: saturates positive negative and exact limit outputs.
TEST(ScalarPIDTest, SaturatesPositiveNegativeAndExactLimitOutputs)
{
  ScalarPID pid(2.0, 0.0, 0.0, 100.0, 12.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(10.0, 1.0, output));
  EXPECT_NEAR(output, 12.0, kGeomTol);
  EXPECT_TRUE(pid.getMaxSat());

  ASSERT_TRUE(pid.Run(1.0, 2.0, output));
  EXPECT_NEAR(output, 2.0, kGeomTol);
  EXPECT_FALSE(pid.getMaxSat());

  ASSERT_TRUE(pid.Run(-10.0, 3.0, output));
  EXPECT_NEAR(output, -12.0, kGeomTol);
  EXPECT_TRUE(pid.getMaxSat());

  ASSERT_TRUE(pid.Run(6.0, 4.0, output));
  EXPECT_NEAR(output, 12.0, kGeomTol);
  EXPECT_TRUE(pid.getMaxSat());
}

// Covers scalar PID behavior: zero time after saturation returns prior output but clears sat flag.
TEST(ScalarPIDTest, ZeroTimeAfterSaturationReturnsPriorOutputButClearsSatFlag)
{
  ScalarPID pid(2.0, 0.0, 0.0, 100.0, 12.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(10.0, 1.0, output));
  EXPECT_NEAR(output, 12.0, kGeomTol);
  EXPECT_TRUE(pid.getMaxSat());

  output = 0.0;
  ASSERT_TRUE(pid.Run(1.0, 1.0, output));
  EXPECT_NEAR(output, 12.0, kGeomTol);
  EXPECT_FALSE(pid.getMaxSat());
}

// Covers scalar PID behavior: updated limits apply to subsequent runs.
TEST(ScalarPIDTest, UpdatedLimitsApplyToSubsequentRuns)
{
  ScalarPID pid(10.0, 0.0, 0.0, 100.0, 100.0);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(3.0, 1.0, output));
  EXPECT_NEAR(output, 30.0, kGeomTol);
  EXPECT_FALSE(pid.getMaxSat());

  pid.SetLimits(100.0, 20.0);
  ASSERT_TRUE(pid.Run(3.0, 2.0, output));
  EXPECT_NEAR(output, 20.0, kGeomTol);
  EXPECT_TRUE(pid.getMaxSat());
}

// Covers scalar PID behavior: negative and zero output limits saturate by absolute magnitude.
TEST(ScalarPIDTest, NegativeAndZeroOutputLimitsSaturateByAbsoluteMagnitude)
{
  ScalarPID negative_limit(2.0, 0.0, 0.0, 100.0, -12.0);
  double output = 0.0;
  ASSERT_TRUE(negative_limit.Run(10.0, 1.0, output));
  EXPECT_NEAR(output, 12.0, kGeomTol);
  EXPECT_TRUE(negative_limit.getMaxSat());

  ScalarPID zero_limit(2.0, 0.0, 0.0, 100.0, 0.0);
  ASSERT_TRUE(zero_limit.Run(-10.0, 1.0, output));
  EXPECT_NEAR(output, 0.0, kGeomTol);
  EXPECT_TRUE(zero_limit.getMaxSat());
}

// Covers scalar PID behavior: debug string tracks inputs timing derivative and output.
TEST(ScalarPIDTest, DebugStringTracksInputsTimingDerivativeAndOutput)
{
  ScalarPID pid(1.0, 1.0, 0.0, 100.0, 100.0);
  pid.setDebug(true);
  double output = 0.0;

  ASSERT_TRUE(pid.Run(2.0, 4.0, output));
  std::string first = pid.getDebugStr();
  EXPECT_NE(first.find("dfeIn=2"), std::string::npos);
  EXPECT_NE(first.find("dfErrorTime=4"), std::string::npos);
  EXPECT_NE(first.find("m_dfOut(1)2"), std::string::npos);

  ASSERT_TRUE(pid.Run(4.0, 5.0, output));
  std::string second = pid.getDebugStr();
  EXPECT_NE(second.find("m_dfDT=1"), std::string::npos);
  EXPECT_NE(second.find("dfDiffNow=2"), std::string::npos);
  EXPECT_NE(second.find("DiffHistSize=1"), std::string::npos);
}

// Covers scalar PID behavior: copy and assignment preserve tuning but reset runtime state.
TEST(ScalarPIDTest, CopyAndAssignmentPreserveTuningButResetRuntimeState)
{
  ScalarPID original(2.0, 5.0, 1.0, 10.0, 100.0);
  original.setDebug(true);
  double output = 0.0;
  ASSERT_TRUE(original.Run(10.0, 1.0, output));
  ASSERT_TRUE(original.Run(12.0, 2.0, output));
  EXPECT_NEAR(output, 44.0, kGeomTol);

  ScalarPID copied(original);
  EXPECT_DOUBLE_EQ(copied.getKP(), 2.0);
  EXPECT_DOUBLE_EQ(copied.getKD(), 5.0);
  EXPECT_DOUBLE_EQ(copied.getKI(), 1.0);
  EXPECT_DOUBLE_EQ(copied.getIL(), 10.0);
  EXPECT_FALSE(copied.getDebug());
  ASSERT_TRUE(copied.Run(12.0, 20.0, output));
  EXPECT_NEAR(output, 24.0, kGeomTol);

  ScalarPID assigned;
  assigned = original;
  EXPECT_DOUBLE_EQ(assigned.getKP(), 2.0);
  EXPECT_DOUBLE_EQ(assigned.getKD(), 5.0);
  EXPECT_DOUBLE_EQ(assigned.getKI(), 1.0);
  EXPECT_DOUBLE_EQ(assigned.getIL(), 10.0);
  EXPECT_FALSE(assigned.getDebug());
  ASSERT_TRUE(assigned.Run(12.0, 20.0, output));
  EXPECT_NEAR(output, 24.0, kGeomTol);
}

// Covers scalar PID behavior: logging writes named PID file with goal and actual columns.
TEST(ScalarPIDTest, LoggingWritesNamedPidFileWithGoalAndActualColumns)
{
  TempDir dir("scalarpid_log");

  ScalarPID pid(1.0, 0.0, 0.0, 100.0, 100.0);
  std::string name = "unit_pid_";
  // ScalarPID expects a directory-style log path ending in '/', then appends
  // its generated timestamped .pid file name.
  std::string path = dir.path() + "/";
  pid.SetName(name);
  pid.SetGoal(10.0);
  pid.SetLogPath(path);

  double output = 0.0;
  ASSERT_TRUE(pid.Run(2.0, 3.0, output));

  std::vector<std::string> files = listFiles(dir.path());
  ASSERT_EQ(files.size(), 1u);
  EXPECT_EQ(files[0].find(name), 0u);
  EXPECT_NE(files[0].find(".pid"), std::string::npos);

  std::string log_path = path + files[0];
  std::string contents = readFile(log_path);
  EXPECT_NE(contents.find("%% Kp = 1"), std::string::npos);
  EXPECT_NE(contents.find("Desired"), std::string::npos);
  EXPECT_NE(contents.find("Actual"), std::string::npos);
  EXPECT_NE(contents.find("10"), std::string::npos);
  EXPECT_NE(contents.find("8"), std::string::npos);
}

// Covers scalar PID behavior: logging can be disabled after path enables it.
TEST(ScalarPIDTest, LoggingCanBeDisabledAfterPathEnablesIt)
{
  TempDir dir("scalarpid_disabled");

  ScalarPID pid(1.0, 0.0, 0.0, 100.0, 100.0);
  std::string path = dir.path() + "/";
  pid.SetLogPath(path);
  // SetLog(false) must override the earlier SetLogPath() side effect that
  // enabled logging.
  pid.SetLog(false);

  double output = 0.0;
  ASSERT_TRUE(pid.Run(2.0, 3.0, output));
  EXPECT_TRUE(listFiles(dir.path()).empty());
}

// Covers scalar PID behavior: empty log path disables previously enabled logging.
TEST(ScalarPIDTest, EmptyLogPathDisablesPreviouslyEnabledLogging)
{
  TempDir dir("scalarpid_empty_path");

  ScalarPID pid(1.0, 0.0, 0.0, 100.0, 100.0);
  std::string path = dir.path() + "/";
  pid.SetLogPath(path);
  std::string empty;
  // An empty path is the supported way to clear an enabled log destination.
  pid.SetLogPath(empty);

  double output = 0.0;
  ASSERT_TRUE(pid.Run(2.0, 3.0, output));
  EXPECT_NEAR(output, 2.0, kGeomTol);
  EXPECT_TRUE(listFiles(dir.path()).empty());
}

// Covers scalar PID behavior: invalid log path disables logging without failing run.
TEST(ScalarPIDTest, InvalidLogPathDisablesLoggingWithoutFailingRun)
{
  TempDir dir("scalarpid_invalid");
  std::string missing_path = dir.filePath("missing") + "/";

  ScalarPID pid(1.0, 0.0, 0.0, 100.0, 100.0);
  pid.SetLogPath(missing_path);

  double output = 0.0;
  ASSERT_TRUE(pid.Run(2.0, 3.0, output));
  EXPECT_NEAR(output, 2.0, kGeomTol);
  EXPECT_TRUE(listFiles(dir.path()).empty());
}

// Covers scalar PID behavior: self assignment preserves tuning and continues from reset state.
TEST(ScalarPIDTest, SelfAssignmentPreservesTuningAndContinuesFromResetState)
{
  ScalarPID pid(3.0, 4.0, 5.0, 6.0, 100.0);
  const ScalarPID* address = &(pid = pid);

  EXPECT_EQ(address, &pid);
  EXPECT_DOUBLE_EQ(pid.getKP(), 3.0);
  EXPECT_DOUBLE_EQ(pid.getKD(), 4.0);
  EXPECT_DOUBLE_EQ(pid.getKI(), 5.0);
  EXPECT_DOUBLE_EQ(pid.getIL(), 6.0);

  double output = 0.0;
  ASSERT_TRUE(pid.Run(2.0, 10.0, output));
  EXPECT_NEAR(output, 6.0, kGeomTol);
}
