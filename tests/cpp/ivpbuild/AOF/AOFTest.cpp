#include <gtest/gtest.h>

#include <cmath>
#include <sstream>
#include <vector>

#include "AOF_Gaussian.h"
#include "AOF_Linear.h"
#include "AOF_MGaussian.h"
#include "AOF_Quadratic.h"
#include "AOF_Ring.h"
#include "AOF_Rings.h"
#include "IvPBuildTestUtils.h"
#include "NumericAssertions.h"

namespace {

IvPDomain makeFFViewDomain()
{
  IvPDomain domain;
  domain.addDomain("x", 0, 500, 501);
  domain.addDomain("y", 0, 500, 501);
  return domain;
}

IvPDomain makeFFViewSignedDomain()
{
  IvPDomain domain;
  domain.addDomain("x", -250, 249, 500);
  domain.addDomain("y", -250, 249, 500);
  return domain;
}

AOF_Ring makeFFViewRing(const IvPDomain& domain,
                        const std::string& location,
                        double radius,
                        double exponent,
                        double base,
                        double range,
                        const std::string& peak)
{
  AOF_Ring ring(domain);
  ring.setParam("location", location);
  ring.setParam("radius", radius);
  ring.setParam("exp", exponent);
  ring.setParam("base", base);
  ring.setParam("range", range);
  ring.setParam("peak", peak);
  return ring;
}

}  // namespace

TEST(AOFLinearTest, EvaluatesMissionXYPayloadStyleLinearUtility)
{
  AOF_Linear aof(makeXYDomain());
  EXPECT_TRUE(aof.setParam("mcoeff", 2));
  EXPECT_TRUE(aof.setParam("ncoeff", -1));
  EXPECT_TRUE(aof.setParam("bscalar", 5));
  EXPECT_FALSE(aof.setParam("unknown", 1));

  IvPBox point = makePointBox(2, {10, 20});
  EXPECT_NEAR(aof.evalBox(&point), 5.0, kGeomTol);
}

TEST(AOFLinearTest, ReplaysAppFFViewLinearDataFileDomainAndCoefficients)
{
  AOF_Linear aof(makeFFViewSignedDomain());
  ASSERT_TRUE(aof.setParam("mcoeff", 0.3));
  ASSERT_TRUE(aof.setParam("ncoeff", 0));
  ASSERT_TRUE(aof.setParam("bscalar", 8));

  IvPBox low_x = makePointBox(2, {0, 250});
  IvPBox zero_x = makePointBox(2, {250, 250});
  IvPBox high_x = makePointBox(2, {499, 250});

  EXPECT_NEAR(aof.evalBox(&low_x), -67.0, kGeomTol);
  EXPECT_NEAR(aof.evalBox(&zero_x), 8.0, kGeomTol);
  EXPECT_NEAR(aof.evalBox(&high_x), 82.7, kLooseGeomTol);
}

TEST(AOFQuadraticTest, RequiresXYDomainAndEvaluatesOffsetBowl)
{
  AOF_Quadratic aof(makeXYDomain());
  ASSERT_TRUE(aof.initialize());
  EXPECT_TRUE(aof.setParam("mcoeff", 2));
  EXPECT_TRUE(aof.setParam("ncoeff", 3));
  EXPECT_TRUE(aof.setParam("x_center", 40));
  EXPECT_TRUE(aof.setParam("y_center", 50));
  EXPECT_FALSE(aof.setParam("bogus", 1));

  IvPBox center = makePointBox(2, {40, 50});
  IvPBox offset = makePointBox(2, {43, 54});
  EXPECT_NEAR(aof.evalBox(&center), 0.0, kGeomTol);
  EXPECT_NEAR(aof.evalBox(&offset), 66.0, kGeomTol);

  IvPDomain speed_only;
  speed_only.addDomain("speed", 0, 5, 6);
  AOF_Quadratic missing_xy(speed_only);
  EXPECT_FALSE(missing_xy.initialize());
}

TEST(AOFGaussianTest, EvaluatesCenterAndSigmaFalloff)
{
  AOF_Gaussian aof(makeXYDomain());
  EXPECT_TRUE(aof.setParam("xcent", 50));
  EXPECT_TRUE(aof.setParam("ycent", 40));
  EXPECT_TRUE(aof.setParam("sigma", 10));
  EXPECT_TRUE(aof.setParam("range", 100));
  EXPECT_FALSE(aof.setParam("unknown", 1));

  EXPECT_NEAR(aof.evalPoint({50, 40}), 100.0, kGeomTol);
  EXPECT_NEAR(aof.evalPoint({60, 40}), 60.65306597, kLooseGeomTol);

  IvPBox point = makePointBox(2, {60, 40});
  EXPECT_NEAR(aof.evalBox(&point), aof.evalPoint({60, 40}), kGeomTol);
}

TEST(AOFGaussianTest, ReplaysAppFFViewGaussianDataFileWithDomainValues)
{
  AOF_Gaussian aof(makeFFViewDomain());
  ASSERT_TRUE(aof.setParam("xcent", 50));
  ASSERT_TRUE(aof.setParam("ycent", 400));
  ASSERT_TRUE(aof.setParam("sigma", 22.4));
  ASSERT_TRUE(aof.setParam("range", 150));

  IvPBox center = makePointBox(2, {50, 400});
  IvPBox y_offset = makePointBox(2, {50, 422});
  const double expected =
      150.0 * std::exp(-((22.0 * 22.0) / (2.0 * 22.4 * 22.4)));

  EXPECT_NEAR(aof.evalBox(&center), 150.0, kGeomTol);
  EXPECT_NEAR(aof.evalBox(&y_offset), expected, kLooseGeomTol);
  EXPECT_NEAR(aof.evalPoint({50, 422}), expected, kLooseGeomTol);
}

TEST(AOFMultiGaussianTest, SumsConfiguredMissionPeaksAndRejectsWrongPointDim)
{
  AOF_MGaussian aof(makeXYDomain());
  EXPECT_TRUE(aof.setParam("unused", "xcent=0,ycent=0,range=100,sigma=10"));
  EXPECT_TRUE(aof.setParam("unused", "xcent=10,ycent=0,range=50,sigma=5"));

  const double expected = 100.0 + 50.0 * std::exp(-(10.0 * 10.0) / (2.0 * 5.0 * 5.0));
  EXPECT_NEAR(aof.evalPoint({0, 0}), expected, kLooseGeomTol);
  EXPECT_NEAR(aof.evalPoint({0}), 0.0, kGeomTol);
}

TEST(AOFRingTest, PinsRingPeakValleyAndGradientModes)
{
  AOF_Ring ring(makeXYDomain());
  EXPECT_TRUE(ring.setParam("location", "50,50"));
  EXPECT_TRUE(ring.setParam("peak", "true"));
  EXPECT_TRUE(ring.setParam("base", 10));
  EXPECT_TRUE(ring.setParam("range", 90));
  EXPECT_TRUE(ring.setParam("radius", 10));
  EXPECT_TRUE(ring.setParam("exp", 2));
  EXPECT_TRUE(ring.setParam("plateau", 0));
  EXPECT_TRUE(ring.setParam("gradient_dist", 100));
  EXPECT_TRUE(ring.setParam("gradient_type", "linear"));
  EXPECT_FALSE(ring.setParam("gradient_type", "unknown"));
  EXPECT_FALSE(ring.setParam("location", "50"));
  EXPECT_FALSE(ring.setParam("peak", "maybe"));
  EXPECT_FALSE(ring.setParam("unknown", 1));

  IvPBox on_ring = makePointBox(2, {60, 50});
  IvPBox center = makePointBox(2, {50, 50});
  EXPECT_NEAR(ring.evalBox(&on_ring), 100.0, kGeomTol);
  EXPECT_LT(ring.evalBox(&center), ring.evalBox(&on_ring));
  EXPECT_EQ(ring.getRAD(), 10u);
  EXPECT_EQ(ring.getEXP(), 2u);
  EXPECT_TRUE(ring.getRPK());
  EXPECT_NEAR(ring.getRNG(), 90.0, kGeomTol);
  EXPECT_NEAR(ring.getBAS(), 10.0, kGeomTol);
  EXPECT_EQ(ring.getLOC().pt(0), 50);
  EXPECT_TRUE(ring.latexSTR(1).find("f(x, y)") != std::string::npos);

  EXPECT_TRUE(ring.setParam("peak", "false"));
  EXPECT_GT(ring.evalBox(&center), ring.evalBox(&on_ring));
  EXPECT_TRUE(ring.setParam("gradient_type", "sigmoid"));
  EXPECT_TRUE(std::isfinite(ring.evalBox(&center)));
  EXPECT_TRUE(ring.setParam("gradient_type", "exponential"));
  EXPECT_TRUE(std::isfinite(ring.evalBox(&center)));
}

TEST(AOFRingTest, LinearGradientPlateauCreatesBandAroundConfiguredRadius)
{
  AOF_Ring ring(makeXYDomain());
  ASSERT_TRUE(ring.setParam("location", "50,50"));
  ASSERT_TRUE(ring.setParam("peak", "true"));
  ASSERT_TRUE(ring.setParam("base", 10));
  ASSERT_TRUE(ring.setParam("range", 90));
  ASSERT_TRUE(ring.setParam("radius", 10));
  ASSERT_TRUE(ring.setParam("exp", 1));
  ASSERT_TRUE(ring.setParam("plateau", 5));
  ASSERT_TRUE(ring.setParam("gradient_dist", 100));
  ASSERT_TRUE(ring.setParam("gradient_type", "linear"));

  IvPBox inner_plateau_edge = makePointBox(2, {55, 50});
  IvPBox on_ring = makePointBox(2, {60, 50});
  IvPBox outer_plateau_edge = makePointBox(2, {65, 50});
  IvPBox center = makePointBox(2, {50, 50});

  EXPECT_NEAR(ring.evalBox(&inner_plateau_edge), 100.0, kGeomTol);
  EXPECT_NEAR(ring.evalBox(&on_ring), 100.0, kGeomTol);
  EXPECT_NEAR(ring.evalBox(&outer_plateau_edge), 100.0, kGeomTol);
  EXPECT_LT(ring.evalBox(&center), 100.0);
}

TEST(AOFRingsTest, AggregatesMultipleFfviewStyleRingSpecs)
{
  AOF_Rings rings(makeXYDomain());
  EXPECT_EQ(rings.size(), 0);
  EXPECT_FALSE(rings.setParam("radius", 10));

  EXPECT_TRUE(rings.setParam("location", "25,25"));
  EXPECT_TRUE(rings.setParam("radius", 5));
  EXPECT_TRUE(rings.setParam("range", 100));
  EXPECT_TRUE(rings.setParam("base", 0));
  EXPECT_TRUE(rings.setParam("peak", "true"));
  EXPECT_TRUE(rings.setParam("location", "75,75"));
  EXPECT_TRUE(rings.setParam("radius", 10));
  EXPECT_TRUE(rings.setParam("range", 80));
  EXPECT_TRUE(rings.setParam("base", 5));
  EXPECT_TRUE(rings.setParam("snapval", 5));

  EXPECT_EQ(rings.size(), 2);
  IvPBox sample = makePointBox(2, {30, 25});
  EXPECT_TRUE(std::isfinite(rings.evalBox(&sample)));
  EXPECT_EQ(std::fmod(std::fabs(rings.evalBox(&sample)), 5.0), 0.0);
  EXPECT_TRUE(rings.latexSTR(1).find("f(x, y)") != std::string::npos);
}

TEST(AOFRingsTest, ReplaysAppFFViewNewRing06DataFileSemantics)
{
  IvPDomain domain = makeFFViewDomain();
  AOF_Rings rings(domain);

  ASSERT_TRUE(rings.setParam("location", "344,264"));
  EXPECT_TRUE(rings.setParam("radius", 3));
  EXPECT_TRUE(rings.setParam("exp", 1));
  EXPECT_TRUE(rings.setParam("base", -100));
  EXPECT_TRUE(rings.setParam("range", 200));
  EXPECT_TRUE(rings.setParam("peak", "false"));

  ASSERT_TRUE(rings.setParam("location", "346,46"));
  EXPECT_TRUE(rings.setParam("radius", 11));
  EXPECT_TRUE(rings.setParam("exp", 14));
  EXPECT_TRUE(rings.setParam("base", -100));
  EXPECT_TRUE(rings.setParam("range", 200));
  EXPECT_TRUE(rings.setParam("peak", "true"));

  ASSERT_TRUE(rings.setParam("location", "298,461"));
  EXPECT_TRUE(rings.setParam("radius", 17));
  EXPECT_TRUE(rings.setParam("exp", 12));
  EXPECT_TRUE(rings.setParam("base", -100));
  EXPECT_TRUE(rings.setParam("range", 200));
  EXPECT_TRUE(rings.setParam("peak", "false"));

  ASSERT_TRUE(rings.setParam("location", "357,424"));
  EXPECT_TRUE(rings.setParam("radius", 15));
  EXPECT_TRUE(rings.setParam("exp", 14));
  EXPECT_TRUE(rings.setParam("base", -100));
  EXPECT_TRUE(rings.setParam("range", 200));
  EXPECT_TRUE(rings.setParam("peak", "false"));

  std::vector<AOF_Ring> expected_rings;
  expected_rings.push_back(
      makeFFViewRing(domain, "344,264", 3, 1, -100, 200, "false"));
  expected_rings.push_back(
      makeFFViewRing(domain, "346,46", 11, 14, -100, 200, "true"));
  expected_rings.push_back(
      makeFFViewRing(domain, "298,461", 17, 12, -100, 200, "false"));
  expected_rings.push_back(
      makeFFViewRing(domain, "357,424", 15, 14, -100, 200, "false"));

  for(const std::vector<int>& pt :
      {std::vector<int>{346, 57}, std::vector<int>{344, 264},
       std::vector<int>{298, 461}, std::vector<int>{250, 250}}) {
    IvPBox sample = makePointBox(2, pt);
    double expected = 0.0;
    for(const AOF_Ring& ring : expected_rings)
      expected += ring.evalBox(&sample);
    expected /= expected_rings.size();

    EXPECT_NEAR(rings.evalBox(&sample), expected, kGeomTol);
    EXPECT_TRUE(std::isfinite(rings.evalBox(&sample)));
  }
}
