#include <gtest/gtest.h>

#include <cstdlib>
#include <vector>

#include "GeomUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsPoly.h"
#include "XYSegList.h"

// Covers geom utils projection behavior: projects points using MOOS heading convention.
TEST(GeomUtilsProjectionTest, ProjectsPointsUsingMoosHeadingConvention)
{
  double x = 0;
  double y = 0;
  projectPoint(0, 10, 1, 2, x, y);
  EXPECT_NEAR(x, 1.0, kGeomTol);
  EXPECT_NEAR(y, 12.0, kGeomTol);

  XYPoint east = projectPoint(90, 10, 1, 2);
  EXPECT_NEAR(east.x(), 11.0, kGeomTol);
  EXPECT_NEAR(east.y(), 2.0, kGeomTol);

  XYPoint shifted = projectPoint(180, 3, XYPoint(5, 5));
  EXPECT_NEAR(shifted.x(), 5.0, kGeomTol);
  EXPECT_NEAR(shifted.y(), 2.0, kGeomTol);

  XYPoint midpoint = midPoint(XYPoint(-2, 4), XYPoint(6, -2));
  EXPECT_NEAR(midpoint.x(), 2.0, kGeomTol);
  EXPECT_NEAR(midpoint.y(), 1.0, kGeomTol);
}

// Covers geom utils projection behavior: combines heading magnitude vectors.
TEST(GeomUtilsProjectionTest, CombinesHeadingMagnitudeVectors)
{
  double heading = 0;
  double magnitude = 0;
  addVectors(0, 10, 90, 10, heading, magnitude);

  EXPECT_NEAR(heading, 45.0, kLooseGeomTol);
  EXPECT_NEAR(magnitude, 14.1421356237, kLooseGeomTol);
}

// Covers geom utils distance behavior: computes point segment and line distances with closest point.
TEST(GeomUtilsDistanceTest, ComputesPointSegmentAndLineDistancesWithClosestPoint)
{
  double ix = 0;
  double iy = 0;

  EXPECT_NEAR(distPointToPoint(0, 0, 3, 4), 5.0, kGeomTol);
  EXPECT_NEAR(distPointToPoint(XYPoint(0, 0), XYPoint(3, 4)), 5.0, kGeomTol);
  EXPECT_NEAR(distPointToSeg(0, 0, 10, 0, 5, 4, ix, iy), 4.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  EXPECT_NEAR(distPointToSeg(0, 0, 10, 0, 15, 4, ix, iy), 6.4031242374, kLooseGeomTol);
  EXPECT_NEAR(ix, 10.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  EXPECT_NEAR(distPointToSeg(0, 0, 10, 0, 5, -5, 0), 5.0, kGeomTol);
  EXPECT_NEAR(distPointToSeg(0, 0, 10, 0, 5, -5, 180), -1.0, kGeomTol);
  EXPECT_NEAR(distPointToLine(5, 4, 0, 0, 10, 0), 4.0, kGeomTol);
}

// Covers geom utils distance behavior: projects perpendiculars to lines and segments.
TEST(GeomUtilsDistanceTest, ProjectsPerpendicularsToLinesAndSegments)
{
  double ix = 0;
  double iy = 0;

  perpLineIntPt(0, 0, 10, 0, 5, 4, ix, iy);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  perpSegIntPt(0, 0, 10, 0, 15, 4, ix, iy);
  EXPECT_NEAR(ix, 10.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
}

// Covers geom utils intersection behavior: finds line and segment crossings.
TEST(GeomUtilsIntersectionTest, FindsLineAndSegmentCrossings)
{
  double ix = 0;
  double iy = 0;

  EXPECT_TRUE(linesCross(0, 0, 10, 10, 0, 10, 10, 0, ix, iy));
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 5.0, kGeomTol);
  EXPECT_FALSE(linesCross(0, 0, 10, 0, 0, 1, 10, 1, ix, iy));

  EXPECT_TRUE(segmentsCross(0, 0, 10, 10, 0, 10, 10, 0, ix, iy));
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 5.0, kGeomTol);
  EXPECT_FALSE(segmentsCross(0, 0, 10, 0, 0, 1, 10, 1));

  EXPECT_TRUE(lineRayCross(0, 0, 90, 5, -5, 5, 5, ix, iy));
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
  EXPECT_FALSE(lineRayCross(0, 0, 270, 5, -5, 5, 5, ix, iy));

  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;
  EXPECT_TRUE(lineSegCross(0, 0, 10, 0, 5, -5, 5, 5, ix1, iy1, ix2, iy2));
  EXPECT_NEAR(ix1, 5.0, kGeomTol);
  EXPECT_NEAR(iy1, 0.0, kGeomTol);
  EXPECT_NEAR(ix2, 5.0, kGeomTol);
  EXPECT_NEAR(iy2, 0.0, kGeomTol);
  EXPECT_TRUE(lineSegCross(0, 0, 10, 0, -5, 0, 15, 0, ix1, iy1, ix2, iy2));
  EXPECT_NEAR(ix1, 0.0, kGeomTol);
  EXPECT_NEAR(iy1, 0.0, kGeomTol);
  EXPECT_NEAR(ix2, 10.0, kGeomTol);
  EXPECT_NEAR(iy2, 0.0, kGeomTol);
  EXPECT_FALSE(lineSegCross(0, 0, 10, 0, 5, 5, 10, 5, ix1, iy1, ix2, iy2));

  EXPECT_NEAR(segmentAngle(0, 0, 0, 10, 10, 10), 90.0, kGeomTol);
  EXPECT_NEAR(segmentAngle(0, 0, 10, 0, 20, -10), 45.0, kGeomTol);
  EXPECT_NEAR(segmentAngle(0, 0, 0, 0, 10, 0), 0.0, kGeomTol);
}

// Covers geom utils intersection behavior: computes distances between segments.
TEST(GeomUtilsIntersectionTest, ComputesDistancesBetweenSegments)
{
  double ix = 0;
  double iy = 0;

  EXPECT_NEAR(distSegToSeg(0, 0, 10, 0, 5, -5, 5, 5, ix, iy), 0.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  EXPECT_NEAR(distSegToSeg(0, 0, 10, 0, 5, 5, 5, 10, ix, iy), 5.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
}

// Covers geom utils ray behavior: computes ray crossings and closest approach.
TEST(GeomUtilsRayTest, ComputesRayCrossingsAndClosestApproach)
{
  double ix = 0;
  double iy = 0;

  EXPECT_TRUE(crossRaySeg(0, 0, 90, 5, -5, 5, 5, ix, iy));
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
  EXPECT_FALSE(crossRaySeg(0, 0, 270, 5, -5, 5, 5, ix, iy));

  EXPECT_NEAR(distPointToRay(5, 5, 0, 0, 90, ix, iy), 5.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  EXPECT_NEAR(segRayCPA(0, 0, 90, 5, -5, 5, 5, ix, iy), 5.0, kGeomTol);
  EXPECT_NEAR(ix, 0.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
}

// Covers geom utils ray behavior: computes seglist and polygon ray CPA for obstacle lookahead.
TEST(GeomUtilsRayTest, ComputesSeglistAndPolygonRayCpaForObstacleLookahead)
{
  double ix = 0;
  double iy = 0;

  XYSegList lane;
  lane.add_vertex(5, -5);
  lane.add_vertex(5, 5);
  lane.add_vertex(10, 5);
  double range = 0;
  EXPECT_TRUE(crossRaySegl(0, 0, 90, lane, ix, iy, range));
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
  EXPECT_NEAR(range, 5.0, kGeomTol);

  EXPECT_NEAR(seglRayCPA(0, 0, 90, lane, ix, iy), 5.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  XYPolygon obstacle = string2Poly("pts={5,-5:10,-5:10,5:5,5}");
  EXPECT_NEAR(polyRayCPA(0, 0, 90, obstacle, ix, iy), 0.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
}

// Covers geom utils seg list behavior: computes distances to seglist and polygon.
TEST(GeomUtilsSegListTest, ComputesDistancesToSeglistAndPolygon)
{
  double ix = 0;
  double iy = 0;

  XYSegList lane;
  lane.add_vertex(5, -5);
  lane.add_vertex(5, 5);
  lane.add_vertex(10, 5);

  EXPECT_NEAR(distPointToSegl(5, 3, lane), 0.0, kGeomTol);
  EXPECT_NEAR(distSeglToPoint(lane, XYPoint(0, 0), ix, iy), 5.0, kGeomTol);
  EXPECT_NEAR(ix, 5.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);

  XYPolygon obstacle = string2Poly("pts={20,0:30,0:30,10:20,10}");
  EXPECT_NEAR(segSeglDist(0, 0, 10, 0, lane, ix, iy), 5.0, kGeomTol);
  EXPECT_NEAR(distSegToPoly(0, 0, 10, 0, obstacle, ix, iy), 10.0, kGeomTol);
  EXPECT_NEAR(ix, 10.0, kGeomTol);
  EXPECT_NEAR(iy, 0.0, kGeomTol);
  EXPECT_NEAR(distSeglToPoly(lane, obstacle, ix, iy), 10.0, kGeomTol);

  EXPECT_NEAR(stemDistSeglFromPoint(lane, 5, 3), 8.0, kGeomTol);
}

// Covers geom utils segment modifier behavior: adjusts segment length angle and location.
TEST(GeomUtilsSegmentModifierTest, AdjustsSegmentLengthAngleAndLocation)
{
  double rx1 = 0;
  double ry1 = 0;
  double rx2 = 0;
  double ry2 = 0;

  EXPECT_TRUE(modSegLen(0, 0, 10, 0, rx1, ry1, rx2, ry2, 20));
  EXPECT_NEAR(rx1, -5.0, kGeomTol);
  EXPECT_NEAR(ry1, 0.0, kGeomTol);
  EXPECT_NEAR(rx2, 15.0, kGeomTol);
  EXPECT_NEAR(ry2, 0.0, kGeomTol);

  EXPECT_TRUE(modSegAng(0, 0, 10, 0, rx1, ry1, rx2, ry2, 0));
  EXPECT_NEAR(rx1, 5.0, kGeomTol);
  EXPECT_NEAR(ry1, -5.0, kGeomTol);
  EXPECT_NEAR(rx2, 5.0, kGeomTol);
  EXPECT_NEAR(ry2, 5.0, kGeomTol);

  EXPECT_TRUE(modSegLoc(0, 0, 10, 0, rx1, ry1, rx2, ry2, 0, 0));
  EXPECT_NEAR(rx1, -5.0, kGeomTol);
  EXPECT_NEAR(ry1, 0.0, kGeomTol);
  EXPECT_NEAR(rx2, 5.0, kGeomTol);
  EXPECT_NEAR(ry2, 0.0, kGeomTol);

  XYPoint center = getSegCenter(XYPoint(0, 0), XYPoint(10, 0));
  EXPECT_NEAR(center.x(), 5.0, kGeomTol);
  EXPECT_NEAR(center.y(), 0.0, kGeomTol);
}

// Covers geom utils circle behavior: computes line circle intersection cases.
TEST(GeomUtilsCircleTest, ComputesLineCircleIntersectionCases)
{
  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;

  EXPECT_EQ(lineCircleIntPts(-10, 3, 10, 3, 0, 0, 5, ix1, iy1, ix2, iy2), 2);
  EXPECT_NEAR(ix1, -4.0, kGeomTol);
  EXPECT_NEAR(iy1, 3.0, kGeomTol);
  EXPECT_NEAR(ix2, 4.0, kGeomTol);
  EXPECT_NEAR(iy2, 3.0, kGeomTol);

  EXPECT_EQ(lineCircleIntPts(0, 5, 10, 5, 5, 0, 5, ix1, iy1, ix2, iy2), 1);
  EXPECT_NEAR(ix1, 5.0, kGeomTol);
  EXPECT_NEAR(iy1, 5.0, kGeomTol);

  EXPECT_EQ(lineCircleIntPts(0, 6, 10, 6, 5, 0, 5, ix1, iy1, ix2, iy2), 0);
  EXPECT_NEAR(ix1, 0.0, kGeomTol);
  EXPECT_NEAR(iy1, 0.0, kGeomTol);

  EXPECT_NEAR(distCircleToLine(0, 0, 5, -10, 10, 10, 10), 5.0, kGeomTol);
  EXPECT_NEAR(distCircleToLine(0, 0, 5, -10, 3, 10, 3), 0.0, kGeomTol);
}

// Covers geom utils circle behavior: filters circle intersections to segment extent.
TEST(GeomUtilsCircleTest, FiltersCircleIntersectionsToSegmentExtent)
{
  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;

  EXPECT_EQ(segCircleIntPts(-10, 3, 10, 3, 0, 0, 5, ix1, iy1, ix2, iy2), 2);
  EXPECT_NEAR(ix1, -4.0, kGeomTol);
  EXPECT_NEAR(iy1, 3.0, kGeomTol);
  EXPECT_NEAR(ix2, 4.0, kGeomTol);
  EXPECT_NEAR(iy2, 3.0, kGeomTol);

  EXPECT_EQ(segCircleIntPts(-10, 3, 0, 3, 0, 0, 5, ix1, iy1, ix2, iy2), 1);
  EXPECT_NEAR(ix1, -4.0, kGeomTol);
  EXPECT_NEAR(iy1, 3.0, kGeomTol);
  EXPECT_NEAR(ix2, 0.0, kGeomTol);
  EXPECT_NEAR(iy2, 0.0, kGeomTol);

  EXPECT_EQ(segCircleIntPts(6, 0, 10, 0, 0, 0, 5, ix1, iy1, ix2, iy2), 0);
}

// Covers geom utils polygon measure behavior: computes rotated width height and aspect ratio.
TEST(GeomUtilsPolygonMeasureTest, ComputesRotatedWidthHeightAndAspectRatio)
{
  XYPolygon rect = string2Poly("pts={0,0:20,0:20,10:0,10}");

  ASSERT_TRUE(rect.is_convex());
  EXPECT_NEAR(polyWidth(rect, 0), 20.0, kGeomTol);
  EXPECT_NEAR(polyHeight(rect, 0), 10.0, kGeomTol);
  EXPECT_NEAR(polyWidth(rect, 90), 10.0, kLooseGeomTol);
  EXPECT_NEAR(polyHeight(rect, 90), 20.0, kLooseGeomTol);
  EXPECT_NEAR(polyAspectRatio(rect), 1.1180339887, kLooseGeomTol);

  XYPolygon diamond = string2Poly("pts={0,10:10,0:20,10:10,20}");
  ASSERT_TRUE(diamond.is_convex());
  EXPECT_NEAR(polyWidth(diamond, 45), 14.1421356237, kLooseGeomTol);
  EXPECT_NEAR(polyHeight(diamond, 45), 14.1421356237, kLooseGeomTol);
  EXPECT_NEAR(polyAspectRatio(diamond), 1.4142135624, kLooseGeomTol);
}

// Covers geom utils polygon measure behavior: computes bearing cone and samples points for obstacle views.
TEST(GeomUtilsPolygonMeasureTest, ComputesBearingConeAndSamplesPointsForObstacleViews)
{
  XYPolygon obstacle = string2Poly("pts={10,-5:20,-5:20,5:10,5}");
  ASSERT_TRUE(obstacle.is_convex());

  double bmin = 0;
  double bmax = 0;
  double bmin_dist = 0;
  double bmax_dist = 0;
  ASSERT_TRUE(bearingMinMaxToPolyX(0, 0, obstacle, bmin, bmax, bmin_dist, bmax_dist));
  EXPECT_NEAR(bmin, 63.4349488229, kLooseGeomTol);
  EXPECT_NEAR(bmax, 116.5650511771, kLooseGeomTol);
  EXPECT_NEAR(bmin_dist, 11.1803398875, kLooseGeomTol);
  EXPECT_NEAR(bmax_dist, 11.1803398875, kLooseGeomTol);

  EXPECT_TRUE(bearingMinMaxToPoly(0, 0, obstacle, bmin, bmax));
  EXPECT_FALSE(bearingMinMaxToPoly(15, 0, obstacle, bmin, bmax));

  std::srand(1);
  double rx = 0;
  double ry = 0;
  EXPECT_TRUE(randPointOnPoly(0, 0, obstacle, rx, ry));
  EXPECT_NEAR(obstacle.dist_to_poly(rx, ry), 0.0, kLooseGeomTol);

  XYPolygon box = string2Poly("pts={0,0:10,0:10,10:0,10}");
  ASSERT_TRUE(box.is_convex());
  EXPECT_TRUE(randPointInPoly(box, rx, ry, 10));
  EXPECT_TRUE(box.contains(rx, ry));
  EXPECT_FALSE(randPointInPoly(box, rx, ry, 0));
  EXPECT_NEAR(rx, box.get_center_x(), kGeomTol);
  EXPECT_NEAR(ry, box.get_center_y(), kGeomTol);
}

// Covers geom utils mutator behavior: shifts vertex vectors and leaves invalid input unchanged.
TEST(GeomUtilsMutatorTest, ShiftsVertexVectorsAndLeavesInvalidInputUnchanged)
{
  std::vector<double> vx{1, 2, 3};
  std::vector<double> vy{4, 5, 6};
  shiftVertices(vx, vy);

  ASSERT_EQ(vx.size(), 3u);
  ASSERT_EQ(vy.size(), 3u);
  EXPECT_NEAR(vx[0], 2.0, kGeomTol);
  EXPECT_NEAR(vy[0], 5.0, kGeomTol);
  EXPECT_NEAR(vx[2], 1.0, kGeomTol);
  EXPECT_NEAR(vy[2], 4.0, kGeomTol);

  std::vector<double> bad_x{1, 2};
  std::vector<double> bad_y{3};
  shiftVertices(bad_x, bad_y);
  EXPECT_EQ(bad_x.size(), 2u);
  EXPECT_EQ(bad_y.size(), 1u);
  EXPECT_NEAR(bad_x[0], 1.0, kGeomTol);
  EXPECT_NEAR(bad_y[0], 3.0, kGeomTol);
}

// Covers geom utils mutator behavior: parses point and seglist into existing objects atomically.
TEST(GeomUtilsMutatorTest, ParsesPointAndSeglistIntoExistingObjectsAtomically)
{
  XYPoint point(9, 9);
  EXPECT_TRUE(setPointOnString(point, "3,4"));
  EXPECT_NEAR(point.x(), 3.0, kGeomTol);
  EXPECT_NEAR(point.y(), 4.0, kGeomTol);
  EXPECT_FALSE(setPointOnString(point, "bad"));
  EXPECT_NEAR(point.x(), 3.0, kGeomTol);
  EXPECT_NEAR(point.y(), 4.0, kGeomTol);

  XYSegList segl;
  EXPECT_TRUE(setSegListOnString(segl, "0,0:10,0"));
  EXPECT_EQ(segl.size(), 2u);
  EXPECT_FALSE(setSegListOnString(segl, "bad"));
  EXPECT_EQ(segl.size(), 2u);
}

// Covers geom utils deprecated alias behavior: keeps legacy distance wrappers equivalent.
TEST(GeomUtilsDeprecatedAliasTest, KeepsLegacyDistanceWrappersEquivalent)
{
  EXPECT_NEAR(distToPoint(0, 0, 3, 4), distPointToPoint(0, 0, 3, 4), kGeomTol);
  EXPECT_NEAR(distToSegment(0, 0, 10, 0, 5, 4), distPointToSeg(0, 0, 10, 0, 5, 4), kGeomTol);
}

// Covers geom utils segment modifier behavior: rejects zero length segments.
TEST(GeomUtilsSegmentModifierTest, RejectsZeroLengthSegments)
{
  double rx1 = 1;
  double ry1 = 2;
  double rx2 = 3;
  double ry2 = 4;

  EXPECT_FALSE(modSegLen(0, 0, 0, 0, rx1, ry1, rx2, ry2, 10));
  EXPECT_FALSE(modSegAng(0, 0, 0, 0, rx1, ry1, rx2, ry2, 90));
  EXPECT_FALSE(modSegLoc(0, 0, 0, 0, rx1, ry1, rx2, ry2, 1, 1));
}
