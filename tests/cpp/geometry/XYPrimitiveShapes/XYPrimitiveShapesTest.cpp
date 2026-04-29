#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "VPlug_GeoShapes.h"
#include "XYArrow.h"
#include "XYCommsPulse.h"
#include "XYEncoders.h"
#include "XYHexagon.h"
#include "XYPoint.h"
#include "XYRangePulse.h"
#include "XYSegment.h"
#include "XYSquare.h"
#include "XYVector.h"
#include "XYWedge.h"

namespace {

void expectVectorNear(const std::vector<double>& actual,
                      const std::vector<double>& expected,
                      double tol = kGeomTol)
{
  ASSERT_EQ(actual.size(), expected.size());
  for(size_t i = 0; i < actual.size(); ++i)
    EXPECT_NEAR(actual[i], expected[i], tol) << "at index " << i;
}

}  // namespace

TEST(XYArrowPrimitiveTest, DefaultsExposeViewerFriendlyStyleAndNorthFacingGeometry)
{
  XYArrow arrow;

  EXPECT_TRUE(arrow.valid());
  EXPECT_TRUE(arrow.active());
  EXPECT_NEAR(arrow.getCenterX(), 0.0, kGeomTol);
  EXPECT_NEAR(arrow.getCenterY(), 0.0, kGeomTol);
  EXPECT_EQ(arrow.get_color_str("edge"), "invisible");
  EXPECT_EQ(arrow.get_color_str("vertex"), "invisible");
  EXPECT_EQ(arrow.get_color_str("fill"), "yellow");
  EXPECT_TRUE(arrow.transparency_set());
  EXPECT_NEAR(arrow.get_transparency(), 0.5, kGeomTol);

  expectVectorNear(arrow.getBaseVertices(),
                   {2.5, 10.0, 2.5, -10.0, -2.5, -10.0, -2.5, 10.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 20.0, -5.0, 10.0, 5.0, 10.0});
  EXPECT_NEAR(arrow.getHeadCtrX(), 0.0, kGeomTol);
  EXPECT_NEAR(arrow.getHeadCtrY(), 40.0 / 3.0, kGeomTol);
}

TEST(XYArrowPrimitiveTest, ConstructorAndCenterMutatorsInvalidateAndRebuildCache)
{
  XYArrow arrow(10, -4);
  expectVectorNear(arrow.getHeadVertices(),
                   {10.0, 16.0, 5.0, 6.0, 15.0, 6.0});

  arrow.setBaseCenter(-2, 3);
  EXPECT_NEAR(arrow.getCenterX(), -2.0, kGeomTol);
  EXPECT_NEAR(arrow.getCenterY(), 3.0, kGeomTol);
  expectVectorNear(arrow.getHeadVertices(),
                   {-2.0, 23.0, -7.0, 13.0, 3.0, 13.0});

  arrow.setBaseCenterX(4);
  arrow.setBaseCenterY(-6);
  EXPECT_NEAR(arrow.getCenterX(), 4.0, kGeomTol);
  EXPECT_NEAR(arrow.getCenterY(), -6.0, kGeomTol);

  arrow.modCenterX(5);
  arrow.modCenterY(-7);
  expectVectorNear(arrow.getHeadVertices(),
                   {9.0, 7.0, 4.0, -3.0, 14.0, -3.0});
}

TEST(XYArrowPrimitiveTest, AngleUsesMoosHeadingConventionAndNormalizesInput)
{
  XYArrow arrow;
  arrow.setAngle(450);

  expectVectorNear(arrow.getBaseVertices(),
                   {10.0, -2.5, -10.0, -2.5, -10.0, 2.5, 10.0, 2.5});
  expectVectorNear(arrow.getHeadVertices(),
                   {20.0, 0.0, 10.0, 5.0, 10.0, -5.0});

  std::string spec = arrow.get_spec(0);
  EXPECT_TRUE(stringContains(spec, "ang=90"));
}

TEST(XYArrowPrimitiveTest, CardinalAnglesRotateDrawableVerticesClockwise)
{
  XYArrow south;
  south.setAngle(180);
  expectVectorNear(south.getBaseVertices(),
                   {-2.5, -10.0, -2.5, 10.0, 2.5, 10.0, 2.5, -10.0});
  expectVectorNear(south.getHeadVertices(),
                   {0.0, -20.0, 5.0, -10.0, -5.0, -10.0});

  XYArrow west;
  west.setAngle(-90);
  expectVectorNear(west.getBaseVertices(),
                   {-10.0, 2.5, 10.0, 2.5, 10.0, -2.5, -10.0, -2.5});
  expectVectorNear(west.getHeadVertices(),
                   {-20.0, 0.0, -10.0, -5.0, -10.0, 5.0});

  EXPECT_TRUE(stringContains(west.get_spec(0), "ang=270"));
}

TEST(XYArrowPrimitiveTest, IndividualDimensionSettersInvalidateCacheAndSerialize)
{
  XYArrow arrow;
  ASSERT_TRUE(arrow.setBaseWid(6));
  ASSERT_TRUE(arrow.setBaseLen(14));
  ASSERT_TRUE(arrow.setHeadWid(12));
  ASSERT_TRUE(arrow.setHeadLen(8));

  expectVectorNear(arrow.getBaseVertices(),
                   {3.0, 7.0, 3.0, -7.0, -3.0, -7.0, -3.0, 7.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 15.0, -6.0, 7.0, 6.0, 7.0});

  std::string spec = arrow.get_spec(1);
  EXPECT_TRUE(stringContains(spec, "bwid=6"));
  EXPECT_TRUE(stringContains(spec, "blen=14"));
  EXPECT_TRUE(stringContains(spec, "hwid=12"));
  EXPECT_TRUE(stringContains(spec, "hlen=8"));
}

TEST(XYArrowPrimitiveTest, RejectsNonPositiveSizingButKeepsPriorShape)
{
  XYArrow arrow;
  EXPECT_TRUE(arrow.setBase(8, 12));
  EXPECT_TRUE(arrow.setHead(14, 6));
  expectVectorNear(arrow.getBaseVertices(),
                   {4.0, 6.0, 4.0, -6.0, -4.0, -6.0, -4.0, 6.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 12.0, -7.0, 6.0, 7.0, 6.0});

  EXPECT_FALSE(arrow.setBase(0, 20));
  EXPECT_FALSE(arrow.setBaseWid(-1));
  EXPECT_FALSE(arrow.setBaseLen(0));
  EXPECT_FALSE(arrow.setHead(20, -1));
  EXPECT_FALSE(arrow.setHeadWid(0));
  EXPECT_FALSE(arrow.setHeadLen(-2));
  EXPECT_FALSE(arrow.resize(0.01));
  expectVectorNear(arrow.getBaseVertices(),
                   {4.0, 6.0, 4.0, -6.0, -4.0, -6.0, -4.0, 6.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 12.0, -7.0, 6.0, 7.0, 6.0});
}

TEST(XYArrowPrimitiveTest, ResizeScalesAllDrawableDimensions)
{
  XYArrow arrow;
  ASSERT_TRUE(arrow.resize(2));

  expectVectorNear(arrow.getBaseVertices(),
                   {5.0, 20.0, 5.0, -20.0, -5.0, -20.0, -5.0, 20.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 40.0, -10.0, 20.0, 10.0, 20.0});
}

TEST(XYArrowPrimitiveTest, SuperLengthUsesCurrentBaseWidthQuirk)
{
  XYArrow arrow;
  ASSERT_TRUE(arrow.setSuperLength(40));

  // Current implementation scales base width from the previous width,
  // while all lengths are derived from the requested super length.
  expectVectorNear(arrow.getBaseVertices(),
                   {0.625, 15.0, 0.625, -15.0, -0.625, -15.0, -0.625, 15.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 25.0, -1.875, 15.0, 1.875, 15.0});
}

TEST(XYArrowPrimitiveTest, BoundsCacheCurrentlyReportsUpperExtentsForBothMinAndMax)
{
  XYArrow arrow;

  EXPECT_NEAR(arrow.getMinX(), 5.0, kGeomTol);
  EXPECT_NEAR(arrow.getMaxX(), 5.0, kGeomTol);
  EXPECT_NEAR(arrow.getMinY(), 20.0, kGeomTol);
  EXPECT_NEAR(arrow.getMaxY(), 20.0, kGeomTol);
}

TEST(XYArrowPrimitiveTest, HeadCenterRequiresPointCachePopulation)
{
  XYArrow arrow;

  EXPECT_NEAR(arrow.getHeadCtrX(), 0.0, kGeomTol);
  EXPECT_NEAR(arrow.getHeadCtrY(), 0.0, kGeomTol);

  arrow.setPointCache();
  EXPECT_NEAR(arrow.getHeadCtrX(), 0.0, kGeomTol);
  EXPECT_NEAR(arrow.getHeadCtrY(), 40.0 / 3.0, kGeomTol);
}

TEST(XYArrowPrimitiveTest, HeadCenterStaysStaleUntilPointCacheIsRebuilt)
{
  XYArrow arrow;
  ASSERT_FALSE(arrow.getHeadVertices().empty());

  arrow.setBaseCenter(10, 10);
  EXPECT_NEAR(arrow.getHeadCtrX(), 0.0, kGeomTol);
  EXPECT_NEAR(arrow.getHeadCtrY(), 40.0 / 3.0, kGeomTol);

  ASSERT_FALSE(arrow.getHeadVertices().empty());
  EXPECT_NEAR(arrow.getHeadCtrX(), 10.0, kGeomTol);
  EXPECT_NEAR(arrow.getHeadCtrY(), 10.0 + (40.0 / 3.0), kGeomTol);
}

TEST(XYArrowPrimitiveTest, StringPayloadParsesObjectFieldsAndSerializesThem)
{
  XYArrow arrow = stringToArrow("ctrx=1.25,ctry=-2.5,ang=-90,bwid=8,blen=12,"
                                "hwid=14,hlen=6,label=wind,msg=gust,"
                                "edge_color=red,fill_color=green,"
                                "label_color=white,fill_transparency=0.75,"
                                "edge_size=3,duration=4,time=10,active=false");

  EXPECT_FALSE(arrow.active());
  EXPECT_EQ(arrow.get_label(), "wind");
  EXPECT_EQ(arrow.get_msg(), "gust");
  EXPECT_EQ(arrow.get_color_str("edge"), "red");
  EXPECT_EQ(arrow.get_color_str("fill"), "green");
  EXPECT_EQ(arrow.get_color_str("label"), "white");
  EXPECT_NEAR(arrow.get_transparency(), 0.75, kGeomTol);
  EXPECT_NEAR(arrow.get_edge_size(), 3.0, kGeomTol);
  EXPECT_NEAR(arrow.get_duration(), 4.0, kGeomTol);
  EXPECT_NEAR(arrow.get_time(), 10.0, kGeomTol);

  std::string spec = arrow.get_spec(2);
  EXPECT_TRUE(stringContains(spec, "ctrx=1.25"));
  EXPECT_TRUE(stringContains(spec, "ctry=-2.5"));
  EXPECT_TRUE(stringContains(spec, "ang=270"));
  EXPECT_TRUE(stringContains(spec, "bwid=8"));
  EXPECT_TRUE(stringContains(spec, "blen=12"));
  EXPECT_TRUE(stringContains(spec, "hwid=14"));
  EXPECT_TRUE(stringContains(spec, "hlen=6"));
  EXPECT_TRUE(stringContains(spec, "active=false"));
  EXPECT_TRUE(stringContains(spec, "label=wind"));
  EXPECT_TRUE(stringContains(spec, "msg=gust"));
}

TEST(XYArrowPrimitiveTest, ParsesX1AlphaMissionViewArrowPayload)
{
  // ivp/missions/x1_alpha/alpha.moos has button-posted VIEW_ARROW specs
  // for checking pMarineViewer arrow rendering and replacement by label.
  XYArrow arrow = stringToArrow(
      "ctrx=20,ctry=-40,ang=135,bwid=10,blen=30,hwid=20,hlen=10,"
      "label=a,fill_color=yellow,fill_transparency=0.2,edge_color=off");

  EXPECT_TRUE(arrow.valid());
  EXPECT_NEAR(arrow.getCenterX(), 20.0, kGeomTol);
  EXPECT_NEAR(arrow.getCenterY(), -40.0, kGeomTol);
  EXPECT_EQ(arrow.get_label(), "a");
  EXPECT_EQ(arrow.get_color_str("fill"), "yellow");
  EXPECT_EQ(arrow.get_color_str("edge"), "invisible");
  EXPECT_NEAR(arrow.get_transparency(), 0.2, kGeomTol);

  expectVectorNear(arrow.getHeadVertices(),
                   {37.6776695, -57.6776695,
                    37.6776695, -43.5355339,
                    23.5355339, -57.6776695},
                   1e-6);

  std::string spec = arrow.get_spec(1);
  EXPECT_TRUE(stringContains(spec, "ctrx=20"));
  EXPECT_TRUE(stringContains(spec, "ctry=-40"));
  EXPECT_TRUE(stringContains(spec, "ang=135"));
  EXPECT_TRUE(stringContains(spec, "bwid=10"));
  EXPECT_TRUE(stringContains(spec, "blen=30"));
  EXPECT_TRUE(stringContains(spec, "hwid=20"));
  EXPECT_TRUE(stringContains(spec, "hlen=10"));
  EXPECT_TRUE(stringContains(spec, "label=a"));
  EXPECT_TRUE(stringContains(spec, "fill_color=yellow"));
  EXPECT_TRUE(stringContains(spec, "edge_color=invisible"));
  EXPECT_TRUE(stringContains(spec, "fill_transparency=0.2"));
}

TEST(XYArrowPrimitiveTest, MalformedPayloadFallsBackToDefaultArrow)
{
  XYArrow unknown = stringToArrow("ctrx=9,bogus=1,label=bad");
  EXPECT_NEAR(unknown.getCenterX(), 0.0, kGeomTol);
  EXPECT_EQ(unknown.get_label(), "");
  EXPECT_TRUE(unknown.active());

  XYArrow nonnumeric = stringToArrow("ctrx=not-a-number,label=bad");
  EXPECT_NEAR(nonnumeric.getCenterX(), 0.0, kGeomTol);
  EXPECT_EQ(nonnumeric.get_label(), "");
}

TEST(XYArrowPrimitiveTest, NegativeNumericPayloadDimensionsAreAcceptedButIgnored)
{
  XYArrow arrow = stringToArrow("bwid=-8,blen=-12,hwid=-14,hlen=-6,label=kept");

  EXPECT_EQ(arrow.get_label(), "kept");
  expectVectorNear(arrow.getBaseVertices(),
                   {2.5, 10.0, 2.5, -10.0, -2.5, -10.0, -2.5, 10.0});
  expectVectorNear(arrow.getHeadVertices(),
                   {0.0, 20.0, -5.0, 10.0, 5.0, 10.0});
}

TEST(XYArrowPrimitiveTest, PayloadParsingIsCaseInsensitiveForParameterNames)
{
  XYArrow arrow = stringToArrow("CTRX=3,CTRY=4,ANG=180,BWID=6,BLEN=8,"
                                "HWID=10,HLEN=4,LABEL=case");

  EXPECT_EQ(arrow.get_label(), "case");
  EXPECT_NEAR(arrow.getCenterX(), 3.0, kGeomTol);
  EXPECT_NEAR(arrow.getCenterY(), 4.0, kGeomTol);
  expectVectorNear(arrow.getHeadVertices(),
                   {3.0, -4.0, 8.0, 0.0, -2.0, 0.0});
}

TEST(XYArrowPrimitiveTest, GeoShapesArrowPayloadStoresTimestampsAndInactivePayloadErasesByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addArrow("label=wind,ctrx=2,ctry=3", 123.45));
  ASSERT_EQ(shapes.sizeArrows(), 1u);
  ASSERT_EQ(shapes.getArrows().count("wind"), 1u);
  EXPECT_NEAR(shapes.getArrows().at("wind").get_time(), 123.45, kGeomTol);

  ASSERT_TRUE(shapes.addArrow("label=wind,active=false", 200.0));
  EXPECT_EQ(shapes.sizeArrows(), 0u);
}

TEST(XYArrowPrimitiveTest, GeoShapesManageMemoryExpiresArrowPayloads)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addArrow("label=short,duration=5", 10.0));
  ASSERT_TRUE(shapes.addArrow("label=forever,ctrx=1", 10.0));
  ASSERT_EQ(shapes.sizeArrows(), 2u);

  shapes.manageMemory(14.0);
  EXPECT_EQ(shapes.sizeArrows(), 2u);

  shapes.manageMemory(16.0);
  EXPECT_EQ(shapes.sizeArrows(), 1u);
  EXPECT_EQ(shapes.getArrows().count("forever"), 1u);
}

TEST(XYArrowPrimitiveTest, GeoShapesReplacesX1AlphaButtonArrowByLabel)
{
  VPlug_GeoShapes shapes;

  ASSERT_TRUE(shapes.addArrow(
      "ctrx=20,ctry=-40,ang=135,bwid=10,blen=30,hwid=20,hlen=10,"
      "label=a",
      100.0));
  ASSERT_EQ(shapes.sizeArrows(), 1u);
  EXPECT_NEAR(shapes.getArrows().at("a").get_time(), 100.0, kGeomTol);
  EXPECT_TRUE(stringContains(shapes.getArrows().at("a").get_spec(1),
                             "ang=135"));

  ASSERT_TRUE(shapes.addArrow(
      "ctrx=20,ctry=-40,ang=270,bwid=10,blen=30,hwid=20,hlen=10,"
      "label=a,fill_color=white,fill_transparency=0.1,edge_color=off",
      0));
  ASSERT_EQ(shapes.sizeArrows(), 1u);
  const XYArrow& replacement = shapes.getArrows().at("a");
  EXPECT_EQ(replacement.get_color_str("fill"), "white");
  EXPECT_EQ(replacement.get_color_str("edge"), "invisible");
  EXPECT_NEAR(replacement.get_transparency(), 0.1, kGeomTol);
  EXPECT_TRUE(stringContains(replacement.get_spec(1), "ang=270"));
}

TEST(VPlugGeoShapesPrimitiveVectorTest, DirectAddGetAndForgetCoversVectorBackedShapeFamilies)
{
  VPlug_GeoShapes shapes;

  XYPolygon poly = makeSquarePoly(0, 0, 10, 10);
  poly.set_label("poly");
  shapes.addPolygon(poly);
  EXPECT_EQ(shapes.sizePolygons(), 1u);
  EXPECT_EQ(shapes.getPolygons().size(), 1u);
  EXPECT_EQ(shapes.poly(0).get_label(), "poly");
  EXPECT_EQ(shapes.sizeArcs(), 0u);
  EXPECT_EQ(shapes.getArcs().size(), 0u);

  XYVessel vessel(1, 2, 90, 5);
  vessel.set_label("vessel");
  shapes.addVessel(vessel);
  EXPECT_EQ(shapes.getVessels().size(), 1u);

  XYWedge wedge;
  wedge.setX(0);
  wedge.setY(0);
  wedge.setRadLow(5);
  wedge.setRadHigh(20);
  wedge.setLangle(0);
  wedge.setHangle(90);
  wedge.set_label("wedge");
  ASSERT_TRUE(wedge.initialize());
  shapes.addWedge(wedge);
  EXPECT_EQ(shapes.sizeWedges(), 1u);
  EXPECT_EQ(shapes.getWedges().size(), 1u);

  XYHexagon hex;
  ASSERT_TRUE(hex.initialize(0, 0, 5));
  hex.set_label("hex");
  shapes.addHexagon(hex);
  EXPECT_EQ(shapes.sizeHexagons(), 1u);
  EXPECT_EQ(shapes.getHexagons().size(), 1u);

  XYVector vector(1, 2, 3, 45);
  vector.set_label("vector");
  shapes.addVector(vector);
  EXPECT_EQ(shapes.sizeVectors(), 1u);

  XYRangePulse range(0, 0);
  range.set_label("range");
  shapes.addRangePulse(range);
  EXPECT_EQ(shapes.sizeRangePulses(), 1u);

  XYCommsPulse comms(0, 0, 10, 0);
  comms.set_label("comms");
  shapes.addCommsPulse(comms);
  EXPECT_EQ(shapes.sizeCommsPulses(), 1u);

  XYSegList segl;
  segl.add_vertex(0, 0);
  segl.add_vertex(10, 0);
  segl.set_label("segl");
  shapes.addSegList(segl);
  EXPECT_EQ(shapes.getSegLists().count("segl"), 1u);

  XYSeglr seglr(0, 0, 10, 0);
  seglr.set_label("seglr");
  shapes.addSeglr(seglr);
  EXPECT_EQ(shapes.getSeglrs().count("seglr"), 1u);

  shapes.forgetPolygon("poly");
  shapes.forgetVessel("vessel");
  shapes.forgetSegList("segl");
  shapes.forgetWedge("wedge");
  shapes.forgetHexagon("hex");
  shapes.forgetVector("vector");
  shapes.forgetRangePulse("range");
  shapes.forgetCommsPulse("comms");
  EXPECT_EQ(shapes.sizePolygons(), 0u);
  EXPECT_EQ(shapes.sizeVessels(), 0u);
  EXPECT_EQ(shapes.getSegLists().count("segl"), 1u);
  EXPECT_EQ(shapes.sizeSeglrs(), 1u);
  EXPECT_EQ(shapes.sizeWedges(), 0u);
  EXPECT_EQ(shapes.sizeHexagons(), 0u);
  EXPECT_EQ(shapes.sizeVectors(), 0u);
  EXPECT_EQ(shapes.sizeRangePulses(), 0u);
  EXPECT_EQ(shapes.sizeCommsPulses(), 0u);
}

TEST(XYSegmentPrimitiveTest, DefaultsAndSetComputeLengthAndMoosHeading)
{
  XYSegment seg;
  EXPECT_NEAR(seg.length(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 0.0, kGeomTol);

  seg.set(0, 0, 0, 10);
  EXPECT_NEAR(seg.length(), 10.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 180.0, kGeomTol);

  seg.set(0, 0, 10, 0);
  EXPECT_NEAR(seg.length(), 10.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 90.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 270.0, kGeomTol);
}

TEST(XYSegmentPrimitiveTest, DiagonalConstructorUsesMoosCompassBearings)
{
  XYSegment northeast(-2, -3, 2, 1);
  EXPECT_NEAR(northeast.length(), std::sqrt(32.0), kGeomTol);
  EXPECT_NEAR(northeast.getRAng12(), 45.0, kGeomTol);
  EXPECT_NEAR(northeast.getRAng21(), 225.0, kGeomTol);

  XYSegment southwest(2, 1, -2, -3);
  EXPECT_NEAR(southwest.length(), std::sqrt(32.0), kGeomTol);
  EXPECT_NEAR(southwest.getRAng12(), 225.0, kGeomTol);
  EXPECT_NEAR(southwest.getRAng21(), 45.0, kGeomTol);
}

TEST(XYSegmentPrimitiveTest, PointSetterUsesPointCoordinates)
{
  XYPoint one(1, 2);
  XYPoint two(4, 6);
  XYSegment seg;

  seg.set(one, two);
  EXPECT_NEAR(seg.get_x1(), 1.0, kGeomTol);
  EXPECT_NEAR(seg.get_y1(), 2.0, kGeomTol);
  EXPECT_NEAR(seg.get_x2(), 4.0, kGeomTol);
  EXPECT_NEAR(seg.get_y2(), 6.0, kGeomTol);
  EXPECT_NEAR(seg.length(), 5.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 36.86989765, kLooseGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 216.86989765, kLooseGeomTol);
}

TEST(XYSegmentPrimitiveTest, ShiftTranslatesEndpointsWithoutChangingCachedGeometry)
{
  XYSegment seg(0, 0, 3, 4);

  seg.shift_horz(10);
  seg.shift_vert(-2);

  EXPECT_NEAR(seg.get_x1(), 10.0, kGeomTol);
  EXPECT_NEAR(seg.get_y1(), -2.0, kGeomTol);
  EXPECT_NEAR(seg.get_x2(), 13.0, kGeomTol);
  EXPECT_NEAR(seg.get_y2(), 2.0, kGeomTol);
  EXPECT_NEAR(seg.length(), 5.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 36.86989765, kLooseGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 216.86989765, kLooseGeomTol);
}

TEST(XYSegmentPrimitiveTest, ReverseSwapsEndpointsButLeavesCachedBearingsAsConstructed)
{
  XYSegment seg(0, 0, 0, 10);

  seg.reverse();
  EXPECT_NEAR(seg.get_x1(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.get_y1(), 10.0, kGeomTol);
  EXPECT_NEAR(seg.get_x2(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.get_y2(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.length(), 10.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 180.0, kGeomTol);
}

TEST(XYSegmentPrimitiveTest, ClearResetsCoordinatesAndCachedValues)
{
  XYSegment seg(1, 2, 4, 6);
  seg.clear();

  EXPECT_NEAR(seg.get_x1(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.get_y1(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.get_x2(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.get_y2(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.length(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 0.0, kGeomTol);
}

TEST(XYSegmentPrimitiveTest, ZeroLengthSetPinsPointSegmentBearings)
{
  XYSegment seg(1, 2, 3, 4);
  seg.set(-7, 9, -7, 9);

  EXPECT_NEAR(seg.get_x1(), -7.0, kGeomTol);
  EXPECT_NEAR(seg.get_y1(), 9.0, kGeomTol);
  EXPECT_NEAR(seg.get_x2(), -7.0, kGeomTol);
  EXPECT_NEAR(seg.get_y2(), 9.0, kGeomTol);
  EXPECT_NEAR(seg.length(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng12(), 0.0, kGeomTol);
  EXPECT_NEAR(seg.getRAng21(), 0.0, kGeomTol);
}

TEST(XYSegmentPrimitiveTest, IntersectsIncludesCrossingsAndSharedEndpoints)
{
  XYSegment horizontal(0, 0, 10, 0);
  XYSegment vertical(5, -5, 5, 5);
  XYSegment touching(10, 0, 10, 10);
  XYSegment disjoint(0, 1, 10, 1);

  EXPECT_TRUE(horizontal.intersects(vertical));
  EXPECT_TRUE(horizontal.intersects(touching));
  EXPECT_FALSE(horizontal.intersects(disjoint));
}

TEST(XYSegmentPrimitiveTest, IntersectsIncludesCollinearOverlapButRejectsSeparatedCollinearSegments)
{
  XYSegment horizontal(0, 0, 10, 0);
  XYSegment overlapping(4, 0, 12, 0);
  XYSegment contained(2, 0, 8, 0);
  XYSegment separated(11, 0, 20, 0);
  XYSegment parallel(0, 1, 10, 1);

  EXPECT_TRUE(horizontal.intersects(overlapping));
  EXPECT_TRUE(horizontal.intersects(contained));
  EXPECT_FALSE(horizontal.intersects(separated));
  EXPECT_FALSE(horizontal.intersects(parallel));

  XYSegment vertical(5, -5, 5, 5);
  XYSegment vertical_overlap(5, 2, 5, 7);
  XYSegment vertical_separated(5, 6, 5, 9);
  EXPECT_TRUE(vertical.intersects(vertical_overlap));
  EXPECT_FALSE(vertical.intersects(vertical_separated));
}

TEST(XYSegmentPrimitiveTest, SerializesWithPrecisionClampingAndLabelPrefix)
{
  XYSegment seg(1.234567, -2.345678, 9.876543, 0.444444);
  seg.set_label("leg");

  EXPECT_EQ(seg.get_spec(-5), "label=leg#pts=1,-2:10,0");
  EXPECT_EQ(seg.get_spec(2), "label=leg#pts=1.23,-2.35:9.88,0.44");
  EXPECT_EQ(seg.get_spec(99), "label=leg#pts=1.234567,-2.345678:9.876543,0.444444");
}

TEST(XYSegmentPrimitiveTest, SerializesWithoutLabelPrefixWhenLabelIsEmpty)
{
  XYSegment seg(-1.2, 3.4, 5.6, -7.8);

  EXPECT_EQ(seg.get_spec(1), "pts=-1.2,3.4:5.6,-7.8");
}

TEST(XYSquarePrimitiveTest, ConstructorsValidityAndEqualityUseStoredBounds)
{
  XYSquare empty;
  EXPECT_FALSE(empty.valid());
  EXPECT_NEAR(empty.getLengthX(), 0.0, kGeomTol);
  EXPECT_NEAR(empty.getLengthY(), 0.0, kGeomTol);

  XYSquare unit(5);
  EXPECT_TRUE(unit.valid());
  EXPECT_NEAR(unit.get_min_x(), 0.0, kGeomTol);
  EXPECT_NEAR(unit.get_max_x(), 5.0, kGeomTol);
  EXPECT_NEAR(unit.get_min_y(), 0.0, kGeomTol);
  EXPECT_NEAR(unit.get_max_y(), 5.0, kGeomTol);

  XYSquare rect(2, 3);
  EXPECT_TRUE(rect.valid());
  EXPECT_NEAR(rect.getLengthX(), 2.0, kGeomTol);
  EXPECT_NEAR(rect.getLengthY(), 3.0, kGeomTol);

  XYSquare bounds(-1, 4, -2, 6);
  EXPECT_TRUE(bounds.valid());
  EXPECT_TRUE(bounds == XYSquare(-1, 4, -2, 6));
  EXPECT_FALSE(bounds != XYSquare(-1, 4, -2, 6));
  EXPECT_TRUE(bounds != unit);
}

TEST(XYSquarePrimitiveTest, SetAcceptsZeroAreaButMarksReversedBoundsInvalid)
{
  XYSquare square;
  square.set(2, 2, -1, -1);
  EXPECT_TRUE(square.valid());
  EXPECT_NEAR(square.getLengthX(), 0.0, kGeomTol);
  EXPECT_NEAR(square.getLengthY(), 0.0, kGeomTol);
  EXPECT_TRUE(square.containsPoint(2, -1));

  square.set(5, 1, 0, 10);
  EXPECT_FALSE(square.valid());
  EXPECT_NEAR(square.get_min_x(), 5.0, kGeomTol);
  EXPECT_NEAR(square.get_max_x(), 1.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, InvalidReversedBoundsStillExposeRawNegativeLengths)
{
  XYSquare square(5, 1, 10, 0);

  EXPECT_FALSE(square.valid());
  EXPECT_NEAR(square.getLengthX(), -4.0, kGeomTol);
  EXPECT_NEAR(square.getLengthY(), -10.0, kGeomTol);
  EXPECT_NEAR(square.getCenterX(), 3.0, kGeomTol);
  EXPECT_NEAR(square.getCenterY(), 5.0, kGeomTol);
  EXPECT_FALSE(square.containsPoint(3, 5));
}

TEST(XYSquarePrimitiveTest, ContainsPointIncludesBoundary)
{
  XYSquare square(0, 10, -5, 5);

  EXPECT_TRUE(square.containsPoint(5, 0));
  EXPECT_TRUE(square.containsPoint(0, -5));
  EXPECT_TRUE(square.containsPoint(10, 5));
  EXPECT_FALSE(square.containsPoint(-0.01, 0));
  EXPECT_FALSE(square.containsPoint(10.01, 0));
  EXPECT_FALSE(square.containsPoint(5, -5.01));
  EXPECT_FALSE(square.containsPoint(5, 5.01));
}

TEST(XYSquarePrimitiveTest, ShiftsPreserveSizeAndMoveCenter)
{
  XYSquare square(0, 10, 0, 20);

  square.shiftX(-4);
  square.shiftY(3);

  EXPECT_NEAR(square.get_min_x(), -4.0, kGeomTol);
  EXPECT_NEAR(square.get_max_x(), 6.0, kGeomTol);
  EXPECT_NEAR(square.get_min_y(), 3.0, kGeomTol);
  EXPECT_NEAR(square.get_max_y(), 23.0, kGeomTol);
  EXPECT_NEAR(square.getCenterX(), 1.0, kGeomTol);
  EXPECT_NEAR(square.getCenterY(), 13.0, kGeomTol);
  EXPECT_NEAR(square.getLengthX(), 10.0, kGeomTol);
  EXPECT_NEAR(square.getLengthY(), 20.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, SegmentIntersectionLengthClipsAxisAlignedSegments)
{
  XYSquare square(0, 10, 0, 10);

  EXPECT_NEAR(square.segIntersectLength(-5, 5, 15, 5), 10.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(5, -5, 5, 15), 10.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(2, 8, 8, 8), 6.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(-5, 11, 15, 11), 0.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(11, -5, 11, 15), 0.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, SegmentIntersectionLengthClipsDiagonalSegments)
{
  XYSquare square(0, 10, 0, 10);

  EXPECT_NEAR(square.segIntersectLength(-5, -5, 15, 15),
              std::sqrt(200.0), kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(-5, 2, 8, 15),
              std::sqrt(18.0), kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(-10, -10, 0, 0), 0.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(-10, -10, -1, -1), 0.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, SegmentIntersectionLengthHandlesInsideBoundaryAndDegenerateSquares)
{
  XYSquare square(0, 10, 0, 10);

  EXPECT_NEAR(square.segIntersectLength(2, 2, 8, 8), std::sqrt(72.0), kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(0, 0, 10, 0), 10.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(10, 0, 10, 10), 10.0, kGeomTol);
  EXPECT_NEAR(square.segIntersectLength(5, 5, 5, 5), 0.0, kGeomTol);

  XYSquare vertical_line(2, 2, 0, 10);
  EXPECT_TRUE(vertical_line.valid());
  EXPECT_NEAR(vertical_line.segIntersectLength(2, -5, 2, 15), 10.0, kGeomTol);
  EXPECT_NEAR(vertical_line.segIntersectLength(1, -5, 1, 15), 0.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, SegmentDistanceMeasuresDistanceToPerimeter)
{
  XYSquare square(0, 10, 0, 10);

  EXPECT_NEAR(square.segDistToSquare(-5, 5, 15, 5), 0.0, kGeomTol);
  EXPECT_NEAR(square.segDistToSquare(-5, -5, -1, -1), std::sqrt(2.0), kGeomTol);
  EXPECT_NEAR(square.segDistToSquare(5, 5, 6, 5), 4.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, SegmentDistanceHandlesInsideDiagonalAndExteriorAxisAlignedSegments)
{
  XYSquare square(0, 10, 0, 10);

  EXPECT_NEAR(square.segDistToSquare(4, 4, 6, 6), 4.0, kGeomTol);
  EXPECT_NEAR(square.segDistToSquare(-5, 3, -1, 3), 1.0, kGeomTol);
  EXPECT_NEAR(square.segDistToSquare(3, 12, 7, 12), 2.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, PointDistanceUsesSquareCenter)
{
  XYSquare square(0, 10, -10, 10);

  EXPECT_NEAR(square.ptDistToSquareCtr(5, 0), 0.0, kGeomTol);
  EXPECT_NEAR(square.ptDistToSquareCtr(8, 4), 5.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, GetValSelectsLowHighXAndY)
{
  XYSquare square(-1, 4, -2, 6);

  EXPECT_NEAR(square.getVal(0, 0), -1.0, kGeomTol);
  EXPECT_NEAR(square.getVal(0, 1), 4.0, kGeomTol);
  EXPECT_NEAR(square.getVal(1, 0), -2.0, kGeomTol);
  EXPECT_NEAR(square.getVal(1, 1), 6.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, GetValTreatsAnyNonzeroSelectorsAsYOrHigh)
{
  XYSquare square(-1, 4, -2, 6);

  EXPECT_NEAR(square.getVal(0, -1), 4.0, kGeomTol);
  EXPECT_NEAR(square.getVal(0, 99), 4.0, kGeomTol);
  EXPECT_NEAR(square.getVal(7, 0), -2.0, kGeomTol);
  EXPECT_NEAR(square.getVal(7, 99), 6.0, kGeomTol);
}

TEST(XYSquarePrimitiveTest, SerializesAsBoundsPolyAndEncoderPayload)
{
  XYSquare square(-1.25, 4.5, -2.0, 6.75);
  square.set_label("cell");
  square.set_active(false);

  std::string spec = square.get_spec();
  EXPECT_TRUE(stringContains(spec, "xlow=-1.25"));
  EXPECT_TRUE(stringContains(spec, "xhigh=4.5"));
  EXPECT_TRUE(stringContains(spec, "ylow=-2"));
  EXPECT_TRUE(stringContains(spec, "yhigh=6.75"));
  EXPECT_TRUE(stringContains(spec, "active=false"));
  EXPECT_TRUE(stringContains(spec, "label=cell"));

  std::string active_spec = square.get_spec("active=true");
  EXPECT_TRUE(stringContains(active_spec, "active=true"));

  EXPECT_EQ(square.get_spec_as_poly(),
            "pts={-1.25,-2:4.5,-2:4.5,6.75:-1.25,6.75},active=false,label=cell");
  EXPECT_EQ(XYSquareToString(square), "-1.25,4.5,-2,6.75");
}
