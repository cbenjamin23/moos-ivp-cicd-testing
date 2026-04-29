#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "ConvexHullGenerator.h"
#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYPatternBlock.h"
#include "XYPolyExpander.h"
#include "XYPolygon.h"
#include "XYSegList.h"

namespace {

double minX(const XYSegList& segl)
{
  double result = segl.get_vx(0);
  for(unsigned int i = 1; i < segl.size(); ++i)
    result = std::min(result, segl.get_vx(i));
  return result;
}

double maxX(const XYSegList& segl)
{
  double result = segl.get_vx(0);
  for(unsigned int i = 1; i < segl.size(); ++i)
    result = std::max(result, segl.get_vx(i));
  return result;
}

double minY(const XYSegList& segl)
{
  double result = segl.get_vy(0);
  for(unsigned int i = 1; i < segl.size(); ++i)
    result = std::min(result, segl.get_vy(i));
  return result;
}

double maxY(const XYSegList& segl)
{
  double result = segl.get_vy(0);
  for(unsigned int i = 1; i < segl.size(); ++i)
    result = std::max(result, segl.get_vy(i));
  return result;
}

XYPolygon makeNonConvexArrow()
{
  XYPolygon poly;
  poly.add_vertex(0, 0, false);
  poly.add_vertex(10, 0, false);
  poly.add_vertex(5, 5, false);
  poly.add_vertex(10, 10, false);
  poly.add_vertex(0, 10, false);
  poly.determine_convexity();
  return poly;
}

}  // namespace

TEST(ConvexHullGeneratorTest, EmptyInputReturnsNullPolygon)
{
  ConvexHullGenerator generator;

  XYPolygon hull = generator.generateConvexHull();

  EXPECT_EQ(hull.size(), 0u);
  EXPECT_FALSE(hull.is_convex());
}

TEST(ConvexHullGeneratorTest, SinglePointProducesTinyTriangleAroundPoint)
{
  ConvexHullGenerator generator;
  generator.addPoint(10, 20);

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  ASSERT_EQ(hull.size(), 3u);
  EXPECT_TRUE(hull.contains(10, 20));
  EXPECT_NEAR(hull.area(), 0.0129903811, kLooseGeomTol);
  EXPECT_NEAR(hull.max_dist_to_ctr(), 0.1145643924, kLooseGeomTol);
}

TEST(ConvexHullGeneratorTest, TwoDistinctPointsProduceThinRectangleOneTenthAsWideAsLength)
{
  ConvexHullGenerator generator;
  generator.addPoint(0, 0);
  generator.addPoint(10, 0);

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  ASSERT_EQ(hull.size(), 4u);
  EXPECT_TRUE(hull.contains(5, 0));
  EXPECT_NEAR(hull.area(), 20.0, kGeomTol);
  EXPECT_NEAR(minX(hull), 0.0, kGeomTol);
  EXPECT_NEAR(maxX(hull), 10.0, kGeomTol);
  EXPECT_NEAR(minY(hull), -1.0, kGeomTol);
  EXPECT_NEAR(maxY(hull), 1.0, kGeomTol);
}

TEST(ConvexHullGeneratorTest, TwoNearlyCoincidentPointsCollapseToSinglePointSpecialCase)
{
  ConvexHullGenerator generator;
  generator.addPoint(0, 0);
  generator.addPoint(0.0005, 0);

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  EXPECT_EQ(hull.size(), 3u);
  EXPECT_TRUE(hull.contains(0, 0));
  EXPECT_NEAR(hull.area(), 0.0129903811, kLooseGeomTol);
  EXPECT_NEAR(hull.max_dist_to_ctr(), 0.1145643924, kLooseGeomTol);
}

TEST(ConvexHullGeneratorTest, RootIsLowestYThenLowestX)
{
  ConvexHullGenerator generator;
  generator.addPoint(5, 5);
  generator.addPoint(3, 0);
  generator.addPoint(2, 0);
  generator.addPoint(0, 10);

  XYPolygon hull = generator.generateConvexHull();
  XYPoint root = generator.getRootPoint();

  ASSERT_TRUE(hull.is_convex());
  EXPECT_NEAR(root.x(), 2.0, kGeomTol);
  EXPECT_NEAR(root.y(), 0.0, kGeomTol);
}

TEST(ConvexHullGeneratorTest, DuplicateRadialAngleKeepsFarthestPointAndDropsInteriorPoint)
{
  ConvexHullGenerator generator;
  generator.addPoint(0, 0);
  generator.addPoint(10, 10);
  generator.addPoint(20, 20);
  generator.addPoint(20, 0);
  generator.addPoint(0, 20);

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  ASSERT_EQ(hull.size(), 4u);
  EXPECT_TRUE(hull.contains(10, 10));
  EXPECT_NEAR(hull.area(), 400.0, kGeomTol);
  EXPECT_NEAR(minX(hull), 0.0, kGeomTol);
  EXPECT_NEAR(maxX(hull), 20.0, kGeomTol);
  EXPECT_NEAR(minY(hull), 0.0, kGeomTol);
  EXPECT_NEAR(maxY(hull), 20.0, kGeomTol);
}

TEST(ConvexHullGeneratorTest, DisableSettlingStillBuildsAlreadyConvexHull)
{
  ConvexHullGenerator generator;
  generator.disableSettling();
  generator.addPoint(0, 0, "root");
  generator.addPoint(20, 0, "east");
  generator.addPoint(20, 20, "north_east");
  generator.addPoint(0, 20, "north_west");
  generator.addPoint(10, 10, "interior");

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  EXPECT_EQ(hull.size(), 4u);
  EXPECT_TRUE(hull.contains(10, 10));
  EXPECT_NEAR(hull.area(), 400.0, kGeomTol);
}

TEST(ConvexHullGeneratorTest, NearlyDuplicateRadialAnglesWithinPointOneDegreesAlsoKeepFarthest)
{
  ConvexHullGenerator generator;
  generator.addPoint(0, 0);
  generator.addPoint(10, 10);
  generator.addPoint(20.017, 20);
  generator.addPoint(20, 0);
  generator.addPoint(0, 20);

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  ASSERT_EQ(hull.size(), 4u);
  EXPECT_TRUE(hull.contains(10, 10));
  EXPECT_GT(maxX(hull), 20.0);
}

TEST(ConvexHullGeneratorTest, BuildsObstacleManagerStyleHullFromSensorPointCloud)
{
  ConvexHullGenerator generator;
  generator.addPoint(90.2, -80.4, "contact_a");
  generator.addPoint(82.0, -88.0, "contact_b");
  generator.addPoint(82.1, -83.7, "contact_c");
  generator.addPoint(85.4, -80.4, "contact_d");
  generator.addPoint(89.0, -86.0, "interior_a");
  generator.addPoint(86.0, -84.0, "interior_b");

  XYPolygon hull = generator.generateConvexHull();

  ASSERT_TRUE(hull.is_convex());
  EXPECT_GE(hull.size(), 3u);
  EXPECT_LE(hull.size(), 5u);
  EXPECT_TRUE(hull.contains(89.0, -86.0));
  EXPECT_TRUE(hull.contains(86.0, -84.0));
  EXPECT_NEAR(minX(hull), 82.0, kLooseGeomTol);
  EXPECT_NEAR(maxX(hull), 90.2, kLooseGeomTol);
  EXPECT_NEAR(minY(hull), -88.0, kLooseGeomTol);
  EXPECT_NEAR(maxY(hull), -80.4, kLooseGeomTol);

  hull.set_label("obmgr_buoy");
  std::string alert_update = "name=avd_obstacles_buoy#poly=" +
      hull.get_spec_pts(5) + ",label=buoy#id=buoy";
  EXPECT_TRUE(stringContains(alert_update, "name=avd_obstacles_buoy#poly=pts={"));
  EXPECT_TRUE(stringContains(alert_update, "label=buoy#id=buoy"));
}

TEST(XYPolyExpanderTest, RejectsNonConvexSourcePolygonAndKeepsNullBuffer)
{
  XYPolyExpander expander;
  XYPolygon arrow = makeNonConvexArrow();

  EXPECT_FALSE(arrow.is_convex());
  EXPECT_FALSE(expander.setPoly(arrow));

  XYPolygon buffer = expander.getBufferPoly(5);
  EXPECT_EQ(buffer.size(), 0u);
  EXPECT_FALSE(buffer.is_convex());
}

TEST(XYPolyExpanderTest, ZeroBufferReturnsSameSquareEnvelope)
{
  XYPolyExpander expander;
  XYPolygon square = makeSquarePoly(0, 0, 10, 10);

  ASSERT_TRUE(expander.setPoly(square));
  XYPolygon buffer = expander.getBufferPoly(0);

  ASSERT_TRUE(buffer.is_convex());
  ASSERT_EQ(buffer.size(), 4u);
  EXPECT_NEAR(buffer.area(), square.area(), kGeomTol);
  EXPECT_NEAR(minX(buffer), 0.0, kGeomTol);
  EXPECT_NEAR(maxX(buffer), 10.0, kGeomTol);
  EXPECT_NEAR(minY(buffer), 0.0, kGeomTol);
  EXPECT_NEAR(maxY(buffer), 10.0, kGeomTol);
}

TEST(XYPolyExpanderTest, ExpandsSquareByBufferDistanceWithRoundedCornerVertices)
{
  XYPolyExpander expander;
  expander.setDegreeDelta(45);
  XYPolygon square = makeSquarePoly(0, 0, 10, 10);

  ASSERT_TRUE(expander.setPoly(square));
  XYPolygon buffer = expander.getBufferPoly(2);

  ASSERT_TRUE(buffer.is_convex());
  EXPECT_EQ(buffer.size(), 12u);
  EXPECT_TRUE(buffer.contains(square));
  EXPECT_GT(buffer.area(), square.area());
  EXPECT_NEAR(minX(buffer), -2.0, kGeomTol);
  EXPECT_NEAR(maxX(buffer), 12.0, kGeomTol);
  EXPECT_NEAR(minY(buffer), -2.0, kGeomTol);
  EXPECT_NEAR(maxY(buffer), 12.0, kGeomTol);
  EXPECT_FALSE(buffer.contains(13, 5));
}

TEST(XYPolyExpanderTest, ExpandsIrregularObstacleHullForAlertBufferUseCase)
{
  XYPolygon obstacle;
  obstacle.add_vertex(82.0, -88.0, false);
  obstacle.add_vertex(90.2, -80.4, false);
  obstacle.add_vertex(85.4, -80.4, false);
  obstacle.add_vertex(82.1, -83.7, false);
  obstacle.determine_convexity();

  XYPolyExpander expander;
  expander.setDegreeDelta(15);
  ASSERT_TRUE(obstacle.is_convex());
  ASSERT_TRUE(expander.setPoly(obstacle));

  XYPolygon buffer = expander.getBufferPoly(5);

  ASSERT_TRUE(buffer.is_convex());
  EXPECT_TRUE(buffer.contains(obstacle));
  EXPECT_GT(buffer.size(), obstacle.size());
  EXPECT_LT(minX(buffer), minX(obstacle));
  EXPECT_GT(maxX(buffer), maxX(obstacle));
  EXPECT_LT(minY(buffer), minY(obstacle));
  EXPECT_GT(maxY(buffer), maxY(obstacle));
  EXPECT_TRUE(buffer.contains(89.0, -86.0));
}

TEST(XYPolyExpanderTest, LargeDegreeDeltaIsClampedToFortyFiveDegrees)
{
  XYPolyExpander expander;
  expander.setDegreeDelta(100);
  XYPolygon square = makeSquarePoly(0, 0, 10, 10);

  ASSERT_TRUE(expander.setPoly(square));
  XYPolygon buffer = expander.getBufferPoly(2);

  ASSERT_TRUE(buffer.is_convex());
  EXPECT_EQ(buffer.size(), 12u);
}

TEST(XYPolyExpanderTest, VertexProximityThresholdStillReturnsValidContainingBuffer)
{
  XYPolyExpander expander;
  expander.setDegreeDelta(45);
  expander.setVertexProximityThresh(100);
  XYPolygon square = makeSquarePoly(0, 0, 3, 3);

  ASSERT_TRUE(expander.setPoly(square));
  XYPolygon buffer = expander.getBufferPoly(2);

  ASSERT_TRUE(buffer.is_convex());
  EXPECT_EQ(buffer.size(), 12u);
  EXPECT_TRUE(buffer.contains(square));
}

TEST(XYPolyExpanderTest, DisableSettlingStillReturnsConvexBufferForCleanSquare)
{
  XYPolyExpander expander;
  expander.disableSettling();
  expander.setDegreeDelta(45);
  XYPolygon square = makeSquarePoly(0, 0, 10, 10);

  ASSERT_TRUE(expander.setPoly(square));
  XYPolygon buffer = expander.getBufferPoly(2);

  ASSERT_TRUE(buffer.is_convex());
  EXPECT_EQ(buffer.size(), 12u);
  EXPECT_TRUE(buffer.contains(square));
}

TEST(XYPolyExpanderTest, NegativeBufferPartiallyShrinksSquareWithCurrentStitchingBehavior)
{
  XYPolyExpander expander;
  expander.setDegreeDelta(45);
  XYPolygon square = makeSquarePoly(0, 0, 10, 10);

  ASSERT_TRUE(expander.setPoly(square));
  XYPolygon buffer = expander.getBufferPoly(-2);

  ASSERT_TRUE(buffer.is_convex());
  // Negative buffers are accepted, but current stitching does not make a
  // symmetric inset: the low/left edges remain on the original envelope.
  EXPECT_LT(buffer.area(), square.area());
  EXPECT_NEAR(minX(buffer), 0.0, kGeomTol);
  EXPECT_NEAR(maxX(buffer), 10.0, kGeomTol);
  EXPECT_NEAR(minY(buffer), 0.0, kGeomTol);
  EXPECT_NEAR(maxY(buffer), 8.0, kGeomTol);
  EXPECT_TRUE(buffer.contains(1, 5));
  EXPECT_FALSE(buffer.contains(5, 9));
}

TEST(XYPatternBlockTest, DefaultsBuildFiveNorthSouthLaneSegmentsAroundIdPoint)
{
  XYPatternBlock block;
  block.addIDPoint(XYPoint(0, 0));

  block.buildCompositeSegList(-20, 50);

  std::vector<XYPoint> lane_points = block.getLanePoints();
  std::vector<XYSegList> lane_segments = block.getLaneSegments();
  XYSegList route = block.getCompositeSegList();

  ASSERT_EQ(lane_points.size(), 5u);
  ASSERT_EQ(lane_segments.size(), 5u);
  EXPECT_NEAR(lane_points[0].x(), -20.0, kGeomTol);
  EXPECT_NEAR(lane_points[0].y(), 0.0, kGeomTol);
  EXPECT_NEAR(lane_points[2].x(), 0.0, kGeomTol);
  EXPECT_NEAR(lane_points[4].x(), 20.0, kGeomTol);
  EXPECT_EQ(lane_points[0].get_label(), "0");
  EXPECT_EQ(lane_points[4].get_label(), "4");

  ASSERT_EQ(route.size(), 10u);
  EXPECT_NEAR(route.get_vx(0), -20.0, kGeomTol);
  EXPECT_NEAR(route.get_vy(0), 40.0, kGeomTol);
  EXPECT_EQ(route.get_edge_tags().size(), 5u);
  EXPECT_TRUE(route.get_edge_tags().matches(0, 1, "lane"));
  EXPECT_TRUE(route.get_edge_tags().matches(8, 9, "lane"));
}

TEST(XYPatternBlockTest, AutoDropToggleCurrentlyPreservesGeneratedCoverageRoute)
{
  XYPatternBlock block;
  block.setAutoDrop(true);
  block.addIDPoint(XYPoint(0, 0));

  block.buildCompositeSegList(-20, 50);

  EXPECT_EQ(block.getLanePoints().size(), 5u);
  EXPECT_EQ(block.getLaneSegments().size(), 5u);
  EXPECT_EQ(block.getCompositeSegList().size(), 10u);
}

TEST(XYPatternBlockTest, ParsesIdPointStringAndUsesAverageOfMultipleIdPointsAsCenter)
{
  XYPatternBlock block;
  EXPECT_TRUE(block.setParam("id_point", "10,-5"));
  block.addIDPoint(XYPoint(30, 15));
  EXPECT_TRUE(block.setParam("block_width", "20"));
  EXPECT_TRUE(block.setParam("swath_width", "10"));

  block.buildCompositeSegList(0, 0);

  std::vector<XYPoint> lane_points = block.getLanePoints();
  ASSERT_EQ(lane_points.size(), 3u);
  EXPECT_NEAR(lane_points[1].x(), 20.0, kGeomTol);
  EXPECT_NEAR(lane_points[1].y(), 5.0, kGeomTol);
}

TEST(XYPatternBlockTest, RejectsInvalidStringParams)
{
  XYPatternBlock block;

  EXPECT_FALSE(block.setParam("id_point", "10"));
  EXPECT_FALSE(block.setParam("id_point", "ten,20"));
  EXPECT_FALSE(block.setParam("block_width", "wide"));
}

TEST(XYPatternBlockTest, AngleZeroIsRejectedEvenThoughDefaultAngleIsZero)
{
  XYPatternBlock block;
  block.addIDPoint(XYPoint(0, 0));
  ASSERT_TRUE(block.setParam("block_length", 20));
  ASSERT_TRUE(block.setParam("block_width", 20));
  ASSERT_TRUE(block.setParam("swath_width", 10));

  EXPECT_FALSE(block.setParam("angle", 0));
  EXPECT_FALSE(block.setParam("angle", "0"));
  EXPECT_TRUE(block.setParam("angle", 90));
  EXPECT_FALSE(block.setParam("angle", 0));

  block.buildCompositeSegList(20, -10);
  std::vector<XYSegList> lane_segments = block.getLaneSegments();

  ASSERT_EQ(lane_segments.size(), 3u);
  EXPECT_NEAR(lane_segments[0].get_vx(0), 10.0, kGeomTol);
  EXPECT_NEAR(lane_segments[0].get_vy(0), -10.0, kGeomTol);
  EXPECT_NEAR(lane_segments[0].get_vx(1), -10.0, kGeomTol);
  EXPECT_NEAR(lane_segments[0].get_vy(1), -10.0, kGeomTol);
}

TEST(XYPatternBlockTest, CoreWidthMutatesBlockWidthOnRepeatedBuilds)
{
  XYPatternBlock block;
  block.addIDPoint(XYPoint(0, 0));
  ASSERT_TRUE(block.setParam("block_width", 40));
  ASSERT_TRUE(block.setParam("swath_width", 10));
  block.setCoreWidth(true);

  block.buildCompositeSegList(0, 0);
  EXPECT_EQ(block.getLanePoints().size(), 4u);

  block.buildCompositeSegList(0, 0);
  EXPECT_EQ(block.getLanePoints().size(), 3u);

  block.buildCompositeSegList(0, 0);
  EXPECT_EQ(block.getLanePoints().size(), 2u);
}

TEST(XYPatternBlockTest, CompositeRouteStartsAtClosestOuterEndpointAndTagsLaneLegs)
{
  XYPatternBlock block;
  block.addIDPoint(XYPoint(0, 0));
  ASSERT_TRUE(block.setParam("block_length", 20));
  ASSERT_TRUE(block.setParam("block_width", 20));
  ASSERT_TRUE(block.setParam("swath_width", 10));

  block.buildCompositeSegList(100, -10);
  XYSegList route = block.getCompositeSegList();

  ASSERT_EQ(route.size(), 6u);
  EXPECT_NEAR(route.get_vx(0), 10.0, kGeomTol);
  EXPECT_NEAR(route.get_vy(0), -10.0, kGeomTol);
  EXPECT_NEAR(route.get_vx(1), 10.0, kGeomTol);
  EXPECT_NEAR(route.get_vy(1), 10.0, kGeomTol);
  EXPECT_EQ(route.get_edge_tags().size(), 3u);
  EXPECT_TRUE(route.get_edge_tags().matches(0, 1, "lane"));
  EXPECT_TRUE(route.get_edge_tags().matches(2, 3, "lane"));
  EXPECT_TRUE(route.get_edge_tags().matches(4, 5, "lane"));
  EXPECT_FALSE(route.get_edge_tags().matches(1, 2, "lane"));
}

TEST(XYPatternBlockTest, DistanceHelpersUseGeneratedLaneGeometry)
{
  XYPatternBlock block;
  block.addIDPoint(XYPoint(0, 0));
  ASSERT_TRUE(block.setParam("block_length", 20));
  ASSERT_TRUE(block.setParam("block_width", 20));
  ASSERT_TRUE(block.setParam("swath_width", 10));

  block.buildCompositeSegList(0, 0);

  EXPECT_NEAR(block.distanceToCrossAxis(0, 30), 30.0, kGeomTol);
  EXPECT_NEAR(block.distanceToClosestEntry(-10, 12), 2.0, kGeomTol);
  EXPECT_NEAR(block.distanceToClosestEntry(0, 12, false), 10.1980390272, kLooseGeomTol);
}
