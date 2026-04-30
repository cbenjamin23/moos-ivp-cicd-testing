#include <gtest/gtest.h>

#include "MBTimer.h"

// Covers mb timer behavior: tracks configuration and zero timeout state.
TEST(MBTimerTest, TracksConfigurationAndZeroTimeoutState)
{
  MBTimer timer;
  EXPECT_EQ(timer.getMaxTime(), 0);
  EXPECT_FALSE(timer.isTimedOut());
  EXPECT_FALSE(timer.timeOutCheck());

  timer.reset(5);
  EXPECT_EQ(timer.getMaxTime(), 5);
  EXPECT_FALSE(timer.isTimedOut());
}

// Covers mb timer behavior: reports zero elapsed time before start.
TEST(MBTimerTest, ReportsZeroElapsedTimeBeforeStart)
{
  MBTimer timer;
  EXPECT_EQ(timer.get_wall_time(), 0);
  EXPECT_EQ(timer.get_user_cpu_time(), 0);
  EXPECT_EQ(timer.get_system_cpu_time(), 0);
  EXPECT_EQ(timer.get_cpu_time(), 0);

  EXPECT_EQ(timer.get_wall_time(7), 0);
  EXPECT_EQ(timer.get_cpu_time(7), 0);
}

// Covers mb timer behavior: constructor can set timeout and negative timeout does not trip before start.
TEST(MBTimerTest, ConstructorCanSetTimeoutAndNegativeTimeoutDoesNotTripBeforeStart)
{
  MBTimer timer(3);
  EXPECT_EQ(timer.getMaxTime(), 3);
  EXPECT_FALSE(timer.isTimedOut());
  EXPECT_FALSE(timer.timeOutCheck());

  MBTimer negative(-1);
  EXPECT_EQ(negative.getMaxTime(), -1);
  EXPECT_FALSE(negative.timeOutCheck());
  EXPECT_FALSE(negative.isTimedOut());
}

// Covers mb timer behavior: reset clears accumulated times and reconfigures max time.
TEST(MBTimerTest, ResetClearsAccumulatedTimesAndReconfiguresMaxTime)
{
  MBTimer timer(3);
  timer.start();
  timer.stop();
  timer.reset(10);

  EXPECT_FALSE(timer.isTimedOut());
  EXPECT_EQ(timer.getMaxTime(), 10);
  EXPECT_EQ(timer.get_wall_time(), 0);
  EXPECT_EQ(timer.get_cpu_time(), 0);
}

// Covers mb timer behavior: accessors while running do not stop timer or emit errors.
TEST(MBTimerTest, AccessorsWhileRunningDoNotStopTimerOrEmitErrors)
{
  MBTimer timer(5);
  timer.start();

  testing::internal::CaptureStdout();
  EXPECT_GE(timer.get_wall_time(), 0);
  EXPECT_GE(timer.get_user_cpu_time(), 0);
  EXPECT_GE(timer.get_system_cpu_time(), 0);
  EXPECT_GE(timer.get_cpu_time(), 0);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "");

  testing::internal::CaptureStdout();
  timer.stop();
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "");
}

// Covers mb timer behavior: double start and double stop report documented errors.
TEST(MBTimerTest, DoubleStartAndDoubleStopReportDocumentedErrors)
{
  MBTimer timer;

  testing::internal::CaptureStdout();
  timer.start();
  timer.start();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Timer Error: Tried to turn on running timer"),
            std::string::npos);

  testing::internal::CaptureStdout();
  timer.stop();
  timer.stop();
  output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Timer Error: Tried to turn off stopped timer"),
            std::string::npos);
}

// Covers mb timer behavior: reset while running stops timer without accumulating.
TEST(MBTimerTest, ResetWhileRunningStopsTimerWithoutAccumulating)
{
  MBTimer timer(3);
  timer.start();
  timer.reset(8);

  EXPECT_EQ(timer.getMaxTime(), 8);
  EXPECT_EQ(timer.get_wall_time(), 0);
  EXPECT_EQ(timer.get_cpu_time(), 0);

  testing::internal::CaptureStdout();
  timer.stop();
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("Timer Error: Tried to turn off stopped timer"),
            std::string::npos);
}

// Covers mb timer behavior: float accessors and unsupported precision stay zero before start.
TEST(MBTimerTest, FloatAccessorsAndUnsupportedPrecisionStayZeroBeforeStart)
{
  MBTimer timer;

  EXPECT_FLOAT_EQ(timer.get_float_wall_time(), 0.0f);
  EXPECT_FLOAT_EQ(timer.get_float_cpu_time(), 0.0f);
  EXPECT_FLOAT_EQ(timer.get_float_user_cpu_time(), 0.0f);
  EXPECT_FLOAT_EQ(timer.get_float_system_cpu_time(), 0.0f);

  EXPECT_EQ(timer.get_wall_time(1000), 0);
  EXPECT_EQ(timer.get_user_cpu_time(1000), 0);
  EXPECT_EQ(timer.get_system_cpu_time(1000), 0);
  EXPECT_EQ(timer.get_cpu_time(1000), 0);
  EXPECT_EQ(timer.get_system_cpu_time(42), 0);
}
