#include <gtest/gtest.h>

#include <string>

#include "ThrustMap.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Covers uSimMarine thrust behavior: the legacy thrust factor maps speed and thrust both ways.
TEST(ThrustMapTest, ThrustFactorMapsSpeedAndThrustWhenNoPositivePairsExist)
{
  ThrustMap map;
  EXPECT_TRUE(map.usingThrustFactor());
  EXPECT_DOUBLE_EQ(map.getSpeedValue(40), 0);
  EXPECT_DOUBLE_EQ(map.getThrustValue(2), 0);

  map.setThrustFactor(20);
  EXPECT_TRUE(map.usingThrustFactor());
  EXPECT_DOUBLE_EQ(map.getSpeedValue(40), 2);
  EXPECT_DOUBLE_EQ(map.getThrustValue(2.5), 50);

  map.setThrustFactor(-4);
  EXPECT_DOUBLE_EQ(map.getThrustFactor(), 20);
}

// Covers uSimMarine thrust behavior: positive mappings interpolate and clamp at configured bounds.
TEST(ThrustMapTest, PositiveMapInterpolatesAndClamps)
{
  ThrustMap map;
  ASSERT_TRUE(map.addPair(20, 1));
  ASSERT_TRUE(map.addPair(60, 3));
  ASSERT_TRUE(map.addPair(100, 4));

  EXPECT_FALSE(map.usingThrustFactor());
  EXPECT_DOUBLE_EQ(map.getSpeedValue(0), 0);
  EXPECT_DOUBLE_EQ(map.getSpeedValue(40), 2);
  EXPECT_DOUBLE_EQ(map.getSpeedValue(200), 4);
  EXPECT_DOUBLE_EQ(map.getThrustValue(2), 40);
  EXPECT_DOUBLE_EQ(map.getThrustValue(99), 100);
  EXPECT_TRUE(Contains(map.getMapPos(), "20,1"));
  EXPECT_TRUE(Contains(map.getMapPos(), "60,3"));
}

// Covers uSimMarine thrust behavior: invalid signs, out-of-range thrust, and non-ascending pairs are rejected.
TEST(ThrustMapTest, RejectsInvalidPairsWithoutReplacingAcceptedMap)
{
  ThrustMap map;
  ASSERT_TRUE(map.addPair(20, 1));
  ASSERT_TRUE(map.addPair(60, 3));

  EXPECT_FALSE(map.addPair(40, 0.5));
  EXPECT_FALSE(map.addPair(30, -1));
  EXPECT_FALSE(map.addPair(-30, 1));
  EXPECT_FALSE(map.addPair(101, 5));
  EXPECT_FALSE(map.addPair(0, 1));

  EXPECT_DOUBLE_EQ(map.getSpeedValue(40), 2);
  EXPECT_TRUE(map.isAscending());
}

// Covers uSimMarine thrust behavior: negative maps interpolate independently when reflection is disabled.
TEST(ThrustMapTest, NegativeMapInterpolatesIndependently)
{
  ThrustMap map;
  ASSERT_TRUE(map.addPair(-100, -3));
  ASSERT_TRUE(map.addPair(-40, -1));

  EXPECT_DOUBLE_EQ(map.getSpeedValue(-70), -2);
  EXPECT_DOUBLE_EQ(map.getSpeedValue(-200), -3);
  EXPECT_NEAR(map.getThrustValue(-2), -70, 1e-9);
  EXPECT_DOUBLE_EQ(map.getThrustValue(-99), -100);
  EXPECT_TRUE(Contains(map.getMapNeg(), "-100:-3"));
}

// Covers uSimMarine thrust behavior: reflected negative thrust mirrors the positive map.
TEST(ThrustMapTest, ReflectNegativeMirrorsPositiveMap)
{
  ThrustMap map;
  ASSERT_TRUE(map.addPair(25, 1));
  ASSERT_TRUE(map.addPair(75, 3));
  map.setReflect(true);

  EXPECT_TRUE(map.usingReflect());
  EXPECT_DOUBLE_EQ(map.getSpeedValue(-50), -2);
  EXPECT_DOUBLE_EQ(map.getThrustValue(-2), -50);
  EXPECT_EQ(map.getMapNeg(), "Positive thrust-map reflected");
}

// Covers uSimMarine thrust behavior: min/max changes prune mappings and invalid ranges are ignored.
TEST(ThrustMapTest, MinMaxRangePrunesMappingsAndRejectsInvertedRange)
{
  ThrustMap map;
  ASSERT_TRUE(map.addPair(20, 1));
  ASSERT_TRUE(map.addPair(60, 3));
  ASSERT_TRUE(map.addPair(100, 4));
  ASSERT_TRUE(map.addPair(-80, -2));

  map.setMinMaxThrust(-50, 70);
  EXPECT_FALSE(Contains(map.getMapPos(), "100,4"));
  EXPECT_FALSE(Contains(map.getMapNeg(), "-80:-2"));
  EXPECT_DOUBLE_EQ(map.getSpeedValue(100), 3);

  map.setMinMaxThrust(80, 20);
  EXPECT_DOUBLE_EQ(map.getSpeedValue(100), 3);
}

// Covers uSimMarine thrust behavior: clear restores the unconfigured factor mode.
TEST(ThrustMapTest, ClearRestoresUnconfiguredFactorMode)
{
  ThrustMap map;
  map.setThrustFactor(20);
  ASSERT_TRUE(map.addPair(50, 2));

  map.clear();

  EXPECT_TRUE(map.usingThrustFactor());
  EXPECT_DOUBLE_EQ(map.getThrustFactor(), 0);
  EXPECT_EQ(map.getMapPos(), "");
  EXPECT_DOUBLE_EQ(map.getSpeedValue(50), 0);
}
