#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "AOF_Gaussian.h"
#include "AOF_Linear.h"
#include "FunctionEncoder.h"
#include "IvPBuildTestUtils.h"
#include "NumericAssertions.h"
#include "OF_Coupler.h"
#include "OF_Rater.h"
#include "PDMapBuilder.h"
#include "ZAIC_PEAK.h"
#include "ZAIC_SPD.h"

namespace {

std::string packetPrefix(const std::string& function_id,
                         std::size_t total,
                         std::size_t index)
{
  // IvP function postings use one-based packet indices in P,<id>,<total>,<ix>.
  return "P," + function_id + "," + std::to_string(total) + "," +
         std::to_string(index) + ",";
}

}  // namespace

// Covers pd map builder behavior: builds piecewise one dimensional utility and reports bad inputs.
TEST(PDMapBuilderTest, BuildsPiecewiseOneDimensionalUtilityAndReportsBadInputs)
{
  IvPDomain domain;
  domain.addDomain("speed", 0, 5, 6);

  PDMapBuilder builder;
  builder.setIvPDomain(domain);
  builder.setDomainVals({0, 2, 5});
  builder.setRangeVals({0, 100, 25});
  std::unique_ptr<PDMap> pdmap(builder.getPDMap());
  ASSERT_NE(pdmap, nullptr);
  EXPECT_EQ(pdmap->size(), 2);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 0), 0.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 2), 100.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 5), 25.0, kGeomTol);
  EXPECT_FALSE(builder.hasWarnings());

  builder.clearWarnings();
  builder.setDomainVals({1, 5});
  builder.setRangeVals({0, 10});
  EXPECT_EQ(builder.getPDMap(), nullptr);
  EXPECT_TRUE(builder.hasWarnings());
  EXPECT_TRUE(builder.getWarnings().find("preprocess failed") != std::string::npos);
}

// Covers pd map builder behavior: handles single point and consecutive point cases.
TEST(PDMapBuilderTest, HandlesSinglePointAndConsecutivePointCases)
{
  // PDMapBuilder has special handling for degenerate one-point domains and
  // consecutive breakpoints that would otherwise create zero-width intervals.
  IvPDomain single;
  single.addDomain("speed", 2, 2, 1);
  PDMapBuilder one;
  one.setIvPDomain(single);
  one.setDomainVals({0});
  one.setRangeVals({42});
  std::unique_ptr<PDMap> one_map(one.getPDMap());
  ASSERT_NE(one_map, nullptr);
  EXPECT_EQ(one_map->size(), 1);
  EXPECT_NEAR(evalPDMapAtIndex(*one_map, 0), 42.0, kGeomTol);

  IvPDomain domain;
  domain.addDomain("speed", 0, 2, 3);
  PDMapBuilder consecutive;
  consecutive.setIvPDomain(domain);
  consecutive.setDomainVals({0, 1, 2});
  consecutive.setRangeVals({0, 50, 100});
  std::unique_ptr<PDMap> pdmap(consecutive.getPDMap());
  ASSERT_NE(pdmap, nullptr);
  EXPECT_EQ(pdmap->size(), 2);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 1), 50.0, kGeomTol);
}

// Covers pd map builder behavior: builds non consecutive intervals without overlapping breakpoints.
TEST(PDMapBuilderTest, BuildsNonConsecutiveIntervalsWithoutOverlappingBreakpoints)
{
  IvPDomain domain;
  domain.addDomain("speed", 0, 10, 11);

  PDMapBuilder builder;
  builder.setIvPDomain(domain);
  builder.setDomainVals({0, 3, 10});
  builder.setRangeVals({0, 30, 100});

  std::unique_ptr<PDMap> pdmap(builder.getPDMap());
  ASSERT_NE(pdmap, nullptr);
  EXPECT_EQ(pdmap->size(), 2);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 0), 0.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 2), 20.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 3), 30.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 4), 40.0, kGeomTol);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 10), 100.0, kGeomTol);

  PDMapBuilder multidim;
  multidim.setIvPDomain(makeCourseSpeedDomain());
  multidim.setDomainVals({0, 1});
  multidim.setRangeVals({0, 100});
  EXPECT_EQ(multidim.getPDMap(), nullptr);
  EXPECT_TRUE(multidim.hasWarnings());
}

// Covers pd map builder behavior: rejects malformed breakpoint tables before building pieces.
TEST(PDMapBuilderTest, RejectsMalformedBreakpointTablesBeforeBuildingPieces)
{
  IvPDomain domain;
  domain.addDomain("speed", 0, 5, 6);

  PDMapBuilder missing_range;
  missing_range.setIvPDomain(domain);
  missing_range.setDomainVals({0, 5});
  missing_range.setRangeVals({0});
  EXPECT_EQ(missing_range.getPDMap(), nullptr);
  EXPECT_TRUE(missing_range.hasWarnings());

  PDMapBuilder missing_left_endpoint;
  missing_left_endpoint.setIvPDomain(domain);
  missing_left_endpoint.setDomainVals({1, 5});
  missing_left_endpoint.setRangeVals({0, 100});
  EXPECT_EQ(missing_left_endpoint.getPDMap(), nullptr);
  EXPECT_TRUE(missing_left_endpoint.hasWarnings());

  PDMapBuilder missing_right_endpoint;
  missing_right_endpoint.setIvPDomain(domain);
  missing_right_endpoint.setDomainVals({0, 4});
  missing_right_endpoint.setRangeVals({0, 100});
  EXPECT_EQ(missing_right_endpoint.getPDMap(), nullptr);
  EXPECT_TRUE(missing_right_endpoint.hasWarnings());

  PDMapBuilder duplicate_breakpoint;
  duplicate_breakpoint.setIvPDomain(domain);
  duplicate_breakpoint.setDomainVals({0, 2, 2, 5});
  duplicate_breakpoint.setRangeVals({0, 40, 80, 100});
  EXPECT_EQ(duplicate_breakpoint.getPDMap(), nullptr);
  EXPECT_TRUE(duplicate_breakpoint.hasWarnings());

  PDMapBuilder descending_breakpoint;
  descending_breakpoint.setIvPDomain(domain);
  descending_breakpoint.setDomainVals({0, 4, 3, 5});
  descending_breakpoint.setRangeVals({0, 80, 60, 100});
  EXPECT_EQ(descending_breakpoint.getPDMap(), nullptr);
  EXPECT_TRUE(descending_breakpoint.hasWarnings());
}

// Covers function encoder behavior: round trips IvP function string and packets like helm postings.
TEST(FunctionEncoderTest, RoundTripsIvPFunctionStringAndPacketsLikeHelmPostings)
{
  // This mirrors the helm/uFunctionVis path: encode an IvP function, packetize
  // it for posting, then decode it back into an objective function.
  ZAIC_PEAK zaic(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(zaic.setParams(90, 10, 40, 0, 0, 100));
  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  ipf->setPWT(75);
  ipf->setContextStr("bhv=test platform=abe");

  std::string encoded = IvPFunctionToString(ipf.get());
  ASSERT_FALSE(encoded.empty());
  EXPECT_EQ(StringToIvPContext(encoded), "bhv=test platform=abe");

  IvPDomain parsed_domain = IPFStringToIvPDomain(encoded);
  EXPECT_TRUE(parsed_domain.hasOnlyDomain("course"));
  EXPECT_EQ(parsed_domain.getVarPoints("course"), 360u);

  std::vector<std::string> packets = IvPFunctionToVector(encoded, "ipf-id", 80);
  ASSERT_GT(packets.size(), 1u);
  EXPECT_TRUE(packets.front().find("P,ipf-id,") == 0);

  std::unique_ptr<IvPFunction> decoded(StringToIvPFunction(encoded));
  ASSERT_NE(decoded, nullptr);
  EXPECT_TRUE(decoded->valid());
  EXPECT_EQ(decoded->getContextStr(), "bhv=test platform=abe");
  EXPECT_EQ(decoded->getDim(), ipf->getDim());
  EXPECT_EQ(decoded->size(), ipf->size());
  EXPECT_NEAR(decoded->getPWT(), 75.0, kGeomTol);
}

// Covers function encoder behavior: packetizes and reassembles helm posting fragments in order.
TEST(FunctionEncoderTest, PacketizesAndReassemblesHelmPostingFragmentsInOrder)
{
  ZAIC_PEAK zaic(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(zaic.setParams(45, 5, 80, 0, 0, 100));
  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  ASSERT_NE(ipf, nullptr);
  ipf->setContextStr("behavior=BHV_Waypoint name=waypt_alpha iteration=42");

  const std::string encoded = IvPFunctionToString(ipf.get());
  ASSERT_FALSE(encoded.empty());

  const std::string function_id = "helm_iter_42_waypt_alpha";
  std::vector<std::string> packets =
      IvPFunctionToVector(encoded, function_id, 120);
  ASSERT_GT(packets.size(), 2u);

  // Reassembly is just packet-body concatenation once prefixes verify the
  // expected function id, packet count, and one-based packet order.
  std::string reassembled;
  for(std::size_t i = 0; i < packets.size(); ++i) {
    const std::string prefix = packetPrefix(function_id, packets.size(), i + 1);
    ASSERT_EQ(packets[i].find(prefix), 0u);
    EXPECT_LE(packets[i].size(), 120u);
    reassembled += packets[i].substr(prefix.size());
  }

  EXPECT_EQ(reassembled, encoded);
  std::unique_ptr<IvPFunction> decoded(StringToIvPFunction(reassembled));
  ASSERT_NE(decoded, nullptr);
  EXPECT_EQ(decoded->getContextStr(),
            "behavior=BHV_Waypoint name=waypt_alpha iteration=42");
  EXPECT_EQ(decoded->size(), ipf->size());
}

// Covers function encoder behavior: rejects empty encoded function string.
TEST(FunctionEncoderTest, RejectsEmptyEncodedFunctionString)
{
  EXPECT_EQ(StringToIvPFunction(""), nullptr);
}

// Covers of coupler behavior: couples independent course and speed objective functions.
TEST(OFCouplerTest, CouplesIndependentCourseAndSpeedObjectiveFunctions)
{
  ZAIC_PEAK course(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(course.setParams(180, 10, 60, 0, 0, 100));
  ZAIC_SPD speed(makeCourseSpeedDomain(), "speed");
  ASSERT_TRUE(speed.setParams(2, 0, 4, 0, 0, 0, 0, 100));

  OF_Coupler coupler;
  coupler.enableNormalize(0, 200);
  std::unique_ptr<IvPFunction> coupled(
      coupler.couple(course.extractIvPFunction(), speed.extractIvPFunction(), 70, 30));
  ASSERT_NE(coupled, nullptr);
  EXPECT_TRUE(coupled->valid());
  EXPECT_EQ(coupled->getDim(), 2);
  EXPECT_TRUE(coupled->getPDMap()->getDomain().hasDomain("course"));
  EXPECT_TRUE(coupled->getPDMap()->getDomain().hasDomain("speed"));
  EXPECT_GE(coupled->getValMinUtil(), -kLooseGeomTol);
  EXPECT_LE(coupled->getValMaxUtil(), 200.0 + kLooseGeomTol);

  OF_Coupler raw;
  raw.disableNormalize();
  std::unique_ptr<IvPFunction> raw_coupled(
      raw.couple(course.extractIvPFunction(), speed.extractIvPFunction()));
  ASSERT_NE(raw_coupled, nullptr);
  EXPECT_TRUE(raw_coupled->valid());

  EXPECT_EQ(coupler.couple(nullptr, speed.extractIvPFunction()), nullptr);
}

// Covers of coupler behavior: coupled function evaluates course speed peak above off peak choices.
TEST(OFCouplerTest, CoupledFunctionEvaluatesCourseSpeedPeakAboveOffPeakChoices)
{
  ZAIC_PEAK course(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(course.setParams(180, 10, 60, 0, 0, 100));
  ZAIC_SPD speed(makeCourseSpeedDomain(), "speed");
  ASSERT_TRUE(speed.setParams(2, 0, 4, 0, 0, 0, 0, 100));

  OF_Coupler coupler;
  coupler.disableNormalize();
  std::unique_ptr<IvPFunction> coupled(
      coupler.couple(course.extractIvPFunction(), speed.extractIvPFunction(),
                     70, 30));
  ASSERT_NE(coupled, nullptr);
  ASSERT_NE(coupled->getPDMap(), nullptr);
  EXPECT_TRUE(coupled->valid());

  IvPBox peak = makePointBox(2, {180, 2});
  IvPBox wrong_course = makePointBox(2, {0, 2});
  IvPBox wrong_speed = makePointBox(2, {180, 5});
  IvPBox both_wrong = makePointBox(2, {0, 5});

  const double peak_util = coupled->getPDMap()->evalPoint(&peak);
  EXPECT_NEAR(peak_util, 100.0, kLooseGeomTol);
  EXPECT_GT(peak_util, coupled->getPDMap()->evalPoint(&wrong_course));
  EXPECT_GT(peak_util, coupled->getPDMap()->evalPoint(&wrong_speed));
  EXPECT_GT(coupled->getPDMap()->evalPoint(&wrong_course),
            coupled->getPDMap()->evalPoint(&both_wrong));
  EXPECT_GT(coupled->getPDMap()->evalPoint(&wrong_speed),
            coupled->getPDMap()->evalPoint(&both_wrong));
}

// Covers of coupler behavior: rejects non positive coupling weights and takes ownership.
TEST(OFCouplerTest, RejectsNonPositiveCouplingWeightsAndTakesOwnership)
{
  ZAIC_PEAK course(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(course.setParams(180, 10, 60, 0, 0, 100));
  ZAIC_SPD speed(makeCourseSpeedDomain(), "speed");
  ASSERT_TRUE(speed.setParams(2, 0, 4, 0, 0, 0, 0, 100));

  OF_Coupler coupler;
  EXPECT_EQ(coupler.couple(course.extractIvPFunction(),
                           speed.extractIvPFunction(), 0, 50),
            nullptr);
}

// Covers of coupler behavior: rejects overlapping domains.
TEST(OFCouplerTest, RejectsOverlappingDomains)
{
  ZAIC_PEAK first(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(first.setParams(90, 10, 30, 0, 0, 100));
  ZAIC_PEAK second(makeCourseSpeedDomain(), "course");
  ASSERT_TRUE(second.setParams(270, 10, 30, 0, 0, 100));

  OF_Coupler coupler;
  EXPECT_EQ(coupler.couple(first.extractIvPFunction(), second.extractIvPFunction()), nullptr);
}

// Covers of rater behavior: samples approximation error against gaussian AOF.
TEST(OFRaterTest, SamplesApproximationErrorAgainstGaussianAOF)
{
  AOF_Gaussian aof(makeXYDomain());
  ASSERT_TRUE(aof.setParam("xcent", 50));
  ASSERT_TRUE(aof.setParam("ycent", 50));
  ASSERT_TRUE(aof.setParam("sigma", 20));
  ASSERT_TRUE(aof.setParam("range", 100));

  PDMapBuilder builder;
  IvPDomain speed_domain;
  speed_domain.addDomain("speed", 0, 5, 6);
  builder.setIvPDomain(speed_domain);
  builder.setDomainVals({0, 5});
  builder.setRangeVals({0, 100});
  std::unique_ptr<PDMap> pdmap(builder.getPDMap());
  ASSERT_NE(pdmap, nullptr);

  OF_Rater rater;
  EXPECT_EQ(rater.getSampleCount(), 0);
  rater.takeSamples(10);
  EXPECT_EQ(rater.getSampleCount(), 0);

  rater.setAOF(&aof);
  rater.setPDMap(pdmap.get());
  rater.takeSamples(5);
  EXPECT_EQ(rater.getSampleCount(), 5);
  EXPECT_GE(rater.getSampHigh(), rater.getSampLow());
  EXPECT_GE(rater.getAvgErr(), 0.0);
  EXPECT_GE(rater.getWorstErr(), 0.0);
  EXPECT_GE(rater.getSquaredErr(), 0.0);

  rater.resetSamples();
  EXPECT_EQ(rater.getSampleCount(), 0);
}

// Covers of rater behavior: reports zero error for exact linear approximation.
TEST(OFRaterTest, ReportsZeroErrorForExactLinearApproximation)
{
  IvPDomain domain;
  domain.addDomain("x", 0, 10, 11);
  domain.addDomain("y", 0, 10, 11);

  AOF_Linear aof(domain);
  ASSERT_TRUE(aof.setParam("mcoeff", 2.0));
  ASSERT_TRUE(aof.setParam("ncoeff", 3.0));
  ASSERT_TRUE(aof.setParam("bscalar", 5.0));

  std::unique_ptr<IvPBox> piece(new IvPBox(2, 1));
  piece->setPTS(0, 0, 10);
  piece->setPTS(1, 0, 10);
  piece->wt(0) = 2.0;
  piece->wt(1) = 3.0;
  piece->wt(2) = 5.0;

  std::unique_ptr<PDMap> pdmap(new PDMap(1, domain, 1));
  pdmap->bx(0) = piece.release();

  OF_Rater rater(pdmap.get(), &aof);
  rater.takeSamples(25);

  EXPECT_EQ(rater.getSampleCount(), 25);
  EXPECT_GE(rater.getSampHigh(), rater.getSampLow());
  EXPECT_NEAR(rater.getAvgErr(), 0.0, kGeomTol);
  EXPECT_NEAR(rater.getWorstErr(), 0.0, kGeomTol);
  EXPECT_NEAR(rater.getSquaredErr(), 0.0, kGeomTol);
}

// Covers of rater behavior: resets samples when inputs change and ignores invalid amounts.
TEST(OFRaterTest, ResetsSamplesWhenInputsChangeAndIgnoresInvalidAmounts)
{
  IvPDomain domain;
  domain.addDomain("x", 0, 10, 11);
  domain.addDomain("y", 0, 10, 11);

  AOF_Linear aof(domain);
  ASSERT_TRUE(aof.setParam("mcoeff", 1.0));
  ASSERT_TRUE(aof.setParam("ncoeff", 0.0));
  ASSERT_TRUE(aof.setParam("bscalar", 0.0));

  std::unique_ptr<IvPBox> piece(new IvPBox(2, 1));
  piece->setPTS(0, 0, 10);
  piece->setPTS(1, 0, 10);
  piece->wt(0) = 1.0;
  piece->wt(1) = 0.0;
  piece->wt(2) = 0.0;

  std::unique_ptr<PDMap> pdmap(new PDMap(1, domain, 1));
  pdmap->bx(0) = piece.release();

  OF_Rater rater(pdmap.get(), &aof);
  rater.takeSamples(3);
  EXPECT_EQ(rater.getSampleCount(), 3);

  rater.takeSamples(0);
  rater.takeSamples(-5);
  EXPECT_EQ(rater.getSampleCount(), 3);

  rater.setPDMap(pdmap.get());
  EXPECT_EQ(rater.getSampleCount(), 0);

  rater.takeSamples(2);
  EXPECT_EQ(rater.getSampleCount(), 2);

  rater.setAOF(&aof);
  EXPECT_EQ(rater.getSampleCount(), 0);
}
