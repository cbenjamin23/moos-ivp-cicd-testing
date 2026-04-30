#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "VPlug_GeoShapes.h"
#include "XYOval.h"
#include "XYPoint.h"
#include "XYPolygon.h"
#include "XYTextBox.h"
#include "XYVessel.h"

namespace {

void ExpectPointNear(const std::vector<double>& cache, unsigned int index,
                     double expected_x, double expected_y,
                     double tol = kGeomTol)
{
  ASSERT_LT((index * 2) + 1, cache.size());
  EXPECT_NEAR(cache[index * 2], expected_x, tol);
  EXPECT_NEAR(cache[(index * 2) + 1], expected_y, tol);
}

void SetOvalPointCacheQuietly(XYOval& oval, double draw_degs)
{
  testing::internal::CaptureStdout();
  oval.setPointCache(draw_degs);
  testing::internal::GetCapturedStdout();
}

XYPolygon GetOvalPolyQuietly(XYOval& oval, double draw_degs)
{
  testing::internal::CaptureStdout();
  XYPolygon poly = oval.getOvalPoly(draw_degs);
  testing::internal::GetCapturedStdout();
  return poly;
}

}  // namespace

// Covers XY oval behavior: parses canonical view oval payload used by marine viewer.
TEST(XYOvalTest, ParsesCanonicalViewOvalPayloadUsedByMarineViewer)
{
  XYOval oval = stringToOval(
      "x=20,y=-30,rad=8,len=30,ang=45,draw_degs=10,"
      "label=turn_corridor,msg=hint,source=pHelmIvP,vsource=abe,type=viewer,"
      "id=oval-1,edge_color=yellow,fill_color=white,label_color=gray50,"
      "vertex_color=green,edge_size=2,vertex_size=3,"
      "fill_transparency=0.25,time=10,duration=5,active=false");

  ASSERT_TRUE(oval.valid());
  EXPECT_NEAR(oval.getX(), 20.0, kGeomTol);
  EXPECT_NEAR(oval.getY(), -30.0, kGeomTol);
  EXPECT_NEAR(oval.getRadius(), 8.0, kGeomTol);
  EXPECT_NEAR(oval.getLength(), 30.0, kGeomTol);
  EXPECT_NEAR(oval.getAngle(), 45.0, kGeomTol);
  EXPECT_NEAR(oval.getDrawDegs(), 10.0, kGeomTol);
  EXPECT_EQ(oval.get_label(), "turn_corridor");
  EXPECT_EQ(oval.get_msg(), "hint");
  EXPECT_EQ(oval.get_source(), "pHelmIvP");
  EXPECT_EQ(oval.get_vsource(), "abe");
  EXPECT_EQ(oval.get_type(), "viewer");
  EXPECT_EQ(oval.get_id(), "oval-1");
  EXPECT_EQ(oval.get_color_str("edge"), "yellow");
  EXPECT_EQ(oval.get_color_str("fill"), "white");
  EXPECT_EQ(oval.get_color_str("label"), "gray50");
  EXPECT_EQ(oval.get_color_str("vertex"), "green");
  EXPECT_NEAR(oval.get_edge_size(), 2.0, kGeomTol);
  EXPECT_NEAR(oval.get_vertex_size(), 3.0, kGeomTol);
  EXPECT_NEAR(oval.get_transparency(), 0.25, kGeomTol);
  EXPECT_NEAR(oval.get_time(), 10.0, kGeomTol);
  EXPECT_NEAR(oval.get_duration(), 5.0, kGeomTol);
  EXPECT_FALSE(oval.active());
}

// Covers XY oval behavior: requires positive radius and length at least diameter.
TEST(XYOvalTest, RequiresPositiveRadiusAndLengthAtLeastDiameter)
{
  EXPECT_FALSE(stringToOval("x=0,y=0,rad=0,len=10,ang=0").valid());
  EXPECT_FALSE(stringToOval("x=0,y=0,rad=-1,len=10,ang=0").valid());
  EXPECT_FALSE(stringToOval("x=0,y=0,rad=5,len=9.99,ang=0").valid());

  XYOval circle_like = stringToOval("x=0,y=0,rad=5,len=10,ang=0");
  EXPECT_TRUE(circle_like.valid());
}

// Covers XY oval behavior: parser uses atof for coordinates angles and ignores unknown fields.
TEST(XYOvalTest, ParserUsesAtofForCoordinatesAnglesAndIgnoresUnknownFields)
{
  XYOval oval = stringToOval(
      "x=not-a-number,y=2,rad=3,len=8,ang=bad-angle,"
      "bogus=ignored,active=maybe,fill_transparency=2,duration=-5,"
      "edge_size=-1,vertex_size=-2,edge_color=notacolor");

  ASSERT_TRUE(oval.valid());
  EXPECT_NEAR(oval.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(oval.getY(), 2.0, kGeomTol);
  EXPECT_NEAR(oval.getAngle(), 0.0, kGeomTol);
  EXPECT_TRUE(oval.active());
  EXPECT_TRUE(oval.transparency_set());
  EXPECT_NEAR(oval.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(oval.duration_set());
  EXPECT_NEAR(oval.get_duration(), -1.0, kGeomTol);
  EXPECT_FALSE(oval.edge_size_set());
  EXPECT_FALSE(oval.vertex_size_set());
  if(oval.color_set("edge"))
    EXPECT_FALSE(oval.get_color_str("edge").empty());
}

// Covers XY oval behavior: setters invalidate caches and reject non positive radius length.
TEST(XYOvalTest, SettersInvalidateCachesAndRejectNonPositiveRadiusLength)
{
  XYOval oval(10, -5, 2, 10, 90);
  oval.setBoundaryCache();
  SetOvalPointCacheQuietly(oval, 15);
  ASSERT_TRUE(oval.isSetPointCache());

  oval.setX(11);
  EXPECT_FALSE(oval.isSetPointCache());
  oval.setBoundaryCache();
  XYPoint end1 = oval.getEndPt1();
  EXPECT_NEAR(end1.x(), 14.0, kGeomTol);
  EXPECT_NEAR(end1.y(), -5.0, kGeomTol);

  oval.setRadius(-4);
  oval.setLength(0);
  EXPECT_NEAR(oval.getRadius(), 2.0, kGeomTol);
  EXPECT_NEAR(oval.getLength(), 10.0, kGeomTol);

  oval.setAngle(450);
  EXPECT_NEAR(oval.getAngle(), 450.0, kGeomTol);

  oval.modRadius(1);
  oval.modLength(2);
  oval.modAngle(-90);
  EXPECT_NEAR(oval.getRadius(), 3.0, kGeomTol);
  EXPECT_NEAR(oval.getLength(), 12.0, kGeomTol);
  EXPECT_NEAR(oval.getAngle(), 360.0, kGeomTol);
}

// Covers XY oval behavior: boundary cache uses capsule ends and axis aligned circle bounds.
TEST(XYOvalTest, BoundaryCacheUsesCapsuleEndsAndAxisAlignedCircleBounds)
{
  XYOval oval(10, -5, 2, 10, 90);
  oval.setBoundaryCache();

  EXPECT_NEAR(oval.get_min_x(), 5.0, kGeomTol);
  EXPECT_NEAR(oval.get_max_x(), 15.0, kGeomTol);
  EXPECT_NEAR(oval.get_min_y(), -7.0, kGeomTol);
  EXPECT_NEAR(oval.get_max_y(), -3.0, kGeomTol);

  XYPoint center = oval.getCenterPt();
  XYPoint end1 = oval.getEndPt1();
  XYPoint end2 = oval.getEndPt2();
  XYPolygon rect = oval.getRectPoly();
  EXPECT_NEAR(center.x(), 10.0, kGeomTol);
  EXPECT_NEAR(center.y(), -5.0, kGeomTol);
  EXPECT_NEAR(end1.x(), 13.0, kGeomTol);
  EXPECT_NEAR(end1.y(), -5.0, kGeomTol);
  EXPECT_NEAR(end2.x(), 7.0, kGeomTol);
  EXPECT_NEAR(end2.y(), -5.0, kGeomTol);
  EXPECT_TRUE(rect.is_convex());
  EXPECT_TRUE(rect.contains(10, -5));
}

// Covers XY oval behavior: contains point covers rectangle and both rounded ends.
TEST(XYOvalTest, ContainsPointCoversRectangleAndBothRoundedEnds)
{
  XYOval oval(10, -5, 2, 10, 90);

  EXPECT_TRUE(oval.containsPoint(10, -5));
  EXPECT_TRUE(oval.containsPoint(10, -7));
  EXPECT_TRUE(oval.containsPoint(15, -5));
  EXPECT_TRUE(oval.containsPoint(5, -5));
  EXPECT_FALSE(oval.containsPoint(15.1, -5));
  EXPECT_FALSE(oval.containsPoint(10, -7.1));
}

// Covers XY oval behavior: point cache uses MOOS heading convention and whitelisted resolution.
TEST(XYOvalTest, PointCacheUsesMoosHeadingConventionAndWhitelistedResolution)
{
  XYOval oval(10, -5, 2, 10, 90);
  oval.setBoundaryCache();
  SetOvalPointCacheQuietly(oval, 15);

  std::vector<double> cache = oval.getPointCache();
  ASSERT_EQ(cache.size(), 52u);
  ExpectPointNear(cache, 0, 13.0, -3.0);
  ExpectPointNear(cache, 6, 15.0, -5.0);
  ExpectPointNear(cache, 12, 13.0, -7.0);
  ExpectPointNear(cache, 13, 7.0, -7.0);
  ExpectPointNear(cache, 19, 5.0, -5.0);
  ExpectPointNear(cache, 25, 7.0, -3.0);

  SetOvalPointCacheQuietly(oval, 20);
  EXPECT_EQ(oval.getPointCache().size(), 148u);

  oval.clearPointCache();
  EXPECT_FALSE(oval.isSetPointCache());
}

// Covers XY oval behavior: oval polygon builds from point cache.
TEST(XYOvalTest, OvalPolygonBuildsFromPointCache)
{
  XYOval oval(10, -5, 2, 10, 90);

  XYPolygon poly = GetOvalPolyQuietly(oval, 10);
  EXPECT_EQ(poly.size(), 38u);
  EXPECT_TRUE(poly.is_convex());
  EXPECT_TRUE(poly.contains(10, -5));
}

// Covers XY oval behavior: diameter length oval is valid but current boundary cache stays empty.
TEST(XYOvalTest, DiameterLengthOvalIsValidButCurrentBoundaryCacheStaysEmpty)
{
  XYOval oval(10, -5, 2, 4, 90);
  ASSERT_TRUE(oval.valid());

  oval.setBoundaryCache();
  SetOvalPointCacheQuietly(oval, 10);

  // Current implementation returns early when the inner rectangle has zero
  // length, so a valid diameter-length oval never builds its draw cache.
  EXPECT_FALSE(oval.isSetPointCache());
  EXPECT_FALSE(oval.containsPoint(10, -5));
}

// Covers XY oval behavior: serializes spec with object metadata and precision.
TEST(XYOvalTest, SerializesSpecWithObjectMetadataAndPrecision)
{
  XYOval oval(1.23456, -2.34567, 3.45678, 12.34567, -45.6789);
  oval.set_spec_digits(3);
  oval.setDrawDegs(10);
  oval.set_label("turn_oval");
  oval.set_msg("preview");
  oval.set_source("unit");
  oval.set_time(42.25);
  oval.set_duration(3);
  oval.set_color("edge", "white");
  oval.set_color("fill", "aqua");
  oval.set_edge_size(2);
  oval.set_transparency(0.4);

  std::string spec = oval.get_spec("active=true");
  EXPECT_TRUE(stringContains(spec, "x=1.235"));
  EXPECT_TRUE(stringContains(spec, "y=-2.346"));
  EXPECT_TRUE(stringContains(spec, "rad=3.457"));
  EXPECT_TRUE(stringContains(spec, "len=12.346"));
  EXPECT_TRUE(stringContains(spec, "ang=-45.679"));
  EXPECT_TRUE(stringContains(spec, "draw_degs=10"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=turn_oval"));
  EXPECT_TRUE(stringContains(spec, "msg=preview"));
  EXPECT_TRUE(stringContains(spec, "source=unit"));
  EXPECT_TRUE(stringContains(spec, "time=42.25"));
  EXPECT_TRUE(stringContains(spec, "duration=3"));
  EXPECT_TRUE(stringContains(spec, "edge_color=white"));
  EXPECT_TRUE(stringContains(spec, "fill_color=aqua"));
  EXPECT_TRUE(stringContains(spec, "edge_size=2"));
  EXPECT_TRUE(stringContains(spec, "fill_transparency=0.4"));

  testing::internal::CaptureStdout();
  oval.print();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(stringContains(out, "x:1.23456"));
}

// Covers XY oval behavior: geo shapes adds caches expires and erases by label.
TEST(XYOvalTest, GeoShapesAddsCachesExpiresAndErasesByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addOval("x=10,y=-5,rad=2,len=10,ang=90,label=keep,duration=5",
                             15, 100.0));
  ASSERT_EQ(shapes.sizeOvals(), 1u);
  const XYOval& stored = shapes.getOvals().at("keep");
  EXPECT_NEAR(stored.get_time(), 100.0, kGeomTol);
  EXPECT_EQ(stored.getPointCache().size(), 52u);

  shapes.manageMemory(104.0);
  EXPECT_EQ(shapes.sizeOvals(), 1u);
  shapes.manageMemory(106.0);
  EXPECT_EQ(shapes.sizeOvals(), 0u);

  ASSERT_TRUE(shapes.addOval("x=10,y=-5,rad=2,len=10,ang=90,label=keep", 10, 0));
  ASSERT_TRUE(shapes.addOval("x=0,y=0,rad=2,len=10,ang=0,label=keep,active=false",
                             10, 0));
  EXPECT_EQ(shapes.sizeOvals(), 0u);
}

// Covers XY oval behavior: geo shapes rejects invalid ovals and auto labels unlabeled ones.
TEST(XYOvalTest, GeoShapesRejectsInvalidOvalsAndAutoLabelsUnlabeledOnes)
{
  VPlug_GeoShapes shapes;

  EXPECT_FALSE(shapes.addOval("x=0,y=0,rad=5,len=9,ang=0", 10, 0));
  EXPECT_EQ(shapes.sizeOvals(), 0u);

  ASSERT_TRUE(shapes.addOval("x=0,y=0,rad=2,len=10,ang=0", 10, 0));
  ASSERT_TRUE(shapes.addOval("x=5,y=0,rad=2,len=10,ang=0", 10, 0));
  EXPECT_EQ(shapes.sizeOvals(), 2u);
  EXPECT_EQ(shapes.getOvals().count("0"), 1u);
  EXPECT_EQ(shapes.getOvals().count("1"), 1u);
}

// Covers XY text box behavior: parses canonical view text box payload used by marine viewer.
TEST(XYTextBoxTest, ParsesCanonicalViewTextBoxPayloadUsedByMarineViewer)
{
  XYTextBox tbox = stringToTextBox(
      "x=-25,y=-85,msg=\"turn_rate=12,avg_rad=40\",msg=\"mode=arc\","
      "fsize=18,mcolor=yellow,label=ft_report_abe,source=pHelmIvP,"
      "vsource=abe,type=report,id=tbox-1,time=10,duration=5,active=false,"
      "label_color=gray50");

  ASSERT_TRUE(tbox.valid());
  EXPECT_FALSE(tbox.active());
  EXPECT_NEAR(tbox.x(), -25.0, kGeomTol);
  EXPECT_NEAR(tbox.y(), -85.0, kGeomTol);
  EXPECT_EQ(tbox.size(), 2u);
  EXPECT_EQ(tbox.getMsg(0), "turn_rate=12,avg_rad=40");
  EXPECT_EQ(tbox.getMsg(1), "mode=arc");
  EXPECT_EQ(tbox.getFSize(), 18);
  EXPECT_EQ(tbox.getMColor(), "yellow");
  EXPECT_EQ(tbox.get_label(), "ft_report_abe");
  EXPECT_EQ(tbox.get_source(), "pHelmIvP");
  EXPECT_EQ(tbox.get_vsource(), "abe");
  EXPECT_EQ(tbox.get_type(), "report");
  EXPECT_EQ(tbox.get_id(), "tbox-1");
  EXPECT_NEAR(tbox.get_time(), 10.0, kGeomTol);
  EXPECT_NEAR(tbox.get_duration(), 5.0, kGeomTol);
  EXPECT_EQ(tbox.get_color_str("label"), "gray50");
}

// Covers XY text box behavior: constructor setters snap and clear maintain validity.
TEST(XYTextBoxTest, ConstructorSettersSnapAndClearMaintainValidity)
{
  XYTextBox tbox(1.2, -2.6, "status");
  EXPECT_TRUE(tbox.valid());
  EXPECT_EQ(tbox.get_label(), "status");

  tbox.setMsg("line1");
  tbox.addMsg("line2");
  tbox.setFSize(24);
  tbox.setMColor("green");
  tbox.applySnap(0.5);

  EXPECT_NEAR(tbox.x(), 1.0, kGeomTol);
  EXPECT_NEAR(tbox.y(), -2.5, kGeomTol);
  EXPECT_EQ(tbox.size(), 2u);
  EXPECT_EQ(tbox.getMsg(0), "line1");
  EXPECT_EQ(tbox.getMsg(1), "line2");
  EXPECT_EQ(tbox.getMsg(2), "");
  EXPECT_EQ(tbox.getFSize(), 24);
  EXPECT_EQ(tbox.getMColor(), "green");

  tbox.clear();
  EXPECT_FALSE(tbox.valid());
  EXPECT_TRUE(tbox.active());
  EXPECT_EQ(tbox.size(), 0u);
  EXPECT_EQ(tbox.getFSize(), 10);
}

// Covers XY text box behavior: rejects unsupported font sizes and invalid message color.
TEST(XYTextBoxTest, RejectsUnsupportedFontSizesAndInvalidMessageColor)
{
  XYTextBox tbox(0, 0);
  EXPECT_EQ(tbox.getFSize(), 10);
  tbox.setFSize(11);
  tbox.setMColor("notacolor");

  EXPECT_EQ(tbox.getFSize(), 10);
  EXPECT_EQ(tbox.getMColor(), "");

  tbox.setFSize(9);
  tbox.setMColor("red");
  EXPECT_EQ(tbox.getFSize(), 9);
  EXPECT_EQ(tbox.getMColor(), "red");
}

// Covers XY text box behavior: requires coordinates and rejects unknown fields.
TEST(XYTextBoxTest, RequiresCoordinatesAndRejectsUnknownFields)
{
  EXPECT_FALSE(stringToTextBox("y=2,msg=missing_x").valid());
  EXPECT_FALSE(stringToTextBox("x=1,msg=missing_y").valid());
  EXPECT_FALSE(stringToTextBox("x=1,y=2,msg=ok,unknown=value").valid());
}

// Covers XY text box behavior: parser uses atof for coordinates and can parse bare color.
TEST(XYTextBoxTest, ParserUsesAtofForCoordinatesAndCanParseBareColor)
{
  XYTextBox tbox = stringToTextBox(
      "x=bad,y=-3,msg=hello,red,fill_transparency=2,"
      "duration=-4,edge_size=-1,vertex_size=-2");

  ASSERT_TRUE(tbox.valid());
  EXPECT_NEAR(tbox.x(), 0.0, kGeomTol);
  EXPECT_NEAR(tbox.y(), -3.0, kGeomTol);
  EXPECT_EQ(tbox.getMsg(0), "hello");
  EXPECT_EQ(tbox.getMColor(), "red");
  EXPECT_TRUE(tbox.active());
  EXPECT_TRUE(tbox.transparency_set());
  EXPECT_NEAR(tbox.get_transparency(), 1.0, kGeomTol);
  EXPECT_TRUE(tbox.duration_set());
  EXPECT_NEAR(tbox.get_duration(), -1.0, kGeomTol);
  EXPECT_FALSE(tbox.edge_size_set());
  EXPECT_FALSE(tbox.vertex_size_set());
}

// Covers XY text box behavior: invalid active token currently invalidates whole payload.
TEST(XYTextBoxTest, InvalidActiveTokenCurrentlyInvalidatesWholePayload)
{
  XYTextBox tbox = stringToTextBox("x=1,y=2,msg=hello,active=maybe");

  EXPECT_FALSE(tbox.valid());
  EXPECT_EQ(tbox.size(), 0u);
}

// Covers XY text box behavior: unquoted comma message currently invalidates behavior style payload.
TEST(XYTextBoxTest, UnquotedCommaMessageCurrentlyInvalidatesBehaviorStylePayload)
{
  XYTextBox tbox = stringToTextBox(
      "x=-25,y=-85,msg=turn_rate=12,avg_rad=40,min_rad=20");

  // lib_dep_behaviors/BHV_FixTurn posts this style. The textbox parser treats
  // avg_rad/min_rad as unknown top-level fields unless the msg is quoted.
  EXPECT_FALSE(tbox.valid());
  EXPECT_EQ(tbox.size(), 0u);
}

// Covers XY text box behavior: empty textbox without duration becomes short lived erase payload.
TEST(XYTextBoxTest, EmptyTextboxWithoutDurationBecomesShortLivedErasePayload)
{
  XYTextBox tbox = stringToTextBox("x=1,y=2,label=status");

  ASSERT_TRUE(tbox.valid());
  EXPECT_EQ(tbox.size(), 0u);
  EXPECT_TRUE(tbox.duration_set());
  EXPECT_NEAR(tbox.get_duration(), 1.0, kGeomTol);

  XYTextBox explicit_duration = stringToTextBox("x=1,y=2,label=status,duration=7");
  ASSERT_TRUE(explicit_duration.valid());
  EXPECT_TRUE(explicit_duration.duration_set());
  EXPECT_NEAR(explicit_duration.get_duration(), 7.0, kGeomTol);
}

// Covers XY text box behavior: serializes quoted messages and inactive erase spec.
TEST(XYTextBoxTest, SerializesQuotedMessagesAndInactiveEraseSpec)
{
  XYTextBox tbox(1.234, -2.345, "status");
  tbox.setMsg("speed=2.5,heading=90");
  tbox.addMsg("mode=loiter");
  tbox.setFSize(14);
  tbox.setMColor("aqua");
  tbox.set_time(42.25);
  tbox.set_duration(3);

  std::string spec = tbox.get_spec("active=true");
  EXPECT_TRUE(stringContains(spec, "x=1.23"));
  EXPECT_TRUE(stringContains(spec, "y=-2.35"));
  EXPECT_TRUE(stringContains(spec, "msg=\"speed=2.5,heading=90\""));
  EXPECT_TRUE(stringContains(spec, "msg=\"mode=loiter\""));
  EXPECT_TRUE(stringContains(spec, "fsize=14"));
  EXPECT_TRUE(stringContains(spec, "mcolor=aqua"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=status"));
  EXPECT_TRUE(stringContains(spec, "time=42.25"));
  EXPECT_TRUE(stringContains(spec, "duration=3"));

  EXPECT_EQ(tbox.get_spec_inactive(), "x=0,y=0,msg=null,active=false,label=status");
}

// Covers XY text box behavior: geo shapes adds expires auto labels and erases by label.
TEST(XYTextBoxTest, GeoShapesAddsExpiresAutoLabelsAndErasesByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addTextBox("x=-25,y=-85,msg=\"mode=turn\",label=report,"
                                "duration=5", 100.0));
  ASSERT_EQ(shapes.sizeTextBoxes(), 1u);
  EXPECT_EQ(shapes.getTextBoxes().count("report"), 1u);
  EXPECT_NEAR(shapes.getTextBoxes().at("report").get_time(), 100.0, kGeomTol);

  shapes.manageMemory(104.0);
  EXPECT_EQ(shapes.sizeTextBoxes(), 1u);
  shapes.manageMemory(106.0);
  EXPECT_EQ(shapes.sizeTextBoxes(), 0u);

  ASSERT_TRUE(shapes.addTextBox("x=3.14,y=-2.71,msg=unlabeled", 0));
  EXPECT_EQ(shapes.getTextBoxes().count("tbox_3.1_-2.7"), 1u);

  ASSERT_TRUE(shapes.addTextBox("x=0,y=0,label=tbox_3.1_-2.7,active=false", 0));
  EXPECT_EQ(shapes.sizeTextBoxes(), 0u);
}

// Covers XY text box behavior: geo shapes rejects invalid textboxes and replaces matching labels.
TEST(XYTextBoxTest, GeoShapesRejectsInvalidTextboxesAndReplacesMatchingLabels)
{
  VPlug_GeoShapes shapes;

  EXPECT_FALSE(shapes.addTextBox("x=1,msg=missing_y", 0));
  EXPECT_EQ(shapes.sizeTextBoxes(), 0u);

  ASSERT_TRUE(shapes.addTextBox("x=1,y=2,msg=first,label=status", 0));
  ASSERT_TRUE(shapes.addTextBox("x=3,y=4,msg=second,label=status", 0));
  ASSERT_EQ(shapes.sizeTextBoxes(), 1u);
  EXPECT_NEAR(shapes.getTextBoxes().at("status").x(), 3.0, kGeomTol);
  EXPECT_EQ(shapes.getTextBoxes().at("status").getMsg(0), "second");
}

// Covers XY vessel behavior: defaults represent ship and have no geometry validity gate.
TEST(XYVesselTest, DefaultsRepresentShipAndHaveNoGeometryValidityGate)
{
  XYVessel vessel;

  EXPECT_TRUE(vessel.valid());
  EXPECT_TRUE(vessel.active());
  EXPECT_NEAR(vessel.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(vessel.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(vessel.getHdg(), 0.0, kGeomTol);
  EXPECT_NEAR(vessel.getLen(), 0.0, kGeomTol);
  EXPECT_EQ(vessel.get_type(), "ship");
}

// Covers XY vessel behavior: parses view vessel payload converted from NODE_REPORT.
TEST(XYVesselTest, ParsesViewVesselPayloadConvertedFromNodeReport)
{
  XYVessel vessel = stringToVessel(
      "label=abe,x=12.34,y=-56.78,hdg=91.5,len=4.2,type=kayak,"
      "color=blue,time=123.45,dur=3");

  EXPECT_TRUE(vessel.valid());
  EXPECT_EQ(vessel.get_label(), "abe");
  EXPECT_NEAR(vessel.getX(), 12.34, kGeomTol);
  EXPECT_NEAR(vessel.getY(), -56.78, kGeomTol);
  EXPECT_NEAR(vessel.getHdg(), 91.5, kGeomTol);
  EXPECT_NEAR(vessel.getLen(), 4.2, kGeomTol);
  EXPECT_EQ(vessel.get_type(), "kayak");
  EXPECT_EQ(vessel.get_color_str("fill"), "blue");
  EXPECT_TRUE(vessel.time_set());
  EXPECT_NEAR(vessel.get_time(), 123.45, kGeomTol);
  EXPECT_TRUE(vessel.duration_set());
  EXPECT_NEAR(vessel.get_duration(), 3.0, kGeomTol);
}

// Covers XY vessel behavior: setters reject non positive length but do not normalize heading.
TEST(XYVesselTest, SettersRejectNonPositiveLengthButDoNotNormalizeHeading)
{
  XYVessel vessel(1, 2, 370, 5);

  vessel.setXY(-3, 4);
  vessel.setLen(-1);
  vessel.setHdg(-90);

  EXPECT_NEAR(vessel.getX(), -3.0, kGeomTol);
  EXPECT_NEAR(vessel.getY(), 4.0, kGeomTol);
  EXPECT_NEAR(vessel.getLen(), 5.0, kGeomTol);
  EXPECT_NEAR(vessel.getHdg(), -90.0, kGeomTol);

  vessel.modX(10);
  vessel.modY(-20);
  vessel.modLen(2);
  EXPECT_NEAR(vessel.getX(), 7.0, kGeomTol);
  EXPECT_NEAR(vessel.getY(), -16.0, kGeomTol);
  EXPECT_NEAR(vessel.getLen(), 7.0, kGeomTol);
}

// Covers XY vessel behavior: parser ignores malformed heading length unknown fields and invalid type.
TEST(XYVesselTest, ParserIgnoresMalformedHeadingLengthUnknownFieldsAndInvalidType)
{
  XYVessel vessel = stringToVessel(
      "x=bad,y=2,hdg=bad,len=bad,type=spaceship,color=notacolor,"
      "active=maybe,unknown=value");

  EXPECT_TRUE(vessel.valid());
  EXPECT_NEAR(vessel.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(vessel.getY(), 2.0, kGeomTol);
  EXPECT_NEAR(vessel.getHdg(), 0.0, kGeomTol);
  EXPECT_NEAR(vessel.getLen(), 0.0, kGeomTol);
  // The explicit vessel type gate rejects unknown types, but the fallback
  // XYObject parser accepts the same type token immediately afterward.
  EXPECT_EQ(vessel.get_type(), "spaceship");
  EXPECT_FALSE(vessel.color_set("fill"));
  EXPECT_TRUE(vessel.active());
}

// Covers XY vessel behavior: parser accepts known vehicle types and color alias only.
TEST(XYVesselTest, ParserAcceptsKnownVehicleTypesAndColorAliasOnly)
{
  XYVessel vessel = stringToVessel(
      "x=1,y=2,hdg=3,len=4,type=heron,color=green,fill_color=red,"
      "label_color=white,edge_color=yellow");

  EXPECT_EQ(vessel.get_type(), "heron");
  EXPECT_EQ(vessel.get_color_str("fill"), "red");
  EXPECT_EQ(vessel.get_color_str("label"), "white");
  EXPECT_EQ(vessel.get_color_str("edge"), "yellow");
}

// Covers XY vessel behavior: serializes spec with trailing comma quirk and metadata.
TEST(XYVesselTest, SerializesSpecWithTrailingCommaQuirkAndMetadata)
{
  XYVessel vessel(1.234, -2.345, 361.5, 8.765);
  vessel.set_label("abe");
  vessel.set_type("kayak");
  vessel.set_color("fill", "blue");
  vessel.set_time(42.25);
  vessel.set_duration(3);
  vessel.set_active(false);

  std::string spec = vessel.get_spec();
  EXPECT_TRUE(stringContains(spec, "x=1.23"));
  EXPECT_TRUE(stringContains(spec, "y=-2.35"));
  EXPECT_TRUE(stringContains(spec, "hdg=361.5"));
  EXPECT_TRUE(stringContains(spec, "len=8.77"));
  EXPECT_TRUE(stringContains(spec, "type=kayak,"));
  EXPECT_TRUE(stringContains(spec, ",,active=false"));
  EXPECT_TRUE(stringContains(spec, "label=abe"));
  EXPECT_TRUE(stringContains(spec, "fill_color=blue"));
  EXPECT_TRUE(stringContains(spec, "time=42.25"));
  EXPECT_TRUE(stringContains(spec, "duration=3"));
}

// Covers XY vessel behavior: geo shapes adds replaces expires and erases by label.
TEST(XYVesselTest, GeoShapesAddsReplacesExpiresAndErasesByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addVessel("label=abe,x=10,y=20,hdg=90,len=5,duration=5",
                               100.0));
  ASSERT_EQ(shapes.sizeVessels(), 1u);
  ASSERT_EQ(shapes.getVessels().size(), 1u);
  EXPECT_NEAR(shapes.getVessel(0).get_time(), 100.0, kGeomTol);

  ASSERT_TRUE(shapes.addVessel("label=abe,x=30,y=40,hdg=180,len=6", 0));
  ASSERT_EQ(shapes.sizeVessels(), 1u);
  EXPECT_NEAR(shapes.getVessel(0).getX(), 30.0, kGeomTol);
  EXPECT_NEAR(shapes.getVessel(0).getLen(), 6.0, kGeomTol);

  ASSERT_TRUE(shapes.addVessel("label=abe,active=false", 0));
  EXPECT_EQ(shapes.sizeVessels(), 0u);

  ASSERT_TRUE(shapes.addVessel("label=short,x=1,y=1,duration=5", 100.0));
  shapes.manageMemory(106.0);
  EXPECT_EQ(shapes.sizeVessels(), 0u);
}

// Covers XY vessel behavior: geo shapes allows unlabeled duplicates and out of range getter sentinel.
TEST(XYVesselTest, GeoShapesAllowsUnlabeledDuplicatesAndOutOfRangeGetterSentinel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addVessel("x=1,y=2,hdg=3,len=4", 0));
  ASSERT_TRUE(shapes.addVessel("x=5,y=6,hdg=7,len=8", 0));
  EXPECT_EQ(shapes.sizeVessels(), 2u);

  XYVessel sentinel = shapes.getVessel(3);
  EXPECT_NEAR(sentinel.getX(), 1.0, kGeomTol);
  EXPECT_NEAR(sentinel.getY(), 2.0, kGeomTol);
  EXPECT_NEAR(sentinel.getHdg(), 3.0, kGeomTol);
  EXPECT_NEAR(sentinel.getLen(), 4.0, kGeomTol);
}

// Covers XY vessel behavior: geo shapes bounds ignore only zero zero vessels.
TEST(XYVesselTest, GeoShapesBoundsIgnoreOnlyZeroZeroVessels)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addVessel("label=origin,x=0,y=0,hdg=0,len=4", 0));
  EXPECT_NEAR(shapes.getXMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getXMax(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMax(), 0.0, kGeomTol);

  ASSERT_TRUE(shapes.addVessel("label=east,x=0,y=5,hdg=0,len=4", 0));
  EXPECT_NEAR(shapes.getXMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getXMax(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMin(), 0.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMax(), 5.0, kGeomTol);

  ASSERT_TRUE(shapes.addVessel("label=west,x=-3,y=-4,hdg=0,len=4", 0));
  EXPECT_NEAR(shapes.getXMin(), -3.0, kGeomTol);
  EXPECT_NEAR(shapes.getYMin(), -4.0, kGeomTol);
}
