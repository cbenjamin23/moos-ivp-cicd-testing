#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "AngleUtils.h"
#include "ArcUtils.h"
#include "DubinsCache.h"
#include "DubinsTurn.h"
#include "GeomUtils.h"
#include "NumericAssertions.h"
#include "XYSegList.h"

namespace {

constexpr double kPi = 3.14159265358979323846;

struct ArcSpec {
  double ax = 0;
  double ay = 0;
  double ar = 0;
  double langle = 0;
  double rangle = 0;
};

struct RaySpec {
  double x = 0;
  double y = 0;
};

void ExpectNearAngle(double actual, double expected)
{
  EXPECT_NEAR(angleDiff(actual, expected), 0.0, kLooseGeomTol)
      << "actual=" << actual << " expected=" << expected;
}

void ExpectArc(const DubinsTurn& turn, double ax, double ay, double ar,
               double langle, double rangle)
{
  EXPECT_NEAR(turn.getArcCX(), ax, kLooseGeomTol);
  EXPECT_NEAR(turn.getArcCY(), ay, kLooseGeomTol);
  EXPECT_NEAR(turn.getArcRad(), ar, kLooseGeomTol);
  ExpectNearAngle(turn.getArcLangle(), langle);
  ExpectNearAngle(turn.getArcRangle(), rangle);
}

void ExpectArc(const ArcSpec& arc, double ax, double ay, double ar,
               double langle, double rangle)
{
  EXPECT_NEAR(arc.ax, ax, kLooseGeomTol);
  EXPECT_NEAR(arc.ay, ay, kLooseGeomTol);
  EXPECT_NEAR(arc.ar, ar, kLooseGeomTol);
  ExpectNearAngle(arc.langle, langle);
  ExpectNearAngle(arc.rangle, rangle);
}

void ExpectRay(const DubinsTurn& turn, double x, double y)
{
  EXPECT_NEAR(turn.getOSRayX(), x, kLooseGeomTol);
  EXPECT_NEAR(turn.getOSRayY(), y, kLooseGeomTol);
}

ArcSpec GetArcIX(const DubinsCache& cache, unsigned int ix)
{
  ArcSpec arc;
  EXPECT_TRUE(cache.getArcIX(ix, arc.ax, arc.ay, arc.ar, arc.langle, arc.rangle));
  return arc;
}

RaySpec GetRayIX(const DubinsCache& cache, unsigned int ix)
{
  RaySpec ray;
  EXPECT_TRUE(cache.getRayIX(ix, ray.x, ray.y));
  return ray;
}

ArcSpec MakeArcSpec(double ax, double ay, double ar, double langle, double rangle)
{
  ArcSpec arc;
  arc.ax = ax;
  arc.ay = ay;
  arc.ar = ar;
  arc.langle = langle;
  arc.rangle = rangle;
  return arc;
}

void ExpectCacheMatchesDirectTurn(const DubinsCache& cache, double osx, double osy,
                                  double osh, double radius, unsigned int ix)
{
  DubinsTurn direct(osx, osy, osh, radius);
  direct.setTurn(ix);

  ArcSpec arc = GetArcIX(cache, ix);
  EXPECT_NEAR(arc.ax, direct.getArcCX(), kLooseGeomTol);
  EXPECT_NEAR(arc.ay, direct.getArcCY(), kLooseGeomTol);
  EXPECT_NEAR(arc.ar, direct.getArcRad(), kLooseGeomTol);
  ExpectNearAngle(arc.langle, direct.getArcLangle());
  ExpectNearAngle(arc.rangle, direct.getArcRangle());

  RaySpec ray = GetRayIX(cache, ix);
  EXPECT_NEAR(ray.x, direct.getOSRayX(), kLooseGeomTol);
  EXPECT_NEAR(ray.y, direct.getOSRayY(), kLooseGeomTol);
  EXPECT_NEAR(cache.getArcLenIX(ix), direct.getArcLen(), kLooseGeomTol);
  EXPECT_NEAR(cache.getTurnHdgIX(ix), direct.getTurnHdg(), kLooseGeomTol);
  EXPECT_EQ(cache.starTurnIX(ix), direct.starTurn());
  EXPECT_EQ(cache.portTurnIX(ix), direct.portTurn());
}

}  // namespace

TEST(DubinsTurnTest, NoTurnKeepsRayAtOwnshipAndBuildsZeroLengthBeamArc)
{
  DubinsTurn turn(4, -2, 90, 5);
  turn.setTurn(90);

  EXPECT_TRUE(turn.noTurn());
  EXPECT_FALSE(turn.starTurn());
  EXPECT_FALSE(turn.portTurn());
  ExpectArc(turn, 4, 3, 5, 180, 180);
  ExpectRay(turn, 4, -2);
  EXPECT_NEAR(turn.getArcLen(), 0.0, kGeomTol);
  EXPECT_NEAR(turn.getTurnHdg(), 0.0, kGeomTol);
}

TEST(DubinsTurnTest, ComputesStarboardQuarterTurnFromNorthToEast)
{
  DubinsTurn turn(0, 0, 0, 10);
  turn.setTurn(90);

  EXPECT_TRUE(turn.starTurn());
  EXPECT_FALSE(turn.portTurn());
  EXPECT_FALSE(turn.noTurn());
  EXPECT_NEAR(turn.getTurnHdg(), 90.0, kGeomTol);
  ExpectArc(turn, 10, 0, 10, 270, 0);
  ExpectRay(turn, 10, 10);
  EXPECT_NEAR(turn.getArcLen(), 5.0 * kPi, kLooseGeomTol);
  EXPECT_NEAR(distPointOnArc(turn.getOSRayX(), turn.getOSRayY(),
                             turn.getArcCX(), turn.getArcCY(), turn.getArcRad(),
                             turn.getArcLangle(), turn.getArcRangle(), true),
              turn.getArcLen(), kLooseGeomTol);
}

TEST(DubinsTurnTest, ComputesPortQuarterTurnFromNorthToWest)
{
  DubinsTurn turn(0, 0, 0, 10);
  turn.setTurn(270);

  EXPECT_TRUE(turn.portTurn());
  EXPECT_FALSE(turn.starTurn());
  EXPECT_FALSE(turn.noTurn());
  EXPECT_NEAR(turn.getTurnHdg(), 270.0, kGeomTol);
  ExpectArc(turn, -10, 0, 10, 0, 90);
  ExpectRay(turn, -10, 10);
  EXPECT_NEAR(turn.getArcLen(), 5.0 * kPi, kLooseGeomTol);
  EXPECT_NEAR(distPointOnArc(turn.getOSRayX(), turn.getOSRayY(),
                             turn.getArcCX(), turn.getArcCY(), turn.getArcRad(),
                             turn.getArcLangle(), turn.getArcRangle(), false),
              turn.getArcLen(), kLooseGeomTol);
}

TEST(DubinsTurnTest, ChoosesStarboardForOneHundredEightyDegreeTie)
{
  DubinsTurn turn(0, 0, 0, 10);
  turn.setTurn(180);

  EXPECT_TRUE(turn.starTurn());
  EXPECT_FALSE(turn.portTurn());
  ExpectArc(turn, 10, 0, 10, 270, 90);
  ExpectRay(turn, 20, 0);
  EXPECT_NEAR(turn.getArcLen(), kPi * 10.0, kLooseGeomTol);
}

TEST(DubinsTurnTest, UsesMoosHeadingConventionForOffsetOwnshipPose)
{
  const double osx = 100;
  const double osy = -50;
  const double osh = 45;
  const double radius = 20;
  const double desired = 135;

  DubinsTurn turn(osx, osy, osh, radius);
  turn.setTurn(desired);

  double center_x = 0;
  double center_y = 0;
  projectPoint(osh + 90, radius, osx, osy, center_x, center_y);
  double ray_x = 0;
  double ray_y = 0;
  projectPoint(angle360((osh - 90) + angleDiff(osh, desired)), radius,
               center_x, center_y, ray_x, ray_y);

  EXPECT_TRUE(turn.starTurn());
  ExpectArc(turn, center_x, center_y, radius, osh - 90, osh);
  ExpectRay(turn, ray_x, ray_y);
  EXPECT_NEAR(turn.getArcLen(), 0.5 * kPi * radius, kLooseGeomTol);
}

TEST(DubinsTurnTest, NormalizesDesiredHeadingAfterTurnDecision)
{
  DubinsTurn starboard(0, 0, 0, 10);
  starboard.setTurn(450);
  EXPECT_TRUE(starboard.starTurn());
  EXPECT_NEAR(starboard.getTurnHdg(), 90.0, kGeomTol);
  ExpectRay(starboard, 10, 10);

  DubinsTurn port(0, 0, 0, 10);
  port.setTurn(-90);
  EXPECT_TRUE(port.portTurn());
  EXPECT_NEAR(port.getTurnHdg(), 270.0, kGeomTol);
  ExpectRay(port, -10, 10);
}

TEST(DubinsTurnTest, NormalizedNoTurnLeavesStoredDesiredHeadingAtDefault)
{
  DubinsTurn turn(1, 2, 0, 15);
  turn.setTurn(360);

  EXPECT_TRUE(turn.noTurn());
  EXPECT_NEAR(turn.getTurnHdg(), 0.0, kGeomTol);
  ExpectRay(turn, 1, 2);
  ExpectArc(turn, -14, 2, 15, 90, 90);
}

TEST(DubinsTurnTest, ZeroRadiusBuildsDegenerateDirectionalTurn)
{
  DubinsTurn turn(0, 0, 0, 0);
  turn.setTurn(90);

  EXPECT_TRUE(turn.starTurn());
  EXPECT_FALSE(turn.portTurn());
  EXPECT_FALSE(turn.noTurn());
  EXPECT_NEAR(turn.getTurnHdg(), 90.0, kGeomTol);
  ExpectArc(turn, 0, 0, 0, 270, 0);
  ExpectRay(turn, 0, 0);
  EXPECT_NEAR(turn.getArcLen(), 0.0, kGeomTol);
}

TEST(DubinsTurnTest, DirectTurnAllowsNegativeRadiusAndNegativeArcLength)
{
  DubinsTurn turn(0, 0, 0, -10);
  turn.setTurn(90);

  EXPECT_TRUE(turn.starTurn());
  EXPECT_NEAR(turn.getTurnHdg(), 90.0, kGeomTol);
  ExpectArc(turn, -10, 0, -10, 270, 0);
  ExpectRay(turn, -10, -10);
  EXPECT_NEAR(turn.getArcLen(), -5.0 * kPi, kLooseGeomTol);
}

TEST(DubinsTurnTest, UnnormalizedOwnshipHeadingCanProduceNoTurnBeforeDesiredNormalization)
{
  DubinsTurn turn(1, 2, 450, 10);
  turn.setTurn(90);

  EXPECT_TRUE(turn.noTurn());
  EXPECT_NEAR(turn.getTurnHdg(), 0.0, kGeomTol);
  ExpectArc(turn, 1, 12, 10, 180, 180);
  ExpectRay(turn, 1, 2);
  EXPECT_NEAR(turn.getArcLen(), 0.0, kGeomTol);
}

TEST(DubinsTurnWallLookaheadTest, RayEndpointFeedsWallEngineRayCpaUseCase)
{
  DubinsTurn turn(0, 0, 0, 10);
  turn.setTurn(90);

  XYSegList wall;
  wall.add_vertex(30, 5);
  wall.add_vertex(30, 15);

  std::vector<double> cpa;
  std::vector<double> ray_dist;
  EXPECT_EQ(cpasRaySegl(turn.getOSRayX(), turn.getOSRayY(), turn.getTurnHdg(),
                        wall, 1.0, cpa, ray_dist),
            1u);
  ASSERT_EQ(cpa.size(), 1u);
  ASSERT_EQ(ray_dist.size(), 1u);
  EXPECT_NEAR(cpa[0], 0.0, kGeomTol);
  EXPECT_NEAR(ray_dist[0], 20.0, kGeomTol);
}

TEST(DubinsTurnWallLookaheadTest, ArcGeometryFeedsWallEngineArcCpaUseCase)
{
  DubinsTurn turn(0, 0, 0, 10);
  turn.setTurn(90);

  XYSegList wall;
  wall.add_vertex(10, 10);
  wall.add_vertex(10, 20);

  std::vector<double> cpa;
  std::vector<double> arc_dist;
  EXPECT_GE(cpasArcSegl(turn.getArcCX(), turn.getArcCY(), turn.getArcRad(),
                        turn.getArcLangle(), turn.getArcRangle(), wall, 2.0,
                        true, cpa, arc_dist),
            1u);
  ASSERT_EQ(cpa.size(), arc_dist.size());
  EXPECT_NEAR(cpa[0], 0.0, kLooseGeomTol);
  EXPECT_NEAR(arc_dist[0], turn.getArcLen(), kLooseGeomTol);
}

TEST(DubinsCacheValidationTest, StartsInvalidAndRejectsBuildBeforeParameters)
{
  DubinsCache cache;

  EXPECT_FALSE(cache.valid());
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_FALSE(cache.buildCache());
  EXPECT_FALSE(cache.valid());
}

TEST(DubinsCacheValidationTest, RejectsNegativeRadiusWithoutClearingExistingCache)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());
  ASSERT_TRUE(cache.valid());
  ASSERT_EQ(cache.size(), 360u);

  EXPECT_FALSE(cache.setParams(0, 0, 0, -1));
  EXPECT_TRUE(cache.valid());
  EXPECT_EQ(cache.size(), 360u);
}

TEST(DubinsCacheValidationTest, SettingValidParametersClearsExistingCache)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());
  ASSERT_TRUE(cache.valid());

  EXPECT_TRUE(cache.setParams(5, 6, 90, 0));
  EXPECT_FALSE(cache.valid());
  EXPECT_EQ(cache.size(), 0u);
  EXPECT_TRUE(cache.buildCache());
  EXPECT_TRUE(cache.valid());
  EXPECT_EQ(cache.size(), 360u);
  EXPECT_NEAR(cache.getArcLenIX(90), 0.0, kGeomTol);
}

TEST(DubinsCacheValidationTest, RejectsTooCoarseHeadingChoiceWithoutClearingCache)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());

  EXPECT_FALSE(cache.buildCache(11));
  EXPECT_TRUE(cache.valid());
  EXPECT_EQ(cache.size(), 360u);
}

TEST(DubinsCacheQuirkTest, BuildCacheValidatesButDoesNotAdoptHeadingChoiceArgument)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));

  ASSERT_TRUE(cache.buildCache(12));
  EXPECT_EQ(cache.size(), 360u);
  EXPECT_NEAR(cache.getTurnHdgIX(15), 15.0, kGeomTol);
  EXPECT_NEAR(cache.getArcLen(15), cache.getArcLenIX(15), kGeomTol);
}

TEST(DubinsCacheLookupTest, CachedTurnsMatchDirectDubinsTurnGeometry)
{
  const double osx = 2;
  const double osy = -3;
  const double osh = 0;
  const double radius = 12;

  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(osx, osy, osh, radius));
  ASSERT_TRUE(cache.buildCache());

  ExpectCacheMatchesDirectTurn(cache, osx, osy, osh, radius, 0);
  ExpectCacheMatchesDirectTurn(cache, osx, osy, osh, radius, 45);
  ExpectCacheMatchesDirectTurn(cache, osx, osy, osh, radius, 180);
  ExpectCacheMatchesDirectTurn(cache, osx, osy, osh, radius, 270);
  ExpectCacheMatchesDirectTurn(cache, osx, osy, osh, radius, 359);
}

TEST(DubinsCacheLookupTest, HeadingQueriesNormalizeAndFloorToCurrentOneDegreeGrid)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());

  double x = 0;
  double y = 0;
  ASSERT_TRUE(cache.getRay(90.9, x, y));
  RaySpec ix90 = GetRayIX(cache, 90);
  EXPECT_NEAR(x, ix90.x, kLooseGeomTol);
  EXPECT_NEAR(y, ix90.y, kLooseGeomTol);

  ASSERT_TRUE(cache.getRay(-1, x, y));
  RaySpec ix359 = GetRayIX(cache, 359);
  EXPECT_NEAR(x, ix359.x, kLooseGeomTol);
  EXPECT_NEAR(y, ix359.y, kLooseGeomTol);

  EXPECT_NEAR(cache.getArcLen(765.25), cache.getArcLenIX(45), kGeomTol);
}

TEST(DubinsCacheLookupTest, GetArcAndRayNormalizeOutOfRangeHeadings)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());

  ArcSpec negative_heading;
  ASSERT_TRUE(cache.getArc(-90, negative_heading.ax, negative_heading.ay,
                           negative_heading.ar, negative_heading.langle,
                           negative_heading.rangle));
  ExpectArc(negative_heading, -10, 0, 10, 0, 90);

  double ray_x = 0;
  double ray_y = 0;
  ASSERT_TRUE(cache.getRay(450, ray_x, ray_y));
  EXPECT_NEAR(ray_x, 10.0, kLooseGeomTol);
  EXPECT_NEAR(ray_y, 10.0, kLooseGeomTol);
}

TEST(DubinsCacheLookupTest, CacheKeepsUnnormalizedOwnshipHeadingForNoTurnIndex)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 450, 10));
  ASSERT_TRUE(cache.buildCache());

  ArcSpec arc = GetArcIX(cache, 90);
  ExpectArc(arc, 0, 10, 10, 180, 180);
  RaySpec ray = GetRayIX(cache, 90);
  EXPECT_NEAR(ray.x, 0.0, kLooseGeomTol);
  EXPECT_NEAR(ray.y, 0.0, kLooseGeomTol);
  EXPECT_NEAR(cache.getArcLenIX(90), 0.0, kGeomTol);
  EXPECT_NEAR(cache.getTurnHdgIX(90), 0.0, kGeomTol);
  EXPECT_FALSE(cache.starTurnIX(90));
  EXPECT_FALSE(cache.portTurnIX(90));
}

TEST(DubinsCacheInvalidQueryTest, QueriesBeforeBuildFailWithoutLeavingStaleOutputs)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));

  double ax = 1;
  double ay = 2;
  double ar = 3;
  double langle = 4;
  double rangle = 5;
  EXPECT_FALSE(cache.getArc(90, ax, ay, ar, langle, rangle));
  EXPECT_NEAR(ax, 0.0, kGeomTol);
  EXPECT_NEAR(ay, 0.0, kGeomTol);
  EXPECT_NEAR(ar, 0.0, kGeomTol);
  EXPECT_NEAR(langle, 0.0, kGeomTol);
  EXPECT_NEAR(rangle, 0.0, kGeomTol);

  double ray_x = 6;
  double ray_y = 7;
  EXPECT_FALSE(cache.getRay(90, ray_x, ray_y));
  EXPECT_NEAR(ray_x, 0.0, kGeomTol);
  EXPECT_NEAR(ray_y, 0.0, kGeomTol);
  EXPECT_NEAR(cache.getArcLen(90), -1.0, kGeomTol);
}

TEST(DubinsCacheInvalidQueryTest, InvalidIndexQueriesReturnSentinels)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());

  double ax = 1;
  double ay = 2;
  double ar = 3;
  double langle = 4;
  double rangle = 5;
  EXPECT_FALSE(cache.getArcIX(360, ax, ay, ar, langle, rangle));
  EXPECT_NEAR(ax, 0.0, kGeomTol);
  EXPECT_NEAR(ay, 0.0, kGeomTol);
  EXPECT_NEAR(ar, 0.0, kGeomTol);
  EXPECT_NEAR(langle, 0.0, kGeomTol);
  EXPECT_NEAR(rangle, 0.0, kGeomTol);

  double ray_x = 6;
  double ray_y = 7;
  EXPECT_FALSE(cache.getRayIX(999, ray_x, ray_y));
  EXPECT_NEAR(ray_x, 0.0, kGeomTol);
  EXPECT_NEAR(ray_y, 0.0, kGeomTol);
  EXPECT_NEAR(cache.getArcLenIX(360), -1.0, kGeomTol);
  EXPECT_NEAR(cache.getTurnHdgIX(360), -1.0, kGeomTol);
  EXPECT_FALSE(cache.starTurnIX(360));
  EXPECT_FALSE(cache.portTurnIX(360));
}

TEST(DubinsCacheMaxTurnTest, InvalidCacheReturnsMinusOneAndLeavesOutputUntouched)
{
  DubinsCache cache;
  double ax = 1;
  double ay = 2;
  double ar = 3;
  double langle = 4;
  double rangle = 5;

  EXPECT_NEAR(cache.getMaxStarTurn(ax, ay, ar, langle, rangle), -1.0, kGeomTol);
  EXPECT_NEAR(ax, 1.0, kGeomTol);
  EXPECT_NEAR(ay, 2.0, kGeomTol);
  EXPECT_NEAR(ar, 3.0, kGeomTol);
  EXPECT_NEAR(langle, 4.0, kGeomTol);
  EXPECT_NEAR(rangle, 5.0, kGeomTol);

  EXPECT_NEAR(cache.getMaxPortTurn(ax, ay, ar, langle, rangle), -1.0, kGeomTol);
  EXPECT_NEAR(ax, 1.0, kGeomTol);
  EXPECT_NEAR(ay, 2.0, kGeomTol);
  EXPECT_NEAR(ar, 3.0, kGeomTol);
  EXPECT_NEAR(langle, 4.0, kGeomTol);
  EXPECT_NEAR(rangle, 5.0, kGeomTol);
}

TEST(DubinsCacheMaxTurnTest, ReportsCurrentStarboardAndPortBoundaryTurns)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 0, 10));
  ASSERT_TRUE(cache.buildCache());

  double ax = 0;
  double ay = 0;
  double ar = 0;
  double langle = 0;
  double rangle = 0;

  EXPECT_NEAR(cache.getMaxStarTurn(ax, ay, ar, langle, rangle), 180.0, kGeomTol);
  ExpectArc(MakeArcSpec(ax, ay, ar, langle, rangle), 10, 0, 10, 270, 90);

  EXPECT_NEAR(cache.getMaxPortTurn(ax, ay, ar, langle, rangle), 181.0, kGeomTol);
  ExpectArc(MakeArcSpec(ax, ay, ar, langle, rangle), -10, 0, 10, 271, 90);
}

TEST(DubinsCacheMaxTurnTest, MaxPortTurnScanIsAsymmetricForNonZeroOwnshipHeading)
{
  DubinsCache cache;
  ASSERT_TRUE(cache.setParams(0, 0, 45, 10));
  ASSERT_TRUE(cache.buildCache());

  double ax = 0;
  double ay = 0;
  double ar = 0;
  double langle = 0;
  double rangle = 0;

  EXPECT_NEAR(cache.getMaxStarTurn(ax, ay, ar, langle, rangle), 225.0, kGeomTol);
  EXPECT_NEAR(cache.getMaxPortTurn(ax, ay, ar, langle, rangle), 1.0, kGeomTol);
}
