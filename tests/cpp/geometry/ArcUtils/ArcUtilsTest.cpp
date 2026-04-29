#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "ArcUtils.h"
#include "ArcUtilsToDo.h"
#include "CircularUtils.h"
#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYSegList.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

}  // namespace

TEST(ArcUtilsAngleTest, TreatsArcAnglesAsClockwiseHeadingIntervals)
{
  EXPECT_TRUE(angleInArc(45, 180, 45));
  EXPECT_TRUE(angleInArc(45, 180, 120));
  EXPECT_TRUE(angleInArc(45, 180, 180));
  EXPECT_FALSE(angleInArc(45, 180, 220));

  EXPECT_TRUE(angleInArc(350, 10, 0));
  EXPECT_TRUE(angleInArc(350, 10, 355));
  EXPECT_TRUE(angleInArc(350, 10, 5));
  EXPECT_FALSE(angleInArc(350, 10, 180));
}

TEST(ArcUtilsAngleTest, NormalizesAnglesBeforeArcMembershipChecks)
{
  EXPECT_TRUE(angleInArc(-10, 10, 0));
  EXPECT_TRUE(angleInArc(350, 370, 720));
  EXPECT_FALSE(angleInArc(-10, 10, 90));
  EXPECT_TRUE(angleInArc(90, 90, 450));
  EXPECT_FALSE(angleInArc(90, 90, 91));
}

TEST(ArcUtilsPointMembershipTest, ClassifiesPointsByBearingFromArcCenter)
{
  EXPECT_TRUE(pointInArc(0, 10, 0, 0, 10, 350, 10));
  EXPECT_TRUE(pointInArc(10, 0, 0, 0, 10, 80, 100));
  EXPECT_FALSE(pointInArc(-10, 0, 0, 0, 10, 80, 100));
}

TEST(ArcUtilsDistanceTest, ComputesClosestDistanceToTurnArcInteriorAndEndpoints)
{
  EXPECT_NEAR(distPointToArc(0, 5, 0, 0, 10, 270, 90), 5.0, kGeomTol);
  EXPECT_NEAR(distPointToArc(0, 12, 0, 0, 10, 270, 90), 2.0, kGeomTol);

  double ix = 0;
  double iy = 0;
  double dist = distPointToArcPt(5, -5, 0, 0, 10, 270, 90, ix, iy);
  EXPECT_NEAR(dist, std::hypot(5.0, 5.0), kGeomTol);
  EXPECT_NEAR(ix, 10.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
}

TEST(ArcUtilsDistanceTest, ProjectsCenterPointToLeftArcEndpoint)
{
  double ix = 99;
  double iy = 99;

  EXPECT_NEAR(distPointToArcPt(0, 0, 0, 0, 10, 270, 90, ix, iy), 0.0, kGeomTol);
  EXPECT_NEAR(ix, -10.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
}

TEST(ArcUtilsDistanceTest, ComputesSegmentAndSeglistDistancesToArc)
{
  EXPECT_NEAR(distSegToArc(-20, 0, 20, 0, 0, 0, 10, 270, 90), 0.0, kGeomTol);
  EXPECT_NEAR(distSegToArc(-20, -20, 20, -20, 0, 0, 10, 270, 90), 20.0, kGeomTol);

  XYSegList lane;
  lane.add_vertex(-20, -20);
  lane.add_vertex(20, -20);
  lane.add_vertex(20, 20);
  EXPECT_NEAR(distSegListToArc(0, 0, 10, 270, 90, lane), 10.0, kGeomTol);
}

TEST(ArcUtilsDistanceTest, ComputesDistanceAlongArcFromEitherOrigin)
{
  EXPECT_NEAR(arclen(90, 10), 78.53981634, kLooseGeomTol);
  EXPECT_NEAR(arclen(-90, 10), 235.61944902, kLooseGeomTol);
  EXPECT_NEAR(distPointOnArc(0, 10, 0, 0, 10, 270, 90, true), 15.7079632679, kLooseGeomTol);
  EXPECT_NEAR(distPointOnArc(0, 10, 0, 0, 10, 270, 90, false), 15.7079632679, kLooseGeomTol);
  EXPECT_EQ(distPointOnArc(0, 5, 0, 0, 10, 270, 90), -1);
  EXPECT_EQ(distPointOnArc(0, -10, 0, 0, 10, 270, 90), -2);
}

TEST(ArcUtilsDistanceTest, ComputesDistanceBetweenTwoPointsOnArc)
{
  EXPECT_NEAR(distPtsOnArc(-10, 0, 0, 10, 0, 0, 10, 270, 90), 15.7259632679, kLooseGeomTol);
  EXPECT_EQ(distPtsOnArc(-10, 0, 0, -10, 0, 0, 10, 270, 90), -1);
}

TEST(ArcUtilsTurnGeometryTest, ComputesStarboardAndPortFixedTurnEndpoints)
{
  double rx = 0;
  double ry = 0;

  arcturn(0, 0, 0, 90, 10, rx, ry);
  EXPECT_NEAR(rx, 10.0, kGeomTol);
  EXPECT_NEAR(ry, -10.0, kGeomTol);

  arcturn(0, 0, 0, -90, 10, rx, ry);
  EXPECT_NEAR(rx, 10.0, kGeomTol);
  EXPECT_NEAR(ry, -10.0, kGeomTol);
}

TEST(ArcUtilsTurnGeometryTest, LeavesPositionUnchangedForDegenerateTurnInputs)
{
  double rx = 0;
  double ry = 0;

  arcturn(3, 4, 90, 0, 10, rx, ry);
  EXPECT_NEAR(rx, 3.0, kGeomTol);
  EXPECT_NEAR(ry, 4.0, kGeomTol);

  arcturn(3, 4, 90, 45, 0, rx, ry);
  EXPECT_NEAR(rx, 3.0, kGeomTol);
  EXPECT_NEAR(ry, 4.0, kGeomTol);
}

TEST(ArcUtilsTurnGeometryTest, ArcturnCurrentlyIgnoresInputHeading)
{
  // This helper is still used as a low-level turn endpoint calculator. Pin
  // the current heading-independent behavior so any future fix is explicit.
  double rx_north = 0;
  double ry_north = 0;
  double rx_east = 0;
  double ry_east = 0;
  double rx_south = 0;
  double ry_south = 0;

  arcturn(100, -50, 0, 90, 25, rx_north, ry_north);
  arcturn(100, -50, 90, 90, 25, rx_east, ry_east);
  arcturn(100, -50, 180, -90, 25, rx_south, ry_south);

  EXPECT_NEAR(rx_north, 125.0, kGeomTol);
  EXPECT_NEAR(ry_north, -75.0, kGeomTol);
  EXPECT_NEAR(rx_east, 125.0, kGeomTol);
  EXPECT_NEAR(ry_east, -75.0, kGeomTol);
  EXPECT_NEAR(rx_south, 125.0, kGeomTol);
  EXPECT_NEAR(ry_south, -75.0, kGeomTol);
}

TEST(ArcUtilsIntersectionTest, FindsSegmentAndSeglistIntersectionsWithArc)
{
  EXPECT_TRUE(arcSegCross(-20, 0, 20, 0, 0, 0, 10, 270, 90));
  EXPECT_FALSE(arcSegCross(-20, -20, 20, -20, 0, 0, 10, 270, 90));

  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;
  int hits = arcSegCrossPts(-20, 0, 20, 0, 0, 0, 10, 270, 90, ix1, iy1, ix2, iy2);
  EXPECT_EQ(hits, 1);
  EXPECT_NEAR(ix1, 0.0, kGeomTol);
  EXPECT_NEAR(iy1, 10.0, kGeomTol);

  XYSegList lane = makeTransitLane();
  EXPECT_TRUE(arcSegListCross(0, 0, 25, 0, 120, lane));
}

TEST(ArcUtilsIntersectionTest, FindsTurnPreviewChordEndpointsForLegRunStyleTurn)
{
  // BHV_LegRun and fixed-turn visualizations reason about turn arcs against
  // straight legs. A route leg spanning the generated preview arc intersects
  // at the turn start and finish points.
  XYArc turn = arcFlight(100, -50, 45, 25, 25 * kPi / 2, false);
  ASSERT_TRUE(turn.valid());

  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;
  int hits = arcSegCrossPts(95, -50, 140, -50,
                            turn.getX(), turn.getY(), turn.getRad(),
                            turn.getRangle(), turn.getLangle(),
                            ix1, iy1, ix2, iy2);

  EXPECT_EQ(hits, 2);
  EXPECT_NEAR(ix1, 100.0, kLooseGeomTol);
  EXPECT_NEAR(iy1, -50.0, kLooseGeomTol);
  EXPECT_NEAR(ix2, 135.355339059327, kLooseGeomTol);
  EXPECT_NEAR(iy2, -50.0, kLooseGeomTol);

  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<double> dists;
  double min_dist = distSegToArcPts(95, -50, 140, -50,
                                    turn.getX(), turn.getY(), turn.getRad(),
                                    turn.getRangle(), turn.getLangle(),
                                    0.5, xs, ys, dists);
  EXPECT_NEAR(min_dist, 0.0, kGeomTol);
  ASSERT_EQ(xs.size(), 2u);
  ASSERT_EQ(ys.size(), 2u);
  ASSERT_EQ(dists.size(), 2u);
  EXPECT_NEAR(xs[0], 100.0, kLooseGeomTol);
  EXPECT_NEAR(ys[0], -50.0, kLooseGeomTol);
  EXPECT_NEAR(xs[1], 135.355339059327, kLooseGeomTol);
  EXPECT_NEAR(ys[1], -50.0, kLooseGeomTol);
}

TEST(ArcUtilsIntersectionTest, ReportsTwoCrossingPointsWhenBothCircleHitsAreInArc)
{
  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;
  int hits = arcSegCrossPts(-20, 0, 20, 0, 0, 0, 10, 0, 359, ix1, iy1, ix2, iy2);

  EXPECT_EQ(hits, 2);
  EXPECT_NEAR(ix1, 0.0, kGeomTol);
  EXPECT_NEAR(std::abs(iy1), 10.0, kGeomTol);
  EXPECT_NEAR(ix2, 0.0, kGeomTol);
  EXPECT_NEAR(std::abs(iy2), 10.0, kGeomTol);
}

TEST(ArcUtilsIntersectionTest, ReportsSeglistCrossingPointsAndClearsOutputVectors)
{
  XYSegList lane;
  lane.add_vertex(-20, 0);
  lane.add_vertex(20, 0);

  std::vector<double> xs;
  std::vector<double> ys;
  xs.push_back(999);
  ys.push_back(999);

  EXPECT_EQ(arcSegListCrossPts(0, 0, 10, 0, 359, lane, xs, ys), 2);
  ASSERT_EQ(xs.size(), 2u);
  ASSERT_EQ(ys.size(), 2u);
  EXPECT_NEAR(xs[0], 0.0, kGeomTol);
  EXPECT_NEAR(std::abs(ys[0]), 10.0, kGeomTol);
  EXPECT_NEAR(xs[1], 0.0, kGeomTol);
  EXPECT_NEAR(std::abs(ys[1]), 10.0, kGeomTol);
}

TEST(ArcUtilsCpaTest, ReportsRayCpaForSurveyLaneCrossingProjectedRay)
{
  XYSegList lane;
  lane.add_vertex(-10, 10);
  lane.add_vertex(10, 10);

  std::vector<double> cpa;
  std::vector<double> ray;
  EXPECT_EQ(cpasRaySegl(0, 0, 0, lane, 0.5, cpa, ray), 1u);
  ASSERT_EQ(cpa.size(), 1u);
  ASSERT_EQ(ray.size(), 1u);
  EXPECT_NEAR(cpa[0], 0.0, kGeomTol);
  EXPECT_NEAR(ray[0], 10.0, kGeomTol);
}

TEST(ArcUtilsCpaTest, ReportsArcCpaForLaneNearTurnArc)
{
  XYSegList lane;
  lane.add_vertex(-12, 0);
  lane.add_vertex(12, 0);

  std::vector<double> cpa;
  std::vector<double> arc_dist;
  EXPECT_GE(cpasArcSegl(0, 0, 10, 0, 359, lane, 0.5, true, cpa, arc_dist), 2u);
  ASSERT_EQ(cpa.size(), arc_dist.size());
  EXPECT_NEAR(cpa[0], 0.0, kGeomTol);
  EXPECT_GE(arc_dist[0], 0.0);
}

TEST(ArcUtilsToDoDistancePointsTest, ReportsBothIntersectionPointsForFullArcPerimeterCrossing)
{
  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<double> dists;

  double dist = distSegToArcPts(-20, 0, 20, 0, 0, 0, 10, 0, 359, 1, xs, ys, dists);

  EXPECT_NEAR(dist, 0.0, kGeomTol);
  ASSERT_EQ(xs.size(), 2u);
  ASSERT_EQ(ys.size(), 2u);
  ASSERT_EQ(dists.size(), 2u);
  EXPECT_NEAR(xs[0], 0.0, kGeomTol);
  EXPECT_NEAR(std::abs(ys[0]), 10.0, kGeomTol);
  EXPECT_NEAR(dists[0], 0.0, kGeomTol);
  EXPECT_NEAR(xs[1], 0.0, kGeomTol);
  EXPECT_NEAR(std::abs(ys[1]), 10.0, kGeomTol);
  EXPECT_NEAR(dists[1], 0.0, kGeomTol);
}

TEST(ArcUtilsToDoDistancePointsTest, ReportsSingleIntersectionForTransitLegEnteringTurnArc)
{
  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<double> dists;

  double dist = distSegToArcPts(0, 0, 0, 20, 0, 0, 10, 270, 90, 1, xs, ys, dists);

  EXPECT_NEAR(dist, 0.0, kGeomTol);
  ASSERT_EQ(xs.size(), 1u);
  ASSERT_EQ(ys.size(), 1u);
  ASSERT_EQ(dists.size(), 1u);
  EXPECT_NEAR(xs[0], 0.0, kGeomTol);
  EXPECT_NEAR(ys[0], 10.0, kGeomTol);
  EXPECT_NEAR(dists[0], 0.0, kGeomTol);
}

TEST(ArcUtilsToDoDistancePointsTest, ReportsClosestProjectionForParallelSurveyLaneOutsideArc)
{
  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<double> dists;

  double dist = distSegToArcPts(-20, 15, 20, 15, 0, 0, 10, 270, 90, 10, xs, ys, dists);

  EXPECT_NEAR(dist, 5.0, kGeomTol);
  ASSERT_EQ(xs.size(), 1u);
  ASSERT_EQ(ys.size(), 1u);
  ASSERT_EQ(dists.size(), 1u);
  EXPECT_NEAR(xs[0], 0.0, kGeomTol);
  EXPECT_NEAR(ys[0], 15.0, kGeomTol);
  EXPECT_NEAR(dists[0], 5.0, kGeomTol);
}

TEST(ArcUtilsToDoDistancePointsTest, SuppressesClosestPointsWhenMinimumDistanceExceedsThreshold)
{
  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<double> dists;

  double dist = distSegToArcPts(-30, -30, -20, -30, 0, 0, 10, 270, 90, 3, xs, ys, dists);

  EXPECT_NEAR(dist, std::sqrt(1000.0), kLooseGeomTol);
  EXPECT_TRUE(xs.empty());
  EXPECT_TRUE(ys.empty());
  EXPECT_TRUE(dists.empty());
}

TEST(ArcUtilsToDoDistancePointsTest, PinsDuplicateLeftEndpointReportWhenSegmentIsInsideCircle)
{
  std::vector<double> xs;
  std::vector<double> ys;
  std::vector<double> dists;

  double dist = distSegToArcPts(-9, -1, -9, 1, 0, 0, 10, 270, 90, 3, xs, ys, dists);

  EXPECT_NEAR(dist, 0.9446148619, kLooseGeomTol);
  ASSERT_EQ(xs.size(), 2u);
  ASSERT_EQ(ys.size(), 2u);
  ASSERT_EQ(dists.size(), 2u);
  EXPECT_NEAR(xs[0], -10.0, kGeomTol);
  EXPECT_NEAR(ys[0], 0.0, kGeomTol);
  EXPECT_NEAR(dists[0], 1.0, kGeomTol);
  EXPECT_NEAR(xs[1], -10.0, kGeomTol);
  EXPECT_NEAR(ys[1], 0.0, kGeomTol);
  EXPECT_NEAR(dists[1], 1.0, kGeomTol);
}
