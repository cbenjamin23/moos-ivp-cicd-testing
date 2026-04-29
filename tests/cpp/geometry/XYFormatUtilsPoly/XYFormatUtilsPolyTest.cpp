#include <gtest/gtest.h>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsPoly.h"

TEST(XYFormatUtilsPolyTest, ParsesViewPolygonPtsSpecUsedByOpRegionAndObstacles)
{
  XYPolygon poly = stringStandard2Poly("pts={0,0:20,0:20,10:0,10},label=safety_box,edge_color=white,vertex_color=green");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 4u);
  EXPECT_EQ(poly.get_label(), "safety_box");
  EXPECT_NEAR(poly.area(), 200.0, kGeomTol);
  EXPECT_TRUE(poly.contains(10, 5));
  EXPECT_FALSE(poly.contains(25, 5));
  EXPECT_EQ(poly.get_color_str("edge"), "white");
  EXPECT_EQ(poly.get_color_str("vertex"), "green");
}

TEST(XYFormatUtilsPolyTest, ParsesDocumentedObstacleManagerViewPolygon)
{
  XYPolygon poly = string2Poly("pts={32,-100:38,-98:40,-100:32,-104},label=ob_23,duration=12");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 4u);
  EXPECT_EQ(poly.get_label(), "ob_23");
  EXPECT_TRUE(poly.duration_set());
  EXPECT_NEAR(poly.get_duration(), 12.0, kGeomTol);
  EXPECT_NEAR(poly.area(), 24.0, kGeomTol);
  EXPECT_TRUE(poly.contains(36, -100));
  EXPECT_FALSE(poly.contains(30, -100));
}

TEST(XYFormatUtilsPolyTest, ParsesRadialLoiterStyleSpec)
{
  XYPolygon poly = stringRadial2Poly("x=10, y=20, radius=30, pts=12, snap=0.1, label=loiter_region");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 12u);
  EXPECT_EQ(poly.get_label(), "loiter_region");
  EXPECT_TRUE(poly.contains(10, 20));
  EXPECT_NEAR(poly.max_radius(), 30.0, 0.05);
}

TEST(XYFormatUtilsPolyTest, ParsesAbbreviatedPointListWithObjectMetadata)
{
  XYPolygon poly = stringAbbreviated2Poly("0,0:10,0:10,10:0,10:label,abbr_box:source,unit_test:edge_color,blue");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 4u);
  EXPECT_EQ(poly.get_label(), "abbr_box");
  EXPECT_EQ(poly.get_source(), "unit_test");
  EXPECT_EQ(poly.get_color_str("edge"), "blue");
  EXPECT_NEAR(poly.area(), 100.0, kGeomTol);
}

TEST(XYFormatUtilsPolyTest, ParsesBhvWaypointPolygonListUsedByManyAlphaMissions)
{
  XYPolygon poly = string2Poly("60,-40 : 60,-160 : 150,-160 : 180,-100 : 150,-40");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 5u);
  EXPECT_NEAR(poly.area(), 12600.0, kGeomTol);
  EXPECT_TRUE(poly.contains(100, -100));
  EXPECT_FALSE(poly.contains(40, -100));
  EXPECT_NEAR(poly.get_center_x(), 120.0, kLooseGeomTol);
  EXPECT_NEAR(poly.get_center_y(), -100.0, kLooseGeomTol);
}

TEST(XYFormatUtilsPolyTest, ParsesUpOpRegionAndContactManagerPolygonPayloads)
{
  XYPolygon op_region = string2Poly("0,-50:0,-250:150,-250:150,-50");

  ASSERT_TRUE(op_region.is_convex());
  ASSERT_EQ(op_region.size(), 4u);
  EXPECT_TRUE(op_region.contains(75, -150));
  EXPECT_FALSE(op_region.contains(75, -25));
  EXPECT_NEAR(op_region.area(), 30000.0, kGeomTol);

  XYPolygon ignore = string2Poly("pts={60,-40:60,-160:150,-160:150,-40}");
  ASSERT_TRUE(ignore.is_convex());
  EXPECT_TRUE(ignore.contains(100, -100));
  EXPECT_FALSE(ignore.contains(30, -100));
  EXPECT_EQ(ignore.get_spec_pts(), "pts={60,-40:60,-160:150,-160:150,-40}");
}

TEST(XYFormatUtilsPolyTest, ParsesInactiveDeletionPayloadWithoutPoints)
{
  XYPolygon poly = string2Poly("label=old_region,active=false");

  EXPECT_FALSE(poly.active());
  EXPECT_EQ(poly.get_label(), "old_region");
  EXPECT_EQ(poly.size(), 0u);
  EXPECT_EQ(poly.get_spec(), "active=false,label=old_region");
}

TEST(XYFormatUtilsPolyTest, ParsesShortRadialCompatibilitySpec)
{
  XYPolygon poly = stringShortRadial2Poly("0,0,20,8,0.1,short_radial");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 8u);
  EXPECT_EQ(poly.get_label(), "short_radial");
  EXPECT_TRUE(poly.contains(0, 0));
  EXPECT_NEAR(poly.max_radius(), 20.0, 0.1);
}

TEST(XYFormatUtilsPolyTest, ParsesEllipseSpecUsedForDisplayAndRegionApproximation)
{
  XYPolygon poly = stringEllipse2Poly("x=0,y=0,major=40,minor=20,degs=30,pts=16,snap=0.01,label=ellipse_region");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 16u);
  EXPECT_EQ(poly.get_label(), "ellipse_region");
  EXPECT_TRUE(poly.contains(0, 0));
  EXPECT_GT(poly.area(), 500.0);
}

TEST(XYFormatUtilsPolyTest, ParsesFormatEllipsePayloadUsedByLoiterUpdates)
{
  XYPolygon poly = string2Poly("format=ellipse, x=110, y=-75, degs=150, pts=14, major=80, minor=20");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 14u);
  EXPECT_TRUE(poly.contains(110, -75));
  EXPECT_GT(poly.area(), 1100.0);
  EXPECT_LT(poly.area(), 1300.0);
  EXPECT_NEAR(poly.max_radius(), 40.0, kLooseGeomTol);
}

TEST(XYFormatUtilsPolyTest, ParsesPieWedgeAndRejectsNonConvexRangeWedgeSpec)
{
  XYPolygon wedge = stringPieWedge2Poly("x=0,y=0,lang=350,rang=20,range=50,pts=3,label=sector");

  ASSERT_TRUE(wedge.is_convex());
  EXPECT_EQ(wedge.get_label(), "sector");
  EXPECT_EQ(wedge.size(), 5u);
  EXPECT_TRUE(wedge.contains(0, 0));

  XYPolygon range_wedge = stringRangeWedge2Poly(
      "x=0,y=0,lang=350,rang=20,range_min=10,range_max=50,pts=3,label=annular_sector");

  EXPECT_FALSE(range_wedge.is_convex());
  EXPECT_EQ(range_wedge.size(), 0u);
}

TEST(XYFormatUtilsPolyTest, ParsesOvalShapeIntoConvexDisplayPolygon)
{
  XYPolygon oval = stringOval2Poly("x=0,y=0,rad=5,len=20,ang=90,label=turn_corridor");

  ASSERT_TRUE(oval.is_convex());
  EXPECT_GT(oval.size(), 8u);
  EXPECT_TRUE(oval.contains(0, 0));
  EXPECT_EQ(oval.get_label(), "");
}

TEST(XYFormatUtilsPolyTest, ParsesPylonCorridorSpecBetweenTwoPoints)
{
  XYPolygon poly = stringPylon2Poly("x1=0,y1=0,x2=40,y2=0,axis_pad=5,perp_pad=10,label=corridor");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 4u);
  EXPECT_EQ(poly.get_label(), "corridor");
  EXPECT_TRUE(poly.contains(20, 0));
  EXPECT_FALSE(poly.contains(20, 20));
}

TEST(XYFormatUtilsPolyTest, ParsesObstacleManagerKnownObstacleWithFiveVertices)
{
  XYPolygon obstacle = string2Poly("pts={59,-101:64,-105:64,-112:59,-116:53,-108},label=ob_0");

  ASSERT_TRUE(obstacle.is_convex());
  ASSERT_EQ(obstacle.size(), 5u);
  EXPECT_EQ(obstacle.get_label(), "ob_0");
  EXPECT_TRUE(obstacle.contains(59, -108));
  EXPECT_FALSE(obstacle.contains(70, -108));
  EXPECT_NEAR(obstacle.dist_to_poly(70, -108), 6.0, kLooseGeomTol);
}

TEST(XYFormatUtilsPolyTest, RejectsSelfIntersectingPolygonSpec)
{
  XYPolygon poly = string2Poly("pts={0,0:10,10:0,10:10,0},label=bowtie");

  EXPECT_FALSE(poly.is_convex());
  EXPECT_EQ(poly.size(), 0u);
}

TEST(XYFormatUtilsPolyTest, RejectsMissingMandatoryGeneratedShapeFields)
{
  EXPECT_FALSE(string2Poly("radial:: x=0,y=0,radius=10").is_convex());
  EXPECT_FALSE(string2Poly("ellipse:: x=0,y=0,major=10,minor=5,pts=8").is_convex());
  EXPECT_FALSE(string2Poly("rangewedge: x=0,y=0,lang=0,rang=90,range_min=50,range_max=10").is_convex());
  EXPECT_FALSE(string2Poly("pylon: x1=0,y1=0,x2=10,y2=0,axis_pad=5").is_convex());
}

TEST(XYFormatUtilsPolyTest, SetsPolygonAndOverridesLabelFromCaller)
{
  XYPolygon poly;

  ASSERT_TRUE(setPolyOnString(poly, "pts={0,0:5,0:5,5:0,5},label=ignored", "caller_label"));
  EXPECT_TRUE(poly.is_convex());
  EXPECT_EQ(poly.get_label(), "caller_label");
  EXPECT_FALSE(setPolyOnString(poly, "pts={0,0:10,10:0,10:10,0}", "bad"));
  EXPECT_EQ(poly.get_label(), "caller_label");
}

TEST(XYFormatUtilsPolyTest, SerializesPolygonInViewPolygonCompatiblePtsFormat)
{
  XYPolygon poly = makeSquarePoly(0, 0, 10, 10);
  poly.set_label("box");

  std::string spec = poly.get_spec();
  EXPECT_TRUE(stringContains(spec, "pts={"));
  EXPECT_TRUE(stringContains(spec, "0,0"));
  EXPECT_TRUE(stringContains(spec, "10,10"));
  EXPECT_TRUE(stringContains(spec, "label=box"));
}

TEST(XYPolygonTest, RoundTripsAvoidObstacleViewPolygonPayloads)
{
  // BHV_AvoidObstacle posts the original obstacle and buffer polygon on
  // VIEW_POLYGON after applying behavior visual hints.
  XYPolygon obstacle =
      string2Poly("pts={32,-100:38,-98:40,-100:32,-104},label=ob_23");
  ASSERT_TRUE(obstacle.is_convex());
  obstacle.set_active(true);
  obstacle.set_color("edge", "white");
  obstacle.set_color("vertex", "pink");
  obstacle.set_color("fill", "gray60");
  obstacle.set_transparency(0.35);

  XYPolygon parsed_obstacle = string2Poly(obstacle.get_spec(3));
  ASSERT_TRUE(parsed_obstacle.is_convex());
  ASSERT_EQ(parsed_obstacle.size(), 4u);
  EXPECT_EQ(parsed_obstacle.get_label(), "ob_23");
  EXPECT_TRUE(parsed_obstacle.active());
  EXPECT_EQ(parsed_obstacle.get_color_str("edge"), "white");
  EXPECT_EQ(parsed_obstacle.get_color_str("vertex"), "pink");
  EXPECT_EQ(parsed_obstacle.get_color_str("fill"), "gray60");
  EXPECT_TRUE(parsed_obstacle.transparency_set());
  EXPECT_NEAR(parsed_obstacle.get_transparency(), 0.35, kGeomTol);
  EXPECT_NEAR(parsed_obstacle.area(), 24.0, kGeomTol);

  XYPolygon buffer =
      string2Poly("pts={27,-105:38,-108:45,-100:35,-92:25,-96},label=ob_23_buff");
  ASSERT_TRUE(buffer.is_convex());
  buffer.set_active(true);
  buffer.set_color("edge", "orange");
  buffer.set_color("vertex", "yellow");
  buffer.set_color("fill", "red");
  buffer.set_transparency(0.6);

  XYPolygon parsed_buffer = string2Poly(buffer.get_spec(3));
  ASSERT_TRUE(parsed_buffer.is_convex());
  EXPECT_EQ(parsed_buffer.get_label(), "ob_23_buff");
  EXPECT_EQ(parsed_buffer.get_color_str("edge"), "orange");
  EXPECT_EQ(parsed_buffer.get_color_str("vertex"), "yellow");
  EXPECT_EQ(parsed_buffer.get_color_str("fill"), "red");
  EXPECT_TRUE(parsed_buffer.transparency_set());
  EXPECT_NEAR(parsed_buffer.get_transparency(), 0.6, kGeomTol);
  EXPECT_TRUE(parsed_buffer.contains(35, -100));
}

TEST(XYPolygonTest, RoundTripsAvoidObstacleV21PrecisionAndErasePayloads)
{
  // BHV_AvoidObstacleV21 posts obstacle, min buffer, and max buffer polygons
  // with 5-digit vertex precision and later erases them by label.
  XYPolygon obstacle = string2Poly(
      "pts={59.123456,-101.987654:64.123456,-105.987654:"
      "64.123456,-112.987654:59.123456,-116.987654:53.123456,-108.987654},"
      "label=one");
  ASSERT_TRUE(obstacle.is_convex());
  obstacle.set_active(true);
  obstacle.set_color("edge", "white");
  obstacle.set_color("vertex", "green");
  obstacle.set_vertex_size(2);
  obstacle.set_color("fill", "gray50");
  obstacle.set_transparency(0.2);

  XYPolygon parsed = string2Poly(obstacle.get_spec(5));
  ASSERT_TRUE(parsed.is_convex());
  ASSERT_EQ(parsed.size(), 5u);
  EXPECT_NEAR(parsed.get_vx(0), 59.12346, kGeomTol);
  EXPECT_NEAR(parsed.get_vy(0), -101.98765, kGeomTol);
  EXPECT_EQ(parsed.get_label(), "one");
  EXPECT_EQ(parsed.get_color_str("edge"), "white");
  EXPECT_EQ(parsed.get_color_str("vertex"), "green");
  EXPECT_EQ(parsed.get_color_str("fill"), "gray50");
  EXPECT_TRUE(parsed.vertex_size_set());
  EXPECT_NEAR(parsed.get_vertex_size(), 2.0, kGeomTol);
  EXPECT_TRUE(parsed.transparency_set());
  EXPECT_NEAR(parsed.get_transparency(), 0.2, kGeomTol);

  XYPolygon erased = string2Poly(obstacle.get_spec_inactive());
  ASSERT_TRUE(erased.is_convex());
  EXPECT_EQ(erased.get_label(), "one");
  EXPECT_FALSE(erased.active());
  EXPECT_EQ(erased.size(), 3u);
  EXPECT_NEAR(erased.area(), 40.5, kGeomTol);
}

TEST(XYPolygonTest, RoundTripsSimMarineWormholeViewerPolygons)
{
  // uSimMarine V22/V23 asks each WormHole for its convex polygons and posts
  // each generated spec as VIEW_POLYGON.
  XYPolygon weber = string2Poly(
      "pts={-60,-20:-20,-20:-20,20:-60,20},label=weber_hole,"
      "edge_color=cyan,vertex_color=white");
  XYPolygon madrid = string2Poly(
      "pts={120,-40:165,-20:150,35:105,20},label=madrid_hole,"
      "edge_color=magenta,vertex_color=white");
  ASSERT_TRUE(weber.is_convex());
  ASSERT_TRUE(madrid.is_convex());

  XYPolygon parsed_weber = string2Poly(weber.get_spec());
  XYPolygon parsed_madrid = string2Poly(madrid.get_spec());

  ASSERT_TRUE(parsed_weber.is_convex());
  ASSERT_TRUE(parsed_madrid.is_convex());
  EXPECT_EQ(parsed_weber.get_label(), "weber_hole");
  EXPECT_EQ(parsed_madrid.get_label(), "madrid_hole");
  EXPECT_EQ(parsed_weber.get_color_str("edge"), "cyan");
  EXPECT_EQ(parsed_madrid.get_color_str("edge"), "magenta");
  EXPECT_TRUE(parsed_weber.contains(-40, 0));
  EXPECT_TRUE(parsed_madrid.contains(135, 0));
  EXPECT_FALSE(parsed_weber.intersects(parsed_madrid));
}

TEST(XYPolygonTest, ComputesContainmentAreaPerimeterAndBoundaryDistance)
{
  XYPolygon op_region = makeSquarePoly(0, 0, 10, 10);

  ASSERT_TRUE(op_region.is_convex());
  EXPECT_NEAR(op_region.area(), 100.0, kGeomTol);
  EXPECT_NEAR(op_region.perim(), 40.0, kGeomTol);
  EXPECT_NEAR(op_region.max_radius(), 7.0710678119, kLooseGeomTol);
  EXPECT_TRUE(op_region.contains(0, 0));
  EXPECT_TRUE(op_region.contains(10, 5));
  EXPECT_TRUE(op_region.contains(XYPoint(5, 5)));
  EXPECT_FALSE(op_region.contains(11, 5));
  EXPECT_NEAR(op_region.dist_to_poly(15, 5), 5.0, kGeomTol);
  EXPECT_NEAR(op_region.dist_to_poly(5, 5), 5.0, kGeomTol);
}

TEST(XYPolygonTest, ComputesRaySegmentAndLineIntersectionsForObstacleChecks)
{
  XYPolygon obstacle = makeSquarePoly(0, 0, 10, 10);

  EXPECT_TRUE(obstacle.seg_intercepts(-5, 5, 15, 5));
  EXPECT_FALSE(obstacle.seg_intercepts(-5, 15, 15, 15));
  EXPECT_NEAR(obstacle.dist_to_poly(-5, 5, 15, 5), 0.0, kGeomTol);
  EXPECT_NEAR(obstacle.dist_to_poly(-5, 15, 15, 15), 5.0, kGeomTol);
  EXPECT_NEAR(obstacle.dist_to_poly(-5, 5, 90), 5.0, kGeomTol);
  EXPECT_NEAR(obstacle.dist_to_poly(-5, 15, 90), -1.0, kGeomTol);

  double ix1 = 0;
  double iy1 = 0;
  double ix2 = 0;
  double iy2 = 0;
  ASSERT_TRUE(obstacle.line_intersects(-5, 5, 15, 5, ix1, iy1, ix2, iy2));
  EXPECT_NEAR(ix1, 10.0, kGeomTol);
  EXPECT_NEAR(iy1, 5.0, kGeomTol);
  EXPECT_NEAR(ix2, 0.0, kGeomTol);
  EXPECT_NEAR(iy2, 5.0, kGeomTol);
}

TEST(XYPolygonTest, ComputesPolygonContainmentIntersectionAndSeparation)
{
  XYPolygon outer = makeSquarePoly(0, 0, 10, 10);
  XYPolygon inner = makeSquarePoly(2, 2, 4, 4);
  XYPolygon overlap = makeSquarePoly(5, 5, 15, 15);
  XYPolygon far = makeSquarePoly(20, 0, 30, 10);
  XYSegList lane;
  lane.add_vertex(1, 1);
  lane.add_vertex(9, 9);

  EXPECT_TRUE(outer.contains(inner));
  EXPECT_TRUE(outer.contains(lane));
  EXPECT_FALSE(outer.contains(overlap));
  EXPECT_TRUE(outer.intersects(overlap));
  EXPECT_FALSE(outer.intersects(far));
  EXPECT_NEAR(outer.dist_to_poly(overlap), 0.0, kGeomTol);
  EXPECT_NEAR(outer.dist_to_poly(far), 10.0, kGeomTol);
}

TEST(XYPolygonTest, IntersectsSquareForViewerSelectionAndGridWindowing)
{
  XYPolygon poly = makeSquarePoly(0, 0, 10, 10);

  EXPECT_TRUE(poly.intersects(XYSquare(5, 15, 5, 15)));
  EXPECT_TRUE(poly.intersects(XYSquare(-5, 2, 2, 8)));
  EXPECT_TRUE(poly.intersects(XYSquare(2, 8, 2, 8)));
  EXPECT_FALSE(poly.intersects(XYSquare(20, 30, 20, 30)));

  XYPolygon empty;
  EXPECT_FALSE(empty.intersects(XYSquare(0, 0, 1, 1)));
}

TEST(XYPolygonTest, HandlesDegeneratePointAndSegmentPolygonsForDistanceAndIntersections)
{
  XYPolygon point_poly;
  point_poly.add_vertex(3, 4);
  EXPECT_FALSE(point_poly.is_convex());
  EXPECT_NEAR(point_poly.dist_to_poly(0, 0), 5.0, kGeomTol);
  EXPECT_TRUE(point_poly.seg_intercepts(0, 0, 6, 8));
  EXPECT_FALSE(point_poly.seg_intercepts(0, 0, 2, 2));

  double ix1 = -1;
  double iy1 = -1;
  double ix2 = -1;
  double iy2 = -1;
  EXPECT_TRUE(point_poly.line_intersects(0, 0, 6, 8, ix1, iy1, ix2, iy2));
  EXPECT_NEAR(ix1, 3.0, kGeomTol);
  EXPECT_NEAR(iy1, 4.0, kGeomTol);
  EXPECT_NEAR(ix2, 3.0, kGeomTol);
  EXPECT_NEAR(iy2, 4.0, kGeomTol);

  XYPolygon segment_poly;
  segment_poly.add_vertex(0, 0, false);
  segment_poly.add_vertex(10, 0, false);
  segment_poly.determine_convexity();
  EXPECT_FALSE(segment_poly.is_convex());
  EXPECT_NEAR(segment_poly.dist_to_poly(5, 5), 5.0, kGeomTol);
  EXPECT_TRUE(segment_poly.seg_intercepts(5, -5, 5, 5));
  EXPECT_FALSE(segment_poly.seg_intercepts(20, -5, 20, 5));
}

TEST(XYPolygonTest, FindsClosestBoundaryPointAndExportsWaypointLoopFromNearestVertex)
{
  XYPolygon region = makeSquarePoly(0, 0, 10, 10);

  double rx = 0;
  double ry = 0;
  ASSERT_TRUE(region.closest_point_on_poly(15, 5, rx, ry));
  EXPECT_NEAR(rx, 10.0, kGeomTol);
  EXPECT_NEAR(ry, 5.0, kGeomTol);

  XYPoint inside = region.closest_point_on_poly(XYPoint(5, 5));
  EXPECT_NEAR(inside.x(), 5.0, kGeomTol);
  EXPECT_NEAR(inside.y(), 10.0, kGeomTol);

  XYSegList loop = region.exportSegList(9, 9);
  ASSERT_EQ(loop.size(), 4u);
  EXPECT_NEAR(loop.get_vx(0), 10.0, kGeomTol);
  EXPECT_NEAR(loop.get_vy(0), 10.0, kGeomTol);
  EXPECT_NEAR(loop.get_vx(1), 0.0, kGeomTol);
  EXPECT_NEAR(loop.get_vy(1), 10.0, kGeomTol);
}

TEST(XYPolygonTest, TransformsAndMaintainsConvexityForViewableRegions)
{
  XYPolygon poly = makeSquarePoly(0, 0, 10, 10);

  poly.grow_by_pct(0.5);
  EXPECT_TRUE(poly.is_convex());
  EXPECT_NEAR(poly.area(), 225.0, kGeomTol);
  EXPECT_NEAR(poly.get_min_x(), -2.5, kGeomTol);
  EXPECT_NEAR(poly.get_max_x(), 12.5, kGeomTol);

  poly = makeSquarePoly(0, 0, 10, 10);
  poly.grow_by_amt(5);
  EXPECT_TRUE(poly.is_convex());
  EXPECT_NEAR(poly.get_min_x(), -3.5355339059, kLooseGeomTol);
  EXPECT_NEAR(poly.get_max_x(), 13.5355339059, kLooseGeomTol);

  poly = makeSquarePoly(0, 0, 10, 10);
  poly.rotate(90, 0, 0);
  EXPECT_TRUE(poly.is_convex());
  EXPECT_NEAR(poly.get_vx(1), 0.0, kLooseGeomTol);
  EXPECT_NEAR(poly.get_vy(1), -10.0, kLooseGeomTol);

  poly = makeSquarePoly(0, 0, 10, 10);
  EXPECT_TRUE(poly.apply_snap(3));
  EXPECT_TRUE(poly.is_convex());
  EXPECT_NEAR(poly.get_vx(1), 9.0, kGeomTol);
  EXPECT_NEAR(poly.get_vy(2), 9.0, kGeomTol);
}

TEST(XYPolygonTest, ReverseRotateAboutCenterAndClearMaintainExpectedState)
{
  XYPolygon poly = string2Poly("pts={0,0:10,0:10,20:0,20},label=survey_box");
  ASSERT_TRUE(poly.is_convex());

  poly.reverse();
  EXPECT_TRUE(poly.is_convex());
  EXPECT_NEAR(poly.area(), 200.0, kGeomTol);
  EXPECT_NEAR(poly.get_vx(0), 0.0, kGeomTol);
  EXPECT_NEAR(poly.get_vy(0), 20.0, kGeomTol);

  poly.rotate(180);
  EXPECT_TRUE(poly.is_convex());
  EXPECT_TRUE(poly.contains(5, 10));
  EXPECT_NEAR(poly.area(), 200.0, kGeomTol);

  poly.clear();
  EXPECT_FALSE(poly.is_convex());
  EXPECT_EQ(poly.size(), 0u);
  EXPECT_NEAR(poly.area(), 0.0, kGeomTol);
  EXPECT_NEAR(poly.perim(), 0.0, kGeomTol);
  EXPECT_FALSE(poly.contains(0, 0));
}

TEST(XYPolygonTest, EditsAndSimplifiesConvexPolygonVertices)
{
  XYPolygon poly = string2Poly("pts={0,0:1,0:10,0:10,10:0,10}");

  ASSERT_TRUE(poly.is_convex());
  ASSERT_EQ(poly.size(), 5u);
  EXPECT_TRUE(poly.simplify(2));
  EXPECT_EQ(poly.size(), 4u);
  EXPECT_TRUE(poly.is_convex());
  EXPECT_NEAR(poly.get_vx(0), 0.5, kGeomTol);
  EXPECT_NEAR(poly.area(), 97.5, kGeomTol);

  EXPECT_TRUE(poly.insert_vertex(5, 10));
  EXPECT_EQ(poly.size(), 5u);
  EXPECT_TRUE(poly.delete_vertex(5, 10));
  EXPECT_EQ(poly.size(), 4u);
  EXPECT_FALSE(poly.delete_vertex(99));

  XYPolygon delta_poly;
  delta_poly.add_vertex(0, 0, false);
  delta_poly.add_vertex(10, 0, false);
  EXPECT_EQ(delta_poly.size(), 2u);
  delta_poly.add_vertex_delta(10.1, 0.1, 1, false);
  EXPECT_EQ(delta_poly.size(), 2u);
  delta_poly.add_vertex_delta(10, 10, 1, false);
  EXPECT_EQ(delta_poly.size(), 3u);
  delta_poly.determine_convexity();
  EXPECT_TRUE(delta_poly.is_convex());
}

TEST(XYPolygonTest, AlterDeleteAndInsertCanInvalidateConcaveEdits)
{
  XYPolygon poly = makeSquarePoly(0, 0, 10, 10);

  EXPECT_TRUE(poly.alter_vertex(5, 5));
  EXPECT_TRUE(poly.is_convex());
  EXPECT_FALSE(poly.contains(1, 1));

  poly = makeSquarePoly(0, 0, 10, 10);
  EXPECT_TRUE(poly.insert_vertex(5, 0));
  EXPECT_TRUE(poly.is_convex());
  EXPECT_EQ(poly.size(), 5u);
  EXPECT_FALSE(poly.alter_vertex(5, 5));
  EXPECT_FALSE(poly.is_convex());

  poly = makeSquarePoly(0, 0, 10, 10);
  EXPECT_TRUE(poly.delete_vertex(0, 0));
  EXPECT_TRUE(poly.is_convex());
  EXPECT_EQ(poly.size(), 3u);
  EXPECT_FALSE(poly.delete_vertex(10, 0));
  EXPECT_FALSE(poly.is_convex());
  EXPECT_EQ(poly.size(), 2u);
}

TEST(XYPolygonTest, SnapRejectsConvexityBreakingCollapseButAllowsNonConvexCollapse)
{
  XYPolygon thin;
  thin.add_vertex(0, 0, false);
  thin.add_vertex(1, 0, false);
  thin.add_vertex(1, 10, false);
  thin.add_vertex(0, 10, false);
  thin.determine_convexity();
  ASSERT_TRUE(thin.is_convex());

  EXPECT_FALSE(thin.apply_snap(10));
  EXPECT_TRUE(thin.is_convex());
  EXPECT_NEAR(thin.get_vx(1), 1.0, kGeomTol);

  XYPolygon nonconvex;
  nonconvex.add_vertex(0, 0, false);
  nonconvex.add_vertex(10, 0, false);
  nonconvex.add_vertex(5, 5, false);
  nonconvex.add_vertex(10, 10, false);
  nonconvex.add_vertex(0, 10, false);
  nonconvex.determine_convexity();
  ASSERT_FALSE(nonconvex.is_convex());
  EXPECT_TRUE(nonconvex.apply_snap(10));
}

TEST(XYPolygonTest, SettlesNonConvexPolygonByRemovingColinearWeakPoint)
{
  XYPolygon nonconvex;
  nonconvex.add_vertex(0, 0, false);
  nonconvex.add_vertex(10, 0, false);
  nonconvex.add_vertex(5, 3, false);
  nonconvex.add_vertex(10, 10, false);
  nonconvex.add_vertex(0, 10, false);
  nonconvex.determine_convexity();

  ASSERT_FALSE(nonconvex.is_convex());
  EXPECT_NEAR(nonconvex.area(), 75.0, kGeomTol);

  bool ok = false;
  EXPECT_EQ(nonconvex.min_xproduct(ok), 1u);
  EXPECT_TRUE(ok);

  XYPolygon settled = nonconvex.crossProductSettle();
  EXPECT_TRUE(settled.is_convex());
  EXPECT_EQ(settled.size(), 4u);
  EXPECT_NEAR(settled.area(), 60.0, kGeomTol);
  EXPECT_TRUE(settled.contains(5, 3));
}

TEST(XYPolygonTest, BuildsRadialPolygonAndClassifiesViewableVertices)
{
  XYPolygon radial;
  ASSERT_TRUE(radial.setRadial(0, 0, 10, 8, 0.1));
  EXPECT_TRUE(radial.is_convex());
  EXPECT_EQ(radial.size(), 8u);
  EXPECT_TRUE(radial.contains(0, 0));
  EXPECT_NEAR(radial.max_radius(), 10.0, 0.1);

  EXPECT_FALSE(radial.setRadial(0, 0, 0, 8));
  EXPECT_FALSE(radial.setRadial(0, 0, 10, 2));

  XYPolygon square = makeSquarePoly(0, 0, 10, 10);
  EXPECT_FALSE(square.vertex_is_viewable(99, 20, 5));
  EXPECT_FALSE(square.vertex_is_viewable(0, 5, 5));
  EXPECT_TRUE(square.vertex_is_viewable(1, 20, 5));
  EXPECT_TRUE(square.vertex_is_viewable(2, 20, 5));
  EXPECT_FALSE(square.vertex_is_viewable(3, 20, 5));
}
