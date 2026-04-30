#include <gtest/gtest.h>

#include <vector>

#include "NumericAssertions.h"
#include "WaypointEngineX.h"
#include "XYPoint.h"
#include "XYSegList.h"

namespace {

XYSegList threePointRoute()
{
  XYSegList route;
  route.add_vertex(0, 0);
  route.add_vertex(50, 0);
  route.add_vertex(100, 0);
  return route;
}

}  // namespace

// Covers waypoint engine x behavior: parses waypoint behavior parameters and rejects bad values.
TEST(WaypointEngineXTest, ParsesWaypointBehaviorParametersAndRejectsBadValues)
{
  WaypointEngineX engine;

  EXPECT_TRUE(engine.setParam("order", "reverse"));
  EXPECT_TRUE(engine.getReverse());
  EXPECT_TRUE(engine.setParam("order", "toggle"));
  EXPECT_FALSE(engine.getReverse());
  EXPECT_TRUE(engine.setParam("repeat", "forever"));
  EXPECT_EQ(engine.getMaxRepeats(), -1);
  EXPECT_TRUE(engine.setParam("repeat", "2"));
  EXPECT_EQ(engine.getMaxRepeats(), 2);
  EXPECT_TRUE(engine.setParam("capture_line", "absolute"));
  EXPECT_TRUE(engine.usingCaptureLine());
  EXPECT_DOUBLE_EQ(engine.getCaptureRadius(), 0);
  EXPECT_DOUBLE_EQ(engine.getSlipRadius(), 0);
  EXPECT_TRUE(engine.setParam("lead", "12"));
  EXPECT_TRUE(engine.setParam("lead_damper", "4"));

  EXPECT_FALSE(engine.setParam("order", "sideways"));
  EXPECT_FALSE(engine.setParam("repeat", "-1"));
  EXPECT_FALSE(engine.setParam("capture_radius", "-2"));
  EXPECT_FALSE(engine.setParam("capture_line", "maybe"));
  EXPECT_FALSE(engine.setParam("unknown", "1"));
}

// Covers waypoint engine x behavior: advances cycles and completes with state flags.
TEST(WaypointEngineXTest, AdvancesCyclesAndCompletesWithStateFlags)
{
  WaypointEngineX engine;
  engine.setSegList(threePointRoute());
  engine.setRepeat(1);

  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_TRUE(engine.hasAdvanced());
  EXPECT_TRUE(engine.hasStateChange());
  EXPECT_EQ(engine.getCurrIndex(), 1);
  EXPECT_EQ(engine.getCaptureHits(), 1u);

  EXPECT_EQ(engine.setNextWaypoint(50, 0), "advanced");
  EXPECT_EQ(engine.getCurrIndex(), 2);
  EXPECT_EQ(engine.setNextWaypoint(100, 0), "cycled");
  EXPECT_TRUE(engine.hasCycled());
  EXPECT_EQ(engine.getCycleCount(), 1u);
  EXPECT_EQ(engine.repeatsRemaining(), 0u);

  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(50, 0), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(100, 0), "completed");
  EXPECT_TRUE(engine.hasCompleted());
  EXPECT_TRUE(engine.hasAdvanced());
  EXPECT_TRUE(engine.hasCycled());
}

// Covers waypoint engine x behavior: computes distance percent and track point along route.
TEST(WaypointEngineXTest, ComputesDistancePercentAndTrackPointAlongRoute)
{
  WaypointEngineX engine;
  engine.setSegList(threePointRoute());
  engine.setParam("lead", "10");
  engine.setParam("lead_damper", "20");

  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_NEAR(engine.distToPrevWpt(25, 10), 26.9258, 1e-3);
  EXPECT_NEAR(engine.distToNextWpt(25, 10), 26.9258, 1e-3);
  EXPECT_NEAR(engine.pctToNextWpt(25, 0), 0.5, kGeomTol);
  EXPECT_NEAR(engine.distToEnd(25, 0), 75.0, kGeomTol);
  EXPECT_NEAR(engine.distFromBeg(25, 0), 25.0, kGeomTol);
  EXPECT_NEAR(engine.pctToEnd(25, 0), 0.25, kGeomTol);

  EXPECT_TRUE(engine.trackPointSet());
  XYPoint track = engine.getTrackPoint();
  EXPECT_GT(track.x(), 25.0);
  EXPECT_LT(track.x(), 50.0);
  EXPECT_NEAR(track.y(), 0.0, kLooseGeomTol);
}

// Covers waypoint engine x behavior: capture line can advance after crossing absolute line.
TEST(WaypointEngineXTest, CaptureLineCanAdvanceAfterCrossingAbsoluteLine)
{
  WaypointEngineX engine;
  XYSegList route;
  route.add_vertex(0, 0);
  route.add_vertex(50, 0);
  engine.setSegList(route);
  ASSERT_TRUE(engine.setParam("capture_line", "absolute"));

  EXPECT_EQ(engine.setNextWaypoint(-10, 0), "transit");
  EXPECT_FALSE(engine.isUnderWay());
  EXPECT_EQ(engine.setNextWaypoint(1, 0), "advanced");
  EXPECT_TRUE(engine.isUnderWay());
  EXPECT_EQ(engine.getLineHits(), 1u);
}

// Covers waypoint engine x behavior: retain keeps current index when route is updated.
TEST(WaypointEngineXTest, RetainKeepsCurrentIndexWhenRouteIsUpdated)
{
  WaypointEngineX engine;
  engine.setSegList(threePointRoute());
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_EQ(engine.getCurrIndex(), 1);

  XYSegList shifted = threePointRoute();
  shifted.shift_horz(10);
  engine.setSegList(shifted, true);
  EXPECT_EQ(engine.getCurrIndex(), 1);
  EXPECT_NEAR(engine.getPointX(), 60.0, kGeomTol);
}

// Covers waypoint engine x behavior: ignores invalid radius and index updates.
TEST(WaypointEngineXTest, IgnoresInvalidRadiusAndIndexUpdates)
{
  WaypointEngineX engine;
  engine.setSegList(threePointRoute());
  engine.setCaptureRadius(4);
  engine.setSlipRadius(8);

  engine.setCaptureRadius(-1);
  engine.setSlipRadius(-1);
  EXPECT_DOUBLE_EQ(engine.getCaptureRadius(), 4);
  EXPECT_DOUBLE_EQ(engine.getSlipRadius(), 8);

  engine.setCurrIndex(2);
  EXPECT_EQ(engine.getCurrIndex(), 2);
  engine.setCurrIndex(99);
  EXPECT_EQ(engine.getCurrIndex(), 2);
  EXPECT_NEAR(engine.getPointX(), 100.0, kGeomTol);
}

// Covers waypoint engine x behavior: reset state clears traversal counters and route.
TEST(WaypointEngineXTest, ResetStateClearsTraversalCountersAndRoute)
{
  WaypointEngineX engine;
  engine.setSegList(threePointRoute());
  engine.setRepeat(2);
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_GT(engine.getTotalHits(), 0u);

  engine.resetState();
  EXPECT_EQ(engine.size(), 0u);
  EXPECT_EQ(engine.getCurrIndex(), 0);
  EXPECT_EQ(engine.getTotalHits(), 0u);
  EXPECT_EQ(engine.getCycleCount(), 0u);
  EXPECT_FALSE(engine.trackPointSet());
  EXPECT_FALSE(engine.getNextPoint().valid());
  EXPECT_DOUBLE_EQ(engine.distToNextWpt(0, 0), -1);
}

// Covers waypoint engine x behavior: centering shifts raw and active route coordinates.
TEST(WaypointEngineXTest, CenteringShiftsRawAndActiveRouteCoordinates)
{
  WaypointEngineX engine;
  engine.setSegList(threePointRoute());
  engine.setCenter(100, -20);

  EXPECT_NEAR(engine.getPointX(0), 50.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(0), -20.0, kGeomTol);
  EXPECT_NEAR(engine.getPointX(2), 150.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(2), -20.0, kGeomTol);

  engine.setReverse(true);
  EXPECT_NEAR(engine.getPointX(0), 150.0, kGeomTol);
  EXPECT_NEAR(engine.getPointY(0), -20.0, kGeomTol);
}

// Covers waypoint engine x behavior: vector setter reverse toggle and forever repeat behavior.
TEST(WaypointEngineXTest, VectorSetterReverseToggleAndForeverRepeatBehavior)
{
  WaypointEngineX engine;
  engine.setReverse(true);
  std::vector<XYPoint> points = {XYPoint(0, 0), XYPoint(50, 0), XYPoint(100, 0)};
  engine.setSegList(points);

  EXPECT_EQ(engine.size(), 3u);
  EXPECT_TRUE(engine.getReverse());
  EXPECT_NEAR(engine.getPointX(), 100.0, kGeomTol);

  engine.setReverseToggle();
  EXPECT_FALSE(engine.getReverse());
  EXPECT_NEAR(engine.getPointX(), 0.0, kGeomTol);

  ASSERT_TRUE(engine.setParam("repeat", "forever"));
  EXPECT_EQ(engine.getMaxRepeats(), -1);
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(50, 0), "advanced");
  EXPECT_EQ(engine.setNextWaypoint(100, 0), "cycled");
  EXPECT_FALSE(engine.hasCompleted());
  EXPECT_EQ(engine.setNextWaypoint(0, 0), "advanced");
}

// Covers waypoint engine x behavior: slIP radius and traversal reset support reusable runs.
TEST(WaypointEngineXTest, SlipRadiusAndTraversalResetSupportReusableRuns)
{
  WaypointEngineX engine;
  XYSegList route;
  route.add_vertex(10, 0);
  engine.setSegList(route);
  engine.setCaptureRadius(1);
  engine.setSlipRadius(5);

  EXPECT_EQ(engine.setNextWaypoint(6, 0), "transit");
  EXPECT_EQ(engine.setNextWaypoint(8, 0), "transit");
  EXPECT_EQ(engine.setNextWaypoint(14, 0), "completed");
  EXPECT_EQ(engine.getSlipHits(), 1u);
  EXPECT_EQ(engine.getCaptureHits(), 0u);

  engine.resetForNewTraversal();
  EXPECT_FALSE(engine.hasCompleted());
  EXPECT_EQ(engine.getCurrIndex(), 0);
  EXPECT_EQ(engine.getCycleCount(), 0u);
  EXPECT_EQ(engine.getSlipHits(), 1u);
  EXPECT_EQ(engine.setNextWaypoint(10, 0), "completed");
}
