#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsPoint.h"

// Covers XY format utils point behavior: parses view point style standard spec with hints.
TEST(XYFormatUtilsPointTest, ParsesViewPointStyleStandardSpecWithHints)
{
  XYPoint point = stringStandard2Point(
      "x=12.5,y=-3,z=2,label=alpha,type=waypoint,source=pMarineViewer,"
      "vsource=abe,msg=arrived,id=pt-1,vertex_size=4,vertex_color=yellow,"
      "label_color=white,time=10,duration=5,fill_transparency=0.4");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 12.5, kGeomTol);
  EXPECT_NEAR(point.y(), -3.0, kGeomTol);
  EXPECT_NEAR(point.z(), 2.0, kGeomTol);
  EXPECT_EQ(point.get_label(), "alpha");
  EXPECT_EQ(point.get_type(), "waypoint");
  EXPECT_EQ(point.get_source(), "pMarineViewer");
  EXPECT_EQ(point.get_vsource(), "abe");
  EXPECT_EQ(point.get_msg(), "arrived");
  EXPECT_EQ(point.get_id(), "pt-1");
  EXPECT_TRUE(point.vertex_size_set());
  EXPECT_NEAR(point.get_vertex_size(), 4.0, kGeomTol);
  EXPECT_EQ(point.get_color_str("vertex"), "yellow");
  EXPECT_EQ(point.get_color_str("label"), "white");
  EXPECT_TRUE(point.time_set());
  EXPECT_NEAR(point.get_time(), 10.0, kGeomTol);
  EXPECT_TRUE(point.duration_set());
  EXPECT_NEAR(point.get_duration(), 5.0, kGeomTol);
  EXPECT_TRUE(point.transparency_set());
  EXPECT_NEAR(point.get_transparency(), 0.4, kGeomTol);
}

// Covers XY format utils point behavior: parses obstacle manager view point script payload.
TEST(XYFormatUtilsPointTest, ParsesObstacleManagerViewPointScriptPayload)
{
  XYPoint point = string2Point(
      "x=12,y=-34,active=true,label=pt7,msg=a,type=obstacle,"
      "label_color=invisible,vertex_color=yellow,vertex_size=2");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 12.0, kGeomTol);
  EXPECT_NEAR(point.y(), -34.0, kGeomTol);
  EXPECT_TRUE(point.active());
  EXPECT_EQ(point.get_label(), "pt7");
  EXPECT_EQ(point.get_msg(), "a");
  EXPECT_EQ(point.get_type(), "obstacle");
  EXPECT_EQ(point.get_color_str("label"), "invisible");
  EXPECT_EQ(point.get_color_str("vertex"), "yellow");
  EXPECT_TRUE(point.vertex_size_set());
  EXPECT_NEAR(point.get_vertex_size(), 2.0, kGeomTol);
}

// Covers XY format utils point behavior: applies standard spec shift and trans as object transparency.
TEST(XYFormatUtilsPointTest, AppliesStandardSpecShiftAndTransAsObjectTransparency)
{
  XYPoint point = string2Point("x=10,y=20,z=3,shiftx=-2,shifty=5,shiftz=7,trans=2,hdg=270");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 8.0, kGeomTol);
  EXPECT_NEAR(point.y(), 25.0, kGeomTol);
  EXPECT_NEAR(point.z(), 10.0, kGeomTol);
  EXPECT_NEAR(point.get_pt_trans(), 0.0, kGeomTol);
  EXPECT_TRUE(point.transparency_set());
  EXPECT_NEAR(point.get_transparency(), 1.0, kGeomTol);
}

// Covers XY format utils point behavior: clamps object transparency and duration hints from standard spec.
TEST(XYFormatUtilsPointTest, ClampsObjectTransparencyAndDurationHintsFromStandardSpec)
{
  XYPoint low = string2Point("x=1,y=2,fill_transparency=-2,duration=-5");
  ASSERT_TRUE(low.valid());
  EXPECT_TRUE(low.transparency_set());
  EXPECT_NEAR(low.get_transparency(), 0.0, kGeomTol);
  EXPECT_TRUE(low.duration_set());
  EXPECT_NEAR(low.get_duration(), -1.0, kGeomTol);

  XYPoint high = string2Point("x=1,y=2,fill_transparency=2,duration=7");
  ASSERT_TRUE(high.valid());
  EXPECT_TRUE(high.transparency_set());
  EXPECT_NEAR(high.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(high.duration_set());
  EXPECT_NEAR(high.get_duration(), 7.0, kGeomTol);
}

// Covers XY format utils point behavior: supports simple coordinate only forms.
TEST(XYFormatUtilsPointTest, SupportsSimpleCoordinateOnlyForms)
{
  XYPoint comma = string2Point("3,4");
  ASSERT_TRUE(comma.valid());
  EXPECT_NEAR(comma.x(), 3.0, kGeomTol);
  EXPECT_NEAR(comma.y(), 4.0, kGeomTol);

  XYPoint colon = string2Point("5:6");
  ASSERT_TRUE(colon.valid());
  EXPECT_NEAR(colon.x(), 5.0, kGeomTol);
  EXPECT_NEAR(colon.y(), 6.0, kGeomTol);

  XYPoint negative = string2Point("-3.5,-4.25");
  ASSERT_TRUE(negative.valid());
  EXPECT_NEAR(negative.x(), -3.5, kGeomTol);
  EXPECT_NEAR(negative.y(), -4.25, kGeomTol);
}

// Covers XY format utils point behavior: parses abbreviated point spec used in point lists.
TEST(XYFormatUtilsPointTest, ParsesAbbreviatedPointSpecUsedInPointLists)
{
  XYPoint point = stringAbbreviated2Point("5,6,7:label,bravo:vertex_size,3:active,false:edge_color,blue");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 5.0, kGeomTol);
  EXPECT_NEAR(point.y(), 6.0, kGeomTol);
  EXPECT_NEAR(point.z(), 7.0, kGeomTol);
  EXPECT_EQ(point.get_label(), "bravo");
  EXPECT_NEAR(point.get_vertex_size(), 3.0, kGeomTol);
  EXPECT_FALSE(point.active());
  EXPECT_EQ(point.get_color_str("edge"), "blue");
}

// Covers XY format utils point behavior: rejects malformed point spec.
TEST(XYFormatUtilsPointTest, RejectsMalformedPointSpec)
{
  EXPECT_FALSE(string2Point("5").valid());
  EXPECT_FALSE(string2Point("label,missing_vertex").valid());
  EXPECT_FALSE(string2Point("x=5,label=missing_y").valid());
  EXPECT_FALSE(string2Point("y=5,label=missing_x").valid());
  EXPECT_FALSE(string2Point("1,2,3,4:label,too_many_numbers").valid());
}

// Covers XY format utils point behavior: preserves current standard spec numeric coercion.
TEST(XYFormatUtilsPointTest, PreservesCurrentStandardSpecNumericCoercion)
{
  XYPoint point = string2Point("x=not-a-number,y=4,z=also-bad,label=coerced");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 0.0, kGeomTol);
  EXPECT_NEAR(point.y(), 4.0, kGeomTol);
  EXPECT_NEAR(point.z(), 0.0, kGeomTol);
  EXPECT_EQ(point.get_label(), "coerced");
}

// Covers XY format utils point behavior: ignores invalid metadata but keeps valid vertex.
TEST(XYFormatUtilsPointTest, IgnoresInvalidMetadataButKeepsValidVertex)
{
  XYPoint point = string2Point(
      "1,2:active,maybe:vertex_size,-3:edge_size,-1:duration,-5:"
      "fill_transparency,-2:vertex_color,notacolor");

  ASSERT_TRUE(point.valid());
  EXPECT_TRUE(point.active());
  EXPECT_FALSE(point.vertex_size_set());
  EXPECT_FALSE(point.edge_size_set());
  EXPECT_TRUE(point.duration_set());
  EXPECT_NEAR(point.get_duration(), -1.0, kGeomTol);
  EXPECT_TRUE(point.transparency_set());
  EXPECT_NEAR(point.get_transparency(), 0.0, kGeomTol);
  EXPECT_FALSE(point.color_set("vertex"));
}

// Covers XY point behavior: constructs and clears validity and metadata.
TEST(XYPointTest, ConstructsAndClearsValidityAndMetadata)
{
  XYPoint point(1, 2, "marker");

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 1.0, kGeomTol);
  EXPECT_NEAR(point.y(), 2.0, kGeomTol);
  EXPECT_EQ(point.get_label(), "marker");

  point.set_color("vertex", "red");
  point.set_vertex_size(4);
  point.set_time(10);
  point.set_duration(5);
  EXPECT_FALSE(point.expired(14.5));
  EXPECT_TRUE(point.expired(16));

  point.clear();
  EXPECT_FALSE(point.valid());
  EXPECT_EQ(point.get_label(), "");
  EXPECT_FALSE(point.color_set("vertex"));
  EXPECT_FALSE(point.vertex_size_set());
  EXPECT_FALSE(point.time_set());
  EXPECT_FALSE(point.duration_set());
  EXPECT_TRUE(point.active());
}

// Covers XY point behavior: renders generic sensor style point after setters without marking valid.
TEST(XYPointTest, RendersGenericSensorStylePointAfterSettersWithoutMarkingValid)
{
  XYPoint point;
  point.set_vx(1.23456789);
  point.set_vy(9.87654321);
  point.set_active(true);
  point.set_vertex_size(4);
  point.set_color("vertex", "white");

  EXPECT_FALSE(point.valid());
  EXPECT_EQ(point.get_spec_xy(), "1.23,9.88");
  std::string spec = point.get_spec();
  EXPECT_TRUE(stringContains(spec, "x=1.23"));
  EXPECT_TRUE(stringContains(spec, "y=9.88"));
  EXPECT_TRUE(stringContains(spec, "vertex_color=white"));
  EXPECT_TRUE(stringContains(spec, "vertex_size=4"));
}

// Covers XY point behavior: direct vertex mutators shift invalidate and report squared magnitude.
TEST(XYPointTest, DirectVertexMutatorsShiftInvalidateAndReportSquaredMagnitude)
{
  XYPoint point;
  point.set_vertex(3, 4, 5);

  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), 3.0, kGeomTol);
  EXPECT_NEAR(point.y(), 4.0, kGeomTol);
  EXPECT_NEAR(point.z(), 5.0, kGeomTol);
  EXPECT_NEAR(point.magnitude(), 25.0, kGeomTol);

  point.set_vz(6);
  point.shift_x(-1);
  point.shift_y(2);
  point.shift_z(-3);
  EXPECT_NEAR(point.x(), 2.0, kGeomTol);
  EXPECT_NEAR(point.y(), 6.0, kGeomTol);
  EXPECT_NEAR(point.z(), 3.0, kGeomTol);
  EXPECT_NEAR(point.magnitude(), 40.0, kGeomTol);

  point.invalidate();
  EXPECT_FALSE(point.valid());
}

// Covers XY point behavior: applies snap and project point using MOOS heading convention.
TEST(XYPointTest, AppliesSnapAndProjectPointUsingMoosHeadingConvention)
{
  XYPoint point(1.24, 9.76, 2.26);
  point.apply_snap(0.5);
  EXPECT_NEAR(point.x(), 1.0, kGeomTol);
  EXPECT_NEAR(point.y(), 10.0, kGeomTol);
  EXPECT_NEAR(point.z(), 2.5, kGeomTol);

  XYPoint east;
  east.projectPt(XYPoint(0, 0), 90, 10);
  EXPECT_FALSE(east.valid());
  EXPECT_NEAR(east.x(), 10.0, kGeomTol);
  EXPECT_NEAR(east.y(), 0.0, kGeomTol);

  XYPoint north;
  north.projectPt(XYPoint(1, 2), 0, 5);
  EXPECT_NEAR(north.x(), 1.0, kGeomTol);
  EXPECT_NEAR(north.y(), 7.0, kGeomTol);
}

// Covers XY point behavior: serializes point transparency and spec digits.
TEST(XYPointTest, SerializesPointTransparencyAndSpecDigits)
{
  XYPoint point(1.23456789, 9.87654321, 3.456789);
  point.set_pt_trans(-2);
  EXPECT_NEAR(point.get_pt_trans(), 0.0, kGeomTol);
  point.set_pt_trans(2);
  EXPECT_NEAR(point.get_pt_trans(), 1.0, kGeomTol);

  point.set_spec_digits(99);
  std::string high_digits = point.get_spec();
  EXPECT_TRUE(stringContains(high_digits, "x=1.234568"));
  EXPECT_TRUE(stringContains(high_digits, "y=9.876543"));
  EXPECT_TRUE(stringContains(high_digits, "z=3.456789"));
  EXPECT_TRUE(stringContains(high_digits, "trans=1"));

  point.set_spec_digits(0);
  EXPECT_EQ(point.get_spec_xy(':'), "1:10");
}

// Covers XY point behavior: supports value comparison by coordinates and magnitude ordering.
TEST(XYPointTest, SupportsValueComparisonByCoordinatesAndMagnitudeOrdering)
{
  XYPoint near_point(1, 1);
  XYPoint same_xy(1, 1, 5);
  XYPoint far_point(3, 4);

  EXPECT_TRUE(near_point == same_xy);
  EXPECT_FALSE(near_point == far_point);
  EXPECT_TRUE(near_point < far_point);

  std::vector<XYPoint> points;
  points.push_back(far_point);
  points.push_back(near_point);
  std::sort(points.begin(), points.end());
  EXPECT_TRUE(points.front() == near_point);
}

// Covers XY format utils point behavior: serializes view point compatible payloads.
TEST(XYFormatUtilsPointTest, SerializesViewPointCompatiblePayloads)
{
  XYPoint point = string2Point("x=1.234,y=5.678,z=9,label=alpha,vertex_size=2,vertex_color=yellow,active=false");
  point.set_spec_digits(2);

  std::string spec = point.get_spec();
  EXPECT_TRUE(stringContains(spec, "x=1.23"));
  EXPECT_TRUE(stringContains(spec, "y=5.68"));
  EXPECT_TRUE(stringContains(spec, "z=9"));
  EXPECT_TRUE(stringContains(spec, "active=false"));
  EXPECT_TRUE(stringContains(spec, "label=alpha"));
  EXPECT_TRUE(stringContains(spec, "vertex_color=yellow"));
  EXPECT_TRUE(stringContains(spec, "vertex_size=2"));

  EXPECT_EQ(point.get_spec_inactive(), "x=0,y=0,active=false,label=alpha");
}

// Covers XY point behavior: serializes waypoint and loiter view point lifecycle payloads.
TEST(XYPointTest, SerializesWaypointAndLoiterViewPointLifecyclePayloads)
{
  XYPoint nextpt(20, -5, "alpha_waypoint");
  nextpt.set_color("label", "white");
  nextpt.set_color("vertex", "green");
  nextpt.set_vertex_size(6);

  std::string active_spec = nextpt.get_spec("active=true");
  EXPECT_TRUE(stringContains(active_spec, "x=20"));
  EXPECT_TRUE(stringContains(active_spec, "y=-5"));
  EXPECT_TRUE(stringContains(active_spec, "active=true"));
  EXPECT_TRUE(stringContains(active_spec, "label=alpha_waypoint"));
  EXPECT_TRUE(stringContains(active_spec, "label_color=white"));
  EXPECT_TRUE(stringContains(active_spec, "vertex_color=green"));
  EXPECT_TRUE(stringContains(active_spec, "vertex_size=6"));

  nextpt.set_active(false);
  std::string inactive_spec = nextpt.get_spec();
  EXPECT_TRUE(stringContains(inactive_spec, "active=false"));
  EXPECT_EQ(nextpt.get_spec_inactive(), "x=0,y=0,active=false,label=alpha_waypoint");
}

// Covers XY point behavior: round trips attractor trail point lifecycle payloads.
TEST(XYPointTest, RoundTripsAttractorTrailPointLifecyclePayloads)
{
  // BHV_Attractor keeps a cached trail point, updates its vertex from the
  // contact node, toggles active, and posts the generated spec as VIEW_POINT.
  XYPoint trail_point;
  trail_point.set_label("abe_attractor");
  trail_point.set_active("false");
  trail_point.set_vertex(-30.5, 142.25);

  trail_point.set_active(true);
  XYPoint visible = string2Point(trail_point.get_spec());
  ASSERT_TRUE(visible.valid());
  EXPECT_NEAR(visible.x(), -30.5, kGeomTol);
  EXPECT_NEAR(visible.y(), 142.25, kGeomTol);
  EXPECT_EQ(visible.get_label(), "abe_attractor");
  EXPECT_TRUE(visible.active());

  trail_point.set_active(false);
  XYPoint erased = string2Point(trail_point.get_spec());
  ASSERT_TRUE(erased.valid());
  EXPECT_EQ(erased.get_label(), "abe_attractor");
  EXPECT_FALSE(erased.active());
}

// Covers XY point behavior: round trips rubber band station point with behavior source.
TEST(XYPointTest, RoundTripsRubberBandStationPointWithBehaviorSource)
{
  // BHV_RubberBand adds type/source metadata to the station point before it
  // posts VIEW_POINT. XYPoint serializes source, while the deprecated type
  // field is currently not emitted by XYObject::get_spec.
  XYPoint station;
  station.set_label("abe_station");
  station.set_type("station");
  station.set_active("false");
  station.set_source("abe_rubber_band");
  station.set_vertex(80, -35);

  station.set_active(true);
  XYPoint parsed = string2Point(station.get_spec());

  ASSERT_TRUE(parsed.valid());
  EXPECT_NEAR(parsed.x(), 80.0, kGeomTol);
  EXPECT_NEAR(parsed.y(), -35.0, kGeomTol);
  EXPECT_EQ(parsed.get_label(), "abe_station");
  EXPECT_EQ(parsed.get_type(), "");
  EXPECT_EQ(parsed.get_source(), "abe_rubber_band");
  EXPECT_TRUE(parsed.active());
}
