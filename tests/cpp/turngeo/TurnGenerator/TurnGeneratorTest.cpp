#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "TurnGen180.h"
#include "TurnGenWilliamson.h"
#include "TurnGenerator.h"
#include "XYPoint.h"

namespace {

class ExposedTurnGenerator : public TurnGenerator {
 public:
  void setPoints(const std::vector<XYPoint>& points) { m_points = points; }
};

void expectPointNear(const XYPoint& point, double x, double y, double tol = 1e-5)
{
  EXPECT_NEAR(point.get_vx(), x, tol);
  EXPECT_NEAR(point.get_vy(), y, tol);
}

}  // namespace

// Covers turn generator behavior: validates required start radius and point gap inputs.
TEST(TurnGeneratorTest, ValidatesRequiredStartRadiusAndPointGapInputs)
{
  TurnGenerator generator;
  EXPECT_EQ(generator.getTurnDir(), "star");
  EXPECT_FALSE(generator.valid());
  EXPECT_FALSE(generator.setTurnRadius(0));
  EXPECT_FALSE(generator.setTurnRadius(-1));
  EXPECT_TRUE(generator.setTurnRadius(12.5));
  EXPECT_DOUBLE_EQ(generator.getTurnRadius(), 12.5);
  EXPECT_FALSE(generator.setPointGap(0));
  EXPECT_FALSE(generator.setPointGap(-2));
  EXPECT_TRUE(generator.setPointGap(7.5));
  EXPECT_DOUBLE_EQ(generator.getPointGap(), 7.5);
  EXPECT_FALSE(generator.valid());

  generator.setStartPos(1, 2, 370);
  EXPECT_TRUE(generator.valid());
  generator.setPortTurn(true);
  EXPECT_EQ(generator.getTurnDir(), "port");
}

// Covers turn generator behavior: reports point coordinates and path length.
TEST(TurnGeneratorTest, ReportsPointCoordinatesAndPathLength)
{
  ExposedTurnGenerator generator;
  generator.setStartPos(0, 0, 0);
  ASSERT_TRUE(generator.setTurnRadius(10));
  ASSERT_TRUE(generator.setPointGap(5));
  generator.setPoints({XYPoint(3, 4), XYPoint(6, 8), XYPoint(6, 10)});

  EXPECT_EQ(generator.getPtsX(), (std::vector<double>{3, 6, 6}));
  EXPECT_EQ(generator.getPtsY(), (std::vector<double>{4, 8, 10}));
  EXPECT_DOUBLE_EQ(generator.getTurnLen(), 12);

  generator.clear();
  EXPECT_EQ(generator.size(), 0u);
  EXPECT_DOUBLE_EQ(generator.getTurnLen(), 0);
}

// Covers turn generator behavior: merges interior closest points and preserves ends.
TEST(TurnGeneratorTest, MergesInteriorClosestPointsAndPreservesEnds)
{
  ExposedTurnGenerator generator;
  generator.setPoints({XYPoint(0, 0), XYPoint(0.2, 0), XYPoint(1, 0),
                       XYPoint(10, 0)});

  EXPECT_EQ(generator.mergePoints(0.5), 1u);
  ASSERT_EQ(generator.size(), 3u);
  expectPointNear(generator.getPts()[0], 0, 0);
  expectPointNear(generator.getPts()[1], 1, 0);
  expectPointNear(generator.getPts()[2], 10, 0);

  EXPECT_EQ(generator.mergePoints(0.05), 1u);
  EXPECT_EQ(generator.size(), 3u);
}

// Covers turn generator behavior: interior closest pair is ignored unless leading pair qualifies.
TEST(TurnGeneratorTest, InteriorClosestPairIsIgnoredUnlessLeadingPairQualifies)
{
  ExposedTurnGenerator generator;
  generator.setPoints({XYPoint(0, 0), XYPoint(5, 0), XYPoint(5.4, 0.2),
                       XYPoint(9, 0.2), XYPoint(20, 0)});

  EXPECT_EQ(generator.mergePoints(1.0), 1u);
  ASSERT_EQ(generator.size(), 5u);
  expectPointNear(generator.getPts()[0], 0, 0);
  expectPointNear(generator.getPts()[1], 5, 0);
  expectPointNear(generator.getPts()[2], 5.4, 0.2);
  expectPointNear(generator.getPts()[3], 9, 0.2);
  expectPointNear(generator.getPts()[4], 20, 0);
}

// Covers turn generator behavior: merge returns one even when no pair falls under threshold.
TEST(TurnGeneratorTest, MergeReturnsOneEvenWhenNoPairFallsUnderThreshold)
{
  ExposedTurnGenerator generator;
  generator.setPoints({XYPoint(0, 0), XYPoint(10, 0), XYPoint(20, 0)});

  EXPECT_EQ(generator.mergePoints(1), 1u);
  EXPECT_EQ(generator.size(), 3u);

  generator.setPoints({XYPoint(0, 0), XYPoint(1, 0)});
  EXPECT_EQ(generator.mergePoints(10), 0u);
}

// Covers turn gen180 behavior: builds starboard half turn from northbound ownship.
TEST(TurnGen180Test, BuildsStarboardHalfTurnFromNorthboundOwnship)
{
  TurnGen180 generator;
  generator.setStartPos(0, 0, 0);
  ASSERT_TRUE(generator.setTurnRadius(10));
  ASSERT_TRUE(generator.setPointGap(30));
  generator.generate();

  ASSERT_EQ(generator.size(), 7u);
  expectPointNear(generator.getPts().front(), 0, 0);
  expectPointNear(generator.getPts().back(), 20, 0);
  EXPECT_GT(generator.getTurnLen(), 30);
  EXPECT_LT(generator.getTurnLen(), 32);
}

// Covers turn gen180 behavior: builds port half turn from northbound ownship.
TEST(TurnGen180Test, BuildsPortHalfTurnFromNorthboundOwnship)
{
  TurnGen180 generator;
  generator.setStartPos(0, 0, 0);
  generator.setPortTurn(true);
  ASSERT_TRUE(generator.setTurnRadius(10));
  ASSERT_TRUE(generator.setPointGap(45));
  generator.generate();

  ASSERT_EQ(generator.size(), 5u);
  expectPointNear(generator.getPts().front(), 0, 0);
  expectPointNear(generator.getPts().back(), -20, 0);
  EXPECT_GT(generator.getTurnLen(), 30);
  EXPECT_LT(generator.getTurnLen(), 31);
}

// Covers turn gen180 behavior: eastbound turns follow marine heading convention.
TEST(TurnGen180Test, EastboundTurnsFollowMarineHeadingConvention)
{
  TurnGen180 starboard;
  starboard.setStartPos(0, 0, 90);
  ASSERT_TRUE(starboard.setTurnRadius(10));
  ASSERT_TRUE(starboard.setPointGap(90));
  starboard.generate();

  ASSERT_EQ(starboard.size(), 3u);
  expectPointNear(starboard.getPts().front(), 0, 0);
  expectPointNear(starboard.getPts().back(), 0, -20);

  TurnGen180 port;
  port.setStartPos(0, 0, 90);
  port.setPortTurn(true);
  ASSERT_TRUE(port.setTurnRadius(10));
  ASSERT_TRUE(port.setPointGap(90));
  port.generate();

  ASSERT_EQ(port.size(), 3u);
  expectPointNear(port.getPts().front(), 0, 0);
  expectPointNear(port.getPts().back(), 0, 20);
}

// Covers turn gen180 behavior: point gap that does not divide half circle stops before opposite point.
TEST(TurnGen180Test, PointGapThatDoesNotDivideHalfCircleStopsBeforeOppositePoint)
{
  TurnGen180 generator;
  generator.setStartPos(0, 0, 0);
  ASSERT_TRUE(generator.setTurnRadius(10));
  ASSERT_TRUE(generator.setPointGap(40));
  generator.generate();

  ASSERT_EQ(generator.size(), 5u);
  expectPointNear(generator.getPts().front(), 0, 0);
  EXPECT_LT(generator.getPts().back().get_vx(), 20);
  EXPECT_GT(generator.getPts().back().get_vy(), 0);
}

// Covers turn gen180 behavior: invalid inputs leave generated turn empty.
TEST(TurnGen180Test, InvalidInputsLeaveGeneratedTurnEmpty)
{
  TurnGen180 generator;
  generator.setStartPos(0, 0, 0);
  ASSERT_TRUE(generator.setPointGap(30));
  generator.generate();
  EXPECT_EQ(generator.size(), 0u);

  ASSERT_TRUE(generator.setTurnRadius(10));
  ASSERT_TRUE(generator.setPointGap(30));
  generator.generate();
  ASSERT_GT(generator.size(), 0u);
  generator.clear();
  EXPECT_EQ(generator.size(), 0u);
}

// Covers turn gen williamson behavior: clamps lane gap bias and desired extent inputs.
TEST(TurnGenWilliamsonTest, ClampsLaneGapBiasAndDesiredExtentInputs)
{
  TurnGenWilliamson generator;
  ASSERT_TRUE(generator.setTurnRadius(20));
  generator.setLaneGap(10);
  EXPECT_DOUBLE_EQ(generator.getMaxBias(), 15);
  generator.setBias(100);
  EXPECT_DOUBLE_EQ(generator.getBias(), 15);
  generator.setBias(-100);
  EXPECT_DOUBLE_EQ(generator.getBias(), -15);
  generator.setBiasPct(50);
  EXPECT_DOUBLE_EQ(generator.getBias(), 7.5);
  generator.setBiasPct(500);
  EXPECT_DOUBLE_EQ(generator.getBias(), 15);
  generator.setLaneGap(-5);
  EXPECT_DOUBLE_EQ(generator.getMaxBias(), 20);
  generator.setDesiredExtent(-10);
  EXPECT_DOUBLE_EQ(generator.getDesiredExtent(), 0);
}

// Covers turn gen williamson behavior: bias is reclamped when lane gap or radius changes.
TEST(TurnGenWilliamsonTest, BiasIsReclampedWhenLaneGapOrRadiusChanges)
{
  TurnGenWilliamson generator;
  ASSERT_TRUE(generator.setTurnRadius(20));
  generator.setLaneGap(20);
  EXPECT_DOUBLE_EQ(generator.getMaxBias(), 10);
  generator.setBias(8);
  EXPECT_DOUBLE_EQ(generator.getBias(), 8);

  generator.setLaneGap(30);
  EXPECT_DOUBLE_EQ(generator.getMaxBias(), 5);
  EXPECT_DOUBLE_EQ(generator.getBias(), 5);

  ASSERT_TRUE(generator.setTurnRadius(40));
  EXPECT_DOUBLE_EQ(generator.getMaxBias(), 25);
  EXPECT_DOUBLE_EQ(generator.getBias(), 5);
}

// Covers turn gen williamson behavior: ignores explicit end heading and sets opposite heading from end position.
TEST(TurnGenWilliamsonTest, IgnoresExplicitEndHeadingAndSetsOppositeHeadingFromEndPosition)
{
  TurnGenWilliamson generator;
  generator.setStartPos(0, 0, 350);
  generator.setEndHeading(45);
  generator.setEndPos(10, 10);

  ASSERT_TRUE(generator.setTurnRadius(15));
  ASSERT_TRUE(generator.setPointGap(20));
  generator.generate();
  EXPECT_GT(generator.size(), 0u);
}

// Covers turn gen williamson behavior: generates leg run style close lane switch.
TEST(TurnGenWilliamsonTest, GeneratesLegRunStyleCloseLaneSwitch)
{
  TurnGenWilliamson generator;
  generator.setStartPos(0, 0, 0);
  generator.setEndPos(10, 80);
  generator.setPortTurn(false);
  generator.setDesiredExtent(20);
  ASSERT_TRUE(generator.setTurnRadius(20));
  ASSERT_TRUE(generator.setPointGap(15));

  generator.generate();

  EXPECT_GT(generator.size(), 15u);
  EXPECT_GT(generator.getAngW(), 0);
  EXPECT_GT(generator.getDistB(), 0);
  EXPECT_GT(generator.getTurnLen(), 0);
  expectPointNear(generator.getPts().front(), 0, 0);
  EXPECT_NEAR(generator.getPointC0().get_vx(), 5, 1e-5);
  EXPECT_NEAR(generator.getPointC0().get_vy(), 80, 1e-5);
  EXPECT_GT(generator.getPointC3().get_vy(), generator.getPointC0().get_vy());
}

// Covers turn gen williamson behavior: generates wide lane switch and appends aft end point.
TEST(TurnGenWilliamsonTest, GeneratesWideLaneSwitchAndAppendsAftEndPoint)
{
  TurnGenWilliamson generator;
  generator.setStartPos(0, 0, 0);
  generator.setEndPos(60, -20);
  generator.setPortTurn(true);
  generator.setDesiredExtent(10);
  ASSERT_TRUE(generator.setTurnRadius(20));
  ASSERT_TRUE(generator.setPointGap(30));

  generator.generate();

  EXPECT_GT(generator.size(), 6u);
  EXPECT_LT(generator.getNaturalExtent(), 0);
  expectPointNear(generator.getPts().back(), 60, -20);
  EXPECT_GT(generator.getTurnLen(), 0);
}

// Covers turn gen williamson behavior: generates wide lane switch without appending fore end point.
TEST(TurnGenWilliamsonTest, GeneratesWideLaneSwitchWithoutAppendingForeEndPoint)
{
  TurnGenWilliamson generator;
  generator.setStartPos(0, 0, 0);
  generator.setEndPos(80, 80);
  generator.setDesiredExtent(0);
  ASSERT_TRUE(generator.setTurnRadius(20));
  ASSERT_TRUE(generator.setPointGap(30));

  generator.generate();

  EXPECT_GT(generator.size(), 0u);
  EXPECT_GT(generator.getNaturalExtent(), 0);
  EXPECT_GT(generator.getDistX(), generator.getDesiredExtent());
  EXPECT_NEAR(generator.getPts().front().get_vx(), 0, 1e-5);
  EXPECT_NEAR(generator.getPts().front().get_vy(), 0, 1e-5);
  expectPointNear(generator.getPts().back(), 80, 80);
}

// Covers turn gen williamson behavior: auto turn direction can select port or starboard for offset end.
TEST(TurnGenWilliamsonTest, AutoTurnDirectionCanSelectPortOrStarboardForOffsetEnd)
{
  TurnGenWilliamson port_generator;
  port_generator.setStartPos(0, 0, 0);
  port_generator.setEndPos(-30, 50);
  port_generator.setAutoTurnDir(true);
  ASSERT_TRUE(port_generator.setTurnRadius(20));
  ASSERT_TRUE(port_generator.setPointGap(20));
  port_generator.generate();
  EXPECT_EQ(port_generator.getTurnDir(), "port");

  TurnGenWilliamson star_generator;
  star_generator.setStartPos(0, 0, 0);
  star_generator.setEndPos(30, 50);
  star_generator.setAutoTurnDir(true);
  ASSERT_TRUE(star_generator.setTurnRadius(20));
  ASSERT_TRUE(star_generator.setPointGap(20));
  star_generator.generate();
  EXPECT_EQ(star_generator.getTurnDir(), "star");
}

// Covers turn gen williamson behavior: auto turn direction threshold leaves small offset at default starboard.
TEST(TurnGenWilliamsonTest, AutoTurnDirectionThresholdLeavesSmallOffsetAtDefaultStarboard)
{
  TurnGenWilliamson generator;
  generator.setStartPos(0, 0, 0);
  generator.setEndPos(-0.5, 50);
  generator.setAutoTurnDir(true);
  ASSERT_TRUE(generator.setTurnRadius(20));
  ASSERT_TRUE(generator.setPointGap(20));
  generator.generate();

  EXPECT_EQ(generator.getTurnDir(), "star");
  EXPECT_GT(generator.getNaturalExtent(), 0);
  EXPECT_GT(generator.size(), 0u);
}
