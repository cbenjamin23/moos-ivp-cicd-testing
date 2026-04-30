#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "GeometryTestUtils.h"
#include "IO_GeomUtils.h"
#include "NumericAssertions.h"
#include "OpField.h"
#include "Populator_OpField.h"
#include "XYArc.h"
#include "XYCircle.h"
#include "XYEncoders.h"
#include "XYGrid.h"
#include "XYHexagon.h"
#include "XYPolygon.h"
#include "XYPoint.h"
#include "TestFileUtils.h"

namespace {

std::string makeTempFile(const std::string& stem, const std::string& contents)
{
  // Keep fixture files alive for the duration of the test process while giving
  // every test a unique path for parallel CTest runs.
  static std::vector<std::unique_ptr<TempFile>> files;
  files.emplace_back(new TempFile(stem, contents));
  return files.back()->path();
}

std::vector<std::string> sortedAliases(const OpField& field)
{
  std::vector<std::string> aliases = field.getPtAliases();
  std::sort(aliases.begin(), aliases.end());
  return aliases;
}

void expectPoint(const XYPoint& point, double x, double y)
{
  ASSERT_TRUE(point.valid());
  EXPECT_NEAR(point.x(), x, kGeomTol);
  EXPECT_NEAR(point.y(), y, kGeomTol);
}

}  // namespace

// Covers io geom utils poly string file behavior: extracts only polygon aliases and keeps raw right hand specs.
TEST(IOGeomUtilsPolyStringFileTest, ExtractsOnlyPolygonAliasesAndKeepsRawRightHandSpecs)
{
  const std::string path = makeTempFile("io_geom_poly_strings.txt",
      "# mission region file\n"
      "\n"
      " polygon = pts={0,0:10,0:10,10:0,10},label=op_region\n"
      "poly=radial:: x=5,y=5,radius=3,pts=8,label=loiter\n"
      "points = 0,0:5,0:5,5:0,5:label,abbr_box\n"
      "gpoly = pts={99,99:100,99:100,100}\n"
      "polygon = pts={1,1:2,1:2,2:1,2},label=with=equals\n"
      "not_polygon = pts={0,0:1,0:1,1}\n");

  std::vector<std::string> specs = readPolyStringsFromFile(path);

  ASSERT_EQ(specs.size(), 4u);
  EXPECT_EQ(specs[0], "pts={0,0:10,0:10,10:0,10},label=op_region");
  EXPECT_EQ(specs[1], "radial:: x=5,y=5,radius=3,pts=8,label=loiter");
  EXPECT_EQ(specs[2], "0,0:5,0:5,5:0,5:label,abbr_box");
  // The reader splits at the first '=' only, so additional '=' characters in
  // the right-hand payload are preserved.
  EXPECT_EQ(specs[3], "pts={1,1:2,1:2,2:1,2},label=with=equals");
}

// Covers io geom utils poly file behavior: parses mission polygons and generated shape configs.
TEST(IOGeomUtilsPolyFileTest, ParsesMissionPolygonsAndGeneratedShapeConfigs)
{
  const std::string path = makeTempFile("io_geom_polys.txt",
      "# comments and blank lines are skipped\n"
      "\n"
      "gpoly = pts={0,0:40,0:40,20:0,20},label=gpoly_region\n"
      "polygon = pts={10,10:20,10:20,20:10,20},label=posted_poly\n"
      "poly = 0,0:5,0:5,5:0,5:label,abbr_poly\n"
      "points = pts={-10,-10:-5,-10:-5,-5:-10,-5},label=points_poly\n"
      "ellipse = x=0,y=0,major=30,minor=10,degs=15,pts=12,snap=0.1,label=ellipse_poly\n"
      "radial = x=25,y=25,radius=10,pts=10,snap=0.1,label=radial_poly\n"
      "polygon = pts={0,0:5,5:0,5:5,0},label=bowtie_rejected\n"
      "poly = missing_points\n"
      "// C++ style comments are not comments for IO_GeomUtils, but this line lacks '='\n");

  std::vector<XYPolygon> polys = readPolysFromFile(path);

  ASSERT_EQ(polys.size(), 6u);
  EXPECT_EQ(polys[0].get_label(), "gpoly_region");
  EXPECT_NEAR(polys[0].area(), 800.0, kGeomTol);
  EXPECT_EQ(polys[1].get_label(), "posted_poly");
  EXPECT_EQ(polys[2].get_label(), "abbr_poly");
  EXPECT_EQ(polys[3].get_label(), "points_poly");
  EXPECT_EQ(polys[4].get_label(), "ellipse_poly");
  EXPECT_EQ(polys[4].size(), 12u);
  EXPECT_EQ(polys[5].get_label(), "radial_poly");
  EXPECT_EQ(polys[5].size(), 10u);
}

// Covers io geom utils poly file behavior: keeps multi equals payloads but drops malformed polygons.
TEST(IOGeomUtilsPolyFileTest, KeepsMultiEqualsPayloadsButDropsMalformedPolygons)
{
  const std::string path = makeTempFile("io_geom_bad_polys.txt",
      "polygon = pts={0,0:10,0:10,10:0,10},label=bad=extra_equals\n"
      "poly = pts={0,0:5,5:0,5:5,0},label=bowtie\n"
      "ellipse = x=0,y=0,major=30,minor=10,pts=0,label=bad_ellipse\n"
      "radial = x=0,y=0,radius=-5,pts=8,label=bad_radial\n");

  std::vector<std::string> strings = readPolyStringsFromFile(path);
  ASSERT_EQ(strings.size(), 2u);
  EXPECT_EQ(strings[0], "pts={0,0:10,0:10,10:0,10},label=bad=extra_equals");
  EXPECT_EQ(strings[1], "pts={0,0:5,5:0,5:5,0},label=bowtie");

  std::vector<XYPolygon> polys = readPolysFromFile(path);
  ASSERT_EQ(polys.size(), 1u);
  EXPECT_EQ(polys[0].get_label(), "bad=extra_equals");
  EXPECT_NEAR(polys[0].area(), 100.0, kGeomTol);
}

// Covers io geom utils shape file behavior: reads circles arcs and hexagons while skipping bad rows.
TEST(IOGeomUtilsShapeFileTest, ReadsCirclesArcsAndHexagonsWhileSkippingBadRows)
{
  const std::string path = makeTempFile("io_geom_shapes.txt",
      "# MOOS-IvP shape configs\n"
      "circle = 10,20,5,inspection_radius\n"
      "circle = 1,2,-3,bad_radius\n"
      "arc = 0,0,10,45,315\n"
      "arc = 0,0,-1,0,90\n"
      "hexagon = 5,6,2\n"
      "hexagon = 1,2,0\n");

  std::vector<XYCircle> circles = readCirclesFromFile(path);
  std::vector<XYArc> arcs = readArcsFromFile(path);
  std::vector<XYHexagon> hexagons = readHexagonsFromFile(path);

  ASSERT_EQ(circles.size(), 1u);
  EXPECT_TRUE(circles[0].valid());
  EXPECT_NEAR(circles[0].getX(), 10.0, kGeomTol);
  EXPECT_NEAR(circles[0].getY(), 20.0, kGeomTol);
  EXPECT_NEAR(circles[0].getRad(), 5.0, kGeomTol);
  EXPECT_EQ(circles[0].get_label(), "inspection_radius");

  ASSERT_EQ(arcs.size(), 1u);
  EXPECT_NEAR(arcs[0].getX(), 0.0, kGeomTol);
  EXPECT_NEAR(arcs[0].getY(), 0.0, kGeomTol);
  EXPECT_NEAR(arcs[0].getRad(), 10.0, kGeomTol);
  EXPECT_NEAR(arcs[0].getLangle(), 45.0, kGeomTol);
  EXPECT_NEAR(arcs[0].getRangle(), 315.0, kGeomTol);

  ASSERT_EQ(hexagons.size(), 1u);
  EXPECT_NEAR(hexagons[0].get_cx(), 5.0, kGeomTol);
  EXPECT_NEAR(hexagons[0].get_cy(), 6.0, kGeomTol);
  EXPECT_NEAR(hexagons[0].get_dist(), 2.0, kGeomTol);
  EXPECT_EQ(hexagons[0].size(), 6u);
}

// Covers io geom utils grid file behavior: reads search grid and pushes default full grid on decode failures.
TEST(IOGeomUtilsGridFileTest, ReadsSearchGridAndPushesDefaultFullGridOnDecodeFailures)
{
  XYGrid original;
  ASSERT_TRUE(original.initialize(
      "pts={0,0:20,0:20,20:0,20},label=survey_grid@10,10@7"));
  original.setVal(0, 2.5);
  original.setVal(3, 9.25);

  const std::string path = makeTempFile("io_geom_grids.txt",
      "# grid examples\n"
      "searchgrid = pts={0,0:20,0:20,20:0,20},label=search_area@10,10@3\n"
      "searchgrid = pts={0,0:20,0:20,20:0,20}@10,10@3\n"
      "fgrid = " + XYGridToString(original) + "\n"
      "fullgrid = " + GridToString(original) + "\n");

  testing::internal::CaptureStdout();
  std::vector<XYGrid> grids = readGridsFromFile(path);
  std::string out = testing::internal::GetCapturedStdout();

  ASSERT_EQ(grids.size(), 3u);
  EXPECT_EQ(grids[0].getLabel(), "search_area");
  EXPECT_EQ(grids[0].size(), 4);
  EXPECT_NEAR(grids[0].getVal(0), 3.0, kGeomTol);

  // Missing labels reject searchgrid lines. For fgrid/fullgrid, decode failures
  // still push default empty XYGrid instances into the result vector.
  EXPECT_EQ(grids[1].getLabel(), "");
  EXPECT_EQ(grids[1].size(), 0);
  EXPECT_EQ(grids[2].getLabel(), "");
  EXPECT_EQ(grids[2].size(), 0);
  EXPECT_TRUE(stringContains(out, "Un-labeled searchgrid specification\n"));
  EXPECT_TRUE(stringContains(out, "Failed StringToXYGrid 1\n"));

  std::string summary = GridToString(original);
  EXPECT_TRUE(stringContains(summary, "GSIZE:4"));
  EXPECT_TRUE(stringContains(summary, "# 0,2.50"));
  EXPECT_TRUE(stringContains(summary, "# 3,9.25"));
}

// Covers io geom utils output behavior: print square writes legacy labels to stdout.
TEST(IOGeomUtilsOutputTest, PrintSquareWritesLegacyLabelsToStdout)
{
  XYSquare square(1, 2, 3, 4);

  testing::internal::CaptureStdout();
  printSquare(square);
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_EQ(out, "xl:1 xh:2 yl:3 yh:4\n");
}

// Covers op field point behavior: adds sets and rejects duplicate or invalid points.
TEST(OpFieldPointTest, AddsSetsAndRejectsDuplicateOrInvalidPoints)
{
  OpField field;

  EXPECT_TRUE(field.addPoint("dock", "x=10,y=20,label=dock"));
  EXPECT_FALSE(field.addPoint("dock", "x=11,y=21"));
  EXPECT_TRUE(field.setPoint("dock", "12,22"));
  EXPECT_FALSE(field.addPoint("bad", "x=missing_y"));

  XYPoint buoy(30, 40, "buoy");
  EXPECT_TRUE(field.addPoint("buoy", buoy));

  EXPECT_EQ(field.size(), 2u);
  EXPECT_EQ(field.ptSize(), 2u);
  EXPECT_TRUE(field.hasKey("dock"));
  EXPECT_TRUE(field.hasKeyPt("buoy"));
  EXPECT_FALSE(field.hasKey("bad"));
  expectPoint(field.getPoint("dock"), 12, 22);
  expectPoint(field.getPoint("buoy"), 30, 40);
  EXPECT_FALSE(field.getPoint("missing").valid());
}

// Covers op field poly behavior: adds sets and rejects duplicate or invalid polygons.
TEST(OpFieldPolyTest, AddsSetsAndRejectsDuplicateOrInvalidPolygons)
{
  OpField field;

  EXPECT_TRUE(field.addPoly("survey",
      "pts={0,0:20,0:20,20:0,20},label=survey_region"));
  EXPECT_FALSE(field.addPoly("survey",
      "pts={0,0:10,0:10,10:0,10},label=duplicate"));
  EXPECT_TRUE(field.setPoly("survey",
      "radial:: x=10,y=10,radius=5,pts=8,snap=0.1,label=station_keep"));
  EXPECT_FALSE(field.addPoly("bad",
      "pts={0,0:10,10:0,10:10,0},label=bowtie"));

  EXPECT_EQ(field.size(), 1u);
  EXPECT_EQ(field.polySize(), 1u);
  EXPECT_TRUE(field.hasKey("survey"));
  EXPECT_TRUE(field.hasKeyPoly("survey"));
  EXPECT_FALSE(field.hasKeyPoly("bad"));
}

// Covers op field config behavior: parses point and color entries and prints debug lines.
TEST(OpFieldConfigTest, ParsesPointAndColorEntriesAndPrintsDebugLines)
{
  OpField field;

  testing::internal::CaptureStdout();
  bool ok = field.config("sw=0,0 # ne=x=100,y=50 # safe_color=red # bad=x=missing_y");
  std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_EQ(field.size(), 2u);
  expectPoint(field.getPoint("sw"), 0, 0);
  expectPoint(field.getPoint("ne"), 100, 50);
  EXPECT_EQ(field.getColor("safe_color"), "red");
  EXPECT_EQ(field.getColor("missing"), "");

  // config() currently emits debug traces for every non-color entry.
  EXPECT_TRUE(stringContains(out, "alias=sw\n"));
  EXPECT_TRUE(stringContains(out, "value=0,0\n"));
  EXPECT_TRUE(stringContains(out, "ok=true\n"));
  EXPECT_TRUE(stringContains(out, "alias=bad\n"));
  EXPECT_TRUE(stringContains(out, "ok=false\n"));
}

// Covers op field merge and simplify behavior: merges distinct fields and rejects conflicting keys.
TEST(OpFieldMergeAndSimplifyTest, MergesDistinctFieldsAndRejectsConflictingKeys)
{
  OpField left;
  ASSERT_TRUE(left.addPoint("sw", "0,0"));
  ASSERT_TRUE(left.addPoly("box", "pts={0,0:10,0:10,10:0,10},label=box"));

  OpField right;
  ASSERT_TRUE(right.addPoint("ne", "10,10"));
  ASSERT_TRUE(right.addPoly("triangle", "pts={20,0:30,0:25,10},label=triangle"));

  EXPECT_TRUE(left.merge(right));
  EXPECT_EQ(left.ptSize(), 2u);
  EXPECT_EQ(left.polySize(), 2u);
  EXPECT_TRUE(left.hasKey("ne"));
  EXPECT_TRUE(left.hasKey("triangle"));

  OpField conflict;
  ASSERT_TRUE(conflict.addPoint("sw", "1,1"));
  EXPECT_FALSE(left.merge(conflict));
  expectPoint(left.getPoint("sw"), 0, 0);
}

// Covers op field merge and simplify behavior: simplify drops duplicate locations by alias length.
TEST(OpFieldMergeAndSimplifyTest, SimplifyDropsDuplicateLocationsByAliasLength)
{
  OpField field;
  ASSERT_TRUE(field.addPoint("AB", "34,9"));
  ASSERT_TRUE(field.addPoint("ABT", "x=34,y=9,label=same_place"));
  ASSERT_TRUE(field.addPoint("LONG", "x=40,y=9"));
  ASSERT_TRUE(field.addPoint("S", "40,9"));
  ASSERT_TRUE(field.addPoint("EQ", "50,50"));
  ASSERT_TRUE(field.addPoint("ZZ", "50,50"));

  field.simplify();
  std::vector<std::string> aliases = sortedAliases(field);
  EXPECT_EQ(aliases, (std::vector<std::string>{"AB", "EQ", "S", "ZZ"}));

  OpField keep_longer;
  ASSERT_TRUE(keep_longer.addPoint("AB", "34,9"));
  ASSERT_TRUE(keep_longer.addPoint("ABT", "34,9"));
  ASSERT_TRUE(keep_longer.addPoint("C", "40,9"));
  ASSERT_TRUE(keep_longer.addPoint("CCC", "40,9"));
  keep_longer.simplify(false);
  // With keep_shorter=false the current nested comparison marks both aliases
  // in each unequal-length duplicate pair for removal.
  EXPECT_TRUE(sortedAliases(keep_longer).empty());
}

// Covers populator op field file behavior: add file requires readable unique files.
TEST(PopulatorOpFieldFileTest, AddFileRequiresReadableUniqueFiles)
{
  Populator_OpField populator;
  const std::string path = makeTempFile("populator_unique.opf", "alpha=0,0\n");

  EXPECT_TRUE(populator.addFileOPF(path));
  EXPECT_FALSE(populator.addFileOPF(path));
  EXPECT_FALSE(populator.addFileOPF(path + ".missing"));
}

// Covers populator op field file behavior: populate reads mission point payloads and skips comments and blanks.
TEST(PopulatorOpFieldFileTest, PopulateReadsMissionPointPayloadsAndSkipsCommentsAndBlanks)
{
  const std::string path = makeTempFile("populator_points.opf",
      "# op field points\n"
      "\n"
      "// line comment is skipped by Populator_OpField\n"
      "dock = x=10,y=20,label=dock,vertex_color=yellow\n"
      "gate=30,40\n"
      "beacon = 5,6,7:label,beacon:vertex_size,3\n");
  Populator_OpField populator;
  populator.setVerbose();
  ASSERT_TRUE(populator.addFileOPF(path));

  EXPECT_TRUE(populator.populate());
  OpField field = populator.getOpField();

  EXPECT_EQ(field.size(), 3u);
  EXPECT_EQ(sortedAliases(field), (std::vector<std::string>{"beacon", "dock", "gate"}));
  expectPoint(field.getPoint("dock"), 10, 20);
  expectPoint(field.getPoint("gate"), 30, 40);
  expectPoint(field.getPoint("beacon"), 5, 6);
}

// Covers populator op field file behavior: populates projfield pav style aliases used by map markers.
TEST(PopulatorOpFieldFileTest, PopulatesProjfieldPavStyleAliasesUsedByMapMarkers)
{
  // ivp/missions/m3_casar/fld_base.opf is generated by projfield with paired
  // aliases like AA and AAT. pMapMarkers loads the file through this populator
  // and then calls OpField::simplify() before posting markers.
  const std::string path = makeTempFile("populator_projfield_pav.opf",
      "# $ projfield --pav --grid=10\n"
      "AA=x=-110.81,y=-58.06\n"
      "AAT=-110.81,-58.06\n"
      "AB=x=-101.77,y=-53.78\n"
      "ABT=-101.77,-53.78\n"
      "BA=x=-106.52,y=-67.1\n"
      "BAT=-106.52,-67.1\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(path));

  ASSERT_TRUE(populator.populate());
  OpField field = populator.getOpField();
  EXPECT_EQ(sortedAliases(field),
            (std::vector<std::string>{"AA", "AAT", "AB", "ABT", "BA", "BAT"}));
  expectPoint(field.getPoint("AA"), -110.81, -58.06);
  expectPoint(field.getPoint("AAT"), -110.81, -58.06);
  expectPoint(field.getPoint("AB"), -101.77, -53.78);
  expectPoint(field.getPoint("BA"), -106.52, -67.1);

  field.simplify();
  EXPECT_EQ(sortedAliases(field), (std::vector<std::string>{"AA", "AB", "BA"}));
  expectPoint(field.getPoint("AA"), -110.81, -58.06);
  EXPECT_FALSE(field.hasKeyPt("AAT"));
  EXPECT_FALSE(field.hasKeyPt("ABT"));
  EXPECT_FALSE(field.hasKeyPt("BAT"));
}

// Covers populator op field file behavior: aggregates map markers config field with loaded opf.
TEST(PopulatorOpFieldFileTest, AggregatesMapMarkersConfigFieldWithLoadedOpf)
{
  OpField configured;
  testing::internal::CaptureStdout();
  ASSERT_TRUE(configured.config("alpha=10,20 # beta_color=yellow"));
  testing::internal::GetCapturedStdout();

  const std::string path = makeTempFile("populator_mapmarkers_merge.opf",
      "beta=30,40\n"
      "gamma=x=50,y=60,label=gamma\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(path));
  ASSERT_TRUE(populator.populate());

  ASSERT_TRUE(configured.merge(populator.getOpField()));
  configured.simplify();

  EXPECT_EQ(sortedAliases(configured),
            (std::vector<std::string>{"alpha", "beta", "gamma"}));
  expectPoint(configured.getPoint("alpha"), 10, 20);
  expectPoint(configured.getPoint("beta"), 30, 40);
  expectPoint(configured.getPoint("gamma"), 50, 60);
  EXPECT_EQ(configured.getColor("beta_color"), "yellow");
}

// Covers populator op field file behavior: populate rejects lines without equals and reports whole line.
TEST(PopulatorOpFieldFileTest, PopulateRejectsLinesWithoutEqualsAndReportsWholeLine)
{
  const std::string path = makeTempFile("populator_no_equals.opf",
      "5,6\n"
      "station=x=1,y=2\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(path));

  testing::internal::CaptureStdout();
  EXPECT_FALSE(populator.populate());
  std::string out = testing::internal::GetCapturedStdout();
  OpField field = populator.getOpField();

  EXPECT_EQ(field.size(), 0u);
  EXPECT_EQ(out, "Populator_OpField Bad Line: 5,6\n");
}

// Covers populator op field file behavior: populate rejects malformed point and reports alias to stdout.
TEST(PopulatorOpFieldFileTest, PopulateRejectsMalformedPointAndReportsAliasToStdout)
{
  const std::string path = makeTempFile("populator_bad_line.opf",
      "good=1,2\n"
      "broken=x=missing_y\n"
      "after=3,4\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(path));

  testing::internal::CaptureStdout();
  EXPECT_FALSE(populator.populate());
  std::string out = testing::internal::GetCapturedStdout();

  OpField field = populator.getOpField();
  EXPECT_EQ(field.size(), 1u);
  expectPoint(field.getPoint("good"), 1, 2);
  EXPECT_FALSE(field.hasKeyPt("after"));
  EXPECT_EQ(out, "Populator_OpField Bad Line: broken\n");
}

// Covers populator op field file behavior: populate rejects duplicate aliases after keeping first point.
TEST(PopulatorOpFieldFileTest, PopulateRejectsDuplicateAliasesAfterKeepingFirstPoint)
{
  const std::string path = makeTempFile("populator_duplicate.opf",
      "dock=1,2\n"
      "dock=3,4\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(path));

  testing::internal::CaptureStdout();
  EXPECT_FALSE(populator.populate());
  std::string out = testing::internal::GetCapturedStdout();

  OpField field = populator.getOpField();
  EXPECT_EQ(field.size(), 1u);
  expectPoint(field.getPoint("dock"), 1, 2);
  EXPECT_EQ(out, "Populator_OpField Bad Line: dock\n");
}

// Covers populator op field file behavior: populate returns false when readable files contain no points.
TEST(PopulatorOpFieldFileTest, PopulateReturnsFalseWhenReadableFilesContainNoPoints)
{
  const std::string path = makeTempFile("populator_empty.opf",
      "# only comments\n"
      "\n"
      "// and C++ style comments\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(path));

  EXPECT_FALSE(populator.populate());
  EXPECT_EQ(populator.getOpField().size(), 0u);
}

// Covers populator op field file behavior: populate aggregates multiple files in order.
TEST(PopulatorOpFieldFileTest, PopulateAggregatesMultipleFilesInOrder)
{
  const std::string first = makeTempFile("populator_first.opf",
      "sw=0,0\n"
      "se=100,0\n");
  const std::string second = makeTempFile("populator_second.opf",
      "nw=0,100\n"
      "ne=100,100\n");
  Populator_OpField populator;
  ASSERT_TRUE(populator.addFileOPF(first));
  ASSERT_TRUE(populator.addFileOPF(second));

  EXPECT_TRUE(populator.populate());
  OpField field = populator.getOpField();

  EXPECT_EQ(field.size(), 4u);
  EXPECT_EQ(sortedAliases(field), (std::vector<std::string>{"ne", "nw", "se", "sw"}));
  expectPoint(field.getPoint("ne"), 100, 100);
  expectPoint(field.getPoint("sw"), 0, 0);
}
