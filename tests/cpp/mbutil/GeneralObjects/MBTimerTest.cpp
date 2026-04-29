#include <gtest/gtest.h>

#include "MBTimer.h"

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
