#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>
#include <vector>

#include "LinearExtrapolator.h"
#include "NumericAssertions.h"
#include "ProxPoint.h"
#include "WrapDetector.h"
#include "XYSegment.h"

namespace {

class InspectableWrapDetector : public WrapDetector {
public:
  std::size_t legCount() const { return m_os_legs.size(); }
  bool empty() const { return m_empty; }
  double osx() const { return m_osx; }
  double osy() const { return m_osy; }

  XYSegment legAt(std::size_t ix) const
  {
    auto it = m_os_legs.begin();
    std::advance(it, ix);
    return *it;
  }
};

bool failureContains(const LinearExtrapolator& extrapolator,
                     const std::string& needle)
{
  return extrapolator.getFailureReason().find(needle) != std::string::npos;
}

ProxPoint makeProxPoint(double cpa, double cpa_dist, int type)
{
  ProxPoint point;
  point.setCPA(cpa);
  point.setCPADist(cpa_dist);
  point.setType(type);
  return point;
}

void feedSelfCrossingLegRun(InspectableWrapDetector& detector)
{
  detector.updatePosition(0, 0);
  detector.updatePosition(10, 0);
  detector.updatePosition(10, 10);
  detector.updatePosition(0, 10);
  detector.updatePosition(5, -5);
}

}  // namespace

TEST(LinearExtrapolatorStateTest, FailsBeforeOwnshipOrContactPositionIsKnown)
{
  LinearExtrapolator extrapolator;
  double x = 123;
  double y = 456;

  EXPECT_FALSE(extrapolator.getPosition(x, y, 10));
  EXPECT_NEAR(x, 0.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_TRUE(failureContains(extrapolator, "unknown contact position"));
  EXPECT_FALSE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorMotionTest, ProjectsNominalContactMotionUsingMoosHeadingConvention)
{
  LinearExtrapolator extrapolator;
  extrapolator.setPosition(10, 20, 2, 90, 100);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 115));

  EXPECT_NEAR(x, 40.0, kGeomTol);
  EXPECT_NEAR(y, 20.0, kGeomTol);
  EXPECT_EQ(extrapolator.getFailureReason(), "");
  EXPECT_FALSE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorMotionTest, HandlesCardinalHeadingsAndHeadingNormalization)
{
  LinearExtrapolator north;
  north.setPosition(1, 2, 3, 0, 0);
  double x = 0;
  double y = 0;
  EXPECT_TRUE(north.getPosition(x, y, 4));
  EXPECT_NEAR(x, 1.0, kGeomTol);
  EXPECT_NEAR(y, 14.0, kGeomTol);

  LinearExtrapolator west;
  west.setPosition(1, 2, 3, 270, 0);
  EXPECT_TRUE(west.getPosition(x, y, 4));
  EXPECT_NEAR(x, -11.0, kGeomTol);
  EXPECT_NEAR(y, 2.0, kGeomTol);

  LinearExtrapolator wrapped;
  wrapped.setPosition(1, 2, 3, 450, 0);
  EXPECT_TRUE(wrapped.getPosition(x, y, 4));
  EXPECT_NEAR(x, 13.0, kGeomTol);
  EXPECT_NEAR(y, 2.0, kGeomTol);
}

TEST(LinearExtrapolatorMotionTest, AllowsNegativeSpeedAsReverseMotionQuirk)
{
  LinearExtrapolator extrapolator;
  extrapolator.setPosition(4, 5, -2, 0, 10);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 13));

  EXPECT_NEAR(x, 4.0, kGeomTol);
  EXPECT_NEAR(y, -1.0, kGeomTol);
}

TEST(LinearExtrapolatorTimeTest, RejectsClearlyNegativeDeltaTimeAndReportsClockSkew)
{
  LinearExtrapolator extrapolator;
  extrapolator.setPosition(10, 10, 5, 90, 100);

  double x = -1;
  double y = -1;
  EXPECT_FALSE(extrapolator.getPosition(x, y, 99.8));

  EXPECT_NEAR(x, 10.0, kGeomTol);
  EXPECT_NEAR(y, 10.0, kGeomTol);
  EXPECT_TRUE(failureContains(extrapolator, "negative delta time"));
  EXPECT_TRUE(failureContains(extrapolator, "delta_time=-0.20000"));
}

TEST(LinearExtrapolatorTimeTest, AllowsTinyNegativeDeltaTimeAsBackwardProjectionQuirk)
{
  LinearExtrapolator extrapolator;
  extrapolator.setPosition(10, 10, 5, 90, 100);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 99.95));

  EXPECT_NEAR(x, 9.75, kGeomTol);
  EXPECT_NEAR(y, 10.0, kGeomTol);
  EXPECT_EQ(extrapolator.getFailureReason(), "");
}

TEST(LinearExtrapolatorTimeTest, ReturnsCurrentPositionAtExactTimestamp)
{
  LinearExtrapolator extrapolator;
  extrapolator.setPosition(-3, 7, 9, 180, 55);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 55));

  EXPECT_NEAR(x, -3.0, kGeomTol);
  EXPECT_NEAR(y, 7.0, kGeomTol);
}

TEST(LinearExtrapolatorDecayTest, AppliesPiecewiseLinearDecayWindow)
{
  LinearExtrapolator extrapolator;
  ASSERT_TRUE(extrapolator.setDecay(5, 15));
  extrapolator.setPosition(0, 0, 4, 90, 100);

  double x = 0;
  double y = 0;

  EXPECT_TRUE(extrapolator.getPosition(x, y, 105));
  EXPECT_NEAR(x, 20.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_FALSE(extrapolator.isDecayMaxed());

  EXPECT_TRUE(extrapolator.getPosition(x, y, 110));
  EXPECT_NEAR(x, 35.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_FALSE(extrapolator.isDecayMaxed());

  EXPECT_TRUE(extrapolator.getPosition(x, y, 115));
  EXPECT_NEAR(x, 40.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_FALSE(extrapolator.isDecayMaxed());

  EXPECT_TRUE(extrapolator.getPosition(x, y, 120));
  EXPECT_NEAR(x, 40.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_TRUE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorDecayTest, UsesEqualBeginAndEndDecayAsHardClip)
{
  LinearExtrapolator extrapolator;
  ASSERT_TRUE(extrapolator.setDecay(5, 5));
  extrapolator.setPosition(0, 0, 2, 0, 10);

  double x = 0;
  double y = 0;

  EXPECT_TRUE(extrapolator.getPosition(x, y, 15));
  EXPECT_NEAR(x, 0.0, kGeomTol);
  EXPECT_NEAR(y, 10.0, kGeomTol);
  EXPECT_FALSE(extrapolator.isDecayMaxed());

  EXPECT_TRUE(extrapolator.getPosition(x, y, 15.1));
  EXPECT_NEAR(x, 0.0, kGeomTol);
  EXPECT_NEAR(y, 10.0, kGeomTol);
  EXPECT_TRUE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorDecayTest, RejectsInvalidDecaySettingsWithoutReplacingLastValidPolicy)
{
  LinearExtrapolator extrapolator;
  ASSERT_TRUE(extrapolator.setDecay(2, 4));
  EXPECT_FALSE(extrapolator.setDecay(-2, 5));
  EXPECT_FALSE(extrapolator.setDecay(6, 5));

  extrapolator.setPosition(0, 0, 10, 90, 0);
  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 10));

  EXPECT_NEAR(x, 30.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_TRUE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorDecayTest, CanDisableDecayAfterUsingAClippingPolicy)
{
  LinearExtrapolator extrapolator;
  ASSERT_TRUE(extrapolator.setDecay(2, 4));
  ASSERT_TRUE(extrapolator.setDecay(-1, -1));
  extrapolator.setPosition(0, 0, 10, 90, 0);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 10));

  EXPECT_NEAR(x, 100.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_FALSE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorDecayTest, ExactTimestampDoesNotClearPriorDecayMaxedFlagQuirk)
{
  LinearExtrapolator extrapolator;
  ASSERT_TRUE(extrapolator.setDecay(1, 2));
  extrapolator.setPosition(0, 0, 10, 90, 0);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 5));
  ASSERT_TRUE(extrapolator.isDecayMaxed());

  EXPECT_TRUE(extrapolator.getPosition(x, y, 0));
  EXPECT_NEAR(x, 0.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_TRUE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorMoosUseTest,
     NodeRecordUtilsStyleMaxDeltaClipsStaleNodeReport)
{
  // lib_contacts/NodeRecordUtils::extrapolateRecord uses
  // setDecay(max_delta,max_delta) to cap stale NODE_REPORT motion.
  LinearExtrapolator extrapolator;
  ASSERT_TRUE(extrapolator.setDecay(10, 10));
  extrapolator.setPosition(0, 0, 2, 90, 100);

  double x = 0;
  double y = 0;
  EXPECT_TRUE(extrapolator.getPosition(x, y, 105));
  EXPECT_NEAR(x, 10.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_FALSE(extrapolator.isDecayMaxed());

  EXPECT_TRUE(extrapolator.getPosition(x, y, 115));
  EXPECT_NEAR(x, 20.0, kGeomTol);
  EXPECT_NEAR(y, 0.0, kGeomTol);
  EXPECT_TRUE(extrapolator.isDecayMaxed());
}

TEST(LinearExtrapolatorMoosUseTest,
     ContactManagersComputeActualAndExtrapolatedContactRange)
{
  // pContactMgrV20 and dep_pBasicContactMgr preserve actual range from the
  // last report, then recompute range from the extrapolated contact position.
  const double own_x = 0;
  const double own_y = 0;
  const double contact_x = 100;
  const double contact_y = 0;
  const double range_actual = std::hypot(own_x - contact_x, own_y - contact_y);

  LinearExtrapolator extrapolator;
  extrapolator.setPosition(contact_x, contact_y, 2, 270, 10);

  double extrap_x = contact_x;
  double extrap_y = contact_y;
  ASSERT_TRUE(extrapolator.getPosition(extrap_x, extrap_y, 20));

  const double range_extrap =
      std::hypot(own_x - extrap_x, own_y - extrap_y);
  EXPECT_NEAR(extrap_x, 80.0, kGeomTol);
  EXPECT_NEAR(extrap_y, 0.0, kGeomTol);
  EXPECT_NEAR(range_actual, 100.0, kGeomTol);
  EXPECT_NEAR(range_extrap, 80.0, kGeomTol);
}

TEST(WrapDetectorStateTest, StartsEmptyAndStoresFirstPositionWithoutLeg)
{
  InspectableWrapDetector detector;

  EXPECT_TRUE(detector.empty());
  EXPECT_EQ(detector.getWraps(), 0u);

  detector.updatePosition(3, 4);

  EXPECT_FALSE(detector.empty());
  EXPECT_EQ(detector.legCount(), 0u);
  EXPECT_NEAR(detector.osx(), 3.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), 4.0, kGeomTol);
}

TEST(WrapDetectorStateTest, IgnoresLegsBelowMinimumDistanceAndKeepsPreviousAnchor)
{
  InspectableWrapDetector detector;
  detector.setMinLeg(5);

  detector.updatePosition(0, 0);
  detector.updatePosition(3, 0);
  EXPECT_EQ(detector.legCount(), 0u);
  EXPECT_NEAR(detector.osx(), 0.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), 0.0, kGeomTol);

  detector.updatePosition(6, 0);
  ASSERT_EQ(detector.legCount(), 1u);
  EXPECT_NEAR(detector.legAt(0).get_x1(), 0.0, kGeomTol);
  EXPECT_NEAR(detector.legAt(0).get_x2(), 6.0, kGeomTol);
  EXPECT_NEAR(detector.osx(), 6.0, kGeomTol);

  detector.setMinLeg(-1);
  detector.updatePosition(7, 0);
  EXPECT_EQ(detector.legCount(), 1u);
  EXPECT_NEAR(detector.osx(), 6.0, kGeomTol);
}

TEST(WrapDetectorStateTest, AcceptsZeroMinimumLegIncludingRepeatedPositionSegments)
{
  InspectableWrapDetector detector;
  detector.setMinLeg(0);

  detector.updatePosition(2, 2);
  detector.updatePosition(2, 2);

  ASSERT_EQ(detector.legCount(), 1u);
  EXPECT_NEAR(detector.legAt(0).length(), 0.0, kGeomTol);
  EXPECT_NEAR(detector.osx(), 2.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), 2.0, kGeomTol);
}

TEST(WrapDetectorWrapTest, CountsLegRunStyleSelfIntersectionAndClearsHistory)
{
  InspectableWrapDetector detector;

  testing::internal::CaptureStdout();
  feedSelfCrossingLegRun(detector);
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(detector.getWraps(), 1u);
  EXPECT_TRUE(detector.empty());
  EXPECT_EQ(detector.legCount(), 0u);
  EXPECT_NE(output.find("Wrap detected!!!"), std::string::npos);
}

TEST(WrapDetectorWrapTest, DoesNotCountIntersectionWithImmediatePreviousLeg)
{
  InspectableWrapDetector detector;

  detector.updatePosition(0, 0);
  detector.updatePosition(10, 0);
  detector.updatePosition(5, 0);

  EXPECT_EQ(detector.getWraps(), 0u);
  EXPECT_FALSE(detector.empty());
  EXPECT_EQ(detector.legCount(), 2u);
  EXPECT_NEAR(detector.osx(), 5.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), 0.0, kGeomTol);
}

TEST(WrapDetectorWrapTest, CountsFreshWrapsAfterWrapResetFromNextReportedPosition)
{
  InspectableWrapDetector detector;

  testing::internal::CaptureStdout();
  feedSelfCrossingLegRun(detector);
  feedSelfCrossingLegRun(detector);
  testing::internal::GetCapturedStdout();

  EXPECT_EQ(detector.getWraps(), 2u);
  EXPECT_TRUE(detector.empty());
  EXPECT_EQ(detector.legCount(), 0u);
}

TEST(WrapDetectorHistoryTest, PrunesOldLegsAtMaxLegLimit)
{
  InspectableWrapDetector detector;
  detector.setMaxLegs(2);

  detector.updatePosition(0, 0);
  detector.updatePosition(10, 0);
  detector.updatePosition(20, 0);
  detector.updatePosition(30, 0);

  ASSERT_EQ(detector.legCount(), 2u);
  EXPECT_NEAR(detector.legAt(0).get_x1(), 10.0, kGeomTol);
  EXPECT_NEAR(detector.legAt(0).get_x2(), 20.0, kGeomTol);
  EXPECT_NEAR(detector.legAt(1).get_x1(), 20.0, kGeomTol);
  EXPECT_NEAR(detector.legAt(1).get_x2(), 30.0, kGeomTol);
}

TEST(WrapDetectorHistoryTest, ZeroMaxLegsKeepsCurrentAnchorButNoHistory)
{
  InspectableWrapDetector detector;
  detector.setMaxLegs(0);

  detector.updatePosition(0, 0);
  detector.updatePosition(10, 0);
  detector.updatePosition(10, 10);

  EXPECT_EQ(detector.legCount(), 0u);
  EXPECT_FALSE(detector.empty());
  EXPECT_NEAR(detector.osx(), 10.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), 10.0, kGeomTol);
  EXPECT_EQ(detector.getWraps(), 0u);
}

TEST(WrapDetectorHistoryTest, PrunedHistoryNoLongerContributesToWrapDetection)
{
  InspectableWrapDetector detector;
  detector.setMaxLegs(1);

  detector.updatePosition(0, 0);
  detector.updatePosition(10, 0);
  detector.updatePosition(10, 10);
  detector.updatePosition(0, 10);
  detector.updatePosition(5, -5);

  EXPECT_EQ(detector.getWraps(), 0u);
  EXPECT_FALSE(detector.empty());
  EXPECT_EQ(detector.legCount(), 1u);
  EXPECT_NEAR(detector.osx(), 5.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), -5.0, kGeomTol);
}

TEST(WrapDetectorStateTest, ClearAndResetRemoveWrapCountAndLegHistory)
{
  InspectableWrapDetector detector;

  testing::internal::CaptureStdout();
  feedSelfCrossingLegRun(detector);
  testing::internal::GetCapturedStdout();
  ASSERT_EQ(detector.getWraps(), 1u);

  detector.updatePosition(1, 1);
  detector.updatePosition(2, 1);
  ASSERT_EQ(detector.legCount(), 1u);

  detector.clear();
  EXPECT_EQ(detector.getWraps(), 0u);
  EXPECT_TRUE(detector.empty());
  EXPECT_EQ(detector.legCount(), 0u);
  EXPECT_NEAR(detector.osx(), 0.0, kGeomTol);
  EXPECT_NEAR(detector.osy(), 0.0, kGeomTol);

  detector.updatePosition(1, 1);
  detector.updatePosition(2, 1);
  ASSERT_EQ(detector.legCount(), 1u);

  detector.reset();
  EXPECT_EQ(detector.getWraps(), 0u);
  EXPECT_TRUE(detector.empty());
  EXPECT_EQ(detector.legCount(), 0u);
}

TEST(ProxPointStateTest, DefaultsCpaAndDistanceToZero)
{
  ProxPoint point;

  EXPECT_NEAR(point.getCPA(), 0.0, kGeomTol);
  EXPECT_NEAR(point.getCPADist(), 0.0, kGeomTol);
}

TEST(ProxPointStateTest, StoresWallCacheMetadataWithoutValidation)
{
  ProxPoint point;
  point.setCPA(-2.5);
  point.setCPADist(-7.25);
  point.setType(2);

  EXPECT_NEAR(point.getCPA(), -2.5, kGeomTol);
  EXPECT_NEAR(point.getCPADist(), -7.25, kGeomTol);
  EXPECT_EQ(point.getType(), 2);
}

TEST(ProxPointOrderingTest, SortsByDistanceToCpaForWallProximityCaches)
{
  std::vector<ProxPoint> points;
  points.push_back(makeProxPoint(4, 30, 2));
  points.push_back(makeProxPoint(9, 10, 1));
  points.push_back(makeProxPoint(1, 20, 2));

  std::sort(points.begin(), points.end());

  ASSERT_EQ(points.size(), 3u);
  EXPECT_NEAR(points[0].getCPADist(), 10.0, kGeomTol);
  EXPECT_NEAR(points[1].getCPADist(), 20.0, kGeomTol);
  EXPECT_NEAR(points[2].getCPADist(), 30.0, kGeomTol);
  EXPECT_EQ(points[0].getType(), 1);
  EXPECT_EQ(points[1].getType(), 2);
}

TEST(ProxPointOrderingTest, OrderingIgnoresCpaValueAndTypeForEqualDistances)
{
  ProxPoint arc = makeProxPoint(1, 15, 1);
  ProxPoint ray = makeProxPoint(99, 15, 2);

  EXPECT_FALSE(arc < ray);
  EXPECT_FALSE(ray < arc);
}

TEST(ProxPointOrderingTest, StrictLessThanHandlesNegativeDistances)
{
  ProxPoint negative = makeProxPoint(5, -1, 1);
  ProxPoint zero = makeProxPoint(1, 0, 2);

  EXPECT_TRUE(negative < zero);
  EXPECT_FALSE(zero < negative);
}
