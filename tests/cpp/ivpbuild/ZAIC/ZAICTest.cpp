#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "IvPBuildTestUtils.h"
#include "NumericAssertions.h"
#include "ZAIC_HDG.h"
#include "ZAIC_HEQ.h"
#include "ZAIC_HLEQ.h"
#include "ZAIC_LEQ.h"
#include "ZAIC_PEAK.h"
#include "ZAIC_SPD.h"
#include "ZAIC_Vector.h"

TEST(ZAICPeakTest, BuildsCoursePeakAndPinsWarningsForOutOfRangeComponentAccess)
{
  ZAIC_PEAK zaic(makeCourseSpeedDomain(), "course");
  EXPECT_TRUE(zaic.stateOK());
  EXPECT_TRUE(zaic.setParams(90, 10, 40, 5, 0, 100));
  zaic.setSummitInsist(true);
  zaic.setValueWrap(true);
  EXPECT_TRUE(zaic.getValueWrap());
  EXPECT_TRUE(zaic.getSummitInsist());
  EXPECT_EQ(zaic.getSummitCount(), 1);
  EXPECT_NEAR(zaic.getParam("summit"), 90.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("peakwidth"), 10.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("basewidth"), 40.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("summitdelta"), 5.0, kGeomTol);

  int second = zaic.addComponent();
  ASSERT_EQ(second, 1);
  EXPECT_TRUE(zaic.setParams(270, 0, 30, 0, 5, 80, second));
  EXPECT_FALSE(zaic.setSummit(10, 10));
  EXPECT_FALSE(zaic.stateOK());
  EXPECT_TRUE(zaic.getWarnings().find("index out of range") != std::string::npos);

  ZAIC_PEAK valid(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(valid.setParams(180, 5, 30, 10, 0, 100));
  std::unique_ptr<IvPFunction> ipf(valid.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_TRUE(ipf->valid());
  EXPECT_GT(ipf->size(), 0);
  EXPECT_EQ(valid.getIvPDomain().getVarName(0), "course");
}

TEST(ZAICPeakTest, RejectsUnknownDomainAndClampsNegativeWidths)
{
  ZAIC_PEAK clamped(makeCourseSpeedDomain(), "speed");
  EXPECT_TRUE(clamped.setBaseWidth(-1));
  EXPECT_TRUE(clamped.setPeakWidth(-2));
  EXPECT_TRUE(clamped.setSummitDelta(-3));
  EXPECT_FALSE(clamped.setMinMaxUtil(100, 10));
  EXPECT_FALSE(clamped.stateOK());
  EXPECT_NEAR(clamped.getParam("basewidth"), 0.0, kGeomTol);
  EXPECT_NEAR(clamped.getParam("peakwidth"), 0.0, kGeomTol);
  EXPECT_NEAR(clamped.getParam("summitdelta"), 0.0, kGeomTol);
  EXPECT_TRUE(clamped.getWarnings().find("less than zero") != std::string::npos);
  EXPECT_TRUE(clamped.getWarnings().find("min>=max") != std::string::npos);
}

TEST(ZAICPeakTest, CoursePeakWrapsAcrossNorthForHelmHeadingDomains)
{
  ZAIC_PEAK wrapped(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(wrapped.setParams(350, 5, 40, 0, 0, 100));
  wrapped.setValueWrap(true);

  std::unique_ptr<IvPFunction> wrapped_ipf(wrapped.extractIvPFunction());
  ASSERT_NE(wrapped_ipf, nullptr);
  ASSERT_NE(wrapped_ipf->getPDMap(), nullptr);
  EXPECT_TRUE(wrapped_ipf->valid());

  const PDMap& wrapped_map = *wrapped_ipf->getPDMap();
  EXPECT_NEAR(evalPDMapAtIndex(wrapped_map, 350), 100.0, kGeomTol);
  EXPECT_GT(evalPDMapAtIndex(wrapped_map, 0),
            evalPDMapAtIndex(wrapped_map, 180));
  EXPECT_GT(evalPDMapAtIndex(wrapped_map, 359),
            evalPDMapAtIndex(wrapped_map, 180));
  EXPECT_GT(evalPDMapAtIndex(wrapped_map, 10),
            evalPDMapAtIndex(wrapped_map, 180));

  ZAIC_PEAK unwrapped(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(unwrapped.setParams(350, 5, 40, 0, 0, 100));
  unwrapped.setValueWrap(false);
  std::unique_ptr<IvPFunction> unwrapped_ipf(unwrapped.extractIvPFunction());
  ASSERT_NE(unwrapped_ipf, nullptr);
  ASSERT_NE(unwrapped_ipf->getPDMap(), nullptr);

  EXPECT_GT(evalPDMapAtIndex(wrapped_map, 0),
            evalPDMapAtIndex(*unwrapped_ipf->getPDMap(), 0));
}

TEST(ZAICSpeedTest, MatchesTypicalSpeedPreferenceConfigAndSummary)
{
  ZAIC_SPD zaic(makeCourseSpeedDomain(), "speed");
  EXPECT_FALSE(zaic.setParams(2.0, 0.5, 4.0, 20, 30, 0, 10, 100));
  EXPECT_NEAR(zaic.getParam("med_spd"), 2.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("low_spd"), 0.5, kGeomTol);
  EXPECT_NEAR(zaic.getParam("hgh_spd"), 4.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("low_spd_util"), 20.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("hgh_spd_util"), 30.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("min_spd_util"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("max_spd_util"), 10.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("max_util"), 100.0, kGeomTol);
  EXPECT_EQ(zaic.getParam("unknown"), 0.0);
  EXPECT_FALSE(zaic.getSummary().empty());
  EXPECT_TRUE(zaic.setMaxUtil(90));
  EXPECT_TRUE(zaic.setMinSpdUtil(5));
  EXPECT_TRUE(zaic.setMaxSpdUtil(1000));
  EXPECT_TRUE(zaic.adjustParams());
  EXPECT_NEAR(zaic.getParam("max_util"), 90.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("min_spd_util"), 5.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("max_spd_util"), 90.0, kGeomTol);

  zaic.disableLowSpeed();
  zaic.disableHighSpeed();
  EXPECT_NEAR(zaic.getParam("low_spd"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("hgh_spd"), 5.0, kGeomTol);

  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_TRUE(ipf->valid());
}

TEST(ZAICSpeedTest, ClampsSpeedsAndRejectsMissingDomain)
{
  ZAIC_SPD zaic(makeCourseSpeedDomain(), "speed");
  EXPECT_TRUE(zaic.setMedSpeed(99));
  EXPECT_TRUE(zaic.setLowSpeed(-9));
  EXPECT_TRUE(zaic.setHghSpeed(99));
  EXPECT_NEAR(zaic.getParam("medspd"), 5.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("lowspd"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("hghspd"), 5.0, kGeomTol);
  EXPECT_TRUE(zaic.setLowSpeedUtil(-10));
  EXPECT_TRUE(zaic.setHghSpeedUtil(1000));
  EXPECT_NEAR(zaic.getParam("lowspd_util"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("hghspd_util"), 100.0, kGeomTol);

  ZAIC_SPD missing(makeCourseSpeedDomain(), "depth");
  EXPECT_FALSE(missing.setMedSpeed(1));
  EXPECT_EQ(missing.extractOF(), nullptr);
}

TEST(ZAICSpeedTest, GeneratesExpectedBHVWaypointCruiseSpeedUtilityShape)
{
  ZAIC_SPD zaic(makeCourseSpeedDomain(), "speed");
  ASSERT_TRUE(zaic.setParams(2.0, 0.0, 4.0, 0, 50, 0, 10, 100));

  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  ASSERT_NE(ipf->getPDMap(), nullptr);
  const PDMap& pdmap = *ipf->getPDMap();

  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 0), 0.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 1), 50.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 2), 100.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 3), 75.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 4), 50.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 5), 10.0, kGeomTol);
}

TEST(ZAICHeadingTest, BuildsWrapAwareCoursePreference)
{
  ZAIC_HDG zaic(makeCourseSpeedDomain(), "course");
  EXPECT_TRUE(zaic.setParams(350, 30, 40, 70, 60, 5, 10, 100));
  EXPECT_TRUE(zaic.setSummit(-10));
  EXPECT_NEAR(zaic.getParam("summit"), 350.0, kGeomTol);
  EXPECT_TRUE(zaic.setLowDelta(-5));
  EXPECT_TRUE(zaic.setHighDelta(900));
  EXPECT_TRUE(zaic.setLowDeltaUtil(-1));
  EXPECT_TRUE(zaic.setHighDeltaUtil(1000));
  EXPECT_NEAR(zaic.getParam("lowdelta"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("highdelta"), 180.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("lowdeltautil"), 5.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("highdeltautil"), 100.0, kGeomTol);
  EXPECT_TRUE(zaic.setMinMaxUtil(-5, -1, 80));
  EXPECT_NEAR(zaic.getParam("lminutil"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("hminutil"), 0.0, kGeomTol);
  EXPECT_FALSE(zaic.setMinMaxUtil(90, 10, 80));
  EXPECT_TRUE(zaic.setSummit(720));
  EXPECT_NEAR(zaic.getParam("summit"), 0.0, kGeomTol);

  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_TRUE(ipf->valid());
}

TEST(ZAICHeadingTest, RepairsNonHeadingCourseDomainToStandardZeroTo359)
{
  IvPDomain odd_course;
  odd_course.addDomain("course", -180, 180, 19);
  ZAIC_HDG zaic(odd_course, "course");
  EXPECT_EQ(zaic.getIvPDomain().getVarLow(0), 0);
  EXPECT_EQ(zaic.getIvPDomain().getVarHigh(0), 359);
  EXPECT_EQ(zaic.getIvPDomain().getVarPoints(0), 360u);
}

TEST(ZAICHeadingTest, GeneratedCourseUtilityUsesPortAndStarboardDeltasAcrossNorth)
{
  ZAIC_HDG zaic(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(zaic.setParams(350, 30, 40, 70, 60, 5, 10, 100));

  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  ASSERT_NE(ipf->getPDMap(), nullptr);
  const PDMap& pdmap = *ipf->getPDMap();

  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 350), 100.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 340), 90.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 0), 90.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 320), 70.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 30), 60.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 170), 10.0, kGeomTol);
}

TEST(ZAICHleqLeqHeqTest, ConfiguresOneSidedUtilitiesUsedBySpeedBounds)
{
  ZAIC_HLEQ base(makeCourseSpeedDomain(), "speed");
  EXPECT_TRUE(base.stateOK());
  EXPECT_TRUE(base.setSummit(2.5));
  EXPECT_TRUE(base.setSummitDelta(-1));
  EXPECT_TRUE(base.setBaseWidth(1.5));
  EXPECT_TRUE(base.setMinMaxUtil(10, 90));
  EXPECT_NEAR(base.getParam("summit"), 2.5, kGeomTol);
  EXPECT_NEAR(base.getParam("summit_delta"), 0.0, kGeomTol);
  EXPECT_NEAR(base.getParam("basewidth"), 1.5, kGeomTol);
  EXPECT_NEAR(base.getParam("minutil"), 10.0, kGeomTol);
  EXPECT_NEAR(base.getParam("maxutil"), 90.0, kGeomTol);
  EXPECT_FALSE(base.setBaseWidth(-1));
  EXPECT_FALSE(base.setMinMaxUtil(90, 10));

  ZAIC_LEQ leq(makeCourseSpeedDomain(), "speed");
  EXPECT_TRUE(leq.setSummit(2));
  EXPECT_TRUE(leq.setBaseWidth(2));
  EXPECT_TRUE(leq.setSummitDelta(5));
  std::unique_ptr<IvPFunction> leq_ipf(leq.extractIvPFunction());
  ASSERT_NE(leq_ipf, nullptr);
  EXPECT_TRUE(leq_ipf->valid());

  ZAIC_HEQ heq(makeCourseSpeedDomain(), "speed");
  EXPECT_TRUE(heq.setSummit(3));
  EXPECT_TRUE(heq.setBaseWidth(2));
  EXPECT_TRUE(heq.setSummitDelta(5));
  std::unique_ptr<IvPFunction> heq_ipf(heq.extractIvPFunction());
  ASSERT_NE(heq_ipf, nullptr);
  EXPECT_TRUE(heq_ipf->valid());
}

TEST(ZAICHleqLeqHeqTest, GeneratedLeqAndHeqFunctionsPinOneSidedSpeedBounds)
{
  ZAIC_LEQ leq(makeCourseSpeedDomain(), "speed");
  ASSERT_TRUE(leq.setSummit(2));
  ASSERT_TRUE(leq.setBaseWidth(2));
  ASSERT_TRUE(leq.setSummitDelta(5));
  ASSERT_TRUE(leq.setMinMaxUtil(10, 90));
  std::unique_ptr<IvPFunction> leq_ipf(leq.extractIvPFunction());
  ASSERT_NE(leq_ipf, nullptr);
  ASSERT_NE(leq_ipf->getPDMap(), nullptr);

  EXPECT_NEAR(evalPDMapAtIndex(*leq_ipf->getPDMap(), 0), 90.0, kLooseGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*leq_ipf->getPDMap(), 2), 95.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*leq_ipf->getPDMap(), 3), 50.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*leq_ipf->getPDMap(), 4), 10.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*leq_ipf->getPDMap(), 5), 10.0, kLooseGeomTol);

  ZAIC_HEQ heq(makeCourseSpeedDomain(), "speed");
  ASSERT_TRUE(heq.setSummit(3));
  ASSERT_TRUE(heq.setBaseWidth(2));
  ASSERT_TRUE(heq.setSummitDelta(5));
  ASSERT_TRUE(heq.setMinMaxUtil(10, 90));
  std::unique_ptr<IvPFunction> heq_ipf(heq.extractIvPFunction());
  ASSERT_NE(heq_ipf, nullptr);
  ASSERT_NE(heq_ipf->getPDMap(), nullptr);

  EXPECT_NEAR(evalPDMapAtIndex(*heq_ipf->getPDMap(), 0), 10.0, kLooseGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*heq_ipf->getPDMap(), 1), 10.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*heq_ipf->getPDMap(), 2), 50.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*heq_ipf->getPDMap(), 3), 95.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*heq_ipf->getPDMap(), 5), 90.0, kLooseGeomTol);
}

TEST(ZAICVectorTest, BuildsVectorMappingAndReportsConfigWarnings)
{
  ZAIC_Vector zaic(makeCourseSpeedDomain(), "speed");
  zaic.setDomainVals({0, 2, 5});
  zaic.setRangeVals({0, 100, 20});
  zaic.setTolerance(-5);
  EXPECT_NEAR(zaic.getParam("tolerance"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("minutil"), 0.0, kGeomTol);
  EXPECT_NEAR(zaic.getParam("maxutil"), 100.0, kGeomTol);
  EXPECT_FALSE(zaic.hasErrors());
  EXPECT_EQ(zaic.getErrors(), "");
  EXPECT_FALSE(zaic.hasWarnings());

  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  EXPECT_TRUE(ipf->valid());
  EXPECT_EQ(zaic.size(), 6u);
  EXPECT_EQ(zaic.getTotalPieces(), 0u);

  ZAIC_Vector unsorted(makeCourseSpeedDomain(), "speed");
  unsorted.setDomainVals({4, 1, 2});
  unsorted.setRangeVals({10, 90, 30});
  std::unique_ptr<IvPFunction> unsorted_ipf(unsorted.extractOF());
  ASSERT_NE(unsorted_ipf, nullptr);
  EXPECT_TRUE(unsorted.hasWarnings());
  EXPECT_TRUE(unsorted.getWarnings().find("extend to lower bound") != std::string::npos);
  unsorted.clearWarnings();
  EXPECT_FALSE(unsorted.hasWarnings());
  unsorted.clearErrors();
  EXPECT_FALSE(unsorted.hasErrors());

  ZAIC_Vector bad(makeCourseSpeedDomain(), "speed");
  bad.setDomainVals({0, 1});
  bad.setRangeVals({0});
  EXPECT_EQ(bad.extractOF(), nullptr);
  EXPECT_FALSE(bad.hasErrors());
  EXPECT_EQ(bad.getErrors(), "");
}

TEST(ZAICVectorTest, GeneratedVectorMapPreservesExplicitDomainRangePairs)
{
  ZAIC_Vector zaic(makeCourseSpeedDomain(), "speed");
  zaic.setDomainVals({0, 2, 5});
  zaic.setRangeVals({0, 100, 20});

  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  ASSERT_NE(ipf->getPDMap(), nullptr);
  const PDMap& pdmap = *ipf->getPDMap();

  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 0), 0.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 1), 50.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 2), 100.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 3), 73.3333333333, kLooseGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(pdmap, 5), 20.0, kGeomTol);
}
