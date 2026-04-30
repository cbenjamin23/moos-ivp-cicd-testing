#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "CircularUtils.h"
#include "GeometryTestUtils.h"
#include "NumericAssertions.h"
#include "XYArc.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

}  // namespace

// Covers XY arc behavior: initializes io geom utils arc format.
TEST(XYArcTest, InitializesIoGeomUtilsArcFormat)
{
  XYArc arc;
  ASSERT_TRUE(arc.initialize("0, 0, 10, 90, 0"));

  EXPECT_TRUE(arc.valid());
  EXPECT_NEAR(arc.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getRad(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getLangle(), 90.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAX1(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getAY1(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAX2(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAY2(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.lengthDegrees(), 90.0, kGeomTol);
  EXPECT_NEAR(arc.lengthUnits(), 5.0 * kPi, kLooseGeomTol);
}

// Covers XY arc behavior: rejects malformed and negative radius initialization.
TEST(XYArcTest, RejectsMalformedAndNegativeRadiusInitialization)
{
  XYArc arc(1, 2, 3, 90, 0);

  EXPECT_FALSE(arc.initialize("0,0,10,90"));
  EXPECT_FALSE(arc.initialize("0,0,10,90,0,extra"));
  EXPECT_FALSE(arc.initialize("0,0,not-a-radius,90,0"));
  EXPECT_FALSE(arc.initialize("0,0,-10,90,0"));

  EXPECT_NEAR(arc.getX(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getY(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getRad(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getLangle(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 0.0, kGeomTol);
}

// Covers XY arc behavior: normalizes constructor and angle setter inputs.
TEST(XYArcTest, NormalizesConstructorAndAngleSetterInputs)
{
  XYArc arc(0, 0, 10, 450, -90);

  EXPECT_NEAR(arc.getLangle(), 90.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 270.0, kGeomTol);
  EXPECT_NEAR(arc.lengthDegrees(), 180.0, kGeomTol);

  arc.setLangle(-45);
  arc.setRangle(405);
  EXPECT_NEAR(arc.getLangle(), 315.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 45.0, kGeomTol);
  EXPECT_NEAR(arc.getAX1(), -10.0 * std::sqrt(2.0) / 2.0, kGeomTol);
  EXPECT_NEAR(arc.getAY1(), 10.0 * std::sqrt(2.0) / 2.0, kGeomTol);
  EXPECT_NEAR(arc.getAX2(), 10.0 * std::sqrt(2.0) / 2.0, kGeomTol);
  EXPECT_NEAR(arc.getAY2(), 10.0 * std::sqrt(2.0) / 2.0, kGeomTol);
}

// Covers XY arc behavior: contains angles across normal and wrapped ranges.
TEST(XYArcTest, ContainsAnglesAcrossNormalAndWrappedRanges)
{
  XYArc northeast(0, 0, 10, 90, 0);
  EXPECT_TRUE(northeast.containsAngle(0));
  EXPECT_TRUE(northeast.containsAngle(45));
  EXPECT_TRUE(northeast.containsAngle(90));
  EXPECT_FALSE(northeast.containsAngle(180));
  EXPECT_FALSE(northeast.containsAngle(315));

  XYArc north_wrap(0, 0, 10, 45, 315);
  EXPECT_TRUE(north_wrap.containsAngle(315));
  EXPECT_TRUE(north_wrap.containsAngle(0));
  EXPECT_TRUE(north_wrap.containsAngle(45));
  EXPECT_FALSE(north_wrap.containsAngle(180));
}

// Covers XY arc behavior: contains angle uses stored range without normalizing query.
TEST(XYArcTest, ContainsAngleUsesStoredRangeWithoutNormalizingQuery)
{
  XYArc northeast(0, 0, 10, 90, 0);
  EXPECT_FALSE(northeast.containsAngle(450));
  EXPECT_FALSE(northeast.containsAngle(-270));

  XYArc north_wrap(0, 0, 10, 45, 315);
  EXPECT_TRUE(north_wrap.containsAngle(360));
  EXPECT_TRUE(north_wrap.containsAngle(720));
  EXPECT_TRUE(north_wrap.containsAngle(-10));
}

// Covers XY arc behavior: contains points using MOOS heading convention.
TEST(XYArcTest, ContainsPointsUsingMoosHeadingConvention)
{
  XYArc northeast(0, 0, 10, 90, 0);

  EXPECT_TRUE(northeast.containsPoint(0, 10));
  EXPECT_TRUE(northeast.containsPoint(10, 0));
  EXPECT_TRUE(northeast.containsPoint(5, 5));
  EXPECT_FALSE(northeast.containsPoint(-5, 5));
  EXPECT_FALSE(northeast.containsPoint(20, 0));
}

// Covers XY arc behavior: computes point distance to arc or nearest endpoint.
TEST(XYArcTest, ComputesPointDistanceToArcOrNearestEndpoint)
{
  XYArc northeast(0, 0, 10, 90, 0);

  EXPECT_NEAR(northeast.ptDistToArc(0, 0), 10.0, kGeomTol);
  EXPECT_NEAR(northeast.ptDistToArc(20, 0), 10.0, kGeomTol);
  EXPECT_NEAR(northeast.ptDistToArc(5, 5), 10.0 - std::sqrt(50.0), kGeomTol);
  EXPECT_NEAR(northeast.ptDistToArc(-10, 0), std::sqrt(200.0), kGeomTol);
}

// Covers XY arc behavior: reports only segment intersection points on the arc.
TEST(XYArcTest, ReportsOnlySegmentIntersectionPointsOnTheArc)
{
  XYArc northeast(0, 0, 10, 90, 0);
  double rx1 = 0;
  double ry1 = 0;
  double rx2 = 0;
  double ry2 = 0;

  EXPECT_EQ(northeast.segIntersectPts(-20, 0, 20, 0, rx1, ry1, rx2, ry2), 1);
  EXPECT_NEAR(rx1, 10.0, kGeomTol);
  EXPECT_NEAR(ry1, 0.0, kGeomTol);

  EXPECT_EQ(northeast.segIntersectPts(0, -20, 0, 20, rx1, ry1, rx2, ry2), 1);
  EXPECT_NEAR(rx1, 0.0, kGeomTol);
  EXPECT_NEAR(ry1, 10.0, kGeomTol);

  EXPECT_EQ(northeast.segIntersectPts(-20, -20, -10, -10, rx1, ry1, rx2, ry2), 0);
}

// Covers XY arc behavior: strict segment intersection requires crossing arc perimeter.
TEST(XYArcTest, StrictSegmentIntersectionRequiresCrossingArcPerimeter)
{
  XYArc northeast(0, 0, 10, 90, 0);

  EXPECT_TRUE(northeast.segIntersectStrict(-20, 0, 20, 0));
  EXPECT_TRUE(northeast.segIntersectStrict(0, -20, 0, 20));
  EXPECT_FALSE(northeast.segIntersectStrict(-20, 0, -5, 0));
  EXPECT_FALSE(northeast.segIntersectStrict(1, 1, 2, 2));
}

// Covers XY arc behavior: reports endpoint tangent as strict intersection.
TEST(XYArcTest, ReportsEndpointTangentAsStrictIntersection)
{
  XYArc northeast(0, 0, 10, 90, 0);
  double rx1 = 0;
  double ry1 = 0;
  double rx2 = 0;
  double ry2 = 0;

  EXPECT_TRUE(northeast.segIntersectStrict(10, -5, 10, 5));
  EXPECT_EQ(northeast.segIntersectPts(10, -5, 10, 5, rx1, ry1, rx2, ry2), 2);
  EXPECT_NEAR(rx1, 10.0, kGeomTol);
  EXPECT_NEAR(ry1, 0.0, kGeomTol);
  EXPECT_NEAR(rx2, 10.0, kGeomTol);
  EXPECT_NEAR(ry2, 0.0, kGeomTol);
}

// Covers XY arc behavior: serializes current arc string format.
TEST(XYArcTest, SerializesCurrentArcStringFormat)
{
  XYArc arc(1, 2, 3, 90, 0);
  std::string spec = arc.toString();

  EXPECT_TRUE(stringContains(spec, "arc:1"));
  EXPECT_TRUE(stringContains(spec, ",2"));
  EXPECT_TRUE(stringContains(spec, ",3"));
  EXPECT_TRUE(stringContains(spec, ",90"));
  EXPECT_TRUE(stringContains(spec, ",0"));
}

// Covers XY arc behavior: set overloads expose cached endpoint difference.
TEST(XYArcTest, SetOverloadsExposeCachedEndpointDifference)
{
  XYArc arc(0, 0, 10, 90, 0);

  arc.set(10, 10, 5, 180, 90);
  EXPECT_NEAR(arc.getX(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getY(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getRad(), 5.0, kGeomTol);
  EXPECT_NEAR(arc.getLangle(), 180.0, kGeomTol);
  EXPECT_NEAR(arc.getRangle(), 90.0, kGeomTol);
  EXPECT_NEAR(arc.getAX1(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getAY1(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAX2(), 0.0, kGeomTol);
  EXPECT_NEAR(arc.getAY2(), 10.0, kGeomTol);

  arc.set(10, 10, 5, 180, 90, 10, 5, 15, 10);
  EXPECT_NEAR(arc.getAX1(), 10.0, kGeomTol);
  EXPECT_NEAR(arc.getAY1(), 5.0, kGeomTol);
  EXPECT_NEAR(arc.getAX2(), 15.0, kGeomTol);
  EXPECT_NEAR(arc.getAY2(), 10.0, kGeomTol);
}

// Covers XY arc behavior: arc flight builds starboard and port turn geometry.
TEST(XYArcTest, ArcFlightBuildsStarboardAndPortTurnGeometry)
{
  XYArc starboard = arcFlight(0, 0, 0, 10, 5 * kPi, false);

  EXPECT_TRUE(starboard.valid());
  EXPECT_NEAR(starboard.getX(), 10.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getY(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getRad(), 10.0, kGeomTol);
  EXPECT_NEAR(starboard.getLangle(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getRangle(), 270.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAX1(), 10.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAY1(), 10.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAX2(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.getAY2(), 0.0, kLooseGeomTol);

  XYArc port = arcFlight(0, 0, 0, 10, 5 * kPi, true);
  EXPECT_TRUE(port.valid());
  EXPECT_NEAR(port.getX(), -10.0, kLooseGeomTol);
  EXPECT_NEAR(port.getY(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(port.getLangle(), 90.0, kLooseGeomTol);
  EXPECT_NEAR(port.getRangle(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAX1(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAY1(), 0.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAX2(), -10.0, kLooseGeomTol);
  EXPECT_NEAR(port.getAY2(), 10.0, kLooseGeomTol);
}

// Covers XY arc behavior: arc flight preview arc contains only swept turn sector.
TEST(XYArcTest, ArcFlightPreviewArcContainsOnlySweptTurnSector)
{
  // A fixed-turn preview from an offset ownship pose should contain the
  // swept start/end points and reject points on the rest of the circle.
  XYArc starboard = arcFlight(100, -50, 45, 25, 25 * kPi / 2, false);

  ASSERT_TRUE(starboard.valid());
  EXPECT_TRUE(starboard.containsPoint(100, -50));
  EXPECT_TRUE(starboard.containsPoint(135.355339059327, -50));
  EXPECT_TRUE(starboard.containsPoint(117.677669529664, -42.677669529664));
  EXPECT_FALSE(starboard.containsPoint(117.677669529664, -92.677669529664));
  EXPECT_NEAR(starboard.ptDistToArc(117.677669529664, -67.677669529664),
              25.0, kLooseGeomTol);
  EXPECT_NEAR(starboard.ptDistToArc(117.677669529664, -42.677669529664),
              0.0, kLooseGeomTol);
}

// Covers XY arc behavior: arc flight rejects negative inputs with default zero arc.
TEST(XYArcTest, ArcFlightRejectsNegativeInputsWithDefaultZeroArc)
{
  XYArc negative_radius = arcFlight(0, 0, 0, -10, 5, true);
  EXPECT_TRUE(negative_radius.valid());
  EXPECT_NEAR(negative_radius.getRad(), 0.0, kGeomTol);

  XYArc negative_distance = arcFlight(0, 0, 0, 10, -5, true);
  EXPECT_TRUE(negative_distance.valid());
  EXPECT_NEAR(negative_distance.getRad(), 0.0, kGeomTol);
}
