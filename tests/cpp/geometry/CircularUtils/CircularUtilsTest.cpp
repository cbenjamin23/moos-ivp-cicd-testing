#include <gtest/gtest.h>

#include "CircularUtils.h"
#include "NumericAssertions.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

void ExpectArcFlight(double x, double y, double heading, double radius, double dist,
                     bool port, double expected_x, double expected_y,
                     double expected_heading)
{
  double rx = 0;
  double ry = 0;
  double rheading = -1;

  ASSERT_TRUE(arcFlight(x, y, heading, radius, dist, port, rx, ry, rheading));
  EXPECT_NEAR(rx, expected_x, kLooseGeomTol);
  EXPECT_NEAR(ry, expected_y, kLooseGeomTol);
  EXPECT_NEAR(rheading, expected_heading, kLooseGeomTol);
}

}  // namespace

TEST(CircularUtilsTest, ComputesStarboardQuarterTurnFromNorthHeading)
{
  ExpectArcFlight(0, 0, 0, 10, 5 * kPi, false, 10, 10, 90);
}

TEST(CircularUtilsTest, ComputesPortQuarterTurnFromNorthHeading)
{
  ExpectArcFlight(0, 0, 0, 10, 5 * kPi, true, -10, 10, 270);
}

TEST(CircularUtilsTest, ComputesStarboardQuarterTurnFromEastHeading)
{
  ExpectArcFlight(0, 0, 90, 10, 5 * kPi, false, 10, -10, 180);
}

TEST(CircularUtilsTest, ComputesPortQuarterTurnFromEastHeading)
{
  ExpectArcFlight(0, 0, 90, 10, 5 * kPi, true, 10, 10, 0);
}

TEST(CircularUtilsTest, ComputesFixedTurnPreviewFromOffsetOwnshipPose)
{
  // Fixed-turn style behaviors use the current NAV_X/Y/HEADING plus a turn
  // radius/distance to preview where a commanded turn will finish.
  ExpectArcFlight(100, -50, 45, 25, 25 * kPi / 2, false,
                  135.355339059327, -50, 135);
  ExpectArcFlight(100, -50, 45, 25, 25 * kPi / 2, true,
                  100, -14.644660940673, 315);
}

TEST(CircularUtilsTest, LeavesPositionAndHeadingUnchangedForZeroDistance)
{
  ExpectArcFlight(3, -4, 123, 25, 0, false, 3, -4, 123);
  ExpectArcFlight(3, -4, 123, 25, 0, true, 3, -4, 123);
}

TEST(CircularUtilsTest, NormalizesFullCircleDistanceToStartingState)
{
  ExpectArcFlight(0, 0, 37, 10, 20 * kPi, false, 0, 0, 37);
  ExpectArcFlight(0, 0, 37, 10, 20 * kPi, true, 0, 0, 37);
}

TEST(CircularUtilsTest, NormalizesMoreThanFullCircleDistance)
{
  ExpectArcFlight(0, 0, 0, 10, 25 * kPi, false, 10, 10, 90);
  ExpectArcFlight(0, 0, 0, 10, 25 * kPi, true, -10, 10, 270);
}

TEST(CircularUtilsTest, HandlesUnnormalizedHeadingsWithSingleWrapAdjustment)
{
  ExpectArcFlight(0, 0, 450, 10, 5 * kPi, false, 10, -10, 180);
  ExpectArcFlight(0, 0, -90, 10, 5 * kPi, true, -10, -10, 180);
  ExpectArcFlight(3, -4, 450, 25, 0, false, 3, -4, 90);
}

TEST(CircularUtilsTest, ArcOverloadBuildsStarboardTurnPreviewArc)
{
  XYArc arc = arcFlight(0, 0, 0, 10, 5 * kPi, false);

  EXPECT_NEAR(arc.getX(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getRad(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getLangle(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 270.0, kGeomTol);
  EXPECT_NEAR(arc.getAX1(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getAY1(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getAX2(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAY2(), 0.0, kGeomTol);
}

TEST(CircularUtilsTest, ArcOverloadBuildsPortTurnPreviewArc)
{
  XYArc arc = arcFlight(0, 0, 0, 10, 5 * kPi, true);

  EXPECT_NEAR(arc.getX(), -10.0, kGeomTol);
  EXPECT_NEAR(arc.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getRad(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getLangle(), 90.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getAX1(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAY1(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAX2(), -10.0, kGeomTol);
  EXPECT_NEAR(arc.getAY2(), 10.0, kGeomTol);
}

TEST(CircularUtilsTest, ArcOverloadMatchesFixedTurnPreviewGeometry)
{
  XYArc starboard = arcFlight(100, -50, 45, 25, 25 * kPi / 2, false);

  ASSERT_TRUE(starboard.valid());
  EXPECT_NEAR(starboard.getX(), 117.677669529664, kLooseGeomTol);
  EXPECT_NEAR(starboard.getY(), -67.677669529664, kLooseGeomTol);
  EXPECT_NEAR(starboard.getRad(), 25.0, kGeomTol);
  EXPECT_NEAR(starboard.getLangle(), 45.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getRangle(), 315.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.lengthDegrees(), 90.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAX1(), 135.355339059327, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAY1(), -50.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAX2(), 100.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAY2(), -50.0, kLooseGeomTol);

  XYArc port = arcFlight(100, -50, 45, 25, 25 * kPi / 2, true);
  ASSERT_TRUE(port.valid());
  EXPECT_NEAR(port.getX(), 82.322330470336, kLooseGeomTol);
  EXPECT_NEAR(port.getY(), -32.322330470336, kLooseGeomTol);
  EXPECT_NEAR(port.getLangle(), 135.0, kLooseGeomTol);
  EXPECT_NEAR(port.getRangle(), 45.0, kLooseGeomTol);
  EXPECT_NEAR(port.lengthDegrees(), 90.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAX1(), 100.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAY1(), -50.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAX2(), 100.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAY2(), -14.644660940673, kLooseGeomTol);
}

TEST(CircularUtilsTest, ArcOverloadStoresPartiallyNormalizedGeometryForUnnormalizedHeading)
{
  XYArc arc = arcFlight(0, 0, 450, 10, 5 * kPi, false);

  EXPECT_NEAR(arc.getX(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getY(), -10.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getRad(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getLangle(), 90.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getRangle(), 360.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getAX1(), 10.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getAY1(), -10.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getAX2(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(arc.getAY2(), 0.0, kLooseGeomTol);
}

TEST(CircularUtilsTest, ArcOverloadRejectsNegativeInputsWithDefaultArc)
{
  XYArc negative_distance = arcFlight(1, 2, 30, 10, -1, true);
  EXPECT_NEAR(negative_distance.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_distance.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_distance.getRad(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_distance.getLangle(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_distance.getRangle(), 0.0, kGeomTol);

  XYArc negative_radius = arcFlight(1, 2, 30, -10, 1, true);
  EXPECT_NEAR(negative_radius.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_radius.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_radius.getRad(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_radius.getLangle(), 0.0, kGeomTol);
  EXPECT_NEAR(negative_radius.getRangle(), 0.0, kGeomTol);
}

TEST(CircularUtilsTest, RejectsNegativeDistanceOrRadius)
{
  double rx = 99;
  double ry = 88;
  double rheading = 77;

  EXPECT_FALSE(arcFlight(0, 0, 0, 10, -1, true, rx, ry, rheading));
  EXPECT_NEAR(rx, 99.0, kGeomTol);
  EXPECT_NEAR(ry, 88.0, kGeomTol);
  EXPECT_NEAR(rheading, 77.0, kGeomTol);

  EXPECT_FALSE(arcFlight(0, 0, 0, -10, 1, true, rx, ry, rheading));
  EXPECT_NEAR(rx, 99.0, kGeomTol);
  EXPECT_NEAR(ry, 88.0, kGeomTol);
  EXPECT_NEAR(rheading, 77.0, kGeomTol);
}
