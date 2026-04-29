#include <gtest/gtest.h>

#include "GeometryTestUtils.h"
#include "LoiterEngine.h"
#include "NumericAssertions.h"
#include "XYFormatUtilsPoly.h"
#include "XYPolygon.h"

namespace {

XYPolygon loiterSquare()
{
  XYPolygon poly = string2Poly("pts={0,0:20,0:20,20:0,20},label=loiter_box");
  EXPECT_TRUE(poly.is_convex());
  return poly;
}

}  // namespace

TEST(LoiterEngineTest, InitializesAndMutatesLoiterPolygonState)
{
  LoiterEngine engine;
  EXPECT_TRUE(engine.getClockwise());
  EXPECT_DOUBLE_EQ(engine.getSpiralFactor(), -2);

  engine.setPoly(loiterSquare());
  EXPECT_EQ(engine.getPolyPts(), 4u);
  EXPECT_NEAR(engine.getCenterX(), 10.0, kGeomTol);
  EXPECT_NEAR(engine.getCenterY(), 10.0, kGeomTol);

  engine.setCenter(30, -5);
  EXPECT_NEAR(engine.getCenterX(), 30.0, kGeomTol);
  EXPECT_NEAR(engine.getCenterY(), -5.0, kGeomTol);
}

TEST(LoiterEngineTest, SetClockwiseReorientsPolygonTraversal)
{
  LoiterEngine engine;
  engine.setPoly(loiterSquare());

  engine.setClockwise(true);
  EXPECT_TRUE(engine.getClockwise());
  EXPECT_TRUE(engine.getPolygon().is_clockwise());

  engine.setClockwise(false);
  EXPECT_FALSE(engine.getClockwise());
  EXPECT_FALSE(engine.getPolygon().is_clockwise());
}

TEST(LoiterEngineTest, AcquireVertexRejectsInvalidPolygon)
{
  LoiterEngine engine;

  EXPECT_EQ(engine.acquireVertex(90, 0, 0), -1);
}

TEST(LoiterEngineTest, AcquiresInsideVertexNearestCurrentHeading)
{
  LoiterEngine engine;
  engine.setPoly(loiterSquare());

  EXPECT_EQ(engine.acquireVertex(90, 10, 10), 1);
  EXPECT_EQ(engine.acquireVertex(270, 10, 10), 0);
}

TEST(LoiterEngineTest, OutsideAcquisitionDependsOnTraversalDirection)
{
  LoiterEngine engine;
  engine.setPoly(loiterSquare());

  engine.setClockwise(true);
  int clockwise_entry = engine.acquireVertex(0, 10, -20);
  engine.setClockwise(false);
  int counter_entry = engine.acquireVertex(0, 10, -20);

  EXPECT_GE(clockwise_entry, 0);
  EXPECT_GE(counter_entry, 0);
  EXPECT_LT(clockwise_entry, static_cast<int>(engine.getPolyPts()));
  EXPECT_LT(counter_entry, static_cast<int>(engine.getPolyPts()));
  EXPECT_NE(clockwise_entry, counter_entry);
}

TEST(LoiterEngineTest, ResetClockwiseBestKeepsClockwiseForSouthApproachCurrentBehavior)
{
  LoiterEngine engine;
  engine.setPoly(loiterSquare());
  engine.setClockwise(true);

  engine.resetClockwiseBest(270, 10, -20);
  EXPECT_TRUE(engine.getClockwise());
}
