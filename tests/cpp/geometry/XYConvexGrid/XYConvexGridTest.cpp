#include <gtest/gtest.h>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYConvexGrid.h"
#include "XYFormatUtilsConvexGrid.h"
#include "XYGridUpdate.h"

namespace {

void expectSquareNear(const XYSquare& square,
                      double xlow,
                      double xhigh,
                      double ylow,
                      double yhigh)
{
  ASSERT_TRUE(square.valid());
  EXPECT_NEAR(square.get_min_x(), xlow, kGeomTol);
  EXPECT_NEAR(square.get_max_x(), xhigh, kGeomTol);
  EXPECT_NEAR(square.get_min_y(), ylow, kGeomTol);
  EXPECT_NEAR(square.get_max_y(), yhigh, kGeomTol);
}

}  // namespace

TEST(XYConvexGridTest, InitializesSearchGridFromConvexPolygonAndCellVars)
{
  XYConvexGrid grid;
  std::vector<std::string> vars;
  vars.push_back("visited");
  vars.push_back("score");
  std::vector<double> init_vals;
  init_vals.push_back(0);
  init_vals.push_back(5);

  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, vars, init_vals));
  EXPECT_EQ(grid.size(), 4u);
  EXPECT_EQ(grid.getCellVarCnt(), 2u);
  EXPECT_TRUE(grid.hasCellVar("visited"));
  EXPECT_TRUE(grid.hasCellVar("score"));
  EXPECT_NEAR(grid.getVal(0, grid.getCellVarIX("visited")), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(0, grid.getCellVarIX("score")), 5.0, kGeomTol);
  EXPECT_EQ(grid.getVar(grid.getCellVarIX("visited")), "visited");
  EXPECT_EQ(grid.getVar(grid.getCellVarIX("score")), "score");
  EXPECT_NEAR(grid.getInitVal(grid.getCellVarIX("score")), 5.0, kGeomTol);
  EXPECT_NEAR(grid.getCellSize(), 10.0, kGeomTol);
}

TEST(XYConvexGridTest, RetainsPartialCellsOutsidePolygonBoundingSquare)
{
  XYConvexGrid grid;

  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 25, 10), 10, 3.5));
  ASSERT_EQ(grid.size(), 3u);
  expectSquareNear(grid.getElement(0), 0, 10, 0, 10);
  expectSquareNear(grid.getElement(1), 10, 20, 0, 10);
  expectSquareNear(grid.getElement(2), 20, 30, 0, 10);
  expectSquareNear(grid.getSBound(), 0, 25, 0, 10);

  EXPECT_TRUE(grid.ptIntersect(28, 5));
  EXPECT_FALSE(grid.ptIntersectBound(28, 5));
  EXPECT_NEAR(grid.getVal(2), 3.5, kGeomTol);
}

TEST(XYConvexGridTest, InitializesWithDefaultSingleCellVar)
{
  XYConvexGrid grid;

  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, 2.5));
  EXPECT_EQ(grid.size(), 4u);
  EXPECT_TRUE(grid.hasCellVar(""));
  EXPECT_TRUE(grid.hasCellVar("v"));
  EXPECT_EQ(grid.getVar(), "v");
  EXPECT_NEAR(grid.getVal(0), 2.5, kGeomTol);
}

TEST(XYConvexGridTest, RejectsInvalidInitializationInputs)
{
  XYConvexGrid empty_poly_grid;
  XYPolygon empty_poly;
  EXPECT_FALSE(empty_poly_grid.initialize(empty_poly, 10, 0));

  XYConvexGrid mismatched_vars_grid;
  std::vector<std::string> vars;
  vars.push_back("visited");
  vars.push_back("score");
  std::vector<double> init_vals;
  init_vals.push_back(0);
  EXPECT_FALSE(mismatched_vars_grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, vars, init_vals));

  XYConvexGrid oversized_cell_grid;
  EXPECT_FALSE(oversized_cell_grid.initialize(makeSquarePoly(0, 0, 5, 5), 10, 0));
}

TEST(XYConvexGridTest, ReinitializingExistingObjectKeepsPriorCellValueRows)
{
  XYConvexGrid grid;
  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, 1));
  ASSERT_EQ(grid.size(), 4u);
  grid.setVal(0, 7);

  ASSERT_TRUE(grid.initialize(makeSquarePoly(100, 100, 110, 110), 10, 2));
  ASSERT_EQ(grid.size(), 1u);
  expectSquareNear(grid.getElement(0), 100, 110, 100, 110);
  EXPECT_NEAR(grid.getInitVal(), 2.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(0), 7.0, kGeomTol);
}

TEST(XYConvexGridTest, HandlesPointAndSegmentIntersectionsAgainstCellsAndBounds)
{
  XYConvexGrid grid;

  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, 0));
  EXPECT_TRUE(grid.ptIntersect(5, 5));
  EXPECT_FALSE(grid.ptIntersect(25, 5));
  EXPECT_TRUE(grid.ptIntersectBound(20, 20));
  EXPECT_FALSE(grid.ptIntersectBound(21, 20));

  EXPECT_TRUE(grid.ptIntersect(0, 5, 5));
  EXPECT_TRUE(grid.ptIntersect(1, 5, 15));
  EXPECT_TRUE(grid.ptIntersect(2, 15, 5));
  EXPECT_FALSE(grid.ptIntersect(99, 5, 5));

  EXPECT_TRUE(grid.segIntersectBound(-5, 5, 25, 5));
  EXPECT_FALSE(grid.segIntersectBound(-5, -5, -1, -1));
  EXPECT_NEAR(grid.segIntersect(0, -5, 5, 25, 5), 10.0, kGeomTol);
}

TEST(XYConvexGridTest, AppliesCellLimitsForPSearchGridStyleScores)
{
  XYConvexGrid grid;
  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, 0));
  grid.setMinLimit(0);
  grid.setMaxLimit(100);

  grid.setVal(0, 125);
  EXPECT_NEAR(grid.getVal(0), 100.0, kGeomTol);
  grid.incVal(0, -25);
  EXPECT_NEAR(grid.getVal(0), 75.0, kGeomTol);
  grid.incVal(99, 10);
  EXPECT_NEAR(grid.getVal(0), 75.0, kGeomTol);
  grid.setVal(0, -5);
  EXPECT_NEAR(grid.getVal(0), 0.0, kGeomTol);
}

TEST(XYConvexGridTest, ClipsContradictoryMinMaxLimitsToCurrentImplementation)
{
  XYConvexGrid grid;
  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, 0));

  grid.setMinLimit(10);
  grid.setMaxLimit(5);

  EXPECT_TRUE(grid.cellVarMinLimited());
  EXPECT_TRUE(grid.cellVarMaxLimited());
  EXPECT_NEAR(grid.getMinLimit(), 10.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxLimit(), 10.0, kGeomTol);

  grid.setVal(0, 100);
  EXPECT_NEAR(grid.getVal(0), 10.0, kGeomTol);
}

TEST(XYConvexGridTest, ParsedCellEntriesUseSetValLimitsAndTrackMinMax)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=score:5,cell_min=score:0,cell_max=score:50,"
      "cell=1:score:80,cell=2:score:-4,label=search_grid");

  ASSERT_EQ(grid.size(), 4u);
  const unsigned int score_ix = grid.getCellVarIX("score");
  EXPECT_NEAR(grid.getVal(1, score_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(2, score_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMin(score_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMax(score_ix), 50.0, kGeomTol);
}

TEST(XYConvexGridTest, ParsesViewGridSpecWithCellUpdatesAndLimits)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,cell_min=score:0,cell_max=score:100,"
      "cell=1:visited:1:score:80,label=search_grid");

  ASSERT_EQ(grid.size(), 4u);
  EXPECT_EQ(grid.get_label(), "search_grid");
  EXPECT_TRUE(grid.hasCellVar("visited"));
  EXPECT_TRUE(grid.hasCellVar("score"));
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("visited")), 1.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("score")), 80.0, kGeomTol);
  EXPECT_TRUE(grid.cellVarMinLimited(grid.getCellVarIX("score")));
  EXPECT_TRUE(grid.cellVarMaxLimited(grid.getCellVarIX("score")));
}

TEST(XYConvexGridTest, ParsesDocumentedPSearchGridViewGridPublication)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={-50,-40:-10,0:90,0:90,-90:50,-150:-50,-90},"
      "cell_size=5,cell_vars=x:0:y:0:z:0,cell_min=x:0,"
      "cell_max=x:50,cell=211:x:50,cell=212:x:50,"
      "cell=237:x:50,cell=238:x:50,label=psg");

  ASSERT_TRUE(grid.valid());
  EXPECT_EQ(grid.get_label(), "psg");
  EXPECT_EQ(grid.size(), 679u);
  EXPECT_NEAR(grid.getCellSize(), 5.0, kGeomTol);
  EXPECT_TRUE(grid.hasCellVar("x"));
  EXPECT_TRUE(grid.hasCellVar("y"));
  EXPECT_TRUE(grid.hasCellVar("z"));

  const unsigned int x_ix = grid.getCellVarIX("x");
  const unsigned int y_ix = grid.getCellVarIX("y");
  const unsigned int z_ix = grid.getCellVarIX("z");
  EXPECT_TRUE(grid.cellVarMinLimited(x_ix));
  EXPECT_TRUE(grid.cellVarMaxLimited(x_ix));
  EXPECT_NEAR(grid.getMinLimit(x_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxLimit(x_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(211, x_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(212, x_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(237, x_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(238, x_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(211, y_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(211, z_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMin(x_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getMax(x_ix), 50.0, kGeomTol);
  expectSquareNear(grid.getSBound(), -50, 90, -150, 0);
}

TEST(XYConvexGridTest, ParsesPSearchGridMoosConfigFoldedIntoViewGridPayload)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={-50,-40:-10,0:180,0:180,-150:-50,-150},"
      "cell_size=5,cell_vars=x:0:y:0:z:0,cell_min=x:0,"
      "cell_max=x:10,cell_min=y:0,cell_max=y:1000,label=psg");

  ASSERT_TRUE(grid.valid());
  EXPECT_EQ(grid.get_label(), "psg");
  EXPECT_GT(grid.size(), 1000u);
  EXPECT_NEAR(grid.getCellSize(), 5.0, kGeomTol);
  expectSquareNear(grid.getSBound(), -50, 180, -150, 0);

  const unsigned int x_ix = grid.getCellVarIX("x");
  const unsigned int y_ix = grid.getCellVarIX("y");
  const unsigned int z_ix = grid.getCellVarIX("z");
  EXPECT_TRUE(grid.cellVarMinLimited(x_ix));
  EXPECT_TRUE(grid.cellVarMaxLimited(x_ix));
  EXPECT_TRUE(grid.cellVarMinLimited(y_ix));
  EXPECT_TRUE(grid.cellVarMaxLimited(y_ix));
  EXPECT_FALSE(grid.cellVarMinLimited(z_ix));
  EXPECT_FALSE(grid.cellVarMaxLimited(z_ix));
  EXPECT_NEAR(grid.getMinLimit(x_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxLimit(x_ix), 10.0, kGeomTol);
  EXPECT_NEAR(grid.getMinLimit(y_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxLimit(y_ix), 1000.0, kGeomTol);

  std::string config = grid.getConfigStr();
  EXPECT_TRUE(stringContains(config, "cell_vars=x:0:y:0:z:0"));
  EXPECT_TRUE(stringContains(config, "cell_min=x:0"));
  EXPECT_TRUE(stringContains(config, "cell_max=x:10"));
  EXPECT_TRUE(stringContains(config, "cell_min=y:0"));
  EXPECT_TRUE(stringContains(config, "cell_max=y:1000"));
}

TEST(XYConvexGridTest, AppliesPSearchGridViewGridDeltaMessages)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={-50,-40:-10,0:180,0:180,-150:-50,-150},"
      "cell_size=5,cell_vars=x:0:y:0,label=psg");

  ASSERT_TRUE(grid.valid());
  ASSERT_TRUE(grid.processDelta("psg@211,x,3:212,x,2:211,y,7"));
  EXPECT_NEAR(grid.getVal(211, grid.getCellVarIX("x")), 3.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(212, grid.getCellVarIX("x")), 2.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(211, grid.getCellVarIX("y")), 7.0, kGeomTol);

  ASSERT_TRUE(grid.processDelta("psg@211,x,4"));
  EXPECT_NEAR(grid.getVal(211, grid.getCellVarIX("x")), 7.0, kGeomTol);

  ASSERT_TRUE(grid.processDelta("psg@replace@212,x,9:212,y,11"));
  EXPECT_NEAR(grid.getVal(212, grid.getCellVarIX("x")), 9.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(212, grid.getCellVarIX("y")), 11.0, kGeomTol);
}

TEST(XYConvexGridTest, RejectsMalformedViewGridSpecs)
{
  EXPECT_EQ(string2ConvexGrid("cell_size=10,cell_vars=visited:0").size(), 0u);
  EXPECT_EQ(string2ConvexGrid("pts={0,0:10,10:0,10:10,0},cell_size=10").size(), 0u);
  EXPECT_EQ(string2ConvexGrid("pts={0,0:20,0:20,20:0,20},cell_size=bad").size(), 0u);
}

TEST(XYConvexGridTest, ProcessesNamedGridDeltaUpdates)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,label=search_grid");

  ASSERT_TRUE(grid.processDelta("search_grid@2,visited,1:2,score,12"));
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("visited")), 1.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("score")), 17.0, kGeomTol);

  ASSERT_TRUE(grid.processDelta("search_grid@replace@2,visited,0:2,score,12"));
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("visited")), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("score")), 12.0, kGeomTol);

  EXPECT_FALSE(grid.processDelta("wrong_grid@2,visited,0"));
}

TEST(XYConvexGridTest, DefaultVariableDeltaAppliesToSingleCellVarViewGrid)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,init_val=5,label=single");

  ASSERT_EQ(grid.size(), 4u);
  ASSERT_TRUE(grid.processDelta("single@1,3"));
  EXPECT_NEAR(grid.getVal(1), 8.0, kGeomTol);

  ASSERT_TRUE(grid.processDelta("single@replace@1,-2"));
  EXPECT_NEAR(grid.getVal(1), -2.0, kGeomTol);
}

TEST(XYConvexGridTest, ProcessDeltaBypassesLimitsAndMinMaxSofarTracking)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=score:5,cell_min=score:0,cell_max=score:10,"
      "label=search_grid");

  ASSERT_EQ(grid.size(), 4u);
  const unsigned int score_ix = grid.getCellVarIX("score");
  ASSERT_TRUE(grid.processDelta("search_grid@replace@1,score,50"));
  EXPECT_NEAR(grid.getVal(1, score_ix), 50.0, kGeomTol);
  EXPECT_NEAR(grid.getMin(score_ix), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMax(score_ix), 0.0, kGeomTol);
}

TEST(XYConvexGridTest, RejectsInvalidDeltaAtomically)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,label=search_grid");

  EXPECT_FALSE(grid.processDelta("search_grid@1,visited,1:99,score,8"));
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("visited")), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("score")), 5.0, kGeomTol);

  EXPECT_FALSE(grid.processDelta("search_grid@1,unknown,1"));
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("visited")), 0.0, kGeomTol);
}

TEST(XYConvexGridTest, AcceptsAverageUpdatePayloadButLeavesValuesUnchanged)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,label=search_grid");

  ASSERT_TRUE(grid.processDelta("search_grid@avg@1,score,12"));
  EXPECT_NEAR(grid.getVal(1, grid.getCellVarIX("score")), 5.0, kGeomTol);
}

TEST(XYConvexGridTest, ResetsAllCellsOrOneCellVariableToInitialValues)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,label=search_grid");

  ASSERT_TRUE(grid.processDelta("search_grid@replace@2,visited,1:2,score,12"));
  grid.reset("visited");
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("visited")), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("score")), 12.0, kGeomTol);

  grid.reset();
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("visited")), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(2, grid.getCellVarIX("score")), 5.0, kGeomTol);
}

TEST(XYConvexGridTest, ResetRestoresValuesButLeavesHistoricalMinMaxSofar)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,label=search_grid");

  const unsigned int score_ix = grid.getCellVarIX("score");
  grid.setVal(0, 20, score_ix);
  grid.setVal(1, -3, score_ix);
  EXPECT_NEAR(grid.getMin(score_ix), -3.0, kGeomTol);
  EXPECT_NEAR(grid.getMax(score_ix), 20.0, kGeomTol);

  grid.reset();
  EXPECT_NEAR(grid.getVal(0, score_ix), 5.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1, score_ix), 5.0, kGeomTol);
  EXPECT_NEAR(grid.getMin(score_ix), -3.0, kGeomTol);
  EXPECT_NEAR(grid.getMax(score_ix), 20.0, kGeomTol);

  grid.setVal(2, 7, score_ix);
  EXPECT_NEAR(grid.getMin(score_ix), -3.0, kGeomTol);
  EXPECT_NEAR(grid.getMax(score_ix), 20.0, kGeomTol);
}

TEST(XYConvexGridTest, EdgeCacheScalesCellCornersAndNoOpsUnchangedScale)
{
  XYConvexGrid grid;
  ASSERT_TRUE(grid.initialize(makeSquarePoly(0, 0, 20, 20), 10, 0));

  EXPECT_TRUE(grid.setEdgeCache(2, -3));
  EXPECT_FALSE(grid.setEdgeCache(2, -3));
  EXPECT_NEAR(grid.getPixPerMtrX(), 2.0, kGeomTol);
  EXPECT_NEAR(grid.getPixPerMtrY(), -3.0, kGeomTol);

  std::vector<std::vector<double> > cache = grid.getEdgeCache();
  ASSERT_EQ(cache.size(), 4u);
  ASSERT_EQ(cache[0].size(), 8u);
  EXPECT_NEAR(cache[0][0], 0.0, kGeomTol);
  EXPECT_NEAR(cache[0][1], 0.0, kGeomTol);
  EXPECT_NEAR(cache[0][2], 20.0, kGeomTol);
  EXPECT_NEAR(cache[0][3], 0.0, kGeomTol);
  EXPECT_NEAR(cache[0][4], 20.0, kGeomTol);
  EXPECT_NEAR(cache[0][5], -30.0, kGeomTol);
  EXPECT_NEAR(cache[0][6], 0.0, kGeomTol);
  EXPECT_NEAR(cache[0][7], -30.0, kGeomTol);
}

TEST(XYConvexGridTest, SerializesConfigAndChangedCellsAsViewGridPayload)
{
  XYConvexGrid grid = string2ConvexGrid(
      "pts={0,0:20,0:20,20:0,20},cell_size=10,"
      "cell_vars=visited:0:score:5,cell_min=score:0,cell_max=score:100,"
      "label=search_grid,edge_color=white");

  ASSERT_TRUE(grid.processDelta("search_grid@replace@1,visited,1:1,score,80"));
  std::string spec = grid.get_spec();
  std::string config = grid.getConfigStr();

  EXPECT_TRUE(stringContains(config, "pts={"));
  EXPECT_TRUE(stringContains(config, "cell_size=10"));
  EXPECT_TRUE(stringContains(config, "cell_vars=visited:0:score:5"));
  EXPECT_TRUE(stringContains(spec, "pts={"));
  EXPECT_TRUE(stringContains(spec, "cell_size=10"));
  EXPECT_TRUE(stringContains(spec, "cell_vars=visited:0:score:5"));
  EXPECT_TRUE(stringContains(spec, "cell_min=score:0"));
  EXPECT_TRUE(stringContains(spec, "cell_max=score:100"));
  EXPECT_TRUE(stringContains(spec, "cell=1:visited:1:score:80"));
  EXPECT_TRUE(stringContains(spec, "label=search_grid"));
  EXPECT_TRUE(stringContains(spec, "edge_color=white"));

  XYConvexGrid round_trip = string2ConvexGrid(spec);
  ASSERT_EQ(round_trip.size(), 4u);
  EXPECT_EQ(round_trip.get_label(), "search_grid");
  EXPECT_NEAR(round_trip.getVal(1, round_trip.getCellVarIX("visited")), 1.0, kGeomTol);
  EXPECT_NEAR(round_trip.getVal(1, round_trip.getCellVarIX("score")), 80.0, kGeomTol);

  testing::internal::CaptureStdout();
  grid.print();
  std::string out = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(stringContains(out, "[0]:"));
}

TEST(XYGridUpdateTest, ParsesAndSerializesGridUpdatePayloads)
{
  XYGridUpdate delta = stringToGridUpdate("search_grid@2,visited,1:2,score,12");
  ASSERT_TRUE(delta.valid());
  EXPECT_EQ(delta.getGridName(), "search_grid");
  EXPECT_TRUE(delta.isUpdateTypeDelta());
  ASSERT_EQ(delta.size(), 2u);
  EXPECT_EQ(delta.getCellIX(0), 2u);
  EXPECT_EQ(delta.getCellVar(0), "visited");
  EXPECT_NEAR(delta.getCellVal(1), 12.0, kGeomTol);
  EXPECT_EQ(delta.get_spec(), "search_grid@2,visited,1:2,score,12");

  XYGridUpdate replace = stringToGridUpdate("search_grid@replace@3,score,44");
  ASSERT_TRUE(replace.valid());
  EXPECT_TRUE(replace.isUpdateTypeReplace());
  EXPECT_EQ(replace.get_spec(), "search_grid@replace@3,score,44");

  EXPECT_FALSE(stringToGridUpdate("@2,score,1").valid());
  EXPECT_FALSE(stringToGridUpdate("search_grid@bogus@2,score,1").valid());
}
