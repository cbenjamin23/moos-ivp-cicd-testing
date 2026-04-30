#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsWedge.h"
#include "XYWedge.h"

namespace {

bool IsValidQuietly(const XYWedge& wedge)
{
  testing::internal::CaptureStdout();
  const bool valid = wedge.isValid();
  testing::internal::GetCapturedStdout();
  return valid;
}

bool InitializeQuietly(XYWedge& wedge, double degrees_per_pt = 1)
{
  testing::internal::CaptureStdout();
  const bool initialized = wedge.initialize(degrees_per_pt);
  testing::internal::GetCapturedStdout();
  return initialized;
}

void ExpectPointNear(const std::vector<double>& cache, unsigned int index,
                     double expected_x, double expected_y)
{
  ASSERT_LT((index * 2) + 1, cache.size());
  EXPECT_NEAR(cache[index * 2], expected_x, kLooseGeomTol);
  EXPECT_NEAR(cache[(index * 2) + 1], expected_y, kLooseGeomTol);
}

}  // namespace

// Covers XY format utils wedge behavior: parses canonical view wedge payload used by marine viewer.
TEST(XYFormatUtilsWedgeTest, ParsesCanonicalViewWedgePayloadUsedByMarineViewer)
{
  XYWedge wedge = string2Wedge(
      "x=20,y=-30,rad_low=40,rad_hgh=80,ang_low=350,ang_hgh=20,"
      "label=contact_bearing_sector,edge_color=yellow,fill_color=white,"
      "fill_transparency=0.25,edge_size=2,vertex_color=green,"
      "vertex_size=3,label_color=gray50,active=false");

  EXPECT_TRUE(IsValidQuietly(wedge));
  EXPECT_NEAR(wedge.getX(), 20.0, kGeomTol);
  EXPECT_NEAR(wedge.getY(), -30.0, kGeomTol);
  EXPECT_NEAR(wedge.getRadLow(), 40.0, kGeomTol);
  EXPECT_NEAR(wedge.getRadHigh(), 80.0, kGeomTol);
  EXPECT_NEAR(wedge.getLangle(), 350.0, kGeomTol);
  EXPECT_NEAR(wedge.getHangle(), 20.0, kGeomTol);
  EXPECT_EQ(wedge.get_label(), "contact_bearing_sector");
  EXPECT_EQ(wedge.get_color_str("edge"), "yellow");
  EXPECT_EQ(wedge.get_color_str("fill"), "white");
  EXPECT_EQ(wedge.get_color_str("vertex"), "green");
  EXPECT_EQ(wedge.get_color_str("label"), "gray50");
  EXPECT_TRUE(wedge.transparency_set());
  EXPECT_NEAR(wedge.get_transparency(), 0.25, kGeomTol);
  EXPECT_TRUE(wedge.edge_size_set());
  EXPECT_NEAR(wedge.get_edge_size(), 2.0, kGeomTol);
  EXPECT_TRUE(wedge.vertex_size_set());
  EXPECT_NEAR(wedge.get_vertex_size(), 3.0, kGeomTol);
  EXPECT_FALSE(wedge.active());
}

// Covers XY format utils wedge behavior: ignores unsupported object metadata in parser.
TEST(XYFormatUtilsWedgeTest, IgnoresUnsupportedObjectMetadataInParser)
{
  XYWedge wedge = string2Wedge(
      "x=1,y=2,rad_low=3,rad_hgh=4,ang_low=5,ang_hgh=6,"
      "source=pMarineViewer,vsource=abe,type=wedge,msg=sector,id=w1,"
      "time=10,duration=5,active=true");

  EXPECT_TRUE(IsValidQuietly(wedge));
  EXPECT_TRUE(wedge.active());
  EXPECT_EQ(wedge.get_source(), "");
  EXPECT_EQ(wedge.get_vsource(), "");
  EXPECT_EQ(wedge.get_type(), "");
  EXPECT_EQ(wedge.get_msg(), "");
  EXPECT_EQ(wedge.get_id(), "");
  EXPECT_FALSE(wedge.time_set());
  EXPECT_FALSE(wedge.duration_set());
}

// Covers XY format utils wedge behavior: clamps loose drawing hints and treats any non true active as false.
TEST(XYFormatUtilsWedgeTest, ClampsLooseDrawingHintsAndTreatsAnyNonTrueActiveAsFalse)
{
  XYWedge wedge = string2Wedge(
      "x=1,y=2,rad_low=3,rad_hgh=4,ang_low=5,ang_hgh=6,"
      "fill_transparency=2,edge_size=-1,vertex_size=-2,"
      "edge_color=notacolor,active=maybe");

  EXPECT_TRUE(IsValidQuietly(wedge));
  EXPECT_TRUE(wedge.transparency_set());
  EXPECT_NEAR(wedge.get_transparency(), 1.0, kGeomTol);
  EXPECT_FALSE(wedge.edge_size_set());
  EXPECT_FALSE(wedge.vertex_size_set());
  EXPECT_FALSE(wedge.color_set("edge"));
  EXPECT_FALSE(wedge.active());
}

// Covers XY format utils wedge behavior: parser normalizes angles and radii in encounter sector payloads.
TEST(XYFormatUtilsWedgeTest, ParserNormalizesAnglesAndRadiiInEncounterSectorPayloads)
{
  XYWedge wrapped = string2Wedge(
      "x=0,y=0,rad_low=20,rad_hgh=80,ang_low=-45,ang_hgh=405,"
      "label=wrapped_bearing_sector");

  EXPECT_TRUE(IsValidQuietly(wrapped));
  EXPECT_NEAR(wrapped.getRadLow(), 20.0, kGeomTol);
  EXPECT_NEAR(wrapped.getRadHigh(), 80.0, kGeomTol);
  EXPECT_NEAR(wrapped.getLangle(), 315.0, kGeomTol);
  EXPECT_NEAR(wrapped.getHangle(), 45.0, kGeomTol);
  EXPECT_EQ(wrapped.get_label(), "wrapped_bearing_sector");

  XYWedge low_first = string2Wedge(
      "x=0,y=0,rad_low=50,rad_hgh=10,ang_low=0,ang_hgh=90");
  EXPECT_TRUE(IsValidQuietly(low_first));
  EXPECT_NEAR(low_first.getRadLow(), 10.0, kGeomTol);
  EXPECT_NEAR(low_first.getRadHigh(), 10.0, kGeomTol);

  XYWedge high_first = string2Wedge(
      "x=0,y=0,rad_hgh=10,rad_low=50,ang_low=0,ang_hgh=90");
  EXPECT_TRUE(IsValidQuietly(high_first));
  EXPECT_NEAR(high_first.getRadLow(), 50.0, kGeomTol);
  EXPECT_NEAR(high_first.getRadHigh(), 50.0, kGeomTol);
}

// Covers XY format utils wedge behavior: requires all six geometry fields only through is valid.
TEST(XYFormatUtilsWedgeTest, RequiresAllSixGeometryFieldsOnlyThroughIsValid)
{
  XYWedge missing_x = string2Wedge("y=2,rad_low=3,rad_hgh=4,ang_low=5,ang_hgh=6");
  XYWedge missing_y = string2Wedge("x=1,rad_low=3,rad_hgh=4,ang_low=5,ang_hgh=6");
  XYWedge missing_rad_low = string2Wedge("x=1,y=2,rad_hgh=4,ang_low=5,ang_hgh=6");
  XYWedge missing_rad_high = string2Wedge("x=1,y=2,rad_low=3,ang_low=5,ang_hgh=6");
  XYWedge missing_ang_low = string2Wedge("x=1,y=2,rad_low=3,rad_hgh=4,ang_hgh=6");
  XYWedge missing_ang_high = string2Wedge("x=1,y=2,rad_low=3,rad_hgh=4,ang_low=5");

  // string2Wedge currently calls XYObject::valid(), so callers must use
  // XYWedge::isValid() to detect missing geometry fields.
  EXPECT_TRUE(missing_x.valid());
  EXPECT_FALSE(IsValidQuietly(missing_x));
  EXPECT_FALSE(IsValidQuietly(missing_y));
  EXPECT_FALSE(IsValidQuietly(missing_rad_low));
  EXPECT_FALSE(IsValidQuietly(missing_rad_high));
  EXPECT_FALSE(IsValidQuietly(missing_ang_low));
  EXPECT_FALSE(IsValidQuietly(missing_ang_high));
}

// Covers XY format utils wedge behavior: rejects malformed numbers and negative radii by leaving wedge invalid.
TEST(XYFormatUtilsWedgeTest, RejectsMalformedNumbersAndNegativeRadiiByLeavingWedgeInvalid)
{
  EXPECT_FALSE(IsValidQuietly(
      string2Wedge("x=bad,y=2,rad_low=3,rad_hgh=4,ang_low=5,ang_hgh=6")));
  EXPECT_FALSE(IsValidQuietly(
      string2Wedge("x=1,y=2,rad_low=-3,rad_hgh=4,ang_low=5,ang_hgh=6")));
  EXPECT_FALSE(IsValidQuietly(
      string2Wedge("x=1,y=2,rad_low=3,rad_hgh=-4,ang_low=5,ang_hgh=6")));
  EXPECT_FALSE(IsValidQuietly(
      string2Wedge("x=1,y=2,rad_low=3,rad_hgh=4,ang_low=bad,ang_hgh=6")));
}

// Covers XY wedge behavior: setters normalize angles and maintain radius ordering.
TEST(XYWedgeTest, SettersNormalizeAnglesAndMaintainRadiusOrdering)
{
  XYWedge wedge;
  wedge.setX(0);
  wedge.setY(0);
  wedge.setLangle(-10);
  wedge.setHangle(370);

  EXPECT_NEAR(wedge.getLangle(), 350.0, kGeomTol);
  EXPECT_NEAR(wedge.getHangle(), 10.0, kGeomTol);

  EXPECT_TRUE(wedge.setRadLow(80));
  EXPECT_NEAR(wedge.getRadLow(), 80.0, kGeomTol);
  EXPECT_NEAR(wedge.getRadHigh(), 80.0, kGeomTol);
  EXPECT_FALSE(IsValidQuietly(wedge));

  EXPECT_TRUE(wedge.setRadHigh(20));
  EXPECT_NEAR(wedge.getRadLow(), 20.0, kGeomTol);
  EXPECT_NEAR(wedge.getRadHigh(), 20.0, kGeomTol);
  EXPECT_TRUE(IsValidQuietly(wedge));

  EXPECT_FALSE(wedge.setRadLow(-1));
  EXPECT_FALSE(wedge.setRadHigh(-1));
  EXPECT_NEAR(wedge.getRadLow(), 20.0, kGeomTol);
  EXPECT_NEAR(wedge.getRadHigh(), 20.0, kGeomTol);
}

// Covers XY wedge behavior: initializes bounds and point cache using MOOS heading convention.
TEST(XYWedgeTest, InitializesBoundsAndPointCacheUsingMoosHeadingConvention)
{
  XYWedge wedge = string2Wedge("x=0,y=0,rad_low=10,rad_hgh=20,ang_low=0,ang_hgh=90");

  ASSERT_TRUE(IsValidQuietly(wedge));
  ASSERT_TRUE(InitializeQuietly(wedge, 30));

  EXPECT_NEAR(wedge.getMinX(), 0.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxX(), 20.0, kGeomTol);
  EXPECT_NEAR(wedge.getMinY(), 0.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxY(), 20.0, kGeomTol);

  const std::vector<double> cache = wedge.getPointCache();
  ASSERT_EQ(cache.size(), 20u);
  ExpectPointNear(cache, 0, 0.0, 10.0);
  ExpectPointNear(cache, 1, 0.0, 20.0);
  ExpectPointNear(cache, 2, 0.0, 20.0);
  ExpectPointNear(cache, 3, 10.0, 17.320508);
  ExpectPointNear(cache, 4, 17.320508, 10.0);
  ExpectPointNear(cache, 5, 20.0, 0.0);
  ExpectPointNear(cache, 6, 10.0, 0.0);
  ExpectPointNear(cache, 7, 10.0, 0.0);
  ExpectPointNear(cache, 8, 8.660254, 5.0);
  ExpectPointNear(cache, 9, 5.0, 8.660254);
}

// Covers XY wedge behavior: bounds expand across cardinal axes inside arc.
TEST(XYWedgeTest, BoundsExpandAcrossCardinalAxesInsideArc)
{
  XYWedge wedge = string2Wedge("x=5,y=-5,rad_low=2,rad_hgh=10,ang_low=45,ang_hgh=225");

  ASSERT_TRUE(IsValidQuietly(wedge));
  ASSERT_TRUE(InitializeQuietly(wedge, 45));

  EXPECT_NEAR(wedge.getMinX(), -2.071068, kLooseGeomTol);
  EXPECT_NEAR(wedge.getMaxX(), 15.0, kGeomTol);
  EXPECT_NEAR(wedge.getMinY(), -15.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxY(), 2.071068, kLooseGeomTol);
}

// Covers XY wedge behavior: setters invalidate cache and reinitialize shifted sector bounds.
TEST(XYWedgeTest, SettersInvalidateCacheAndReinitializeShiftedSectorBounds)
{
  XYWedge wedge = string2Wedge("x=0,y=0,rad_low=5,rad_hgh=10,ang_low=0,ang_hgh=90");

  ASSERT_TRUE(IsValidQuietly(wedge));
  ASSERT_TRUE(InitializeQuietly(wedge, 45));
  EXPECT_NEAR(wedge.getMaxX(), 10.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxY(), 10.0, kGeomTol);
  ASSERT_GT(wedge.getPointCache().size(), 8u);

  wedge.setX(100);
  wedge.setY(-50);
  wedge.setLangle(180);
  wedge.setHangle(270);
  ASSERT_TRUE(InitializeQuietly(wedge, 45));

  EXPECT_NEAR(wedge.getMinX(), 90.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxX(), 100.0, kGeomTol);
  EXPECT_NEAR(wedge.getMinY(), -60.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxY(), -50.0, kGeomTol);
  const std::vector<double> cache = wedge.getPointCache();
  ASSERT_GT(cache.size(), 8u);
  ExpectPointNear(cache, 0, 100.0, -55.0);
  ExpectPointNear(cache, 1, 100.0, -60.0);
}

// Covers XY wedge behavior: coarse draw resolution still includes radial edges and endcaps.
TEST(XYWedgeTest, CoarseDrawResolutionStillIncludesRadialEdgesAndEndcaps)
{
  XYWedge wedge = string2Wedge("x=0,y=0,rad_low=5,rad_hgh=10,ang_low=10,ang_hgh=20");

  ASSERT_TRUE(IsValidQuietly(wedge));
  ASSERT_TRUE(InitializeQuietly(wedge, 90));

  const std::vector<double> cache = wedge.getPointCache();
  ASSERT_EQ(cache.size(), 12u);
  ExpectPointNear(cache, 0, 0.868241, 4.924039);
  ExpectPointNear(cache, 1, 1.736482, 9.848078);
  ExpectPointNear(cache, 2, 1.736482, 9.848078);
  ExpectPointNear(cache, 3, 3.420201, 9.396926);
  ExpectPointNear(cache, 4, 1.710101, 4.698463);
  ExpectPointNear(cache, 5, 1.710101, 4.698463);
}

// Covers XY wedge behavior: wraparound arc computes bounds but current cache omits arc samples.
TEST(XYWedgeTest, WraparoundArcComputesBoundsButCurrentCacheOmitsArcSamples)
{
  XYWedge wedge = string2Wedge("x=0,y=0,rad_low=5,rad_hgh=10,ang_low=350,ang_hgh=20");

  ASSERT_TRUE(IsValidQuietly(wedge));
  ASSERT_TRUE(InitializeQuietly(wedge, 5));

  EXPECT_NEAR(wedge.getMinX(), -1.736482, kLooseGeomTol);
  EXPECT_NEAR(wedge.getMaxX(), 3.420201, kLooseGeomTol);
  EXPECT_NEAR(wedge.getMinY(), 4.698463, kLooseGeomTol);
  EXPECT_NEAR(wedge.getMaxY(), 10.0, kGeomTol);

  // The bounds use wrapped angle span, but the current cache loops compare
  // raw low/high angles and therefore skip both arcs when low > high.
  const std::vector<double> cache = wedge.getPointCache();
  ASSERT_EQ(cache.size(), 8u);
  ExpectPointNear(cache, 0, -0.868241, 4.924039);
  ExpectPointNear(cache, 1, -1.736482, 9.848078);
  ExpectPointNear(cache, 2, 3.420201, 9.396926);
  ExpectPointNear(cache, 3, 1.710101, 4.698463);
}

// Covers XY wedge behavior: full circle like equal angles degenerates to two radial edges.
TEST(XYWedgeTest, FullCircleLikeEqualAnglesDegeneratesToTwoRadialEdges)
{
  XYWedge wedge = string2Wedge("x=0,y=0,rad_low=5,rad_hgh=10,ang_low=90,ang_hgh=90");

  ASSERT_TRUE(IsValidQuietly(wedge));
  ASSERT_TRUE(InitializeQuietly(wedge, 10));

  EXPECT_NEAR(wedge.getMinX(), 5.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxX(), 10.0, kGeomTol);
  EXPECT_NEAR(wedge.getMinY(), 0.0, kGeomTol);
  EXPECT_NEAR(wedge.getMaxY(), 0.0, kGeomTol);

  const std::vector<double> cache = wedge.getPointCache();
  ASSERT_EQ(cache.size(), 8u);
  ExpectPointNear(cache, 0, 5.0, 0.0);
  ExpectPointNear(cache, 1, 10.0, 0.0);
  ExpectPointNear(cache, 2, 10.0, 0.0);
  ExpectPointNear(cache, 3, 5.0, 0.0);
}

// Covers XY wedge behavior: serializes spec and round trips supported fields.
TEST(XYWedgeTest, SerializesSpecAndRoundTripsSupportedFields)
{
  XYWedge wedge;
  wedge.setX(1.234);
  wedge.setY(-2.345);
  wedge.setRadLow(3.456);
  wedge.setRadHigh(9.876);
  wedge.setLangle(45.5);
  wedge.setHangle(120.25);
  wedge.set_label("sector");
  wedge.set_msg("bearing_window");
  wedge.set_source("unit_test");
  wedge.set_id("wedge-1");
  wedge.set_time(42.25);
  wedge.set_duration(3);
  wedge.set_color("edge", "white");
  wedge.set_color("fill", "aqua");
  wedge.set_vertex_size(2);
  wedge.set_edge_size(1);
  wedge.set_transparency(0.4);

  std::string spec = wedge.get_spec(2, "active=true");
  EXPECT_TRUE(stringContains(spec, "x=1.23"));
  EXPECT_TRUE(stringContains(spec, "y=-2.35"));
  EXPECT_TRUE(stringContains(spec, "rad_low=3.46"));
  EXPECT_TRUE(stringContains(spec, "rad_hgh=9.88"));
  EXPECT_TRUE(stringContains(spec, "ang_low=45.5"));
  EXPECT_TRUE(stringContains(spec, "ang_hgh=120.25"));
  EXPECT_TRUE(stringContains(spec, "active=true"));
  EXPECT_TRUE(stringContains(spec, "label=sector"));
  EXPECT_TRUE(stringContains(spec, "msg=bearing_window"));
  EXPECT_TRUE(stringContains(spec, "source=unit_test"));
  EXPECT_TRUE(stringContains(spec, "id=wedge-1"));
  EXPECT_TRUE(stringContains(spec, "time=42.25"));
  EXPECT_TRUE(stringContains(spec, "duration=3"));
  EXPECT_TRUE(stringContains(spec, "edge_color=white"));
  EXPECT_TRUE(stringContains(spec, "fill_color=aqua"));
  EXPECT_TRUE(stringContains(spec, "vertex_size=2"));
  EXPECT_TRUE(stringContains(spec, "edge_size=1"));
  EXPECT_TRUE(stringContains(spec, "fill_transparency=0.4"));

  XYWedge reparsed = string2Wedge(spec);
  EXPECT_TRUE(IsValidQuietly(reparsed));
  EXPECT_NEAR(reparsed.getX(), 1.23, kGeomTol);
  EXPECT_NEAR(reparsed.getY(), -2.35, kGeomTol);
  EXPECT_NEAR(reparsed.getRadLow(), 3.46, kGeomTol);
  EXPECT_NEAR(reparsed.getRadHigh(), 9.88, kGeomTol);
  EXPECT_NEAR(reparsed.getLangle(), 45.5, kGeomTol);
  EXPECT_NEAR(reparsed.getHangle(), 120.25, kGeomTol);
  EXPECT_EQ(reparsed.get_label(), "sector");
  EXPECT_EQ(reparsed.get_color_str("edge"), "white");
  EXPECT_EQ(reparsed.get_color_str("fill"), "aqua");
  EXPECT_TRUE(reparsed.vertex_size_set());
  EXPECT_TRUE(reparsed.edge_size_set());
  EXPECT_TRUE(reparsed.transparency_set());
  EXPECT_EQ(reparsed.get_msg(), "");
  EXPECT_EQ(reparsed.get_source(), "");
  EXPECT_EQ(reparsed.get_id(), "");
  EXPECT_FALSE(reparsed.time_set());
  EXPECT_FALSE(reparsed.duration_set());
}

// Covers XY wedge behavior: is valid currently emits debug diagnostics.
TEST(XYWedgeTest, IsValidCurrentlyEmitsDebugDiagnostics)
{
  XYWedge wedge = string2Wedge("x=0,y=0,rad_low=1,rad_hgh=2,ang_low=3,ang_hgh=4");

  testing::internal::CaptureStdout();
  EXPECT_TRUE(wedge.isValid());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(stringContains(output, "langle_set true"));
  EXPECT_TRUE(stringContains(output, "hangle_set true"));
  EXPECT_TRUE(stringContains(output, "m_x_set true"));
  EXPECT_TRUE(stringContains(output, "m_y_set true"));
  EXPECT_TRUE(stringContains(output, "m_radlow_set true"));
  EXPECT_TRUE(stringContains(output, "m_radhgh_set true"));
}
