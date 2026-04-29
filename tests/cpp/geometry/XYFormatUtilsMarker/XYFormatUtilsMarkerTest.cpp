#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsMarker.h"
#include "XYMarker.h"

TEST(XYFormatUtilsMarkerTest, ParsesMapMarkersViewMarkerPayload)
{
  XYMarker marker = stringStandard2Marker(
      "x=167.97,y=-202.57,width=3,primary_color=gray40,type=circle,"
      "label=ZT,label_color=gray60,owner=survey");

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_NEAR(marker.get_vx(), 167.97, kGeomTol);
  EXPECT_NEAR(marker.get_vy(), -202.57, kGeomTol);
  EXPECT_TRUE(marker.is_set_width());
  EXPECT_NEAR(marker.get_width(), 3.0, kGeomTol);
  EXPECT_TRUE(marker.is_set_type());
  EXPECT_EQ(marker.get_type(), "circle");
  EXPECT_EQ(marker.get_label(), "ZT");
  EXPECT_EQ(marker.get_owner(), "survey");
  EXPECT_EQ(marker.get_color_str("primary_color"), "gray40");
  EXPECT_EQ(marker.get_color_str("label"), "gray60");
}

TEST(XYFormatUtilsMarkerTest, ParsesBeaconStyleColorAliasAndRange)
{
  XYMarker marker = string2Marker(
      "xpos=400,ypos=-200,scale=8,range=55,color=orange,"
      "secondary_color=blue,type=triangle,active=false");

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_NEAR(marker.get_vx(), 400.0, kGeomTol);
  EXPECT_NEAR(marker.get_vy(), -200.0, kGeomTol);
  EXPECT_TRUE(marker.is_set_width());
  EXPECT_NEAR(marker.get_width(), 8.0, kGeomTol);
  EXPECT_TRUE(marker.is_set_range());
  EXPECT_NEAR(marker.get_range(), 55.0, kGeomTol);
  EXPECT_EQ(marker.get_color_str("primary_color"), "orange");
  EXPECT_EQ(marker.get_color_str("secondary_color"), "blue");
  EXPECT_EQ(marker.get_type(), "triangle");
  EXPECT_FALSE(marker.active());
}

TEST(XYFormatUtilsMarkerTest, ParsesDocumentedBeaconRangeSensorViewMarkerPayload)
{
  XYMarker marker = string2Marker(
      "x=400,y=-200,label=02,color=orange,type=circle,width=4");

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_NEAR(marker.get_vx(), 400.0, kGeomTol);
  EXPECT_NEAR(marker.get_vy(), -200.0, kGeomTol);
  EXPECT_EQ(marker.get_label(), "02");
  EXPECT_EQ(marker.get_color_str("primary_color"), "orange");
  EXPECT_TRUE(marker.is_set_type());
  EXPECT_EQ(marker.get_type(), "circle");
  EXPECT_TRUE(marker.is_set_width());
  EXPECT_NEAR(marker.get_width(), 4.0, kGeomTol);
  EXPECT_TRUE(marker.active());
}

TEST(XYFormatUtilsMarkerTest, RoundTripsBeaconRangeSensorGeneratedMarker)
{
  // uFldBeaconRangeSensor constructs an XYMarker and posts marker.get_spec()
  // for each configured beacon, normally without assigning a label.
  XYMarker generated;
  generated.set_vx(400);
  generated.set_vy(-200);
  generated.set_width(4);
  ASSERT_TRUE(generated.set_type("circle"));
  generated.set_active(true);
  generated.set_color("primary_color", "orange");

  XYMarker marker = string2Marker(generated.get_spec());

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_NEAR(marker.get_vx(), 400.0, kGeomTol);
  EXPECT_NEAR(marker.get_vy(), -200.0, kGeomTol);
  EXPECT_TRUE(marker.is_set_width());
  EXPECT_NEAR(marker.get_width(), 4.0, kGeomTol);
  EXPECT_TRUE(marker.is_set_type());
  EXPECT_EQ(marker.get_type(), "circle");
  EXPECT_EQ(marker.get_label(), "");
  EXPECT_EQ(marker.get_color_str("primary_color"), "orange");
  EXPECT_TRUE(marker.active());
}

TEST(XYFormatUtilsMarkerTest, ParsesMapMarkersHideMarkerErasePayload)
{
  // pMapMarkers posts this when markers are hidden, one erase payload per
  // opfield alias.
  XYMarker marker = string2Marker("x=0,y=0,active=false,label=AA");

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_NEAR(marker.get_vx(), 0.0, kGeomTol);
  EXPECT_NEAR(marker.get_vy(), 0.0, kGeomTol);
  EXPECT_FALSE(marker.active());
  EXPECT_EQ(marker.get_label(), "AA");
  EXPECT_FALSE(marker.is_set_type());
}

TEST(XYFormatUtilsMarkerTest, ParsesCompoundColorsField)
{
  XYMarker marker = string2Marker("x=1,y=2,colors=white:blue,type=square");

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_EQ(marker.get_color_str("primary_color"), "white");
  EXPECT_EQ(marker.get_color_str("secondary_color"), "blue");
  EXPECT_EQ(marker.get_type(), "square");
}

TEST(XYFormatUtilsMarkerTest, ParsesPartialCompoundColorsIndependently)
{
  XYMarker primary_only = string2Marker("x=1,y=2,colors=white:notacolor,type=circle");
  ASSERT_TRUE(primary_only.is_set_x());
  EXPECT_EQ(primary_only.get_color_str("primary_color"), "white");
  EXPECT_FALSE(primary_only.color_set("secondary_color"));

  XYMarker secondary_only = string2Marker("x=1,y=2,colors=notacolor:blue,type=circle");
  ASSERT_TRUE(secondary_only.is_set_x());
  EXPECT_FALSE(secondary_only.color_set("primary_color"));
  EXPECT_EQ(secondary_only.get_color_str("secondary_color"), "blue");
}

TEST(XYFormatUtilsMarkerTest, MissingOrInvalidPositionReturnsUnsetMarker)
{
  XYMarker missing_x = string2Marker("y=2,type=square");
  EXPECT_FALSE(missing_x.is_set_x());
  EXPECT_FALSE(missing_x.is_set_y());

  XYMarker missing_y = string2Marker("x=1,type=square");
  EXPECT_FALSE(missing_y.is_set_x());
  EXPECT_FALSE(missing_y.is_set_y());

  XYMarker invalid_x = string2Marker("x=bad,y=2,type=square");
  EXPECT_FALSE(invalid_x.is_set_x());
  EXPECT_FALSE(invalid_x.is_set_y());
}

TEST(XYFormatUtilsMarkerTest, PreservesLooseHintBehavior)
{
  XYMarker marker = string2Marker(
      "x=1,y=2,width=-5,range=-10,type=unknown,active=maybe,"
      "primary_color=notacolor,fill_transparency=2,duration=-4");

  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_TRUE(marker.is_set_width());
  EXPECT_NEAR(marker.get_width(), 0.0, kGeomTol);
  EXPECT_TRUE(marker.is_set_range());
  EXPECT_NEAR(marker.get_range(), 0.0, kGeomTol);
  EXPECT_FALSE(marker.is_set_type());
  EXPECT_EQ(marker.get_type(), "circle");
  EXPECT_TRUE(marker.active());
  EXPECT_FALSE(marker.color_set("primary_color"));
  EXPECT_TRUE(marker.transparency_set());
  EXPECT_NEAR(marker.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(marker.duration_set());
  EXPECT_NEAR(marker.get_duration(), -1.0, kGeomTol);
}

TEST(XYMarkerTest, DefaultAndConvenienceConstructorsDoNotMarkPositionAsSet)
{
  XYMarker marker;
  EXPECT_FALSE(marker.is_set_x());
  EXPECT_FALSE(marker.is_set_y());
  EXPECT_NEAR(marker.get_vx(), 0.0, kGeomTol);
  EXPECT_NEAR(marker.get_vy(), 0.0, kGeomTol);
  EXPECT_NEAR(marker.get_width(), 5.0, kGeomTol);
  EXPECT_NEAR(marker.get_range(), 0.0, kGeomTol);
  EXPECT_EQ(marker.get_type(), "circle");

  XYMarker located(10, 20);
  EXPECT_FALSE(located.is_set_x());
  EXPECT_FALSE(located.is_set_y());
  EXPECT_NEAR(located.get_vx(), 10.0, kGeomTol);
  EXPECT_NEAR(located.get_vy(), 20.0, kGeomTol);
}

TEST(XYMarkerTest, SettersClampDimensionsAndConstrainMarkerTypes)
{
  XYMarker marker;

  marker.set_vx(1);
  marker.set_vy(2);
  marker.set_width(-5);
  marker.set_range(-10);
  EXPECT_TRUE(marker.is_set_x());
  EXPECT_TRUE(marker.is_set_y());
  EXPECT_TRUE(marker.is_set_width());
  EXPECT_TRUE(marker.is_set_range());
  EXPECT_NEAR(marker.get_width(), 0.0, kGeomTol);
  EXPECT_NEAR(marker.get_range(), 0.0, kGeomTol);

  EXPECT_TRUE(marker.set_type("gateway"));
  EXPECT_TRUE(marker.is_set_type());
  EXPECT_EQ(marker.get_type(), "gateway");
  EXPECT_FALSE(marker.set_type("hexagon"));
  EXPECT_EQ(marker.get_type(), "gateway");

  marker.init();
  EXPECT_FALSE(marker.is_set_x());
  EXPECT_FALSE(marker.is_set_y());
  EXPECT_FALSE(marker.is_set_width());
  EXPECT_FALSE(marker.is_set_range());
  EXPECT_FALSE(marker.is_set_type());
  EXPECT_NEAR(marker.get_width(), 5.0, kGeomTol);
  EXPECT_NEAR(marker.get_range(), 0.0, kGeomTol);
  EXPECT_EQ(marker.get_type(), "circle");
}

TEST(XYMarkerTest, AcceptsFullViewerMarkerTypeSetCaseSensitively)
{
  const std::vector<std::string> marker_types = {
      "circle", "square", "diamond", "efield", "gateway", "triangle"};

  for(const std::string& marker_type : marker_types) {
    XYMarker marker;
    EXPECT_TRUE(marker.set_type(marker_type)) << marker_type;
    EXPECT_TRUE(marker.is_set_type()) << marker_type;
    EXPECT_EQ(marker.get_type(), marker_type);
  }

  XYMarker marker;
  EXPECT_FALSE(marker.set_type("Circle"));
  EXPECT_FALSE(marker.set_type("hexagon"));
  EXPECT_FALSE(marker.is_set_type());
  EXPECT_EQ(marker.get_type(), "circle");
}

TEST(XYMarkerTest, SerializesViewMarkerSpecWithObjectHints)
{
  XYMarker marker;
  marker.set_vx(167.974);
  marker.set_vy(-202.574);
  marker.set_width(3.456);
  marker.set_range(99.994);
  marker.set_type("diamond");
  marker.set_owner("alpha");
  marker.set_label("ZT");
  marker.set_label_color("gray60");
  marker.set_color("primary_color", "gray40");
  marker.set_color("secondary_color", "blue");
  marker.set_edge_color("white");
  marker.set_duration(5);

  std::string spec = marker.get_spec("active=true");
  EXPECT_TRUE(stringContains(spec, "x=167.97"));
  EXPECT_TRUE(stringContains(spec, "y=-202.57"));
  EXPECT_TRUE(stringContains(spec, "width=3.46"));
  EXPECT_TRUE(stringContains(spec, "range=99.99"));
  EXPECT_TRUE(stringContains(spec, "primary_color=gray40"));
  EXPECT_TRUE(stringContains(spec, "secondary_color=blue"));
  EXPECT_TRUE(stringContains(spec, "type=diamond"));
  EXPECT_TRUE(stringContains(spec, "owner=alpha"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=ZT"));
  EXPECT_TRUE(stringContains(spec, "label_color=gray60"));
  EXPECT_TRUE(stringContains(spec, "edge_color=white"));
  EXPECT_TRUE(stringContains(spec, "duration=5"));
}

TEST(XYMarkerTest, SerializesConciseInactiveEraseSpec)
{
  XYMarker marker;
  marker.set_label("old_marker");
  marker.set_duration(0);

  EXPECT_EQ(marker.get_spec_inactive(), "x=0,y=0,active=false,label=old_marker,duration=0");
}
