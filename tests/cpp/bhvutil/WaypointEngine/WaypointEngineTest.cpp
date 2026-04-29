#include <gtest/gtest.h>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "WaypointEngine.h"
#include "XYPoint.h"
#include "XYSegList.h"

namespace {

XYSegList twoPointLane()
{
  XYSegList lane;
  lane.add_vertex(0, 0);
  lane.add_vertex(50, 0);
  return lane;
}

}  // namespace

TEST(WaypointEngineTest, AdvancesThroughWaypointListAndReportsCompletion)
{
  WaypointEngine engine;
  EXPECT_TRUE(engine.isComplete());
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "empty_seglist");

  engine.setSegList(twoPointLane());
  EXPECT_FALSE(engine.isComplete());
  EXPECT_EQ(engine.getCurrIndex(), 0);
  EXPECT_TRUE(engine.currPtChanged());
  EXPECT_FALSE(engine.currPtChanged());

  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_EQ(engine.getCurrIndex(), 1);
  EXPECT_EQ(engine.getCaptureHits(), 1u);
  EXPECT_NEAR(engine.distToPrevWpt(25, 0), 25.0, kGeomTol);
  EXPECT_NEAR(engine.distToNextWpt(25, 0), 25.0, kGeomTol);

  EXPECT_EQ(engine.setNextWaypoint(50, 0), "completed");
  EXPECT_TRUE(engine.isComplete());
  EXPECT_EQ(engine.getCycleCount(), 1u);
  EXPECT_EQ(engine.setNextWaypoint(50, 0), "completed");
}

TEST(WaypointEngineTest, ReversesTraversalAndPreservesRawPointListForToggle)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());

  engine.setReverse(true);
  EXPECT_TRUE(engine.getReverse());
  EXPECT_EQ(engine.getCurrIndex(), 0);
  EXPECT_NEAR(engine.getPointX(), 50.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(), 0.0, kGeomTol);

  engine.setReverse(false);
  EXPECT_FALSE(engine.getReverse());
  EXPECT_NEAR(engine.getPointX(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(), 0.0, kGeomTol);
}

TEST(WaypointEngineTest, SlipRadiusAllowsNonMonotonicWaypointCapture)
{
  WaypointEngine engine;
  XYSegList lane;
  lane.add_vertex(10, 0);
  engine.setSegList(lane);
  engine.setCaptureRadius(1);
  engine.setSlipRadius(5);

  EXPECT_EQ(engine.setNextWaypoint(6, 0), "in-transit");
  EXPECT_EQ(engine.setNextWaypoint(8, 0), "in-transit");
  EXPECT_EQ(engine.setNextWaypoint(14, 0), "completed");
  EXPECT_EQ(engine.getNonmonoHits(), 1u);
  EXPECT_EQ(engine.getCaptureHits(), 0u);
}

TEST(WaypointEngineTest, PerpetualRepeatsCycleUntilAllowedRepeatsAreConsumed)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());
  engine.setPerpetual(true);
  engine.setRepeat(1);

  EXPECT_EQ(engine.resetsRemaining(), 1u);
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(50, 0), "cycled");
  EXPECT_FALSE(engine.isComplete());
  EXPECT_EQ(engine.getCycleCount(), 1u);
  EXPECT_EQ(engine.resetsRemaining(), 0u);

  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(50, 0), "completed");
  EXPECT_TRUE(engine.isComplete());
  EXPECT_EQ(engine.getCycleCount(), 2u);
}

TEST(WaypointEngineTest, ComputesRemainingDistanceForCycleAndFinalTraversal)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());
  engine.setPerpetual(true);
  engine.setRepeat(1);

  EXPECT_NEAR(engine.distToEndCycle(0, 0), 50.0, kGeomTol);
  EXPECT_NEAR(engine.distToEndFinal(0, 0), 150.0, kGeomTol);

  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_NEAR(engine.distToEndCycle(25, 0), 25.0, kGeomTol);
  EXPECT_NEAR(engine.distToEndFinal(25, 0), 125.0, kGeomTol);
}
