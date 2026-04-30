#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "GeomUtils.h"
#include "NumericAssertions.h"
#include "Seglr.h"
#include "SeglrUtils.h"
#include "WallEngine.h"
#include "XYArtifactGrid.h"
#include "XYConvexGrid.h"
#include "XYEncoders.h"
#include "XYFieldGenerator.h"
#include "XYFormatUtilsSegl.h"
#include "XYFormatUtilsConvexGrid.h"
#include "XYGrid.h"
#include "XYPoint.h"
#include "XYPolygon.h"
#include "XYSegList.h"
#include "XYSquare.h"

namespace {

constexpr const char* kArtifactGridSpec =
    "pts={0,0:20,0:20,10:0,10},label=artifact_grid@10,5";

constexpr const char* kLegacyGridEncoding =
    "label=search_grid#size=4#min_val=0#max_val=9#sbound=0,20,0,20#"
    "pbound=0,0:20,0:20,20:0,20#"
    "squares=0,10,0,10,3@0,10,10,20,4@10,20,0,10,8@10,20,10,20,9";

void expectSquareNear(const XYSquare& square,
                      double xlow,
                      double xhigh,
                      double ylow,
                      double yhigh)
{
  ASSERT_TRUE(square.valid());
  EXPECT_NEAR(square.getVal(0, 0), xlow, kGeomTol);
  EXPECT_NEAR(square.getVal(0, 1), xhigh, kGeomTol);
  EXPECT_NEAR(square.getVal(1, 0), ylow, kGeomTol);
  EXPECT_NEAR(square.getVal(1, 1), yhigh, kGeomTol);
}

XYSegList makeWall(double x1, double y1, double x2, double y2)
{
  XYSegList wall;
  wall.add_vertex(x1, y1);
  wall.add_vertex(x2, y2);
  return wall;
}

XYSegList parseWall(const std::string& spec)
{
  XYSegList wall = string2SegList(spec);
  EXPECT_TRUE(wall.valid());
  return wall;
}

XYPolygon labeledSquare(const std::string& label,
                        double low_x,
                        double low_y,
                        double high_x,
                        double high_y)
{
  XYPolygon poly = makeSquarePoly(low_x, low_y, high_x, high_y);
  poly.set_label(label);
  return poly;
}

bool pointInAnyPolygon(const XYPoint& point, const std::vector<XYPolygon>& polys)
{
  for(const XYPolygon& poly : polys) {
    if(poly.contains(point.x(), point.y()))
      return true;
  }
  return false;
}

}  // namespace

// Covers seglr utils behavior: serializes empty and populated seglr with fixed legacy precision.
TEST(SeglrUtilsTest, SerializesEmptyAndPopulatedSeglrWithFixedLegacyPrecision)
{
  EXPECT_EQ(seglrToString(Seglr()), "#0.0");

  Seglr seglr;
  seglr.addVertex(0, 0);
  seglr.addVertex(1.234, -5.678);
  seglr.setRayAngle(92.25);

  EXPECT_EQ(seglrToString(seglr), "0.00,0.00:1.23,-5.68#92.2");
}

// Covers seglr utils behavior: rotates stem around first vertex and normalizes ray angle.
TEST(SeglrUtilsTest, RotatesStemAroundFirstVertexAndNormalizesRayAngle)
{
  Seglr seglr;
  seglr.addVertex(10, 10);
  seglr.addVertex(10, 20);
  seglr.addVertex(20, 20);
  seglr.setRayAngle(350);

  Seglr rotated = rotateSeglr(seglr, 90);

  EXPECT_NEAR(rotated.getVX(0), 10.0, kGeomTol);
  EXPECT_NEAR(rotated.getVY(0), 10.0, kGeomTol);
  EXPECT_NEAR(rotated.getVX(1), 20.0, kLooseGeomTol);
  EXPECT_NEAR(rotated.getVY(1), 10.0, kLooseGeomTol);
  EXPECT_NEAR(rotated.getVX(2), 20.0, kLooseGeomTol);
  EXPECT_NEAR(rotated.getVY(2), 0.0, kLooseGeomTol);
  EXPECT_NEAR(rotated.getRayAngle(), 80.0, kGeomTol);
}

// Covers seglr utils behavior: rotating empty seglr preserves unnormalized ray angle.
TEST(SeglrUtilsTest, RotatingEmptySeglrPreservesUnnormalizedRayAngle)
{
  Seglr seglr;
  seglr.setRayAngle(725);

  Seglr rotated = rotateSeglr(seglr, 45);

  EXPECT_EQ(rotated.size(), 0u);
  EXPECT_NEAR(rotated.getRayAngle(), 725.0, kGeomTol);
}

// Covers wall engine behavior: rejects missing empty and self crossing walls.
TEST(WallEngineTest, RejectsMissingEmptyAndSelfCrossingWalls)
{
  WallEngine engine;
  std::vector<XYSegList> walls;
  EXPECT_FALSE(engine.setEngine(0, 0, 0, 5, walls));

  walls.push_back(XYSegList());
  EXPECT_FALSE(engine.setEngine(0, 0, 0, 5, walls));

  XYSegList crossing;
  crossing.add_vertex(0, 0);
  crossing.add_vertex(10, 10);
  crossing.add_vertex(0, 10);
  crossing.add_vertex(10, 0);
  walls[0] = crossing;
  EXPECT_FALSE(engine.setEngine(0, 0, 0, 5, walls));
}

// Covers wall engine behavior: initializes caches and reports minimum range to nearest wall.
TEST(WallEngineTest, InitializesCachesAndReportsMinimumRangeToNearestWall)
{
  WallEngine engine;
  std::vector<XYSegList> walls;
  walls.push_back(makeWall(20, -10, 20, 10));
  walls.push_back(makeWall(-50, -10, -50, 10));

  testing::internal::CaptureStdout();
  const bool ok = engine.setEngine(0, 0, 0, 5, walls);
  testing::internal::GetCapturedStdout();

  ASSERT_TRUE(ok);
  EXPECT_NEAR(engine.getMinRangeToWall(), 20.0, kLooseGeomTol);
  EXPECT_NEAR(engine.getXCPA(90, 2.0, 0, 0), 0.0, kGeomTol);
  EXPECT_NEAR(engine.getXCPA(450, 2.0, 0, 0), 0.0, kGeomTol);
}

// Covers wall engine behavior: non positive speed uses legacy point one fallback.
TEST(WallEngineTest, NonPositiveSpeedUsesLegacyPointOneFallback)
{
  WallEngine engine;
  std::vector<XYSegList> walls;
  walls.push_back(makeWall(20, -10, 20, 10));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(engine.setEngine(0, 0, 0, 5, walls));
  testing::internal::GetCapturedStdout();

  // WallEngine substitutes 0.1 for non-positive speed when projecting wall CPA.
  const double stopped = engine.getXCPA(90, 0.0, 0, 0);
  const double slow = engine.getXCPA(90, 0.1, 0, 0);
  EXPECT_NEAR(stopped, slow, kGeomTol);
}

// Covers wall engine behavior: parses avoid walls mission payloads and caches nearest wall range.
TEST(WallEngineTest, ParsesAvoidWallsMissionPayloadsAndCachesNearestWallRange)
{
  // These wall specs mirror BHV_AvoidWalls mission payloads with labels and
  // loose whitespace around point separators.
  std::vector<XYSegList> walls;
  walls.push_back(parseWall("pts={0,-72: 47,-72 : 43,-89 : 40,-100}, label=west"));
  walls.push_back(parseWall("pts={120,-72 : 93,-72 : 67,-89 : 47,-121}, label=east"));

  WallEngine engine;
  testing::internal::CaptureStdout();
  const bool ok = engine.setEngine(50, -130, 0, 8, walls);
  testing::internal::GetCapturedStdout();

  ASSERT_TRUE(ok);
  const double expected_min_range =
      std::min(distPointToSegl(50, -130, walls[0]),
               distPointToSegl(50, -130, walls[1]));
  EXPECT_NEAR(engine.getMinRangeToWall(), expected_min_range, kLooseGeomTol);
  EXPECT_NEAR(engine.getXCPA(360, 2.0, 8, 0.5),
              engine.getXCPA(0, 2.0, 8, 0.5), kGeomTol);
  EXPECT_NEAR(engine.getXCPA(-360, 2.0, 8, 0.5),
              engine.getXCPA(0, 2.0, 8, 0.5), kGeomTol);
}

// Covers wall engine behavior: time to collision penalty increases distant wall CPA.
TEST(WallEngineTest, TimeToCollisionPenaltyIncreasesDistantWallCpa)
{
  WallEngine engine;
  std::vector<XYSegList> walls;
  walls.push_back(makeWall(100, -20, 100, 20));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(engine.setEngine(0, 0, 90, 10, walls));
  testing::internal::GetCapturedStdout();

  const double immediate = engine.getXCPA(270, 2.0, 1000, 0.5);
  const double penalized = engine.getXCPA(270, 2.0, 0, 0.5);
  ASSERT_GE(immediate, 0.0);
  ASSERT_GE(penalized, 0.0);
  EXPECT_GT(penalized, immediate);
}

// Covers wall engine behavior: returns negative CPA when heading has no wall intercept.
TEST(WallEngineTest, ReturnsNegativeCpaWhenHeadingHasNoWallIntercept)
{
  WallEngine engine;
  std::vector<XYSegList> walls;
  walls.push_back(makeWall(100, -20, 100, 20));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(engine.setEngine(0, 0, 90, 10, walls));
  testing::internal::GetCapturedStdout();

  EXPECT_GE(engine.getXCPA(270, 2.0, 0, 0), 0.0);
  EXPECT_LT(engine.getXCPA(90, 2.0, 0, 0), 0.0);
}

// Covers XY encoders behavior: serializes square bounds in low high low high order.
TEST(XYEncodersTest, SerializesSquareBoundsInLowHighLowHighOrder)
{
  XYSquare square(-1.5, 2.25, 3.5, 4.75);

  EXPECT_EQ(XYSquareToString(square), "-1.5,2.25,3.5,4.75");
}

// Covers XY encoders behavior: decodes legacy grid payload and restores values.
TEST(XYEncodersTest, DecodesLegacyGridPayloadAndRestoresValues)
{
  XYGrid grid = StringToXYGrid(kLegacyGridEncoding);

  ASSERT_EQ(grid.size(), 4);
  EXPECT_EQ(grid.getLabel(), "search_grid");
  expectSquareNear(grid.getSBound(), 0, 20, 0, 20);
  EXPECT_NEAR(grid.getVal(0), 3.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 4.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(2), 8.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(3), 9.0, kGeomTol);
}

// Covers XY encoders behavior: current grid encoder payload does not round trip through legacy decoder.
TEST(XYEncodersTest, CurrentGridEncoderPayloadDoesNotRoundTripThroughLegacyDecoder)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(
      "pts={0,0:20,0:20,20:0,20},label=search_grid@10,10@2"));
  grid.setVal(0, 3);

  const std::string encoded = XYGridToString(grid);
  EXPECT_TRUE(stringContains(encoded, "pbound=pts={"));

  // StringToXYGrid expects the old bare vertex pbound form. The modern
  // XYPolygon::get_spec payload is therefore rejected today.
  XYGrid round_trip = StringToXYGrid(encoded);
  EXPECT_EQ(round_trip.size(), 0);
}

// Covers XY encoders behavior: rejects malformed legacy grid payloads.
TEST(XYEncodersTest, RejectsMalformedLegacyGridPayloads)
{
  EXPECT_EQ(StringToXYGrid("label=grid#size=1#min_val=0#max_val=1").size(), 0);
  EXPECT_EQ(StringToXYGrid(
                "label=grid#size=1#min_val=0#max_val=1#sbound=0,1,0,1#"
                "pbound=0,0:1,0:1,1:0,1#squares=0,1,0,1").size(),
            0);
  // The decoder ignores the declared size field and trusts the square payload.
  EXPECT_EQ(StringToXYGrid(
                "label=grid#size=2#min_val=0#max_val=1#sbound=0,1,0,1#"
                "pbound=0,0:1,0:1,1:0,1#squares=0,1,0,1,7").size(),
            1);
}

// Covers XY format utils convex grid behavior: parses MOOS view grid with limits and unknown style.
TEST(XYFormatUtilsConvexGridTest, ParsesMoosViewGridWithLimitsAndUnknownStyle)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,cell_min=score:0,cell_max=score:100,"
      "cell=1:visited:1:score:80,label=search_grid,edge_color=white");

  ASSERT_EQ(grid.size(), 4u);
  EXPECT_EQ(grid.get_label(), "search_grid");
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("visited")), 1.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("score")), 80.0, kGeomTol);
  EXPECT_TRUE(grid.cellVarMinLimited(grid.getCellVarIX("score")));
  EXPECT_TRUE(grid.cellVarMaxLimited(grid.getCellVarIX("score")));
  EXPECT_TRUE(stringContains(grid.get_spec(), "edge_color=white"));
}

// Covers XY format utils convex grid behavior: init val applies when cell vars are omitted.
TEST(XYFormatUtilsConvexGridTest, InitValAppliesWhenCellVarsAreOmitted)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:10,0:10,10:0,10},cell_size=5,init_val=7,label=single_var");

  ASSERT_EQ(grid.size(), 4u);
  EXPECT_TRUE(grid.hasCellVar("v"));
  EXPECT_NEAR(grid.getVal(0), 7.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(3), 7.0, kGeomTol);
}

// Covers XY format utils convex grid behavior: rejects malformed geometry cell size and cell vars.
TEST(XYFormatUtilsConvexGridTest, RejectsMalformedGeometryCellSizeAndCellVars)
{
  EXPECT_EQ(string2ConvexGrid("pts={},cell_size=10").size(), 0u);
  EXPECT_EQ(string2ConvexGrid("pts={0,0:10,0:0,10:10,10},cell_size=10").size(), 0u);
  EXPECT_EQ(string2ConvexGrid("pts={0,0:10,0:10,10:0,10},cell_size=0").size(), 0u);
  EXPECT_EQ(string2ConvexGrid("pts={0,0:10,0:10,10:0,10},cell_vars=score:bad").size(), 0u);
}

// Covers XY format utils convex grid behavior: ignores unknown cell vars and atoi parses bad cell index as zero.
TEST(XYFormatUtilsConvexGridTest, IgnoresUnknownCellVarsAndAtoiParsesBadCellIndexAsZero)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=score:5,cell=bad:score:9:unknown:99");

  ASSERT_EQ(grid.size(), 4u);
  EXPECT_NEAR(grid.getVal(0, grid.getCellVarIX("score")), 9.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("score")), 5.0, kGeomTol);
}

// Covers XY artifact grid behavior: initializes labeled MOOS polygon into column major cells.
TEST(XYArtifactGridTest, InitializesLabeledMoosPolygonIntoColumnMajorCells)
{
  XYArtifactGrid grid;

  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));
  EXPECT_EQ(grid.getLabel(), "artifact_grid");
  EXPECT_EQ(grid.getConfigString(), kArtifactGridSpec);
  ASSERT_EQ(grid.size(), 4u);
  EXPECT_EQ(grid.getElements().size(), 4u);
  expectSquareNear(grid.getElement(0), 0, 10, 0, 5);
  expectSquareNear(grid.getElement(1), 0, 10, 5, 10);
  expectSquareNear(grid.getElement(2), 10, 20, 0, 5);
  expectSquareNear(grid.getElement(3), 10, 20, 5, 10);
  EXPECT_NEAR(grid.getAvgClearance(), 0.0, kGeomTol);
}

// Covers XY artifact grid behavior: rejects malformed configs and requires labeled convex polygon.
TEST(XYArtifactGridTest, RejectsMalformedConfigsAndRequiresLabeledConvexPolygon)
{
  XYArtifactGrid grid;
  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));

  EXPECT_FALSE(grid.initialize("pts={0,0:20,0:20,10:0,10},label=artifact_grid"));
  EXPECT_EQ(grid.size(), 0u);
  EXPECT_EQ(grid.getLabel(), "");

  EXPECT_FALSE(grid.initialize("pts={0,0:20,0:20,10:0,10}@10,5"));
  EXPECT_FALSE(grid.initialize("pts={0,0:10,10:0,10:10,0},label=bad@5,5"));
  EXPECT_FALSE(grid.initialize("pts={0,0:5,0:5,5:0,5},label=bad@10,10"));
}

// Covers XY artifact grid behavior: clamps clearance and ignores out of range setters.
TEST(XYArtifactGridTest, ClampsClearanceAndIgnoresOutOfRangeSetters)
{
  XYArtifactGrid grid;
  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));

  grid.setClearance(0, 0.25);
  grid.setClearance(1, 2.0);
  grid.setClearance(2, -1.0);
  grid.setClearance(99, 0.75);

  EXPECT_NEAR(grid.getClearance(0), 0.25, kGeomTol);
  EXPECT_NEAR(grid.getClearance(1), 1.0, kGeomTol);
  EXPECT_NEAR(grid.getClearance(2), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getClearance(99), -1.0, kGeomTol);
  EXPECT_NEAR(grid.getAvgClearance(), 0.3125, kGeomTol);
}

// Covers XY artifact grid behavior: tracks artifact vectors and detected artifact count.
TEST(XYArtifactGridTest, TracksArtifactVectorsAndDetectedArtifactCount)
{
  XYArtifactGrid grid;
  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));

  ArtVec first_cell;
  first_cell.push_back("artifact=A,x=1,y=1");
  first_cell.push_back("artifact=B,x=2,y=2");
  grid.setArtVec(0, first_cell);
  grid.setArtVec(99, first_cell);

  ASSERT_EQ(grid.getArtVec(0).size(), 2u);
  EXPECT_EQ(grid.getArtVec(1).size(), 0u);
  EXPECT_EQ(grid.getArtVec(99).size(), 0u);
  EXPECT_EQ(grid.getDetectedArts(), 2u);
}

// Covers XY artifact grid behavior: point and segment intersection use cells and bounding square.
TEST(XYArtifactGridTest, PointAndSegmentIntersectionUseCellsAndBoundingSquare)
{
  XYArtifactGrid grid;
  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));

  EXPECT_TRUE(grid.containsPoint(5, 5));
  EXPECT_FALSE(grid.containsPoint(25, 5));
  EXPECT_TRUE(grid.ptIntersectBound(20, 10));
  EXPECT_FALSE(grid.ptIntersectBound(21, 10));
  EXPECT_TRUE(grid.segIntersectBound(-1, 5, 21, 5));
  EXPECT_FALSE(grid.segIntersectBound(-5, -5, -1, -1));
}

// Covers XY artifact grid behavior: process delta uses label clamps new values and ignores old values.
TEST(XYArtifactGridTest, ProcessDeltaUsesLabelClampsNewValuesAndIgnoresOldValues)
{
  XYArtifactGrid grid;
  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));

  ASSERT_TRUE(grid.processDelta("artifact_grid@ 0,99,0.5:1,-7,2 "));
  EXPECT_NEAR(grid.getClearance(0), 0.5, kGeomTol);
  EXPECT_NEAR(grid.getClearance(1), 1.0, kGeomTol);

  EXPECT_FALSE(grid.processDelta(" artifact_grid @0,0,0.1"));
  EXPECT_FALSE(grid.processDelta("wrong_grid@0,0,0.1"));
  EXPECT_FALSE(grid.processDelta("artifact_grid@0,0"));
}

// Covers XY artifact grid behavior: invalid delta is not atomic after earlier cells are applied.
TEST(XYArtifactGridTest, InvalidDeltaIsNotAtomicAfterEarlierCellsAreApplied)
{
  XYArtifactGrid grid;
  ASSERT_TRUE(grid.initialize(kArtifactGridSpec));

  EXPECT_FALSE(grid.processDelta("artifact_grid@0,0,0.75:99,0,0.25"));

  EXPECT_NEAR(grid.getClearance(0), 0.75, kGeomTol);
  EXPECT_NEAR(grid.getClearance(1), 0.0, kGeomTol);
}

// Covers XY field generator behavior: accepts convex polygons from object and spec.
TEST(XYFieldGeneratorTest, AcceptsConvexPolygonsFromObjectAndSpec)
{
  XYFieldGenerator generator;

  EXPECT_TRUE(generator.addPolygon(labeledSquare("one", 0, 0, 10, 10)));
  EXPECT_TRUE(generator.addPolygon("pts={20,0:30,0:30,10:20,10},label=two"));
  EXPECT_EQ(generator.polygonCount(), 2u);
  EXPECT_EQ(generator.size(), 2u);
  EXPECT_EQ(generator.getPolygon(0).get_label(), "one");
  EXPECT_EQ(generator.getPolygon(99).size(), 0u);
}

// Covers XY field generator behavior: rejects non convex polygons and invalid snap.
TEST(XYFieldGeneratorTest, RejectsNonConvexPolygonsAndInvalidSnap)
{
  XYFieldGenerator generator;
  XYPolygon bowtie;
  bowtie.add_vertex(0, 0);
  bowtie.add_vertex(10, 10);
  bowtie.add_vertex(0, 10);
  bowtie.add_vertex(10, 0);
  bowtie.determine_convexity();

  EXPECT_FALSE(generator.addPolygon(bowtie));
  EXPECT_FALSE(generator.addPolygon("pts={0,0:10,10:0,10:10,0},label=bad"));
  EXPECT_FALSE(generator.setSnap(0));
  EXPECT_FALSE(generator.setSnap(-1));
  EXPECT_TRUE(generator.setSnap(0.5));
}

// Covers XY field generator behavior: accepts pick pos abbreviated polygon regions.
TEST(XYFieldGeneratorTest, AcceptsPickPosAbbreviatedPolygonRegions)
{
  XYFieldGenerator generator;

  ASSERT_TRUE(generator.addPolygon("-30,-15:-28,-20:35,5:32,13"));
  ASSERT_EQ(generator.polygonCount(), 1u);

  XYPolygon pavlab = generator.getPolygon(0);
  EXPECT_TRUE(pavlab.is_convex());
  EXPECT_TRUE(pavlab.contains(pavlab.get_center_x(), pavlab.get_center_y()));
  EXPECT_FALSE(pavlab.contains(-40, -20));
}

// Covers XY field generator behavior: manual points respect region and buffer rules.
TEST(XYFieldGeneratorTest, ManualPointsRespectRegionAndBufferRules)
{
  XYFieldGenerator generator;
  ASSERT_TRUE(generator.addPolygon(labeledSquare("field", 0, 0, 20, 20)));
  generator.setBufferDist(10);
  generator.setMaxTries(0);

  EXPECT_TRUE(generator.addPoint(1, 1));
  EXPECT_FALSE(generator.addPoint(100, 100));
  EXPECT_TRUE(generator.addPoint(100, 100, false));
  EXPECT_FALSE(generator.addPoint(2, 2));
  EXPECT_TRUE(generator.addPoint(7, 9));

  ASSERT_EQ(generator.getPoints().size(), 3u);
  EXPECT_EQ(generator.getPoints()[0].get_label(), "P0");
  EXPECT_EQ(generator.getPoints()[1].get_label(), "P1");
  EXPECT_EQ(generator.getPoints()[2].get_label(), "P2");
}

// Covers XY field generator behavior: pick pos style generation honors multiple regions and snap.
TEST(XYFieldGeneratorTest, PickPosStyleGenerationHonorsMultipleRegionsAndSnap)
{
  std::vector<XYPolygon> regions;
  regions.push_back(string2Poly("pts={-30,-30:-30,-135:15,-135:10,-30}"));
  regions.push_back(string2Poly("pts={90,20:150,30:200,-25:160,-65:90,-15}"));
  regions.push_back(string2Poly("pts={145,-120:170,-135:140,-175:125,-160}"));

  XYFieldGenerator generator;
  for(const XYPolygon& region : regions)
    ASSERT_TRUE(generator.addPolygon(region));
  ASSERT_TRUE(generator.setSnap(5));
  generator.setBufferDist(10);
  generator.setMaxTries(5000);
  generator.setFlexBuffer(true);

  ASSERT_TRUE(generator.generatePoints(4));
  std::vector<XYPoint> points = generator.getPoints();
  ASSERT_EQ(points.size(), 4u);
  for(const XYPoint& point : points) {
    EXPECT_TRUE(pointInAnyPolygon(point, regions));
    EXPECT_NEAR(std::round(point.x() / 5.0) * 5.0, point.x(), kGeomTol);
    EXPECT_NEAR(std::round(point.y() / 5.0) * 5.0, point.y(), kGeomTol);
  }
}

// Covers XY field generator behavior: nearest distance cache updates and can be forced.
TEST(XYFieldGeneratorTest, NearestDistanceCacheUpdatesAndCanBeForced)
{
  XYFieldGenerator generator;
  ASSERT_TRUE(generator.addPolygon(labeledSquare("field", 0, 0, 20, 20)));
  ASSERT_TRUE(generator.addPoint(0, 0, false));

  std::vector<double> nearest = generator.getNearestVals();
  ASSERT_EQ(nearest.size(), 1u);
  EXPECT_NEAR(nearest[0], -1.0, kGeomTol);
  EXPECT_NEAR(generator.getGlobalNearest(), -1.0, kGeomTol);

  ASSERT_TRUE(generator.addPoint(3, 4, false));
  nearest = generator.getNearestVals();
  ASSERT_EQ(nearest.size(), 2u);
  EXPECT_NEAR(nearest[0], 5.0, kGeomTol);
  EXPECT_NEAR(nearest[1], 5.0, kGeomTol);
  EXPECT_NEAR(generator.getGlobalNearest(), 5.0, kGeomTol);
}

// Covers XY field generator behavior: generate point falls back to first polygon center when max tries is zero.
TEST(XYFieldGeneratorTest, GeneratePointFallsBackToFirstPolygonCenterWhenMaxTriesIsZero)
{
  XYFieldGenerator generator;
  ASSERT_TRUE(generator.addPolygon(labeledSquare("field", 10, 20, 30, 60)));
  generator.setMaxTries(0);

  XYPoint point = generator.generatePoint();

  // The fallback fills coordinates with set_vx/set_vy, which leaves the
  // default XYPoint validity flag false.
  EXPECT_FALSE(point.valid());
  EXPECT_NEAR(point.x(), 20.0, kGeomTol);
  EXPECT_NEAR(point.y(), 40.0, kGeomTol);
}

// Covers XY field generator behavior: generate point without polygons returns invalid origin point.
TEST(XYFieldGeneratorTest, GeneratePointWithoutPolygonsReturnsInvalidOriginPoint)
{
  XYFieldGenerator generator;

  XYPoint point = generator.generatePoint();

  EXPECT_FALSE(point.valid());
  EXPECT_NEAR(point.x(), 0.0, kGeomTol);
  EXPECT_NEAR(point.y(), 0.0, kGeomTol);

  EXPECT_TRUE(generator.generatePoints(1));
  ASSERT_EQ(generator.getPoints().size(), 1u);
  EXPECT_FALSE(generator.getPoints()[0].valid());
  EXPECT_TRUE(generator.addRandomPoint());
  EXPECT_TRUE(generator.addAllRandomPoints(1));
}

// Covers XY field generator behavior: generate points honors target amount and optional flexible buffer.
TEST(XYFieldGeneratorTest, GeneratePointsHonorsTargetAmountAndOptionalFlexibleBuffer)
{
  XYFieldGenerator generator;
  ASSERT_TRUE(generator.addPolygon(labeledSquare("field", 0, 0, 100, 100)));
  generator.setMaxTries(100);
  generator.setTargAmt(3);
  generator.setFlexBuffer(false);
  generator.setVerbose();

  testing::internal::CaptureStdout();
  EXPECT_TRUE(generator.generatePoints());
  std::string verbose = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(stringContains(verbose, "XYFieldGenerator::generatePoints()"));
  EXPECT_EQ(generator.getPoints().size(), 3u);

  generator.setFlexBuffer(true);
  EXPECT_TRUE(generator.addAllRandomPoints(2));
  EXPECT_EQ(generator.getPoints().size(), 2u);
}

// Covers XY field generator behavior: clear removes points but keeps configured polygons.
TEST(XYFieldGeneratorTest, ClearRemovesPointsButKeepsConfiguredPolygons)
{
  XYFieldGenerator generator;
  ASSERT_TRUE(generator.addPolygon(labeledSquare("field", 0, 0, 10, 10)));
  ASSERT_TRUE(generator.addPoint(1, 1));

  // clear() resets generated points but preserves configured generation regions.
  generator.clear();

  EXPECT_EQ(generator.getPoints().size(), 0u);
  EXPECT_EQ(generator.polygonCount(), 1u);
  EXPECT_FALSE(generator.getNewestPoint().valid());
}
