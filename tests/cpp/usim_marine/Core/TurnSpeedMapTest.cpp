#include <gtest/gtest.h>

#include "TurnSpeedMap.h"

// Covers uSimMarine turn-speed behavior: defaults provide full turn authority above zero speed.
TEST(TurnSpeedMapTest, DefaultsReturnNullAtZeroAndFullRateAboveDefaultFullSpeed)
{
  TurnSpeedMap map;

  EXPECT_DOUBLE_EQ(map.getNullSpeed(), 0);
  EXPECT_DOUBLE_EQ(map.getFullSpeed(), 1);
  EXPECT_DOUBLE_EQ(map.getNullRate(), 0);
  EXPECT_DOUBLE_EQ(map.getFullRate(), 100);
  EXPECT_DOUBLE_EQ(map.getTurnRate(0), 0);
  EXPECT_DOUBLE_EQ(map.getTurnRate(0.5), 50);
  EXPECT_DOUBLE_EQ(map.getTurnRate(2), 100);
}

// Covers uSimMarine turn-speed behavior: configured speeds interpolate linearly.
TEST(TurnSpeedMapTest, ConfiguredSpeedsAndRatesInterpolateLinearly)
{
  TurnSpeedMap map;
  ASSERT_TRUE(map.setNullSpeed(1));
  ASSERT_TRUE(map.setFullSpeed(5));
  ASSERT_TRUE(map.setNullRate(20));
  ASSERT_TRUE(map.setFullRate(80));

  EXPECT_DOUBLE_EQ(map.getTurnRate(0.5), 20);
  EXPECT_DOUBLE_EQ(map.getTurnRate(1), 20);
  EXPECT_DOUBLE_EQ(map.getTurnRate(3), 50);
  EXPECT_DOUBLE_EQ(map.getTurnRate(5), 80);
  EXPECT_DOUBLE_EQ(map.getTurnRate(10), 80);
}

// Covers uSimMarine turn-speed behavior: invalid values are rejected without changing state.
TEST(TurnSpeedMapTest, RejectsInvalidSpeedsAndRates)
{
  TurnSpeedMap map;
  ASSERT_TRUE(map.setFullSpeed(4));
  ASSERT_TRUE(map.setNullSpeed(2));
  ASSERT_TRUE(map.setFullRate(70));
  ASSERT_TRUE(map.setNullRate(30));

  EXPECT_FALSE(map.setFullSpeed(0));
  EXPECT_FALSE(map.setNullSpeed(-1));
  EXPECT_FALSE(map.setFullRate(0));
  EXPECT_FALSE(map.setFullRate(101));
  EXPECT_FALSE(map.setNullRate(-0.1));
  EXPECT_FALSE(map.setNullRate(101));

  EXPECT_DOUBLE_EQ(map.getFullSpeed(), 4);
  EXPECT_DOUBLE_EQ(map.getNullSpeed(), 2);
  EXPECT_DOUBLE_EQ(map.getFullRate(), 70);
  EXPECT_DOUBLE_EQ(map.getNullRate(), 30);
}

// Covers uSimMarine turn-speed behavior: setters maintain ordered full/null breakpoints.
TEST(TurnSpeedMapTest, SettersMaintainOrderedBreakpoints)
{
  TurnSpeedMap map;

  ASSERT_TRUE(map.setNullSpeed(3));
  EXPECT_DOUBLE_EQ(map.getFullSpeed(), 3);
  ASSERT_TRUE(map.setFullSpeed(2));
  EXPECT_DOUBLE_EQ(map.getNullSpeed(), 2);

  ASSERT_TRUE(map.setNullRate(60));
  EXPECT_DOUBLE_EQ(map.getFullRate(), 100);
  ASSERT_TRUE(map.setFullRate(40));
  EXPECT_DOUBLE_EQ(map.getNullRate(), 40);
  EXPECT_DOUBLE_EQ(map.getTurnRate(2), 40);
}
