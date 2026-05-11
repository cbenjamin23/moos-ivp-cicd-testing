#include <gtest/gtest.h>

#include <cmath>

#include "NodeRecord.h"
#include "NumericAssertions.h"
#include "SimEngine.h"
#include "ThrustMap.h"
#include "TurnSpeedMap.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

NodeRecord recordAt(double x, double y, double heading, double speed)
{
  NodeRecord record("alpha", "kayak");
  record.setX(x);
  record.setY(y);
  record.setHeading(heading);
  record.setSpeed(speed);
  record.setDepth(0);
  record.setPitch(0);
  record.setTimeStamp(10);
  return record;
}

ThrustMap factorMap(double factor = 20)
{
  ThrustMap map;
  map.setThrustFactor(factor);
  return map;
}

}  // namespace

// Covers uSimMarine motion integration: forward propagation uses heading average plus drift.
TEST(SimEngineTest, PropagateAdvancesPositionTimestampAndGroundTrack)
{
  SimEngine engine;
  NodeRecord record = recordAt(0, 0, 90, 2);

  engine.propagate(record, 3, 90, 2, 0.5, -0.25);

  EXPECT_NEAR(record.getX(), 7.5, kGeomTol);
  EXPECT_NEAR(record.getY(), -0.75, kGeomTol);
  EXPECT_NEAR(record.getTimeStamp(), 13, kGeomTol);
  EXPECT_NEAR(record.getSpeed(), 2, kGeomTol);
  EXPECT_NEAR(record.getSpeedOG(), std::hypot(2.5, -0.25), kGeomTol);
}

// Covers uSimMarine speed behavior: thrust factor, acceleration, deceleration, rudder loss, and sail cap.
TEST(SimEngineTest, PropagateSpeedAppliesLimitsRudderLossAndSailCap)
{
  SimEngine engine;
  ThrustMap map = factorMap();
  NodeRecord record = recordAt(0, 0, 0, 0);

  engine.propagateSpeed(record, map, 1, 100, 0, 1, 0.5);
  EXPECT_NEAR(record.getSpeed(), 1, kGeomTol);

  engine.propagateSpeed(record, map, 1, 100, 100, 10, 10);
  EXPECT_NEAR(record.getSpeed(), 0.75, kGeomTol);

  engine.propagateSpeed(record, map, 1, 100, 0, 10, 10, 2);
  EXPECT_NEAR(record.getSpeed(), 2, kGeomTol);

  engine.propagateSpeed(record, map, 1, 0, 0, 10, 0.25);
  EXPECT_NEAR(record.getSpeed(), 1.75, kGeomTol);
}

// Covers uSimMarine speed behavior: reverse mode flips thrust and rudder signs.
TEST(SimEngineTest, ReverseModeUsesNegativeThrustAndSetsReverseSpeed)
{
  SimEngine engine;
  ThrustMap map = factorMap();
  map.setReflect(true);
  NodeRecord record = recordAt(0, 0, 0, 0);

  engine.setThrustModeReverse(true);
  engine.propagateSpeed(record, map, 1, 40, 0, 10, 10);

  EXPECT_NEAR(record.getSpeed(), -2, kGeomTol);
}

// Covers uSimMarine heading behavior: rudder, thrust, turn map, rotate speed, and yaw stay coupled.
TEST(SimEngineTest, PropagateHeadingUsesRudderThrustTurnMapAndRotateSpeed)
{
  SimEngine engine;
  TurnSpeedMap turn_map;
  ASSERT_TRUE(turn_map.setNullSpeed(1));
  ASSERT_TRUE(turn_map.setFullSpeed(5));
  ASSERT_TRUE(turn_map.setNullRate(20));
  ASSERT_TRUE(turn_map.setFullRate(80));
  engine.setTurnSpeedMap(turn_map);

  NodeRecord record = recordAt(0, 0, 10, 3);
  engine.propagateHeading(record, 2, 50, 50, 90, 5);

  EXPECT_NEAR(record.getHeading(), 75, kGeomTol);
  EXPECT_NEAR(record.getYaw(), -75.0 * kPi / 180.0, kGeomTol);
}

// Covers uSimMarine heading behavior: zero speed suppresses rudder turn but preserves rotate drift.
TEST(SimEngineTest, ZeroSpeedSuppressesRudderTurnButKeepsRotateSpeed)
{
  SimEngine engine;
  NodeRecord record = recordAt(0, 0, 10, 0);

  engine.propagateHeading(record, 2, 100, 100, 90, 3);

  EXPECT_NEAR(record.getHeading(), 16, kGeomTol);
}

// Covers uSimMarine depth behavior: buoyancy, elevator authority, pitch coupling, and depth floor.
TEST(SimEngineTest, PropagateDepthAppliesBuoyancyElevatorPitchAndSurfaceFloor)
{
  SimEngine engine;
  NodeRecord stopped = recordAt(0, 0, 0, 0);
  stopped.setDepth(1);
  engine.propagateDepth(stopped, 10, 0, 0.2, 1, 2);
  EXPECT_NEAR(stopped.getDepth(), 0, kGeomTol);
  EXPECT_NEAR(stopped.getPitch(), 0, kGeomTol);

  NodeRecord diving = recordAt(0, 0, 0, 2);
  diving.setDepth(5);
  diving.setPitch(0);
  engine.propagateDepth(diving, 2, 50, 0.1, 1.0, 2.0);
  EXPECT_NEAR(diving.getDepth(), 5.8, kGeomTol);
  EXPECT_GT(diving.getPitch(), -0.3);
  EXPECT_LT(diving.getPitch(), 0);
}

// Covers uSimMarine differential mode: average thrust controls speed and thrust difference controls heading.
TEST(SimEngineTest, DifferentialModeMapsAverageThrustAndHeadingDifference)
{
  SimEngine engine;
  ThrustMap map = factorMap();
  NodeRecord record = recordAt(0, 0, 100, 0);

  engine.propagateSpeedDiffMode(record, map, 1, 80, 40, 10, 10);
  EXPECT_NEAR(record.getSpeed(), 3 * (1.0 - 0.85 * 0.2), kGeomTol);

  engine.propagateHeadingDiffMode(record, 2, 100, -100, 90, 5);
  EXPECT_NEAR(record.getHeading(), 182, kGeomTol);
}
