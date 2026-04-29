#include <cmath>
#include <limits>
#include <string>

#include <gtest/gtest.h>

#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYEncoders.h"
#include "XYGrid.h"
#include "XYGridUpdate.h"
#include "XYHexGrid.h"
#include "XYHexagon.h"

namespace {

const char* kRectGridSpec =
    "pts={0,0:20,0:20,10:0,10},label=search_grid@10,10@2.5";

void expectSquareNear(const XYSquare& square,
                      double xlow, double xhigh,
                      double ylow, double yhigh)
{
  EXPECT_TRUE(square.valid());
  EXPECT_NEAR(square.get_min_x(), xlow, kGeomTol);
  EXPECT_NEAR(square.get_max_x(), xhigh, kGeomTol);
  EXPECT_NEAR(square.get_min_y(), ylow, kGeomTol);
  EXPECT_NEAR(square.get_max_y(), yhigh, kGeomTol);
}

void expectHexCenter(XYHexagon& hex, double x, double y, double dist)
{
  EXPECT_NEAR(hex.get_cx(), x, kGeomTol);
  EXPECT_NEAR(hex.get_cy(), y, kGeomTol);
  EXPECT_NEAR(hex.get_dist(), dist, kGeomTol);
}

}  // namespace

TEST(XYGridTest, InitializesFromMoosPolySpecWithColumnMajorCellsAndInitialValues)
{
  XYGrid grid;

  ASSERT_TRUE(grid.initialize(kRectGridSpec));
  EXPECT_EQ(grid.getLabel(), "search_grid");
  EXPECT_EQ(grid.getConfigString(), kRectGridSpec);
  ASSERT_EQ(grid.size(), 2);
  EXPECT_EQ(grid.getPBound().get_label(), "search_grid");
  expectSquareNear(grid.getSBound(), 0, 20, 0, 10);
  expectSquareNear(grid.getElement(0), 0, 10, 0, 10);
  expectSquareNear(grid.getElement(1), 10, 20, 0, 10);
  EXPECT_FALSE(grid.getElement(2).valid());
  EXPECT_NEAR(grid.getVal(0), 2.5, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 2.5, kGeomTol);
  EXPECT_NEAR(grid.getVal(99), 0.0, kGeomTol);
}

TEST(XYGridTest, InitializesMultiRowSearchGridColumnMajorFromNegativeOrigin)
{
  XYGrid grid;

  ASSERT_TRUE(grid.initialize(
      "pts={-10,-5:10,-5:10,15:-10,15},label=survey_grid@10,10@1.25"));
  EXPECT_EQ(grid.getLabel(), "survey_grid");
  ASSERT_EQ(grid.size(), 4);
  expectSquareNear(grid.getElement(0), -10, 0, -5, 5);
  expectSquareNear(grid.getElement(1), -10, 0, 5, 15);
  expectSquareNear(grid.getElement(2), 0, 10, -5, 5);
  expectSquareNear(grid.getElement(3), 0, 10, 5, 15);
  EXPECT_NEAR(grid.getVal(0), 1.25, kGeomTol);
  EXPECT_NEAR(grid.getVal(3), 1.25, kGeomTol);
  EXPECT_NEAR(grid.getMinVal(), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 0.0, kGeomTol);
}

TEST(XYGridTest, RetainsPartialCellsThatIntersectPolygonOutsideBoundingSquare)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(
      "pts={0,0:25,0:25,10:0,10},label=search_grid@10,4@3.5"));

  ASSERT_EQ(grid.size(), 9);
  expectSquareNear(grid.getElement(0), 0, 10, 0, 4);
  expectSquareNear(grid.getElement(1), 0, 10, 4, 8);
  expectSquareNear(grid.getElement(2), 0, 10, 8, 12);
  expectSquareNear(grid.getElement(3), 10, 20, 0, 4);
  expectSquareNear(grid.getElement(8), 20, 30, 8, 12);
  expectSquareNear(grid.getSBound(), 0, 25, 0, 10);

  EXPECT_TRUE(grid.ptIntersect(28, 11));
  EXPECT_FALSE(grid.ptIntersectBound(28, 11));
}

TEST(XYGridTest, RejectsMalformedInitializationAndClearsPreviousGrid)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));
  ASSERT_EQ(grid.size(), 2);

  EXPECT_FALSE(grid.initialize("pts={0,0:10,0:10,10:0,10}@5,5"));
  EXPECT_EQ(grid.size(), 0);
  EXPECT_EQ(grid.getConfigString(), "");
  EXPECT_EQ(grid.getLabel(), "search_grid");

  EXPECT_FALSE(grid.initialize("pts={0,0:10,0:10,10:0,10},label=bad@5"));
  EXPECT_FALSE(grid.initialize("pts={0,0:10,0:10,10:0,10},label=bad@10,-1"));
  EXPECT_FALSE(grid.initialize("pts={0,0:5,0:5,5:0,5},label=bad@10,10"));
  EXPECT_FALSE(grid.initialize("not_a_poly@10,10"));
}

TEST(XYGridTest, DistinguishesCellContainmentFromBoundingBoxAndSegmentBounds)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));

  EXPECT_TRUE(grid.ptIntersect(5, 5));
  EXPECT_TRUE(grid.ptIntersect(10, 5));
  EXPECT_FALSE(grid.ptIntersect(25, 5));
  EXPECT_TRUE(grid.ptIntersectBound(20, 10));
  EXPECT_FALSE(grid.ptIntersectBound(20.01, 10));

  EXPECT_TRUE(grid.segIntersectBound(-5, 5, 25, 5));
  EXPECT_TRUE(grid.segIntersectBound(0, -5, 0, 15));
  EXPECT_FALSE(grid.segIntersectBound(-5, -5, -1, -1));
}

TEST(XYGridTest, HandleSegmentStoresPerCellIntersectionLengths)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));

  grid.handleSegment(-5, 5, 25, 5);
  EXPECT_NEAR(grid.getVal(0), 10.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 10.0, kGeomTol);
  EXPECT_NEAR(grid.getMinVal(), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 10.0, kGeomTol);

  grid.handleSegment(5, -5, 5, 15);
  EXPECT_NEAR(grid.getVal(0), 10.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 10.0, kGeomTol);
}

TEST(XYGridTest, TracksMinMaxValuesEverSeenAndResetFromMinimum)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));

  grid.setVal(0, -3);
  grid.setVal(1, 7);
  EXPECT_NEAR(grid.getMinVal(), -3.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 7.0, kGeomTol);

  grid.resetFromMin();
  EXPECT_NEAR(grid.getVal(0), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 10.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 10.0, kGeomTol);
}

TEST(XYGridTest, UtilityRangeClipsExistingUtilitiesAndRejectsInvertedRange)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));

  grid.setUtil(0, 0.75);
  grid.setUtil(1, 0.25);
  grid.setUtilRange(0.3, 0.6);
  EXPECT_NEAR(grid.getUtil(0), 0.6, kGeomTol);
  EXPECT_NEAR(grid.getUtil(1), 0.3, kGeomTol);
  EXPECT_NEAR(grid.getUtil(99), 0.3, kGeomTol);
  EXPECT_NEAR(grid.getMinUtilPoss(), 0.3, kGeomTol);
  EXPECT_NEAR(grid.getMaxUtilPoss(), 0.6, kGeomTol);

  grid.setUtilRange(2, 1);
  EXPECT_NEAR(grid.getMinUtilPoss(), 0.3, kGeomTol);
  EXPECT_NEAR(grid.getMaxUtilPoss(), 0.6, kGeomTol);
}

TEST(XYGridTest, ProcessDeltaAppliesValueAndUtilityUpdatesWithLegacyFiveFieldFormat)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));
  grid.setUtilRange(0, 1);

  EXPECT_TRUE(grid.processDelta("search_grid@0,2.5,6,0,0.75:1,2.5,4,0,2"));
  EXPECT_NEAR(grid.getVal(0), 6.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 4.0, kGeomTol);
  EXPECT_NEAR(grid.getUtil(0), 0.75, kGeomTol);
  EXPECT_NEAR(grid.getUtil(1), 1.0, kGeomTol);

  EXPECT_FALSE(grid.processDelta("wrong_grid@0,0,1"));
  EXPECT_FALSE(grid.processDelta("search_grid@-1,0,1"));
  EXPECT_FALSE(grid.processDelta("search_grid@0,0"));
}

TEST(XYGridTest, InvalidDeltaIsNotAtomicAfterEarlierCellsAreApplied)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));

  EXPECT_FALSE(grid.processDelta("search_grid@0,2.5,9:99,2.5,8"));
  EXPECT_NEAR(grid.getVal(0), 9.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), 2.5, kGeomTol);
}

TEST(XYGridTest, ProcessDeltaTrimsCellsAndUsesAtofForNonNumericPayloadValues)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));

  EXPECT_TRUE(grid.processDelta("search_grid@ 0,2.5,not_number : 1,2.5,-4 "));
  EXPECT_NEAR(grid.getVal(0), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getVal(1), -4.0, kGeomTol);
  EXPECT_NEAR(grid.getMinVal(), -4.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 0.0, kGeomTol);
}

TEST(XYGridTest, UtilitySetterClipsToRangeButUpdatesValueExtremaFields)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));
  grid.setUtilRange(-1, 1);
  grid.setVal(0, 8);

  grid.setUtil(0, 5);
  EXPECT_NEAR(grid.getUtil(0), 1.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxUtil(), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMaxVal(), 1.0, kGeomTol);

  grid.setUtil(1, -5);
  EXPECT_NEAR(grid.getUtil(1), -1.0, kGeomTol);
  EXPECT_NEAR(grid.getMinUtil(), 0.0, kGeomTol);
  EXPECT_NEAR(grid.getMinVal(), -1.0, kGeomTol);
}

TEST(XYGridTest, EncoderPayloadDocumentsLegacyDecoderCompatibility)
{
  XYGrid grid;
  ASSERT_TRUE(grid.initialize(kRectGridSpec));
  grid.setVal(0, 3);
  grid.setVal(1, 9);

  const std::string encoded = XYGridToString(grid);
  EXPECT_TRUE(stringContains(encoded, "label=search_grid#"));
  EXPECT_TRUE(stringContains(encoded, "size=2#"));
  EXPECT_TRUE(stringContains(encoded, "squares="));

  XYGrid standard_round_trip = StringToXYGrid(encoded);
  EXPECT_EQ(standard_round_trip.size(), 0);

  XYGrid legacy_round_trip = StringToXYGrid(
      "label=search_grid#size=2#min_val=0#max_val=9#sbound=0,20,0,10#"
      "pbound=0,0:20,0:20,10:0,10#squares=0,10,0,10,3@10,20,0,10,9");
  ASSERT_EQ(legacy_round_trip.size(), 2);
  EXPECT_EQ(legacy_round_trip.getLabel(), "search_grid");
  EXPECT_NEAR(legacy_round_trip.getVal(0), 3.0, kGeomTol);
  EXPECT_NEAR(legacy_round_trip.getVal(1), 9.0, kGeomTol);
  expectSquareNear(legacy_round_trip.getSBound(), 0, 20, 0, 10);
}

TEST(XYHexagonTest, InitializesFromNumbersWithExpectedVerticesAndContainment)
{
  XYHexagon hex;

  ASSERT_TRUE(hex.initialize(10, -5, 4));
  EXPECT_EQ(hex.size(), 6u);
  expectHexCenter(hex, 10, -5, 4);
  EXPECT_NEAR(hex.get_cz(), 0.0, kGeomTol);
  EXPECT_NEAR(hex.get_vx(0), 12.0, kGeomTol);
  EXPECT_NEAR(hex.get_vy(0), -1.0, kGeomTol);
  EXPECT_NEAR(hex.get_vx(1), 14.0, kGeomTol);
  EXPECT_NEAR(hex.get_vy(1), -5.0, kGeomTol);
  EXPECT_NEAR(hex.get_vx(2), 12.0, kGeomTol);
  EXPECT_NEAR(hex.get_vy(2), -9.0, kGeomTol);
  EXPECT_NEAR(hex.get_vx(3), 8.0, kGeomTol);
  EXPECT_NEAR(hex.get_vy(3), -9.0, kGeomTol);
  EXPECT_NEAR(hex.get_vx(4), 6.0, kGeomTol);
  EXPECT_NEAR(hex.get_vy(4), -5.0, kGeomTol);
  EXPECT_NEAR(hex.get_vx(5), 8.0, kGeomTol);
  EXPECT_NEAR(hex.get_vy(5), -1.0, kGeomTol);
  EXPECT_TRUE(hex.contains(10, -5));
  EXPECT_FALSE(hex.contains(15, -5));
}

TEST(XYHexagonTest, InitializesFromTrimmedStringAndRejectsBadTokens)
{
  XYHexagon hex;

  ASSERT_TRUE(hex.initialize("  3.5, -2, 6  "));
  EXPECT_EQ(hex.size(), 6u);
  expectHexCenter(hex, 3.5, -2, 6);

  EXPECT_FALSE(hex.initialize("3.5,-2"));
  EXPECT_FALSE(hex.initialize("3.5,-2,not_numeric"));
  EXPECT_FALSE(hex.initialize("3.5,-2,0"));
}

TEST(XYHexagonTest, InvalidStringInitializationLeavesExistingHexUntouched)
{
  XYHexagon hex;
  ASSERT_TRUE(hex.initialize(7, 8, 3));

  EXPECT_FALSE(hex.initialize("bad,-2,3"));
  EXPECT_EQ(hex.size(), 6u);
  expectHexCenter(hex, 7, 8, 3);
}

TEST(XYHexagonTest, InvalidNumericInitializationClearsVerticesButLeavesOldCenterFields)
{
  XYHexagon hex;
  ASSERT_TRUE(hex.initialize(7, 8, 3));

  EXPECT_FALSE(hex.initialize(100, 200, -1));
  EXPECT_EQ(hex.size(), 0u);
  expectHexCenter(hex, 7, 8, 3);
}

TEST(XYHexagonTest, AddNeighborCreatesSixAdjacentHexesWithExpectedCenters)
{
  XYHexagon hex;
  ASSERT_TRUE(hex.initialize(0, 0, 10));

  const double expected_centers[6][2] = {
      {0, 20}, {15, 10}, {15, -10}, {0, -20}, {-15, -10}, {-15, 10}};

  for(unsigned int i = 0; i < 6; ++i) {
    XYHexagon neighbor = hex.addNeighbor(i);
    ASSERT_EQ(neighbor.size(), 6u);
    expectHexCenter(neighbor, expected_centers[i][0], expected_centers[i][1], 10);
    EXPECT_TRUE(hex.intersects(neighbor)) << "neighbor " << i;
    EXPECT_FALSE(neighbor.contains(0, 0)) << "neighbor " << i;
  }
}

TEST(XYHexagonTest, PolygonMutatorsAreDisabledForHexagons)
{
  XYHexagon hex;
  ASSERT_TRUE(hex.initialize(0, 0, 5));

  EXPECT_FALSE(hex.add_vertex(100, 100));
  EXPECT_FALSE(hex.alter_vertex(0, 0));
  EXPECT_FALSE(hex.delete_vertex(0, 0));
  EXPECT_FALSE(hex.insert_vertex(1, 1));
  EXPECT_EQ(hex.size(), 6u);
}

TEST(XYHexGridTest, InitializeAcceptsLabeledPolyButCurrentImplementationCreatesNoElements)
{
  XYHexGrid grid;

  ASSERT_TRUE(grid.initialize("pts={0,0:20,0:20,20:0,20},label=hex_search@10,10"));
  EXPECT_EQ(grid.getLabel(), "hex_search");
  EXPECT_EQ(grid.getConfigString(),
            "pts={0,0:20,0:20,20:0,20},label=hex_search@10,10");
  EXPECT_EQ(grid.size(), 0);
  EXPECT_EQ(grid.getElement(0).size(), 0u);
}

TEST(XYHexGridTest, AcceptsInitialValueFieldButStillCreatesNoElements)
{
  XYHexGrid grid;

  ASSERT_TRUE(grid.initialize(
      "pts={0,0:20,0:20,20:0,20},label=hex_search@10,10@42"));
  EXPECT_EQ(grid.getLabel(), "hex_search");
  EXPECT_EQ(grid.getConfigString(),
            "pts={0,0:20,0:20,20:0,20},label=hex_search@10,10@42");
  EXPECT_EQ(grid.size(), 0);
}

TEST(XYHexGridTest, RejectsInvalidSpecsButClearIsCurrentlyNoOp)
{
  XYHexGrid grid;
  ASSERT_TRUE(grid.initialize("pts={0,0:20,0:20,20:0,20},label=hex_search@10,10"));

  EXPECT_FALSE(grid.initialize("pts={0,0:20,0:20,20:0,20}@10,10"));
  EXPECT_EQ(grid.getLabel(), "hex_search");
  EXPECT_EQ(grid.getConfigString(),
            "pts={0,0:20,0:20,20:0,20},label=hex_search@10,10");

  EXPECT_FALSE(grid.initialize("pts={0,0:20,0:20,20:0,20},label=hex_search@10,-1"));
  EXPECT_FALSE(grid.initialize("not_a_poly@10,10"));
}

TEST(XYGridUpdateTest, ProgrammaticUpdatesRequireNameAndAtLeastOneCell)
{
  XYGridUpdate update;
  update.setGridName("search_grid");
  EXPECT_FALSE(update.valid());
  EXPECT_EQ(update.get_spec(), "");

  ASSERT_TRUE(update.addUpdate(4, "visited", 1));
  EXPECT_TRUE(update.valid());
  EXPECT_TRUE(update.isUpdateTypeDelta());
  EXPECT_EQ(update.get_spec(), "search_grid@4,visited,1");
  EXPECT_EQ(update.getCellIX(99), 0u);
  EXPECT_EQ(update.getCellVar(99), "");
  EXPECT_NEAR(update.getCellVal(99), 0.0, kGeomTol);

  update.setUpdateTypeReplace();
  EXPECT_TRUE(update.isUpdateTypeReplace());
  EXPECT_FALSE(update.isUpdateTypeDelta());
  EXPECT_EQ(update.get_spec(), "search_grid@replace@4,visited,1");

  update.setUpdateTypeAverage();
  EXPECT_TRUE(update.isUpdateTypeAverage());
  EXPECT_FALSE(update.isUpdateTypeReplace());
  EXPECT_EQ(update.get_spec(), "search_grid@avg@4,visited,1");
}

TEST(XYGridUpdateTest, ParsesSearchGridPayloadsAndNormalizesExplicitDelta)
{
  XYGridUpdate update =
      stringToGridUpdate("search_grid@delta@2,visited,1:2,score,12.25");

  ASSERT_TRUE(update.valid());
  EXPECT_EQ(update.getGridName(), "search_grid");
  EXPECT_TRUE(update.isUpdateTypeDelta());
  ASSERT_EQ(update.size(), 2u);
  EXPECT_EQ(update.getCellIX(0), 2u);
  EXPECT_EQ(update.getCellVar(0), "visited");
  EXPECT_NEAR(update.getCellVal(0), 1.0, kGeomTol);
  EXPECT_EQ(update.getCellIX(1), 2u);
  EXPECT_EQ(update.getCellVar(1), "score");
  EXPECT_NEAR(update.getCellVal(1), 12.25, kGeomTol);
  EXPECT_EQ(update.get_spec(), "search_grid@2,visited,1:2,score,12.25");
}

TEST(XYGridUpdateTest, ProgrammaticDefaultVarAndExplicitDeltaNormalizeToSameSpec)
{
  XYGridUpdate programmatic("search_grid");
  ASSERT_TRUE(programmatic.addUpdate(8, "", 33));
  programmatic.setUpdateTypeDelta();
  EXPECT_EQ(programmatic.get_spec(), "search_grid@8,33");

  XYGridUpdate parsed = stringToGridUpdate("search_grid@delta@8,33");
  ASSERT_TRUE(parsed.valid());
  EXPECT_TRUE(parsed.isUpdateTypeDelta());
  EXPECT_EQ(parsed.get_spec(), programmatic.get_spec());
}

TEST(XYGridUpdateTest, ParsesReplaceAverageAndDefaultVariablePayloads)
{
  XYGridUpdate replace = stringToGridUpdate("search_grid@replace@3,score,44");
  ASSERT_TRUE(replace.valid());
  EXPECT_TRUE(replace.isUpdateTypeReplace());
  EXPECT_EQ(replace.get_spec(), "search_grid@replace@3,score,44");

  XYGridUpdate average = stringToGridUpdate("search_grid@avg@5,prob,0.5");
  ASSERT_TRUE(average.valid());
  EXPECT_TRUE(average.isUpdateTypeAverage());
  EXPECT_EQ(average.get_spec(), "search_grid@avg@5,prob,0.5");

  XYGridUpdate default_var = stringToGridUpdate("search_grid@8,33");
  ASSERT_TRUE(default_var.valid());
  EXPECT_EQ(default_var.getCellVar(0), "");
  EXPECT_NEAR(default_var.getCellVal(0), 33.0, kGeomTol);
  EXPECT_EQ(default_var.get_spec(), "search_grid@8,33");
}

TEST(XYGridUpdateTest, RejectsMissingGridBadTypeAndMalformedTuples)
{
  EXPECT_FALSE(stringToGridUpdate("@2,score,1").valid());
  EXPECT_FALSE(stringToGridUpdate("search_grid@bogus@2,score,1").valid());
  EXPECT_FALSE(stringToGridUpdate("search_grid@replace@").valid());
  EXPECT_FALSE(stringToGridUpdate("search_grid@2,score,1,extra").valid());
}

TEST(XYGridUpdateTest, ParserCurrentlyAcceptsNonNumericAndNegativeIndicesViaAtoi)
{
  XYGridUpdate non_numeric = stringToGridUpdate("search_grid@abc,score,notnum");
  ASSERT_TRUE(non_numeric.valid());
  EXPECT_EQ(non_numeric.getCellIX(0), 0u);
  EXPECT_EQ(non_numeric.getCellVar(0), "score");
  EXPECT_NEAR(non_numeric.getCellVal(0), 0.0, kGeomTol);
  EXPECT_EQ(non_numeric.get_spec(), "search_grid@0,score,0");

  XYGridUpdate negative = stringToGridUpdate("search_grid@-1,score,5");
  ASSERT_TRUE(negative.valid());
  EXPECT_EQ(negative.getCellIX(0), std::numeric_limits<unsigned int>::max());
  EXPECT_EQ(negative.get_spec(), "search_grid@4294967295,score,5");
}

TEST(XYGridUpdateTest, EmptyVariableNameRoundTripsAsDefaultVariable)
{
  XYGridUpdate update = stringToGridUpdate("search_grid@1,,5");
  ASSERT_TRUE(update.valid());
  EXPECT_EQ(update.getCellIX(0), 1u);
  EXPECT_EQ(update.getCellVar(0), "");
  EXPECT_NEAR(update.getCellVal(0), 5.0, kGeomTol);
  EXPECT_EQ(update.get_spec(), "search_grid@1,5");
}
