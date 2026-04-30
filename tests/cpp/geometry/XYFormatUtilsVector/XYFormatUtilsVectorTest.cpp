#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsVector.h"
#include "XYVector.h"

// Covers XY format utils vector behavior: parses view vector style magnitude angle spec.
TEST(XYFormatUtilsVectorTest, ParsesViewVectorStyleMagnitudeAngleSpec)
{
  XYVector vector = string2Vector(
      "x=12.5,y=-3.5,mag=4,ang=90,label=Drift,type=Current,"
      "source=pMarineViewer,vsource=abe,msg=Flow,id=vec-1,"
      "vertex_size=3,edge_size=2,head_size=5,vertex_color=yellow,"
      "edge_color=white,label_color=gray50,time=10,duration=5,"
      "fill_transparency=0.4,active=false");

  ASSERT_TRUE(vector.valid());
  EXPECT_NEAR(vector.xpos(), 12.5, kGeomTol);
  EXPECT_NEAR(vector.ypos(), -3.5, kGeomTol);
  EXPECT_NEAR(vector.mag(), 4.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 90.0, kGeomTol);
  EXPECT_NEAR(vector.xdot(), 4.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 0.0, kGeomTol);
  EXPECT_EQ(vector.get_label(), "drift");
  EXPECT_EQ(vector.get_type(), "current");
  EXPECT_EQ(vector.get_source(), "pmarineviewer");
  EXPECT_EQ(vector.get_vsource(), "abe");
  EXPECT_EQ(vector.get_msg(), "flow");
  EXPECT_EQ(vector.get_id(), "vec-1");
  EXPECT_TRUE(vector.vertex_size_set());
  EXPECT_NEAR(vector.get_vertex_size(), 3.0, kGeomTol);
  EXPECT_TRUE(vector.edge_size_set());
  EXPECT_NEAR(vector.get_edge_size(), 2.0, kGeomTol);
  EXPECT_TRUE(vector.head_size_set());
  EXPECT_NEAR(vector.headsize(), 5.0, kGeomTol);
  EXPECT_EQ(vector.get_color_str("vertex"), "yellow");
  EXPECT_EQ(vector.get_color_str("edge"), "white");
  EXPECT_EQ(vector.get_color_str("label"), "gray50");
  EXPECT_TRUE(vector.time_set());
  EXPECT_NEAR(vector.get_time(), 10.0, kGeomTol);
  EXPECT_TRUE(vector.duration_set());
  EXPECT_NEAR(vector.get_duration(), 5.0, kGeomTol);
  EXPECT_TRUE(vector.transparency_set());
  EXPECT_NEAR(vector.get_transparency(), 0.4, kGeomTol);
  EXPECT_FALSE(vector.active());
}

// Covers XY format utils vector behavior: parses current field vector spec with xdot ydot components.
TEST(XYFormatUtilsVectorTest, ParsesCurrentFieldVectorSpecWithXdotYdotComponents)
{
  XYVector vector = string2Vector("xpos=0,ypos=0,mag=1,ang=0,xdot=1,ydot=0");

  ASSERT_TRUE(vector.valid());
  EXPECT_NEAR(vector.xpos(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ypos(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.xdot(), 1.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 1.0, kGeomTol);
  EXPECT_NEAR(vector.mag(), std::sqrt(2.0), kGeomTol);
  EXPECT_NEAR(vector.ang(), 45.0, kGeomTol);
}

// Covers XY format utils vector behavior: parses bravo current field mission row.
TEST(XYFormatUtilsVectorTest, ParsesBravoCurrentFieldMissionRow)
{
  // ivp/missions/s2_bravo/bravo.cfd rows are loaded by CurrentField via
  // string2Vector and later rendered as VIEW_VECTOR style geometry.
  XYVector vector = string2Vector("x=45, y=-95,  mag=2.4, ang=130");

  ASSERT_TRUE(vector.valid());
  EXPECT_NEAR(vector.xpos(), 45.0, kGeomTol);
  EXPECT_NEAR(vector.ypos(), -95.0, kGeomTol);
  EXPECT_NEAR(vector.mag(), 2.4, kGeomTol);
  EXPECT_NEAR(vector.ang(), 130.0, kGeomTol);
  EXPECT_NEAR(vector.xdot(), 1.83850666348555, 1e-12);
  EXPECT_NEAR(vector.ydot(), -1.54269026324769, 1e-12);
}

// Covers XY format utils vector behavior: requires position but defaults missing magnitude and angle.
TEST(XYFormatUtilsVectorTest, RequiresPositionButDefaultsMissingMagnitudeAndAngle)
{
  EXPECT_FALSE(string2Vector("y=2,mag=3,ang=4").valid());
  EXPECT_FALSE(string2Vector("x=1,mag=3,ang=4").valid());
  EXPECT_FALSE(string2Vector("x=one,y=2,mag=3,ang=4").valid());

  XYVector vector = string2Vector("x=1,y=2,label=stationary");
  ASSERT_TRUE(vector.valid());
  EXPECT_NEAR(vector.mag(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.xdot(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 0.0, kGeomTol);
}

// Covers XY format utils vector behavior: normalizes parsed negative magnitude and wrapped angles through components.
TEST(XYFormatUtilsVectorTest, NormalizesParsedNegativeMagnitudeAndWrappedAnglesThroughComponents)
{
  XYVector negative_mag = string2Vector("x=0,y=0,mag=-5,ang=90,label=reverse_current");

  ASSERT_TRUE(negative_mag.valid());
  EXPECT_NEAR(negative_mag.mag(), 5.0, kGeomTol);
  EXPECT_NEAR(negative_mag.ang(), 270.0, kGeomTol);
  EXPECT_NEAR(negative_mag.xdot(), -5.0, kGeomTol);
  EXPECT_NEAR(negative_mag.ydot(), 0.0, kGeomTol);

  XYVector wrapped_angle = string2Vector("x=0,y=0,mag=5,ang=450,label=wrapped_current");
  ASSERT_TRUE(wrapped_angle.valid());
  EXPECT_NEAR(wrapped_angle.mag(), 5.0, kGeomTol);
  EXPECT_NEAR(wrapped_angle.ang(), 90.0, kGeomTol);
  EXPECT_NEAR(wrapped_angle.xdot(), 5.0, kGeomTol);
  EXPECT_NEAR(wrapped_angle.ydot(), 0.0, kGeomTol);
}

// Covers XY format utils vector behavior: preserves parser treatment of loose hints.
TEST(XYFormatUtilsVectorTest, PreservesParserTreatmentOfLooseHints)
{
  XYVector vector = string2Vector(
      "x=1,y=2,mag=3,ang=90,vertex_size=-1,edge_size=-1,"
      "head_size=-2,duration=-5,fill_transparency=2,active=maybe,"
      "vertex_color=notacolor");

  ASSERT_TRUE(vector.valid());
  EXPECT_FALSE(vector.vertex_size_set());
  EXPECT_FALSE(vector.edge_size_set());
  EXPECT_FALSE(vector.head_size_set());
  EXPECT_TRUE(vector.duration_set());
  EXPECT_NEAR(vector.get_duration(), -1.0, kGeomTol);
  EXPECT_TRUE(vector.transparency_set());
  EXPECT_NEAR(vector.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(vector.active());
  if(vector.color_set("vertex"))
    EXPECT_FALSE(vector.get_color_str("vertex").empty());
}

// Covers XY vector behavior: constructs and converts magnitude angle using MOOS heading convention.
TEST(XYVectorTest, ConstructsAndConvertsMagnitudeAngleUsingMoosHeadingConvention)
{
  XYVector east(1, 2, 5, 90);

  ASSERT_TRUE(east.valid());
  EXPECT_NEAR(east.xpos(), 1.0, kGeomTol);
  EXPECT_NEAR(east.ypos(), 2.0, kGeomTol);
  EXPECT_NEAR(east.mag(), 5.0, kGeomTol);
  EXPECT_NEAR(east.ang(), 90.0, kGeomTol);
  EXPECT_NEAR(east.xdot(), 5.0, kGeomTol);
  EXPECT_NEAR(east.ydot(), 0.0, kGeomTol);

  XYVector north(0, 0, 5, 0);
  EXPECT_NEAR(north.xdot(), 0.0, kGeomTol);
  EXPECT_NEAR(north.ydot(), 5.0, kGeomTol);
}

// Covers XY vector behavior: setters and merges keep representations in sync.
TEST(XYVectorTest, SettersAndMergesKeepRepresentationsInSync)
{
  XYVector vector;
  EXPECT_FALSE(vector.valid());

  vector.setPosition(0, 0);
  ASSERT_TRUE(vector.valid());
  vector.setVectorMA(2, 0);
  EXPECT_NEAR(vector.xdot(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 2.0, kGeomTol);

  vector.mergeVectorXY(2, 0);
  EXPECT_NEAR(vector.xdot(), 2.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 2.0, kGeomTol);
  EXPECT_NEAR(vector.mag(), std::sqrt(8.0), kGeomTol);
  EXPECT_NEAR(vector.ang(), 45.0, kGeomTol);

  vector.setVectorXY(0, -3);
  EXPECT_NEAR(vector.mag(), 3.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 180.0, kGeomTol);

  vector.mergeVectorMA(3, 0);
  EXPECT_NEAR(vector.xdot(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.mag(), 0.0, kGeomTol);
}

// Covers XY vector behavior: direct magnitude setter does not normalize or mark position valid.
TEST(XYVectorTest, DirectMagnitudeSetterDoesNotNormalizeOrMarkPositionValid)
{
  XYVector vector;
  vector.setVectorMA(-5, 90);

  EXPECT_FALSE(vector.valid());
  EXPECT_NEAR(vector.mag(), -5.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 90.0, kGeomTol);
  EXPECT_NEAR(vector.xdot(), -5.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), 0.0, kGeomTol);
}

// Covers XY vector behavior: augments magnitude and angle with wraparound.
TEST(XYVectorTest, AugmentsMagnitudeAndAngleWithWraparound)
{
  XYVector vector(0, 0, 4, 350);

  vector.augAngle(25);
  EXPECT_NEAR(vector.ang(), 15.0, kGeomTol);
  EXPECT_NEAR(vector.mag(), 4.0, kGeomTol);

  vector.augMagnitude(2);
  EXPECT_NEAR(vector.mag(), 6.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 15.0, kGeomTol);
}

// Covers XY vector behavior: augmenting magnitude can reverse component direction.
TEST(XYVectorTest, AugmentingMagnitudeCanReverseComponentDirection)
{
  XYVector vector(0, 0, 4, 0);

  vector.augMagnitude(-6);
  EXPECT_TRUE(vector.valid());
  EXPECT_NEAR(vector.mag(), -2.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.xdot(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ydot(), -2.0, kGeomTol);
}

// Covers XY vector behavior: applies position transforms and snap.
TEST(XYVectorTest, AppliesPositionTransformsAndSnap)
{
  XYVector vector(1.24, -2.76, 3, 45);

  vector.shift_horz(2);
  vector.shift_vert(-1);
  EXPECT_NEAR(vector.xpos(), 3.24, kGeomTol);
  EXPECT_NEAR(vector.ypos(), -3.76, kGeomTol);

  vector.applySnap(0.5);
  EXPECT_NEAR(vector.xpos(), 3.0, kGeomTol);
  EXPECT_NEAR(vector.ypos(), -4.0, kGeomTol);
}

// Covers XY vector behavior: clears vector and object specific hints.
TEST(XYVectorTest, ClearsVectorAndObjectSpecificHints)
{
  XYVector vector(1, 2, 3, 4);
  vector.setHeadSize(7);
  vector.set_label("flow");

  vector.clear();
  EXPECT_FALSE(vector.valid());
  EXPECT_NEAR(vector.xpos(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ypos(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.mag(), 0.0, kGeomTol);
  EXPECT_NEAR(vector.ang(), 0.0, kGeomTol);
  EXPECT_FALSE(vector.head_size_set());
  EXPECT_EQ(vector.get_label(), "flow");
}

// Covers XY vector behavior: serializes viewer vector spec.
TEST(XYVectorTest, SerializesViewerVectorSpec)
{
  XYVector vector(1.234567, -2.345678, 9.876543, 45.5);
  vector.setHeadSize(2.5);
  vector.set_label("flow");
  vector.set_color("edge", "white");
  vector.set_vertex_size(3);
  vector.set_duration(4);

  std::string spec = vector.get_spec("active=true");
  EXPECT_TRUE(stringContains(spec, "x=1.23457"));
  EXPECT_TRUE(stringContains(spec, "y=-2.34568"));
  EXPECT_TRUE(stringContains(spec, "ang=45.5"));
  EXPECT_TRUE(stringContains(spec, "mag=9.87654"));
  EXPECT_TRUE(stringContains(spec, "head_size=2.5"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=flow"));
  EXPECT_TRUE(stringContains(spec, "edge_color=white"));
  EXPECT_TRUE(stringContains(spec, "vertex_size=3"));
  EXPECT_TRUE(stringContains(spec, "duration=4"));
}
