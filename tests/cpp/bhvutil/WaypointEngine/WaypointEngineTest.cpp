#include <gtest/gtest.h>

#include <string>
#include <vector>

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

// Covers waypoint engine behavior: advances through waypoint list and reports completion.
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

// Covers waypoint engine behavior: reverses traversal and preserves raw point list for toggle.
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

// Covers waypoint engine behavior: vector setter add waypoint and reverse toggle preserve raw route.
TEST(WaypointEngineTest, VectorSetterAddWaypointAndReverseTogglePreserveRawRoute)
{
  WaypointEngine engine;
  std::vector<XYPoint> points = {XYPoint(0, 0), XYPoint(50, 0)};
  engine.setSegList(points);

  EXPECT_EQ(engine.size(), 2u);
  EXPECT_NE(engine.getPointsStr().find("0,0"), std::string::npos);
  EXPECT_NE(engine.getPointsStr().find("50,0"), std::string::npos);

  engine.addWaypoint(XYPoint(100, 0));
  EXPECT_EQ(engine.size(), 3u);
  EXPECT_NEAR(engine.getPointX(2), 100.0, kGeomTol);

  engine.setReverseToggle();
  EXPECT_TRUE(engine.getReverse());
  EXPECT_NEAR(engine.getPointX(0), 100.0, kGeomTol);
  engine.addWaypoint(XYPoint(150, 0));
  EXPECT_EQ(engine.size(), 3u);

  engine.setReverseToggle();
  EXPECT_FALSE(engine.getReverse());
  EXPECT_NEAR(engine.getPointX(0), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getPointX(2), 100.0, kGeomTol);
}

// Covers waypoint engine behavior: slIP radius allows non monotonic waypoint capture.
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

// Covers waypoint engine behavior: capture line records visualization artifacts when crossed.
TEST(WaypointEngineTest, CaptureLineRecordsVisualizationArtifactsWhenCrossed)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());
  engine.setCaptureLine(true);
  engine.setCurrIndex(1);
  engine.setPrevPoint(XYPoint(0, 0));

  EXPECT_EQ(engine.setNextWaypoint(51, 0), "completed");
  EXPECT_TRUE(engine.getCapturePt().valid());
  EXPECT_NEAR(engine.getCapturePt().x(), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getCapturePt().y(), 0.0, kGeomTol);
  EXPECT_EQ(engine.getCaptureSegl().size(), 2u);
}

// Covers waypoint engine behavior: perpetual repeats cycle until allowed repeats are consumed.
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

// Covers waypoint engine behavior: computes remaining distance for cycle and final traversal.
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

// Covers waypoint engine behavior: ignores invalid radius and index updates.
TEST(WaypointEngineTest, IgnoresInvalidRadiusAndIndexUpdates)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());
  engine.setCaptureRadius(4);
  engine.setSlipRadius(8);

  engine.setCaptureRadius(-1);
  engine.setSlipRadius(-1);
  EXPECT_DOUBLE_EQ(engine.getCaptureRadius(), 4);
  EXPECT_DOUBLE_EQ(engine.getSlipRadius(), 8);

  engine.setCurrIndex(1);
  EXPECT_EQ(engine.getCurrIndex(), 1);
  engine.setCurrIndex(9);
  EXPECT_EQ(engine.getCurrIndex(), 1);
  EXPECT_NEAR(engine.getPointX(), 50.0, kGeomTol);
}

// Covers waypoint engine behavior: resets centers and endless repeats for reusable behavior runs.
TEST(WaypointEngineTest, ResetsCentersAndEndlessRepeatsForReusableBehaviorRuns)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());
  engine.setCenter(100, 100);
  EXPECT_NEAR(engine.getPointX(0), 75.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(0), 100.0, kGeomTol);
  EXPECT_NEAR(engine.getPointX(1), 125.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(1), 100.0, kGeomTol);

  engine.setRepeatsEndless(true);
  EXPECT_DOUBLE_EQ(engine.distToEndFinal(75, 100), -1);

  EXPECT_EQ(engine.setNextWaypoint(75, 100), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(125, 100), "completed");
  EXPECT_TRUE(engine.isComplete());
  engine.resetForNewTraversal();
  EXPECT_FALSE(engine.isComplete());
  EXPECT_EQ(engine.getCurrIndex(), 0);
  EXPECT_EQ(engine.getCycleCount(), 1u);
  EXPECT_EQ(engine.getTotalHits(), 2u);
}

// Covers waypoint engine behavior: reset state clears route progress and counters.
TEST(WaypointEngineTest, ResetStateClearsRouteProgressAndCounters)
{
  WaypointEngine engine;
  engine.setSegList(twoPointLane());
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_GT(engine.getTotalHits(), 0u);

  engine.resetState();
  EXPECT_TRUE(engine.isComplete());
  EXPECT_EQ(engine.size(), 0u);
  EXPECT_EQ(engine.getCurrIndex(), 0);
  EXPECT_EQ(engine.getCycleCount(), 0u);
  EXPECT_EQ(engine.getTotalHits(), 0u);
  EXPECT_EQ(engine.resetsRemaining(), engine.getRepeats());
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "empty_seglist");
  EXPECT_DOUBLE_EQ(engine.distToNextWpt(0, 0), -1);
}
