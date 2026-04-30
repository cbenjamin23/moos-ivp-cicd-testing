#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "BoxSet.h"
#include "CompactorNull.h"
#include "IvPBox.h"
#include "IvPBuildTestUtils.h"
#include "IvPDomain.h"
#include "IvPFunction.h"
#include "IvPGrid.h"
#include "PDMap.h"

namespace {

IvPDomain makeCourseOnlyDomain(unsigned int points = 5)
{
  IvPDomain domain;
  domain.addDomain("course", 0, points - 1, points);
  return domain;
}

IvPBox intervalBox(int low, int high, double intercept)
{
  IvPBox box(1, 1);
  box.setPTS(0, low, high);
  box.wt(0) = 1;
  box.wt(1) = intercept;
  return box;
}

IvPBox courseSpeedBox(int crs_low,
                      int crs_high,
                      int spd_low,
                      int spd_high,
                      double crs_weight,
                      double spd_weight,
                      double intercept)
{
  IvPBox box(2, 1);
  box.setPTS(0, crs_low, crs_high);
  box.setPTS(1, spd_low, spd_high);
  box.wt(0) = crs_weight;
  box.wt(1) = spd_weight;
  box.wt(2) = intercept;
  return box;
}

std::unique_ptr<PDMap> makeTwoPieceCourseMap()
{
  IvPDomain domain = makeCourseOnlyDomain();
  std::unique_ptr<PDMap> pdmap(new PDMap(2, domain, 1));
  pdmap->bx(0) = new IvPBox(intervalBox(0, 2, 10));
  pdmap->bx(1) = new IvPBox(intervalBox(3, 4, 20));
  pdmap->bx(0)->ofindex() = 0;
  pdmap->bx(1)->ofindex() = 1;
  return pdmap;
}

std::unique_ptr<PDMap> makeFourPieceCourseSpeedMap()
{
  IvPDomain domain = makeCourseSpeedDomain();
  std::unique_ptr<PDMap> pdmap(new PDMap(4, domain, 1));
  pdmap->bx(0) = new IvPBox(courseSpeedBox(0, 179, 0, 2, 0.1, 1, 10));
  pdmap->bx(1) = new IvPBox(courseSpeedBox(0, 179, 3, 5, 0.1, 1, 20));
  pdmap->bx(2) = new IvPBox(courseSpeedBox(180, 359, 0, 2, -0.05, 2, 50));
  pdmap->bx(3) = new IvPBox(courseSpeedBox(180, 359, 3, 5, -0.05, 2, 60));
  for(int i = 0; i < 4; ++i)
    pdmap->bx(i)->ofindex() = i;
  return pdmap;
}

IvPDomain makeTinyXYDomain()
{
  IvPDomain domain;
  domain.addDomain("x", 0, 3, 4);
  domain.addDomain("y", 0, 2, 3);
  return domain;
}

std::unique_ptr<PDMap> makeTinyDenseXYMap()
{
  IvPDomain domain = makeTinyXYDomain();
  std::unique_ptr<PDMap> pdmap(new PDMap(4, domain, 1));
  pdmap->bx(0) = new IvPBox(courseSpeedBox(0, 1, 0, 1, 2, 3, 10));
  pdmap->bx(1) = new IvPBox(courseSpeedBox(2, 3, 0, 1, -1, 4, 20));
  pdmap->bx(2) = new IvPBox(courseSpeedBox(0, 1, 2, 2, 5, 0, 30));
  pdmap->bx(3) = new IvPBox(courseSpeedBox(2, 3, 2, 2, 0, -2, 40));
  for(int i = 0; i < 4; ++i)
    pdmap->bx(i)->ofindex() = i;
  return pdmap;
}

double tinyDenseExpectedValue(int x, int y)
{
  if(x <= 1 && y <= 1)
    return (2 * x) + (3 * y) + 10;
  if(x >= 2 && y <= 1)
    return (-1 * x) + (4 * y) + 20;
  if(x <= 1 && y == 2)
    return (5 * x) + 30;
  return (-2 * y) + 40;
}

int tinyDenseExpectedIndex(int x, int y)
{
  if(x <= 1 && y <= 1)
    return 0;
  if(x >= 2 && y <= 1)
    return 1;
  if(x <= 1 && y == 2)
    return 2;
  return 3;
}

unsigned int countPointCoverage(const PDMap& pdmap, const IvPBox& point)
{
  unsigned int count = 0;
  for(int i = 0; i < pdmap.size(); ++i) {
    const IvPBox* box = pdmap.getBox(i);
    if(box && point.intersect(box))
      ++count;
  }
  return count;
}

}  // namespace

// Covers IvP domain behavior: builds course speed domain with expected discrete values.
TEST(IvPDomainTest, BuildsCourseSpeedDomainWithExpectedDiscreteValues)
{
  IvPDomain domain;
  EXPECT_TRUE(domain.addDomain("course", 0, 359, 360));
  EXPECT_TRUE(domain.addDomain("speed", 0, 5, 51));
  EXPECT_FALSE(domain.addDomain("course", 0, 10, 11));
  EXPECT_FALSE(domain.addDomain("bad", 10, 0, 11));
  EXPECT_FALSE(domain.addDomain("single", 0, 1, 1));
  EXPECT_TRUE(domain.hasOnlyDomain("course", "speed"));
  EXPECT_TRUE(domain.addDomain("depth", 10, 10, 1));

  EXPECT_EQ(domain.size(), 3u);
  EXPECT_FALSE(domain.hasOnlyDomain("course", "speed"));
  EXPECT_FALSE(domain.hasOnlyDomain("course"));
  EXPECT_EQ(domain.getIndex("speed"), 1);
  EXPECT_DOUBLE_EQ(domain.getVarDelta("speed"), 0.1);
  EXPECT_DOUBLE_EQ(domain.getTotalPts(), 360.0 * 51.0);

  double value = 0;
  EXPECT_TRUE(domain.getVal("speed", 25, value));
  EXPECT_DOUBLE_EQ(value, 2.5);
  EXPECT_FALSE(domain.getVal("speed", 51, value));
  EXPECT_FALSE(domain.getVal("missing", 3, value));
}

// Covers IvP domain behavior: snaps and steps values for helm decision domains.
TEST(IvPDomainTest, SnapsAndStepsValuesForHelmDecisionDomains)
{
  IvPDomain domain;
  domain.addDomain("speed", 2, 7, 11);

  EXPECT_EQ(domain.getDiscreteVal(0, 2.1, 0), 0u);
  EXPECT_EQ(domain.getDiscreteVal(0, 2.1, 1), 1u);
  EXPECT_EQ(domain.getDiscreteVal(0, 2.3, 2), 1u);
  EXPECT_DOUBLE_EQ(domain.getSnappedValFloor(0, 2.9), 2.5);
  EXPECT_DOUBLE_EQ(domain.getSnappedValCeil(0, 2.1), 2.5);
  EXPECT_DOUBLE_EQ(domain.getSnappedValProx(0, 2.24), 2);

  EXPECT_DOUBLE_EQ(domain.getNextLowerVal(0, 2, 2, false), 2);
  EXPECT_DOUBLE_EQ(domain.getNextLowerVal(0, 2, 2, true), 7);
  EXPECT_DOUBLE_EQ(domain.getNextHigherVal(0, 7, 2, false), 7);
  EXPECT_DOUBLE_EQ(domain.getNextHigherVal(0, 7, 2, true), 2);
  EXPECT_DOUBLE_EQ(domain.getEqOrLowerVal(0, 2.5, 2, true), 2.5);
  EXPECT_DOUBLE_EQ(domain.getEqOrHigherVal(0, 2.6, 0, false), 3);
}

// Covers IvP domain behavior: handles single point domains and invalid snap requests.
TEST(IvPDomainTest, HandlesSinglePointDomainsAndInvalidSnapRequests)
{
  IvPDomain domain;
  ASSERT_TRUE(domain.addDomain("depth", 12, 12, 1));
  EXPECT_DOUBLE_EQ(domain.getVarDelta("depth"), 0);
  EXPECT_EQ(domain.getVarPoints("missing"), 0u);
  EXPECT_EQ(domain.getVarPoints(7), 0u);
  EXPECT_EQ(domain.getVarName(7), "");

  double value = -1;
  EXPECT_TRUE(domain.getVal("depth", 0, value));
  EXPECT_DOUBLE_EQ(value, 12);
  EXPECT_FALSE(domain.getVal("depth", 1, value));

  EXPECT_EQ(domain.getDiscreteVal(9, 99, 0), 0u);
  EXPECT_DOUBLE_EQ(domain.getSnappedVal(9, 99, 0), 99);
  EXPECT_DOUBLE_EQ(domain.getSnappedVal(0, 99, 9), 99);
  EXPECT_DOUBLE_EQ(domain.getNextLowerVal(9, 99, 0, true), 99);
  EXPECT_DOUBLE_EQ(domain.getNextHigherVal(9, 99, 0, true), 99);

  IvPDomain helm_domain;
  helm_domain.addDomain("course", 0, 359, 360);
  helm_domain.addDomain("speed", 0, 5, 6);
  EXPECT_EQ(helm_domain.getDiscreteVal(0, -15, 0), 0u);
  EXPECT_EQ(helm_domain.getDiscreteVal(0, 720, 1), 359u);
  EXPECT_DOUBLE_EQ(helm_domain.getNextLowerVal(0, 0, 0, true), 359);
  EXPECT_DOUBLE_EQ(helm_domain.getNextHigherVal(0, 359, 0, true), 0);
  EXPECT_DOUBLE_EQ(helm_domain.getNextLowerVal(7, 3.3, 0), 3.3);
  EXPECT_DOUBLE_EQ(helm_domain.getNextHigherVal(1, 3.3, 8), 3.3);
}

// Covers IvP domain behavior: reports bounds and empty domain fallbacks.
TEST(IvPDomainTest, ReportsBoundsAndEmptyDomainFallbacks)
{
  IvPDomain empty;
  EXPECT_EQ(empty.size(), 0u);
  EXPECT_DOUBLE_EQ(empty.getTotalPts(), 1);
  EXPECT_FALSE(empty.hasDomain("course"));
  EXPECT_FALSE(empty.hasOnlyDomain("course"));
  EXPECT_EQ(empty.getIndex("course"), -1);
  EXPECT_EQ(empty.getVarName(0), "");
  EXPECT_DOUBLE_EQ(empty.getVarLow("course"), 0);
  EXPECT_DOUBLE_EQ(empty.getVarHigh("course"), 0);
  EXPECT_DOUBLE_EQ(empty.getVarDelta("course"), 0);
  EXPECT_EQ(empty.getVarPoints("course"), 0u);
  EXPECT_EQ(empty.getVarPoints(0), 0u);

  double value = 7;
  EXPECT_FALSE(empty.getVal("course", 0, value));
  EXPECT_FALSE(empty.getVal(0, 0, value));
  EXPECT_DOUBLE_EQ(empty.getVal(0, 0), 0);

  IvPDomain domain;
  ASSERT_TRUE(domain.addDomain("course", -180, 180, 361));
  EXPECT_DOUBLE_EQ(domain.getVarLow("course"), -180);
  EXPECT_DOUBLE_EQ(domain.getVarHigh("course"), 180);
  EXPECT_DOUBLE_EQ(domain.getVarDelta("course"), 1);
  EXPECT_DOUBLE_EQ(domain.getVal(0, 180), 0);
}

// Covers IvP domain behavior: handles eq or step values around interior and edges.
TEST(IvPDomainTest, HandlesEqOrStepValuesAroundInteriorAndEdges)
{
  IvPDomain domain;
  ASSERT_TRUE(domain.addDomain("speed", 0, 5, 6));

  EXPECT_DOUBLE_EQ(domain.getEqOrLowerVal(0, 2, 0, false), 2);
  EXPECT_DOUBLE_EQ(domain.getEqOrHigherVal(0, 2, 0, false), 2);
  EXPECT_DOUBLE_EQ(domain.getEqOrLowerVal(0, 2.4, 0, false), 1);
  EXPECT_DOUBLE_EQ(domain.getEqOrHigherVal(0, 2.4, 0, false), 3);

  EXPECT_DOUBLE_EQ(domain.getEqOrLowerVal(0, 0.2, 0, false), 0.2);
  EXPECT_DOUBLE_EQ(domain.getEqOrLowerVal(0, 0.2, 0, true), 5);
  EXPECT_DOUBLE_EQ(domain.getEqOrHigherVal(0, 4.8, 1, false), 4.8);
  EXPECT_DOUBLE_EQ(domain.getEqOrHigherVal(0, 4.8, 1, true), 0);

  EXPECT_DOUBLE_EQ(domain.getNextLowerVal(0, 3, 0, false), 2);
  EXPECT_DOUBLE_EQ(domain.getNextHigherVal(0, 3, 0, false), 4);
}

// Covers IvP domain behavior: snaps signed error domains around zero.
TEST(IvPDomainTest, SnapsSignedErrorDomainsAroundZero)
{
  IvPDomain domain;
  ASSERT_TRUE(domain.addDomain("heading_error", -180, 180, 361));

  EXPECT_EQ(domain.getDiscreteVal(0, -179.2, 0), 0u);
  EXPECT_EQ(domain.getDiscreteVal(0, -179.2, 1), 1u);
  EXPECT_EQ(domain.getDiscreteVal(0, -0.49, 2), 180u);
  EXPECT_EQ(domain.getDiscreteVal(0, 0.51, 2), 181u);
  EXPECT_DOUBLE_EQ(domain.getSnappedValFloor(0, -0.25), -1);
  EXPECT_DOUBLE_EQ(domain.getSnappedValCeil(0, -0.25), 0);
  EXPECT_DOUBLE_EQ(domain.getSnappedValProx(0, -0.5), -1);
  EXPECT_DOUBLE_EQ(domain.getEqOrLowerVal(0, -180, 2, true), -180);
  EXPECT_DOUBLE_EQ(domain.getNextLowerVal(0, -180, 2, true), 180);
  EXPECT_DOUBLE_EQ(domain.getNextHigherVal(0, 180, 2, true), -180);
}

// Covers IvP domain behavior: pins snap helper tie breaks and missing domain quirks.
TEST(IvPDomainTest, PinsSnapHelperTieBreaksAndMissingDomainQuirks)
{
  IvPDomain domain;
  ASSERT_TRUE(domain.addDomain("speed", 0, 5, 6));

  EXPECT_EQ(domain.getDiscreteVal(0, 2.5, 2), 2u);
  EXPECT_DOUBLE_EQ(domain.getSnappedValProx(0, 2.5), 2);
  EXPECT_DOUBLE_EQ(domain.getSnappedValFloor(0, -10), 0);
  EXPECT_DOUBLE_EQ(domain.getSnappedValCeil(0, 99), 5);

  EXPECT_DOUBLE_EQ(domain.getSnappedVal(9, 99, 0), 99);
  EXPECT_DOUBLE_EQ(domain.getSnappedValFloor(9, 99), 0);
  EXPECT_DOUBLE_EQ(domain.getSnappedValCeil(9, 99), 0);
  EXPECT_DOUBLE_EQ(domain.getSnappedValProx(9, 99), 0);
}

// Covers IvP domain behavior: copies named variables and compares ordered domains.
TEST(IvPDomainTest, CopiesNamedVariablesAndComparesOrderedDomains)
{
  IvPDomain source = makeCourseSpeedDomain();
  IvPDomain copied;
  EXPECT_TRUE(copied.addDomain(source, "speed"));
  EXPECT_FALSE(copied.addDomain(source, "missing"));
  EXPECT_EQ(copied.getVarName(0), "speed");
  EXPECT_EQ(copied.getVarPoints(0), 6u);

  IvPDomain same = source;
  EXPECT_TRUE(same == source);
  IvPDomain reordered;
  reordered.addDomain(source, "speed");
  reordered.addDomain(source, "course");
  EXPECT_FALSE(reordered == source);
  same.clear();
  EXPECT_EQ(same.size(), 0u);
}

// Covers IvP domain behavior: has only domain requires distinct requested variables.
TEST(IvPDomainTest, HasOnlyDomainRequiresDistinctRequestedVariables)
{
  IvPDomain domain = makeCourseSpeedDomain();

  EXPECT_TRUE(domain.hasOnlyDomain("course", "speed"));
  EXPECT_TRUE(domain.hasOnlyDomain("speed", "course"));
  EXPECT_FALSE(domain.hasOnlyDomain("course", "course"));
  EXPECT_FALSE(domain.hasOnlyDomain("course"));
  EXPECT_FALSE(domain.hasOnlyDomain("missing", "speed"));
}

// Covers IvP box behavior: evaluates linear pieces and finds extrema.
TEST(IvPBoxTest, EvaluatesLinearPiecesAndFindsExtrema)
{
  IvPBox box(2, 1);
  box.setPTS(0, 0, 10);
  box.setPTS(1, 2, 5);
  box.wt(0) = 2;
  box.wt(1) = -3;
  box.wt(2) = 7;

  EXPECT_DOUBLE_EQ(box.maxVal(), 21);
  EXPECT_DOUBLE_EQ(box.minVal(), -8);
  IvPBox maxpt = box.maxPt();
  EXPECT_EQ(maxpt.pt(0), 10);
  EXPECT_EQ(maxpt.pt(1), 2);
  EXPECT_TRUE(maxpt.isPtBox());

  IvPBox point(2);
  point.setPTS(0, 4, 4);
  point.setPTS(1, 3, 3);
  EXPECT_DOUBLE_EQ(box.ptVal(&point), 6);

  box.moveIntercept(4);
  box.scaleWT(0.5);
  EXPECT_DOUBLE_EQ(box.ptVal(&point), 5);
}

// Covers IvP box behavior: max pt leaves wrong dimension output unchanged.
TEST(IvPBoxTest, MaxPtLeavesWrongDimensionOutputUnchanged)
{
  IvPBox box(2, 1);
  box.setPTS(0, 0, 10);
  box.setPTS(1, 0, 5);
  box.wt(0) = 1;
  box.wt(1) = 1;
  box.wt(2) = 0;

  IvPBox wrong_dim(1);
  wrong_dim.setPTS(0, 3, 3);
  box.maxPt(wrong_dim);

  EXPECT_EQ(wrong_dim.getDim(), 1);
  EXPECT_EQ(wrong_dim.pt(0, 0), 3);
  EXPECT_EQ(wrong_dim.pt(0, 1), 3);
}

// Covers IvP box behavior: handles constant pieces and set wt intercept reset.
TEST(IvPBoxTest, HandlesConstantPiecesAndSetWTInterceptReset)
{
  IvPBox box(2, 0);
  box.setPTS(0, 0, 10);
  box.setPTS(1, 0, 4);
  box.setWT(42);

  EXPECT_EQ(box.getDegree(), 0);
  EXPECT_EQ(box.getWtc(), 1);
  EXPECT_DOUBLE_EQ(box.maxVal(), 42);
  EXPECT_DOUBLE_EQ(box.minVal(), 42);

  IvPBox maxpt = box.maxPt();
  EXPECT_TRUE(maxpt.isPtBox());
  EXPECT_EQ(maxpt.pt(0), 5);
  EXPECT_EQ(maxpt.pt(1), 2);

  IvPBox point(2);
  point.setPTS(0, 7, 7);
  point.setPTS(1, 3, 3);
  EXPECT_DOUBLE_EQ(box.ptVal(&point), 42);
}

// Covers IvP box behavior: set wt resets linear slope components.
TEST(IvPBoxTest, SetWTResetsLinearSlopeComponents)
{
  IvPBox box(2, 1);
  box.setPTS(0, 0, 10);
  box.setPTS(1, 0, 20);
  box.wt(0) = 4;
  box.wt(1) = -8;
  box.wt(2) = 16;

  box.setWT(12);
  EXPECT_DOUBLE_EQ(box.wt(0), 0);
  EXPECT_DOUBLE_EQ(box.wt(1), 0);
  EXPECT_DOUBLE_EQ(box.wt(2), 12);
  EXPECT_DOUBLE_EQ(box.maxVal(), 12);
  EXPECT_DOUBLE_EQ(box.minVal(), 12);

  IvPBox point(2);
  point.setPTS(0, 7, 7);
  point.setPTS(1, 19, 19);
  EXPECT_DOUBLE_EQ(box.ptVal(&point), 12);
}

// Covers IvP box behavior: negative scaling swaps linear extrema.
TEST(IvPBoxTest, NegativeScalingSwapsLinearExtrema)
{
  IvPBox box = intervalBox(0, 4, 10);
  EXPECT_DOUBLE_EQ(box.minVal(), 10);
  EXPECT_DOUBLE_EQ(box.maxVal(), 14);

  box.scaleWT(-2);
  EXPECT_DOUBLE_EQ(box.maxVal(), -20);
  EXPECT_DOUBLE_EQ(box.minVal(), -28);

  IvPBox maxpt = box.maxPt();
  EXPECT_TRUE(maxpt.isPtBox());
  EXPECT_EQ(maxpt.pt(0), 0);

  IvPBox point = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(box.ptVal(&point), -28);
}

// Covers IvP box behavior: set wt resets quadratic coefficients to constant utility.
TEST(IvPBoxTest, SetWTResetsQuadraticCoefficientsToConstantUtility)
{
  IvPBox box(2, 2);
  box.setPTS(0, 0, 4);
  box.setPTS(1, 1, 3);
  box.wt(0) = 1;
  box.wt(1) = -2;
  box.wt(2) = 3;
  box.wt(3) = -4;
  box.wt(4) = 5;

  box.setWT(17);

  for(int i = 0; i < box.getWtc() - 1; ++i)
    EXPECT_DOUBLE_EQ(box.wt(i), 0) << "wt index " << i;
  EXPECT_DOUBLE_EQ(box.wt(box.getWtc() - 1), 17);
  EXPECT_DOUBLE_EQ(box.minVal(), 17);
  EXPECT_DOUBLE_EQ(box.maxVal(), 17);

  IvPBox point = makePointBox(2, {4, 3});
  EXPECT_DOUBLE_EQ(box.ptVal(&point), 17);
}

// Covers IvP box behavior: evaluates quadratic pieces used by second degree approximations.
TEST(IvPBoxTest, EvaluatesQuadraticPiecesUsedBySecondDegreeApproximations)
{
  IvPBox box(1, 2);
  box.setPTS(0, 0, 4);
  box.wt(0) = 1;
  box.wt(1) = 2;
  box.wt(2) = 3;

  IvPBox point = makeOneDimPoint(3);
  EXPECT_DOUBLE_EQ(box.ptVal(&point), 18);
  EXPECT_DOUBLE_EQ(box.minVal(), 3);
  EXPECT_DOUBLE_EQ(box.maxVal(), 27);

  IvPBox maxpt = box.maxPt();
  EXPECT_TRUE(maxpt.isPtBox());
  EXPECT_EQ(maxpt.pt(0), 4);
}

// Covers IvP box behavior: pins size as legacy sum of upper bounds.
TEST(IvPBoxTest, PinsSizeAsLegacySumOfUpperBounds)
{
  IvPBox box(2);
  box.setPTS(0, 2, 5);
  box.setPTS(1, 3, 7);

  EXPECT_EQ(box.size(), 12u);
}

// Covers IvP box behavior: handles point boxes and boundary intersections.
TEST(IvPBoxTest, HandlesPointBoxesAndBoundaryIntersections)
{
  IvPBox closed_point(1);
  closed_point.setPTS(0, 5, 5);
  EXPECT_TRUE(closed_point.isPtBox());

  IvPBox open_point(1);
  open_point.setPTS(0, 5, 5);
  open_point.setBDS(0, false, true);
  EXPECT_FALSE(open_point.isPtBox());

  IvPBox half_open_unit(1);
  half_open_unit.setPTS(0, 5, 6);
  half_open_unit.setBDS(0, false, false);
  EXPECT_TRUE(half_open_unit.isPtBox());

  IvPBox left_closed_unit(1);
  left_closed_unit.setPTS(0, 5, 6);
  left_closed_unit.setBDS(0, true, false);
  EXPECT_FALSE(left_closed_unit.isPtBox());

  IvPBox right_closed_unit(1);
  right_closed_unit.setPTS(0, 5, 6);
  right_closed_unit.setBDS(0, false, true);
  EXPECT_FALSE(right_closed_unit.isPtBox());

  IvPBox left(1);
  left.setPTS(0, 0, 5);
  IvPBox right(1);
  right.setPTS(0, 5, 10);
  EXPECT_TRUE(left.intersect(&right));
  right.setBDS(0, false, true);
  EXPECT_FALSE(left.intersect(&right));

  right.setBDS(0, true, true);
  IvPBox* intersection = nullptr;
  ASSERT_TRUE(left.intersect(&right, intersection));
  ASSERT_NE(intersection, nullptr);
  EXPECT_EQ(intersection->pt(0, 0), 5);
  EXPECT_EQ(intersection->pt(0, 1), 5);
  delete intersection;
}

// Covers IvP box behavior: non intersecting boxes leave output unallocated.
TEST(IvPBoxTest, NonIntersectingBoxesLeaveOutputUnallocated)
{
  IvPBox left(1);
  left.setPTS(0, 0, 1);
  IvPBox right(1);
  right.setPTS(0, 3, 4);

  IvPBox* intersection = nullptr;
  EXPECT_FALSE(left.intersect(&right));
  EXPECT_FALSE(left.intersect(&right, intersection));
  EXPECT_EQ(intersection, nullptr);
}

// Covers IvP box behavior: intersected boxes combine open bounds and interior weights.
TEST(IvPBoxTest, IntersectedBoxesCombineOpenBoundsAndInteriorWeights)
{
  IvPBox left(1, 1);
  left.setPTS(0, 0, 5);
  left.setBDS(0, true, false);
  left.wt(0) = 1;
  left.wt(1) = 10;

  IvPBox right(1, 1);
  right.setPTS(0, 0, 5);
  right.setBDS(0, false, true);
  right.wt(0) = 2;
  right.wt(1) = 20;

  IvPBox* intersection = nullptr;
  ASSERT_TRUE(left.intersect(&right, intersection));
  ASSERT_NE(intersection, nullptr);
  EXPECT_EQ(intersection->pt(0, 0), 0);
  EXPECT_EQ(intersection->pt(0, 1), 5);
  EXPECT_FALSE(intersection->bd(0, 0));
  EXPECT_FALSE(intersection->bd(0, 1));
  EXPECT_DOUBLE_EQ(intersection->wt(0), 3);
  EXPECT_DOUBLE_EQ(intersection->wt(1), 30);
  delete intersection;
}

// Covers IvP box behavior: intersect can reuse caller allocated result box.
TEST(IvPBoxTest, IntersectCanReuseCallerAllocatedResultBox)
{
  IvPBox left = courseSpeedBox(10, 100, 1, 4, 2, 3, 5);
  left.setBDS(0, true, false);
  IvPBox right = courseSpeedBox(50, 120, 3, 5, -1, 4, 7);
  right.setBDS(1, false, true);

  IvPBox reusable(2, 1);
  reusable.setPTS(0, 0, 0);
  reusable.setPTS(1, 0, 0);
  reusable.wt(0) = 99;
  IvPBox* result = &reusable;

  ASSERT_TRUE(left.intersect(&right, result));
  EXPECT_EQ(result, &reusable);
  EXPECT_EQ(reusable.pt(0, 0), 50);
  EXPECT_EQ(reusable.pt(0, 1), 100);
  EXPECT_TRUE(reusable.bd(0, 0));
  EXPECT_FALSE(reusable.bd(0, 1));
  EXPECT_EQ(reusable.pt(1, 0), 3);
  EXPECT_EQ(reusable.pt(1, 1), 4);
  EXPECT_FALSE(reusable.bd(1, 0));
  EXPECT_TRUE(reusable.bd(1, 1));
  EXPECT_DOUBLE_EQ(reusable.wt(0), 1);
  EXPECT_DOUBLE_EQ(reusable.wt(1), 7);
  EXPECT_DOUBLE_EQ(reusable.wt(2), 12);
}

// Covers IvP box behavior: copies assigns and translates domains.
TEST(IvPBoxTest, CopiesAssignsAndTranslatesDomains)
{
  IvPBox original(2, 1);
  original.setPTS(0, 1, 3);
  original.setPTS(1, 4, 6);
  original.wt(0) = 10;
  original.wt(1) = 20;
  original.wt(2) = 30;
  original.ofindex() = 7;
  original.setPlat(3);

  std::unique_ptr<IvPBox> copy(original.copy());
  EXPECT_EQ(copy->pt(0, 0), 1);
  EXPECT_EQ(copy->pt(1, 1), 6);
  EXPECT_DOUBLE_EQ(copy->wt(2), 30);
  EXPECT_EQ(copy->ofindex(), 7);
  EXPECT_EQ(copy->getPlat(), 3);

  IvPBox assigned;
  assigned = original;
  EXPECT_EQ(assigned.getDim(), 2);
  EXPECT_EQ(assigned.getDegree(), 1);
  EXPECT_DOUBLE_EQ(assigned.wt(1), 20);

  const int map[] = {1, 0};
  assigned.transDomain(0, map);
  EXPECT_EQ(assigned.getDim(), 2);
  EXPECT_EQ(assigned.pt(0, 0), 4);
  EXPECT_EQ(assigned.pt(1, 0), 1);
  EXPECT_DOUBLE_EQ(assigned.wt(0), 20);
  EXPECT_DOUBLE_EQ(assigned.wt(1), 10);
}

// Covers IvP box behavior: direct copy preserves metadata boundary flags and weights.
TEST(IvPBoxTest, DirectCopyPreservesMetadataBoundaryFlagsAndWeights)
{
  IvPBox source(2, 1);
  source.setPTS(0, 2, 8);
  source.setPTS(1, 1, 3);
  source.setBDS(0, false, true);
  source.setBDS(1, true, false);
  source.wt(0) = -1.5;
  source.wt(1) = 4.25;
  source.wt(2) = 12;
  source.ofindex() = 42;
  source.mark() = true;
  source.setPlat(7);

  IvPBox copied(2, 1);
  copied.copy(&source);

  EXPECT_EQ(copied.pt(0, 0), 2);
  EXPECT_EQ(copied.pt(0, 1), 8);
  EXPECT_FALSE(copied.bd(0, 0));
  EXPECT_TRUE(copied.bd(0, 1));
  EXPECT_TRUE(copied.bd(1, 0));
  EXPECT_FALSE(copied.bd(1, 1));
  EXPECT_DOUBLE_EQ(copied.wt(0), -1.5);
  EXPECT_DOUBLE_EQ(copied.wt(1), 4.25);
  EXPECT_DOUBLE_EQ(copied.wt(2), 12);
  EXPECT_EQ(copied.ofindex(), 42);
  EXPECT_TRUE(copied.mark());
  EXPECT_EQ(copied.getPlat(), 7);

  source.setPTS(0, 0, 0);
  source.wt(2) = 99;
  source.mark() = false;
  EXPECT_EQ(copied.pt(0, 0), 2);
  EXPECT_DOUBLE_EQ(copied.wt(2), 12);
  EXPECT_TRUE(copied.mark());
}

// Covers IvP box behavior: assignment handles degree changes and self assignment.
TEST(IvPBoxTest, AssignmentHandlesDegreeChangesAndSelfAssignment)
{
  IvPBox linear(2, 1);
  linear.setPTS(0, 1, 2);
  linear.setPTS(1, 3, 4);
  linear.wt(0) = 5;
  linear.wt(1) = 6;
  linear.wt(2) = 7;

  IvPBox quadratic(1, 2);
  quadratic.setPTS(0, 2, 3);
  quadratic.wt(0) = 1;
  quadratic.wt(1) = 2;
  quadratic.wt(2) = 3;
  quadratic.ofindex() = 9;
  quadratic.setPlat(-4);

  linear = quadratic;
  EXPECT_EQ(linear.getDim(), 1);
  EXPECT_EQ(linear.getDegree(), 2);
  EXPECT_EQ(linear.getWtc(), 3);
  EXPECT_EQ(linear.pt(0, 0), 2);
  EXPECT_EQ(linear.pt(0, 1), 3);
  EXPECT_DOUBLE_EQ(linear.wt(0), 1);
  EXPECT_DOUBLE_EQ(linear.wt(1), 2);
  EXPECT_DOUBLE_EQ(linear.wt(2), 3);
  EXPECT_EQ(linear.ofindex(), 9);
  EXPECT_EQ(linear.getPlat(), -4);

  linear = linear;
  EXPECT_EQ(linear.getDim(), 1);
  EXPECT_EQ(linear.getDegree(), 2);
  EXPECT_DOUBLE_EQ(linear.wt(2), 3);
}

// Covers IvP box behavior: assigns null boxes and expands direct domain mappings.
TEST(IvPBoxTest, AssignsNullBoxesAndExpandsDirectDomainMappings)
{
  IvPBox box(1, 1);
  box.setPTS(0, 2, 4);
  box.wt(0) = 3;
  box.wt(1) = 9;

  const int add_speed_before_course[] = {1};
  box.transDomain(1, add_speed_before_course);
  EXPECT_EQ(box.getDim(), 2);
  EXPECT_EQ(box.pt(0, 0), 0);
  EXPECT_EQ(box.pt(0, 1), 0);
  EXPECT_EQ(box.pt(1, 0), 2);
  EXPECT_EQ(box.pt(1, 1), 4);
  EXPECT_DOUBLE_EQ(box.wt(0), 0);
  EXPECT_DOUBLE_EQ(box.wt(1), 3);
  EXPECT_DOUBLE_EQ(box.wt(2), 9);

  IvPBox null_box;
  box = null_box;
  EXPECT_TRUE(box.null());
  EXPECT_EQ(box.getDim(), 0);
}

// Covers IvP box behavior: expands constant boxes without changing utility.
TEST(IvPBoxTest, ExpandsConstantBoxesWithoutChangingUtility)
{
  IvPBox box(1, 0);
  box.setPTS(0, 2, 4);
  box.setWT(33);

  const int map_course_to_middle[] = {1};
  box.transDomain(2, map_course_to_middle);

  EXPECT_EQ(box.getDim(), 3);
  EXPECT_EQ(box.getDegree(), 0);
  EXPECT_EQ(box.getWtc(), 1);
  EXPECT_EQ(box.pt(0, 0), 0);
  EXPECT_EQ(box.pt(0, 1), 0);
  EXPECT_EQ(box.pt(1, 0), 2);
  EXPECT_EQ(box.pt(1, 1), 4);
  EXPECT_EQ(box.pt(2, 0), 0);
  EXPECT_EQ(box.pt(2, 1), 0);
  EXPECT_DOUBLE_EQ(box.maxVal(), 33);
  EXPECT_DOUBLE_EQ(box.minVal(), 33);
}

// Covers box set behavior: adds removes and merges without owning boxes by default.
TEST(BoxSetTest, AddsRemovesAndMergesWithoutOwningBoxesByDefault)
{
  IvPBox a(1);
  IvPBox b(1);
  IvPBox c(1);
  a.setPTS(0, 1, 1);
  b.setPTS(0, 2, 2);
  c.setPTS(0, 3, 3);

  BoxSet set;
  set.addBox(&a, FIRST);
  set.addBox(&b, LAST);
  set.addBox(&c, FIRST);
  ASSERT_EQ(set.size(), 3);
  EXPECT_EQ(set.retBSN(FIRST)->getBox(), &c);
  EXPECT_EQ(set.retBSN(LAST)->getBox(), &b);

  BoxSetNode* first = set.remBSN(FIRST);
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first->getBox(), &c);
  delete first;
  EXPECT_EQ(set.size(), 2);

  BoxSet donor;
  donor.addBox(&c, LAST);
  set.merge(donor);
  EXPECT_EQ(set.size(), 3);
  EXPECT_EQ(donor.size(), 0);
}

// Covers box set behavior: merge copy keeps source and remove dups drops repeated pointers.
TEST(BoxSetTest, MergeCopyKeepsSourceAndRemoveDupsDropsRepeatedPointers)
{
  IvPBox a(1);
  IvPBox b(1);
  BoxSet source;
  source.addBox(&a, LAST);
  source.addBox(&b, LAST);

  BoxSet copy;
  copy.mergeCopy(source);
  EXPECT_EQ(source.size(), 2);
  EXPECT_EQ(copy.size(), 2);

  copy.addBox(&a, LAST);
  copy.addBox(&b, LAST);
  copy.removeDups();
  EXPECT_EQ(copy.size(), 2);
  EXPECT_TRUE(a.mark());
  EXPECT_TRUE(b.mark());
}

// Covers box set behavior: merge copy nodes survive after source is emptied.
TEST(BoxSetTest, MergeCopyNodesSurviveAfterSourceIsEmptied)
{
  IvPBox a(1);
  IvPBox b(1);
  BoxSet source;
  source.addBox(&a, LAST);
  source.addBox(&b, LAST);

  BoxSet copy;
  copy.mergeCopy(source);
  source.makeEmpty();

  EXPECT_EQ(source.size(), 0);
  ASSERT_EQ(copy.size(), 2);
  EXPECT_EQ(copy.retBSN(FIRST)->getBox(), &a);
  EXPECT_EQ(copy.retBSN(LAST)->getBox(), &b);
  EXPECT_NE(copy.retBSN(FIRST), source.retBSN(FIRST));
}

// Covers box set behavior: removes specific middle node and can empty without deleting boxes.
TEST(BoxSetTest, RemovesSpecificMiddleNodeAndCanEmptyWithoutDeletingBoxes)
{
  IvPBox a(1);
  IvPBox b(1);
  IvPBox c(1);
  BoxSet set;
  set.addBox(&a, LAST);
  set.addBox(&b, LAST);
  set.addBox(&c, LAST);
  ASSERT_EQ(set.size(), 3);

  BoxSetNode* middle = set.retBSN(FIRST)->getNext();
  ASSERT_NE(middle, nullptr);
  EXPECT_EQ(middle->getBox(), &b);
  set.remBSN(middle);
  delete middle;
  EXPECT_EQ(set.size(), 2);
  EXPECT_EQ(set.retBSN(FIRST)->getBox(), &a);
  EXPECT_EQ(set.retBSN(LAST)->getBox(), &c);

  set.makeEmpty();
  EXPECT_EQ(set.size(), 0);
  EXPECT_EQ(set.retBSN(FIRST), nullptr);
  EXPECT_EQ(set.retBSN(LAST), nullptr);
}

// Covers box set behavior: make empty and delete boxes owns heap boxes when requested.
TEST(BoxSetTest, MakeEmptyAndDeleteBoxesOwnsHeapBoxesWhenRequested)
{
  BoxSet set;
  set.addBox(new IvPBox(1), LAST);
  set.addBox(new IvPBox(1), LAST);
  ASSERT_EQ(set.size(), 2);

  set.makeEmptyAndDeleteBoxes();
  EXPECT_EQ(set.size(), 0);
  EXPECT_EQ(set.retBSN(FIRST), nullptr);
  EXPECT_EQ(set.retBSN(LAST), nullptr);
}

// Covers box set behavior: empty removal and merging empty sets are no ops.
TEST(BoxSetTest, EmptyRemovalAndMergingEmptySetsAreNoOps)
{
  BoxSet empty;
  EXPECT_EQ(empty.remBSN(FIRST), nullptr);
  EXPECT_EQ(empty.remBSN(LAST), nullptr);
  EXPECT_EQ(empty.retBSN(FIRST), nullptr);
  EXPECT_EQ(empty.retBSN(LAST), nullptr);

  IvPBox a(1);
  BoxSet destination;
  destination.addBox(&a, LAST);
  BoxSet empty_donor;
  destination.merge(empty_donor);
  EXPECT_EQ(destination.size(), 1);
  EXPECT_EQ(empty_donor.size(), 0);
  EXPECT_EQ(destination.retBSN(FIRST)->getBox(), &a);

  IvPBox b(1);
  BoxSet donor;
  donor.addBox(&b, LAST);
  empty.merge(donor);
  EXPECT_EQ(empty.size(), 1);
  EXPECT_EQ(donor.size(), 0);
  EXPECT_EQ(empty.retBSN(FIRST)->getBox(), &b);
}

// Covers box set behavior: add bsn accepts caller created node and owns node.
TEST(BoxSetTest, AddBSNAcceptsCallerCreatedNodeAndOwnsNode)
{
  IvPBox a(1);
  IvPBox b(1);
  BoxSet set;
  BoxSetNode* first = new BoxSetNode(&a);
  BoxSetNode* last = new BoxSetNode(&b);

  set.addBSN(*first, FIRST);
  set.addBSN(*last, LAST);
  EXPECT_EQ(set.size(), 2);
  EXPECT_EQ(set.retBSN(FIRST), first);
  EXPECT_EQ(set.retBSN(LAST), last);
  EXPECT_EQ(set.retBSN(FIRST)->getNext(), last);
  EXPECT_EQ(set.retBSN(LAST)->getPrev(), first);
}

// Covers box set behavior: remove dups clears preexisting marks then marks retained boxes.
TEST(BoxSetTest, RemoveDupsClearsPreexistingMarksThenMarksRetainedBoxes)
{
  IvPBox a(1);
  IvPBox b(1);
  a.mark() = true;
  b.mark() = true;

  BoxSet set;
  set.addBox(&a, LAST);
  set.addBox(&b, LAST);
  set.addBox(&a, LAST);

  set.removeDups();

  ASSERT_EQ(set.size(), 2);
  EXPECT_EQ(set.retBSN(FIRST)->getBox(), &a);
  EXPECT_EQ(set.retBSN(LAST)->getBox(), &b);
  EXPECT_TRUE(a.mark());
  EXPECT_TRUE(b.mark());
}

// Covers IvP grid behavior: indexes boxes bounds and threshold queries for solver search.
TEST(IvPGridTest, IndexesBoxesBoundsAndThresholdQueriesForSolverSearch)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  grid.initialize(gel);

  EXPECT_EQ(grid.getDim(), 1);
  EXPECT_EQ(grid.getTotalGrids(), 4);
  EXPECT_TRUE(grid.isEmpty());
  EXPECT_NE(grid.getGridConfig().find("dim=1"), std::string::npos);

  IvPBox low = intervalBox(0, 4, 5);
  IvPBox high = intervalBox(5, 9, 20);
  grid.addBox(&low);
  grid.addBox(&high);

  EXPECT_FALSE(grid.isEmpty());
  EXPECT_DOUBLE_EQ(grid.getMaxVal(), 29);
  EXPECT_EQ(grid.getMaxPt().pt(0), 9);
  EXPECT_GT(grid.calcBoxesPerGEL(), 0.0);

  IvPBox low_query = makeOneDimPoint(1);
  std::unique_ptr<BoxSet> low_hits(grid.getBS(&low_query));
  ASSERT_EQ(low_hits->size(), 1);
  EXPECT_EQ(low_hits->retBSN()->getBox(), &low);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&low_query), 9);

  IvPBox all(1);
  all.setPTS(0, 0, 9);
  std::unique_ptr<BoxSet> threshold_hits(grid.getBS_Thresh(&all, 10));
  ASSERT_EQ(threshold_hits->size(), 1);
  EXPECT_EQ(threshold_hits->retBSN()->getBox(), &low);

  std::unique_ptr<BoxSet> strict_threshold_hits(grid.getBS_Thresh(&all, 9));
  EXPECT_EQ(strict_threshold_hits->size(), 0);

  grid.scaleBounds(2);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&low_query), 18);
  grid.moveBounds(-8);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&low_query), 10);

  grid.remBox(&low);
  std::unique_ptr<BoxSet> after_remove(grid.getBS(&low_query));
  EXPECT_EQ(after_remove->size(), 0);
}

// Covers IvP grid behavior: separates box storage from upper bound storage.
TEST(IvPGridTest, SeparatesBoxStorageFromUpperBoundStorage)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox stored_only = intervalBox(0, 4, 10);
  IvPBox bound_only = intervalBox(5, 9, 50);
  grid.addBox(&stored_only, true, false);
  grid.addBox(&bound_only, false, true);

  EXPECT_FALSE(grid.isEmpty());
  EXPECT_DOUBLE_EQ(grid.getMaxVal(), 59);
  EXPECT_EQ(grid.getMaxPt().pt(0), 9);

  IvPBox stored_query = makeOneDimPoint(2);
  std::unique_ptr<BoxSet> stored_hits(grid.getBS(&stored_query));
  ASSERT_EQ(stored_hits->size(), 1);
  EXPECT_EQ(stored_hits->retBSN()->getBox(), &stored_only);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&stored_query), -99999.0);

  IvPBox bound_query = makeOneDimPoint(8);
  std::unique_ptr<BoxSet> bound_hits(grid.getBS(&bound_query));
  EXPECT_EQ(bound_hits->size(), 0);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&bound_query), 59);
}

// Covers IvP grid behavior: global cheap bound skips first grid legacy behavior.
TEST(IvPGridTest, GlobalCheapBoundSkipsFirstGridLegacyBehavior)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, false);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox first_cell_only = intervalBox(0, 4, 10);
  grid.addBox(&first_cell_only, false, true);

  IvPBox query = makeOneDimPoint(2);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&query), 14);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(), -99999.0);
}

// Covers IvP grid behavior: removing boxes does not refresh cached bounds or max point.
TEST(IvPGridTest, RemovingBoxesDoesNotRefreshCachedBoundsOrMaxPoint)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox low = intervalBox(0, 4, 5);
  IvPBox high = intervalBox(5, 9, 20);
  grid.addBox(&low);
  grid.addBox(&high);

  IvPBox high_query = makeOneDimPoint(8);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&high_query), 29);
  EXPECT_DOUBLE_EQ(grid.getMaxVal(), 29);
  EXPECT_EQ(grid.getMaxPt().pt(0), 9);

  grid.remBox(&high);
  std::unique_ptr<BoxSet> hits(grid.getBS(&high_query));
  EXPECT_EQ(hits->size(), 0);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&high_query), 29);
  EXPECT_DOUBLE_EQ(grid.getMaxVal(), 29);
  EXPECT_EQ(grid.getMaxPt().pt(0), 9);
}

// Covers IvP grid behavior: removing unknown box leaves stored boxes and bounds untouched.
TEST(IvPGridTest, RemovingUnknownBoxLeavesStoredBoxesAndBoundsUntouched)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox stored = intervalBox(0, 4, 10);
  IvPBox never_added = intervalBox(0, 4, 50);
  grid.addBox(&stored);

  IvPBox query = makeOneDimPoint(2);
  grid.remBox(&never_added);

  std::unique_ptr<BoxSet> hits(grid.getBS(&query));
  ASSERT_EQ(hits->size(), 1);
  EXPECT_EQ(hits->retBSN()->getBox(), &stored);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&query), 14);
}

// Covers IvP grid behavior: repeated single cell insertion returns duplicate hits.
TEST(IvPGridTest, RepeatedSingleCellInsertionReturnsDuplicateHits)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox box = intervalBox(0, 4, 10);
  grid.addBox(&box);
  grid.addBox(&box);

  IvPBox query = makeOneDimPoint(2);
  std::unique_ptr<BoxSet> hits(grid.getBS(&query));
  ASSERT_EQ(hits->size(), 2);
  EXPECT_EQ(hits->retBSN(FIRST)->getBox(), &box);
  EXPECT_EQ(hits->retBSN(LAST)->getBox(), &box);
}

// Covers IvP grid behavior: supports upper bound only mode used during search.
TEST(IvPGridTest, SupportsUpperBoundOnlyModeUsedDuringSearch)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, false);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox low = intervalBox(0, 4, 5);
  IvPBox high = intervalBox(5, 9, 20);
  grid.addBox(&low, false, true);
  grid.addBox(&high, false, true);

  IvPBox low_query = makeOneDimPoint(2);
  IvPBox high_query = makeOneDimPoint(8);
  EXPECT_TRUE(grid.isEmpty());
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&low_query), 9);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&high_query), 29);
  EXPECT_DOUBLE_EQ(grid.getCheapBound(), 29);
  EXPECT_EQ(grid.getMaxPt().pt(0), 9);
}

// Covers IvP grid behavior: can return candidate boxes without intersection filtering.
TEST(IvPGridTest, CanReturnCandidateBoxesWithoutIntersectionFiltering)
{
  IvPDomain domain = makeCourseOnlyDomain(10);
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 4);
  grid.initialize(gel);

  IvPBox first = makeOneDimPoint(0);
  IvPBox second = makeOneDimPoint(4);
  grid.addBox(&first);
  grid.addBox(&second);

  IvPBox middle = makeOneDimPoint(2);
  std::unique_ptr<BoxSet> filtered(grid.getBS(&middle, true));
  EXPECT_EQ(filtered->size(), 0);

  std::unique_ptr<BoxSet> candidates(grid.getBS(&middle, false));
  ASSERT_EQ(candidates->size(), 2);
  EXPECT_EQ(candidates->retBSN(FIRST)->getBox(), &first);
  EXPECT_EQ(candidates->retBSN(LAST)->getBox(), &second);
}

// Covers IvP grid behavior: deduplicates and removes boxes spanning multiple grid cells.
TEST(IvPGridTest, DeduplicatesAndRemovesBoxesSpanningMultipleGridCells)
{
  IvPDomain domain = makeCourseSpeedDomain();
  IvPGrid grid(domain, true);
  IvPBox gel(2);
  gel.setPTS(0, 0, 89);
  gel.setPTS(1, 0, 1);
  grid.initialize(gel);

  IvPBox wide = courseSpeedBox(45, 250, 0, 5, 0.1, 1, 5);
  grid.addBox(&wide);
  EXPECT_FALSE(grid.isEmpty());
  EXPECT_GT(grid.calcBoxesPerGEL(), 0.0);

  IvPBox query(2);
  query.setPTS(0, 0, 359);
  query.setPTS(1, 0, 5);
  std::unique_ptr<BoxSet> hits(grid.getBS(&query));
  ASSERT_EQ(hits->size(), 1);
  EXPECT_EQ(hits->retBSN()->getBox(), &wide);

  std::unique_ptr<BoxSet> low_thresh(grid.getBS_Thresh(&query, 40));
  ASSERT_EQ(low_thresh->size(), 1);
  EXPECT_EQ(low_thresh->retBSN()->getBox(), &wide);
  std::unique_ptr<BoxSet> high_thresh(grid.getBS_Thresh(&query, 20));
  EXPECT_EQ(high_thresh->size(), 0);

  grid.remBox(&wide);
  std::unique_ptr<BoxSet> after_remove(grid.getBS(&query));
  EXPECT_EQ(after_remove->size(), 0);
}

// Covers IvP grid behavior: clamps gel sizes at domain extremes.
TEST(IvPGridTest, ClampsGelSizesAtDomainExtremes)
{
  IvPDomain domain = makeCourseOnlyDomain(5);

  IvPGrid fine_grid(domain, true);
  IvPBox too_small(1);
  too_small.setPTS(0, 0, 0);
  fine_grid.initialize(too_small);
  EXPECT_EQ(fine_grid.getTotalGrids(), 3);
  EXPECT_NE(fine_grid.getGridConfig().find("sz[0]:2"), std::string::npos);
  EXPECT_NE(fine_grid.getGridConfig().find("amt[0]:3"), std::string::npos);

  IvPGrid coarse_grid(domain, true);
  IvPBox too_large(1);
  too_large.setPTS(0, 0, 99);
  coarse_grid.initialize(too_large);
  EXPECT_EQ(coarse_grid.getTotalGrids(), 1);
  EXPECT_NE(coarse_grid.getGridConfig().find("sz[0]:5"), std::string::npos);
  EXPECT_NE(coarse_grid.getGridConfig().find("cells=1"), std::string::npos);
}

// Covers IvP grid behavior: empty initialized grid reports sentinel bounds.
TEST(IvPGridTest, EmptyInitializedGridReportsSentinelBounds)
{
  IvPDomain domain = makeCourseOnlyDomain();
  IvPGrid grid(domain, true);
  IvPBox gel(1);
  gel.setPTS(0, 0, 1);
  grid.initialize(gel);

  IvPBox query = makeOneDimPoint(2);
  EXPECT_TRUE(grid.isEmpty());
  EXPECT_DOUBLE_EQ(grid.getCheapBound(&query), -99999.0);
  std::unique_ptr<BoxSet> hits(grid.getBS(&query));
  EXPECT_EQ(hits->size(), 0);
}

// Covers compactor null behavior: delegates max val without changing boxes.
TEST(CompactorNullTest, DelegatesMaxValWithoutChangingBoxes)
{
  IvPBox box = intervalBox(0, 4, 10);
  CompactorNull compactor;
  bool ok = false;

  EXPECT_TRUE(compactor.compact(&box));
  EXPECT_DOUBLE_EQ(compactor.maxVal(&box, &ok), 14);
  EXPECT_TRUE(ok);
  EXPECT_EQ(box.pt(0, 0), 0);
  EXPECT_EQ(box.pt(0, 1), 4);
}

// Covers compactor null behavior: dispatches through base compactor interface.
TEST(CompactorNullTest, DispatchesThroughBaseCompactorInterface)
{
  IvPBox box = intervalBox(0, 4, 10);
  box.scaleWT(-1);
  std::unique_ptr<Compactor> compactor(new CompactorNull());
  bool ok = false;

  EXPECT_TRUE(compactor->compact(&box));
  EXPECT_DOUBLE_EQ(compactor->maxVal(&box, &ok), -10);
  EXPECT_TRUE(ok);
  EXPECT_EQ(box.pt(0, 0), 0);
  EXPECT_EQ(box.pt(0, 1), 4);
  EXPECT_DOUBLE_EQ(box.wt(0), -1);
  EXPECT_DOUBLE_EQ(box.wt(1), -10);
}

// Covers pd map behavior: evaluates pieces with and without grid for ZAIC style maps.
TEST(PDMapTest, EvaluatesPiecesWithAndWithoutGridForZaicStyleMaps)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  bool covered = false;
  IvPBox point = makeOneDimPoint(1);
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&point, &covered), 11);
  EXPECT_TRUE(covered);

  point.setPTS(0, 4, 4);
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&point, &covered), 24);
  EXPECT_TRUE(covered);

  pdmap->updateGrid();
  EXPECT_NE(pdmap->getGrid(), nullptr);
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&point, &covered), 24);
  EXPECT_TRUE(covered);
  EXPECT_EQ(pdmap->getIX(&point), 1);

  point.setPTS(0, 99, 99);
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&point, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: non point queries evaluate as uncovered.
TEST(PDMapTest, NonPointQueriesEvaluateAsUncovered)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox interval_query(1);
  interval_query.setPTS(0, 1, 2);

  bool covered = true;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&interval_query, &covered), 0);
  EXPECT_FALSE(covered);

  pdmap->updateGrid();
  covered = true;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&interval_query, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: direct box set queries work before grid construction.
TEST(PDMapTest, DirectBoxSetQueriesWorkBeforeGridConstruction)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox first_query = makeOneDimPoint(1);
  std::unique_ptr<BoxSet> first_hits(pdmap->getBS(&first_query));
  ASSERT_EQ(first_hits->size(), 1);
  EXPECT_EQ(first_hits->retBSN()->getBox(), pdmap->getBox(0));

  IvPBox span_query(1);
  span_query.setPTS(0, 1, 4);
  std::unique_ptr<BoxSet> span_hits(pdmap->getBS(&span_query));
  EXPECT_EQ(span_hits->size(), 2);
}

// Covers pd map behavior: evaluates two dimensional course speed utility maps.
TEST(PDMapTest, EvaluatesTwoDimensionalCourseSpeedUtilityMaps)
{
  std::unique_ptr<PDMap> pdmap = makeFourPieceCourseSpeedMap();
  EXPECT_TRUE(pdmap->valid());
  EXPECT_TRUE(pdmap->freeOfNan());
  EXPECT_EQ(pdmap->getDim(), 2);
  EXPECT_EQ(pdmap->getDegree(), 1);
  EXPECT_DOUBLE_EQ(pdmap->getMinWT(), 10);
  EXPECT_DOUBLE_EQ(pdmap->getMaxWT(), 61);

  bool covered = false;
  IvPBox loiter_slow = makePointBox(2, {45, 1});
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&loiter_slow, &covered), 15.5);
  EXPECT_TRUE(covered);

  IvPBox loiter_fast = makePointBox(2, {45, 4});
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&loiter_fast, &covered), 28.5);
  EXPECT_TRUE(covered);

  IvPBox return_slow = makePointBox(2, {270, 2});
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&return_slow, &covered), 40.5);
  EXPECT_TRUE(covered);

  IvPBox return_fast = makePointBox(2, {270, 5});
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&return_fast, &covered), 56.5);
  EXPECT_TRUE(covered);
}

// Covers pd map behavior: two dimensional queries and grid lookup mirror helm course speed use.
TEST(PDMapTest, TwoDimensionalQueriesAndGridLookupMirrorHelmCourseSpeedUse)
{
  std::unique_ptr<PDMap> pdmap = makeFourPieceCourseSpeedMap();
  IvPBox gel(2);
  gel.setPTS(0, 0, 89);
  gel.setPTS(1, 0, 1);
  ASSERT_TRUE(pdmap->setGelBox(gel));

  IvPBox query = makePointBox(2, {270, 4});
  std::unique_ptr<BoxSet> direct_hits(pdmap->getBS(&query));
  ASSERT_EQ(direct_hits->size(), 1);
  EXPECT_EQ(direct_hits->retBSN()->getBox(), pdmap->getBox(3));

  pdmap->updateGrid();
  EXPECT_NE(pdmap->getGrid(), nullptr);
  EXPECT_NE(pdmap->getGridConfig().find("dim=2"), std::string::npos);
  EXPECT_EQ(pdmap->getIX(&query), 3);

  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&query, &covered), 54.5);
  EXPECT_TRUE(covered);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), 61);

  IvPBox span(2);
  span.setPTS(0, 170, 190);
  span.setPTS(1, 2, 3);
  std::unique_ptr<BoxSet> span_hits(pdmap->getBS(&span));
  EXPECT_EQ(span_hits->size(), 4);
}

// Covers pd map behavior: exhaustive tiny domain oracle matches direct and grid evaluation.
TEST(PDMapTest, ExhaustiveTinyDomainOracleMatchesDirectAndGridEvaluation)
{
  std::unique_ptr<PDMap> pdmap = makeTinyDenseXYMap();
  ASSERT_TRUE(pdmap->valid());

  for(int x = 0; x <= 3; ++x) {
    for(int y = 0; y <= 2; ++y) {
      IvPBox point = makePointBox(2, {x, y});
      bool covered = false;
      EXPECT_DOUBLE_EQ(pdmap->evalPoint(&point, &covered),
                       tinyDenseExpectedValue(x, y))
          << "direct eval at " << x << "," << y;
      EXPECT_TRUE(covered) << "direct coverage at " << x << "," << y;

      std::unique_ptr<BoxSet> hits(pdmap->getBS(&point));
      ASSERT_EQ(hits->size(), 1) << "direct hit count at " << x << "," << y;
      EXPECT_EQ(hits->retBSN()->getBox()->ofindex(),
                tinyDenseExpectedIndex(x, y));
      EXPECT_EQ(countPointCoverage(*pdmap, point), 1u);
    }
  }

  IvPBox gel(2);
  gel.setPTS(0, 0, 1);
  gel.setPTS(1, 0, 1);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();

  for(int x = 0; x <= 3; ++x) {
    for(int y = 0; y <= 2; ++y) {
      IvPBox point = makePointBox(2, {x, y});
      bool covered = false;
      EXPECT_DOUBLE_EQ(pdmap->evalPoint(&point, &covered),
                       tinyDenseExpectedValue(x, y))
          << "grid eval at " << x << "," << y;
      EXPECT_TRUE(covered) << "grid coverage at " << x << "," << y;
      EXPECT_EQ(pdmap->getIX(&point), tinyDenseExpectedIndex(x, y))
          << "grid index at " << x << "," << y;

      std::unique_ptr<BoxSet> hits(pdmap->getBS(&point));
      ASSERT_EQ(hits->size(), 1) << "grid hit count at " << x << "," << y;
      EXPECT_EQ(hits->retBSN()->getBox()->ofindex(),
                tinyDenseExpectedIndex(x, y));
      EXPECT_GE(pdmap->getGrid()->getCheapBound(&point),
                tinyDenseExpectedValue(x, y));
    }
  }
}

// Covers pd map behavior: handles zero box and flat utility maps conservatively.
TEST(PDMapTest, HandlesZeroBoxAndFlatUtilityMapsConservatively)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap empty(0, domain, 1);
  EXPECT_EQ(empty.size(), 0);
  EXPECT_DOUBLE_EQ(empty.getMinWT(), 0);
  EXPECT_DOUBLE_EQ(empty.getMaxWT(), 0);
  EXPECT_FALSE(empty.valid());
  EXPECT_TRUE(empty.freeOfNan());
  IvPBox point = makeOneDimPoint(2);
  bool covered = true;
  EXPECT_DOUBLE_EQ(empty.evalPoint(&point, &covered), 0);
  EXPECT_TRUE(covered);

  PDMap flat(1, domain, 1);
  flat.bx(0) = new IvPBox(intervalBox(0, 4, 10));
  flat.bx(0)->wt(0) = 0;
  EXPECT_TRUE(flat.valid());
  EXPECT_DOUBLE_EQ(flat.getMinWT(), 10);
  EXPECT_DOUBLE_EQ(flat.getMaxWT(), 10);
  flat.normalize(0, 100);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(flat, 2), 10);
}

// Covers pd map behavior: flat grid backed normalize is no op for constant utilities.
TEST(PDMapTest, FlatGridBackedNormalizeIsNoOpForConstantUtilities)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap flat(1, domain, 1);
  flat.bx(0) = new IvPBox(intervalBox(0, 4, 10));
  flat.bx(0)->wt(0) = 0;

  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(flat.setGelBox(gel));
  flat.updateGrid();

  IvPBox query = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(flat.getGrid()->getCheapBound(&query), 10);
  flat.normalize(0, 100);
  EXPECT_DOUBLE_EQ(flat.getMinWT(), 10);
  EXPECT_DOUBLE_EQ(flat.getMaxWT(), 10);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(flat, 4), 10);
  EXPECT_DOUBLE_EQ(flat.getGrid()->getCheapBound(&query), 10);
}

// Covers pd map behavior: grid detects ambiguous overlapping coverage as uncovered.
TEST(PDMapTest, GridDetectsAmbiguousOverlappingCoverageAsUncovered)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap pdmap(2, domain, 1);
  pdmap.bx(0) = new IvPBox(intervalBox(0, 3, 10));
  pdmap.bx(1) = new IvPBox(intervalBox(2, 4, 20));

  IvPBox point = makeOneDimPoint(2);
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap.evalPoint(&point, &covered), 12);
  EXPECT_TRUE(covered);

  pdmap.updateGrid();
  EXPECT_DOUBLE_EQ(pdmap.evalPoint(&point, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: applies weight scalar normalize and validity checks.
TEST(PDMapTest, AppliesWeightScalarNormalizeAndValidityChecks)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  EXPECT_TRUE(pdmap->valid());
  EXPECT_TRUE(pdmap->freeOfNan());
  EXPECT_DOUBLE_EQ(pdmap->getMinWT(), 10);
  EXPECT_DOUBLE_EQ(pdmap->getMaxWT(), 24);

  pdmap->applyScalar(5);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 0), 15);
  pdmap->applyWeight(2);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 0), 30);

  pdmap->normalize(0, 100);
  EXPECT_NEAR(pdmap->getMinWT(), 0, 1e-9);
  EXPECT_NEAR(pdmap->getMaxWT(), 100, 1e-9);

  pdmap->bx(1)->setPTS(0, 2, 4);
  EXPECT_FALSE(pdmap->valid());
}

// Covers pd map behavior: normalizes negative utilities with legacy scalar then weight order.
TEST(PDMapTest, NormalizesNegativeUtilitiesWithLegacyScalarThenWeightOrder)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  pdmap->applyScalar(-30);

  EXPECT_DOUBLE_EQ(pdmap->getMinWT(), -20);
  EXPECT_DOUBLE_EQ(pdmap->getMaxWT(), -6);
  pdmap->normalize(10, 20);

  EXPECT_NEAR(pdmap->getMinWT(), 100.0 / 7.0, 1e-9);
  EXPECT_NEAR(pdmap->getMaxWT(), 240.0 / 7.0, 1e-9);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 0), 100.0 / 7.0, 1e-9);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 4), 240.0 / 7.0, 1e-9);
}

// Covers pd map behavior: default gel box stays inside domain for grid building.
TEST(PDMapTest, DefaultGelBoxStaysInsideDomainForGridBuilding)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  pdmap->setGelBox();
  IvPBox gel = pdmap->getGelBox();

  ASSERT_EQ(gel.getDim(), 1);
  EXPECT_TRUE(gel.isPtBox());
  EXPECT_GE(gel.pt(0, 0), 0);
  EXPECT_LE(gel.pt(0, 1), 4);

  pdmap->updateGrid();
  EXPECT_NE(pdmap->getGrid(), nullptr);
  EXPECT_NE(pdmap->getGridConfig().find("dim=1"), std::string::npos);
}

// Covers pd map behavior: default gel box scales for two dimensional helm maps.
TEST(PDMapTest, DefaultGelBoxScalesForTwoDimensionalHelmMaps)
{
  std::unique_ptr<PDMap> pdmap = makeFourPieceCourseSpeedMap();
  pdmap->setGelBox();
  IvPBox gel = pdmap->getGelBox();

  ASSERT_EQ(gel.getDim(), 2);
  EXPECT_TRUE(gel.isPtBox());
  EXPECT_GE(gel.pt(0, 0), 0);
  EXPECT_LE(gel.pt(0, 1), 359);
  EXPECT_GE(gel.pt(1, 0), 0);
  EXPECT_LE(gel.pt(1, 1), 5);

  pdmap->updateGrid();
  EXPECT_NE(pdmap->getGrid(), nullptr);
  EXPECT_NE(pdmap->getGridConfig().find("dim=2"), std::string::npos);
  EXPECT_GT(pdmap->getGrid()->getTotalGrids(), 1);

  IvPBox query = makePointBox(2, {270, 4});
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&query, &covered), 54.5);
  EXPECT_TRUE(covered);
}

// Covers pd map behavior: grid bounds track weight and scalar adjustments.
TEST(PDMapTest, GridBoundsTrackWeightAndScalarAdjustments)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();
  ASSERT_NE(pdmap->getGrid(), nullptr);

  IvPBox query = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), 24);

  pdmap->applyScalar(5);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 4), 29);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), 29);

  pdmap->applyWeight(2);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 4), 58);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), 58);
}

// Covers pd map behavior: negative weights propagate to existing grid bounds.
TEST(PDMapTest, NegativeWeightsPropagateToExistingGridBounds)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();

  IvPBox low_query = makeOneDimPoint(0);
  IvPBox high_query = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), 12);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&high_query), 24);

  pdmap->applyWeight(-1);
  EXPECT_DOUBLE_EQ(pdmap->getMinWT(), -24);
  EXPECT_DOUBLE_EQ(pdmap->getMaxWT(), -10);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 0), -10);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 4), -24);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), -12);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&high_query), -24);
}

// Covers pd map behavior: box only grid evaluates without upper bound storage.
TEST(PDMapTest, BoxOnlyGridEvaluatesWithoutUpperBoundStorage)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));

  pdmap->updateGrid(true, false);
  ASSERT_NE(pdmap->getGrid(), nullptr);

  IvPBox query = makeOneDimPoint(4);
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&query, &covered), 24);
  EXPECT_TRUE(covered);
  EXPECT_EQ(pdmap->getIX(&query), 1);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), -99999.0);
}

// Covers pd map behavior: rebuild grid can switch storage modes without changing utilities.
TEST(PDMapTest, RebuildGridCanSwitchStorageModesWithoutChangingUtilities)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));

  pdmap->updateGrid(false, true);
  ASSERT_NE(pdmap->getGrid(), nullptr);
  EXPECT_TRUE(pdmap->getGrid()->isEmpty());
  IvPBox query = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), 24);

  pdmap->updateGrid(true, true);
  ASSERT_NE(pdmap->getGrid(), nullptr);
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&query, &covered), 24);
  EXPECT_TRUE(covered);
  EXPECT_EQ(pdmap->getIX(&query), 1);

  pdmap->updateGrid(true, false);
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&query, &covered), 24);
  EXPECT_TRUE(covered);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), -99999.0);
}

// Covers pd map behavior: upper bound only grid supports solver bounds without box storage.
TEST(PDMapTest, UpperBoundOnlyGridSupportsSolverBoundsWithoutBoxStorage)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));

  pdmap->updateGrid(false, true);
  ASSERT_NE(pdmap->getGrid(), nullptr);
  EXPECT_TRUE(pdmap->getGrid()->isEmpty());

  IvPBox low_query = makeOneDimPoint(1);
  IvPBox high_query = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), 12);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&high_query), 24);

  pdmap->applyScalar(3);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), 15);
  pdmap->applyWeight(2);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), 30);
}

// Covers pd map behavior: normalize updates existing grid bounds.
TEST(PDMapTest, NormalizeUpdatesExistingGridBounds)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();

  IvPBox low_query = makeOneDimPoint(0);
  IvPBox high_query = makeOneDimPoint(4);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), 12);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&high_query), 24);

  pdmap->normalize(0, 100);
  EXPECT_NEAR(pdmap->getMinWT(), 0, 1e-9);
  EXPECT_NEAR(pdmap->getMaxWT(), 100, 1e-9);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 0), 0, 1e-9);
  EXPECT_NEAR(evalPDMapAtIndex(*pdmap, 4), 100, 1e-9);
  EXPECT_NEAR(pdmap->getGrid()->getCheapBound(&low_query), 100.0 / 7.0, 1e-9);
  EXPECT_NEAR(pdmap->getGrid()->getCheapBound(&high_query), 100, 1e-9);
}

// Covers pd map behavior: zero weight flattens utilities and existing grid bounds.
TEST(PDMapTest, ZeroWeightFlattensUtilitiesAndExistingGridBounds)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();

  IvPBox low_query = makeOneDimPoint(1);
  IvPBox high_query = makeOneDimPoint(4);
  pdmap->applyWeight(0);

  EXPECT_DOUBLE_EQ(pdmap->getMinWT(), 0);
  EXPECT_DOUBLE_EQ(pdmap->getMaxWT(), 0);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 1), 0);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 4), 0);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&low_query), 0);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&high_query), 0);
}

// Covers pd map behavior: detects gaps in domain coverage before helm solving.
TEST(PDMapTest, DetectsGapsInDomainCoverageBeforeHelmSolving)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap pdmap(2, domain, 1);
  pdmap.bx(0) = new IvPBox(intervalBox(0, 1, 10));
  pdmap.bx(1) = new IvPBox(intervalBox(3, 4, 20));

  EXPECT_FALSE(pdmap.valid());
  IvPBox gap = makeOneDimPoint(2);
  bool covered = true;
  EXPECT_DOUBLE_EQ(pdmap.evalPoint(&gap, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: valid rejects open boundary gaps between adjacent pieces.
TEST(PDMapTest, ValidRejectsOpenBoundaryGapsBetweenAdjacentPieces)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap pdmap(2, domain, 1);
  pdmap.bx(0) = new IvPBox(intervalBox(0, 2, 10));
  pdmap.bx(1) = new IvPBox(intervalBox(2, 4, 20));
  pdmap.bx(0)->setBDS(0, true, false);
  pdmap.bx(1)->setBDS(0, false, true);

  EXPECT_FALSE(pdmap.valid());
  IvPBox boundary = makeOneDimPoint(2);
  bool covered = true;
  EXPECT_DOUBLE_EQ(pdmap.evalPoint(&boundary, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: exhaustive coverage oracle matches validity for tiny maps.
TEST(PDMapTest, ExhaustiveCoverageOracleMatchesValidityForTinyMaps)
{
  std::unique_ptr<PDMap> dense = makeTinyDenseXYMap();
  EXPECT_TRUE(dense->valid());
  for(int x = 0; x <= 3; ++x)
    for(int y = 0; y <= 2; ++y)
      EXPECT_EQ(countPointCoverage(*dense, makePointBox(2, {x, y})), 1u)
          << "dense coverage at " << x << "," << y;

  std::unique_ptr<PDMap> gap = makeTinyDenseXYMap();
  delete gap->bx(2);
  gap->bx(2) = new IvPBox(courseSpeedBox(0, 0, 2, 2, 5, 0, 30));
  gap->bx(2)->ofindex() = 2;
  EXPECT_FALSE(gap->valid());
  EXPECT_EQ(countPointCoverage(*gap, makePointBox(2, {1, 2})), 0u);

  std::unique_ptr<PDMap> overlap = makeTinyDenseXYMap();
  overlap->bx(0)->setPTS(0, 0, 2);
  EXPECT_FALSE(overlap->valid());
  EXPECT_EQ(countPointCoverage(*overlap, makePointBox(2, {2, 1})), 2u);
}

// Covers pd map behavior: grows box array for incremental builder style population.
TEST(PDMapTest, GrowsBoxArrayForIncrementalBuilderStylePopulation)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap pdmap(1, domain, 1);
  pdmap.bx(0) = new IvPBox(intervalBox(0, 1, 10));

  pdmap.growBoxArray(2);
  pdmap.growBoxCount(2);
  ASSERT_EQ(pdmap.size(), 3);
  EXPECT_EQ(pdmap.getBox(1), nullptr);
  EXPECT_EQ(pdmap.getBox(2), nullptr);

  pdmap.bx(1) = new IvPBox(intervalBox(2, 3, 20));
  pdmap.bx(2) = new IvPBox(intervalBox(4, 4, 30));
  EXPECT_TRUE(pdmap.valid());
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(pdmap, 0), 10);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(pdmap, 3), 23);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(pdmap, 4), 34);
}

// Covers pd map behavior: copies deeply and detects NaN weights before solver use.
TEST(PDMapTest, CopiesDeeplyAndDetectsNanWeightsBeforeSolverUse)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  pdmap->updateGrid();

  PDMap copied(pdmap.get());
  EXPECT_EQ(copied.size(), pdmap->size());
  EXPECT_EQ(copied.getDomain(), pdmap->getDomain());
  EXPECT_NE(copied.getBox(0), pdmap->getBox(0));
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(copied, 1), 11);

  pdmap->applyScalar(100);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(copied, 1), 11);
  EXPECT_NE(evalPDMapAtIndex(copied, 1), evalPDMapAtIndex(*pdmap, 1));

  IvPDomain domain = makeCourseOnlyDomain();
  std::unique_ptr<PDMap> nan_map(new PDMap(1, domain, 1));
  nan_map->bx(0) = new IvPBox(intervalBox(0, 4, 0));
  nan_map->bx(0)->wt(0) = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(nan_map->freeOfNan());
}

// Covers pd map behavior: manages gel box universe and null removal.
TEST(PDMapTest, ManagesGelBoxUniverseAndNullRemoval)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox universe = pdmap->getUniverse();
  EXPECT_EQ(universe.pt(0, 0), 0);
  EXPECT_EQ(universe.pt(0, 1), 4);

  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  EXPECT_TRUE(pdmap->setGelBox(gel));
  const IvPBox accepted_gel = pdmap->getGelBox();
  gel.setPTS(0, -1, 2);
  EXPECT_FALSE(pdmap->setGelBox(gel));
  EXPECT_EQ(pdmap->getGelBox().pt(0, 0), accepted_gel.pt(0, 0));
  EXPECT_EQ(pdmap->getGelBox().pt(0, 1), accepted_gel.pt(0, 1));

  delete pdmap->bx(1);
  pdmap->bx(1) = nullptr;
  pdmap->removeNULLs();
  EXPECT_EQ(pdmap->size(), 1);
  EXPECT_TRUE(pdmap->freeOfNan());
  ASSERT_NE(pdmap->getGrid(), nullptr);

  IvPBox kept = makeOneDimPoint(1);
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&kept, &covered), 11);
  EXPECT_TRUE(covered);

  IvPBox removed = makeOneDimPoint(4);
  covered = true;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&removed, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: remove nulls rebuilds existing grid and drops uncovered points.
TEST(PDMapTest, RemoveNullsRebuildsExistingGridAndDropsUncoveredPoints)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();

  delete pdmap->bx(1);
  pdmap->bx(1) = nullptr;
  pdmap->removeNULLs();

  ASSERT_NE(pdmap->getGrid(), nullptr);
  EXPECT_EQ(pdmap->size(), 1);

  bool covered = false;
  IvPBox kept = makeOneDimPoint(1);
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&kept, &covered), 11);
  EXPECT_TRUE(covered);

  IvPBox removed = makeOneDimPoint(4);
  covered = true;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&removed, &covered), 0);
  EXPECT_FALSE(covered);
}

// Covers pd map behavior: remove nulls is no op when map is already dense.
TEST(PDMapTest, RemoveNullsIsNoOpWhenMapIsAlreadyDense)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  pdmap->updateGrid();
  const std::string config = pdmap->getGridConfig();

  pdmap->removeNULLs();
  EXPECT_EQ(pdmap->size(), 2);
  EXPECT_EQ(pdmap->getGridConfig(), config);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*pdmap, 4), 24);
}

// Covers pd map behavior: translates domain order and expands new dimensions.
TEST(PDMapTest, TranslatesDomainOrderAndExpandsNewDimensions)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();

  IvPDomain expanded;
  expanded.addDomain("speed", 0, 5, 6);
  expanded.addDomain("course", 0, 4, 5);
  expanded.addDomain("depth", 0, 10, 11);
  const int placement[] = {1};
  ASSERT_TRUE(pdmap->transDomain(expanded, placement));

  EXPECT_EQ(pdmap->getDim(), 3);
  EXPECT_EQ(pdmap->getDomain().getVarName(0), "speed");
  EXPECT_EQ(pdmap->getDomain().getVarName(1), "course");
  EXPECT_EQ(pdmap->getDomain().getVarName(2), "depth");
  EXPECT_EQ(pdmap->getUniverse().pt(0, 0), 0);
  EXPECT_EQ(pdmap->getUniverse().pt(0, 1), 5);
  EXPECT_EQ(pdmap->getUniverse().pt(1, 0), 0);
  EXPECT_EQ(pdmap->getUniverse().pt(1, 1), 4);
  EXPECT_EQ(pdmap->getUniverse().pt(2, 0), 0);
  EXPECT_EQ(pdmap->getUniverse().pt(2, 1), 10);

  const IvPBox* box = pdmap->getBox(0);
  ASSERT_NE(box, nullptr);
  EXPECT_EQ(box->pt(0, 0), 0);
  EXPECT_EQ(box->pt(0, 1), 5);
  EXPECT_EQ(box->pt(1, 0), 0);
  EXPECT_EQ(box->pt(1, 1), 2);
  EXPECT_EQ(box->pt(2, 0), 0);
  EXPECT_EQ(box->pt(2, 1), 10);
  EXPECT_DOUBLE_EQ(box->wt(0), 0);
  EXPECT_DOUBLE_EQ(box->wt(1), 1);
  EXPECT_DOUBLE_EQ(box->wt(2), 0);
  EXPECT_DOUBLE_EQ(box->wt(3), 10);
  EXPECT_TRUE(pdmap->valid());
}

// Covers pd map behavior: translates constant utility map across expanded helm domain.
TEST(PDMapTest, TranslatesConstantUtilityMapAcrossExpandedHelmDomain)
{
  IvPDomain course_domain = makeCourseOnlyDomain();
  PDMap pdmap(1, course_domain, 0);
  pdmap.bx(0) = new IvPBox(1, 0);
  pdmap.bx(0)->setPTS(0, 0, 4);
  pdmap.bx(0)->setWT(77);

  IvPDomain expanded;
  expanded.addDomain("speed", 0, 5, 6);
  expanded.addDomain("course", 0, 4, 5);
  const int placement[] = {1};
  ASSERT_TRUE(pdmap.transDomain(expanded, placement));

  EXPECT_EQ(pdmap.getDim(), 2);
  ASSERT_NE(pdmap.getBox(0), nullptr);
  EXPECT_EQ(pdmap.getBox(0)->getDegree(), 0);
  EXPECT_EQ(pdmap.getBox(0)->pt(0, 0), 0);
  EXPECT_EQ(pdmap.getBox(0)->pt(0, 1), 5);
  EXPECT_EQ(pdmap.getBox(0)->pt(1, 0), 0);
  EXPECT_EQ(pdmap.getBox(0)->pt(1, 1), 4);
  EXPECT_TRUE(pdmap.valid());

  IvPBox query = makePointBox(2, {3, 2});
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap.evalPoint(&query, &covered), 77);
  EXPECT_TRUE(covered);
}

// Covers pd map behavior: translates grid backed map and rebuilds grid for expanded helm domain.
TEST(PDMapTest, TranslatesGridBackedMapAndRebuildsGridForExpandedHelmDomain)
{
  std::unique_ptr<PDMap> pdmap = makeTwoPieceCourseMap();
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(pdmap->setGelBox(gel));
  pdmap->updateGrid();
  ASSERT_NE(pdmap->getGridConfig().find("dim=1"), std::string::npos);

  IvPDomain expanded;
  expanded.addDomain("speed", 0, 5, 6);
  expanded.addDomain("course", 0, 4, 5);
  expanded.addDomain("depth", 0, 10, 11);
  const int placement[] = {1};
  ASSERT_TRUE(pdmap->transDomain(expanded, placement));

  EXPECT_EQ(pdmap->getDim(), 3);
  EXPECT_NE(pdmap->getGridConfig().find("dim=3"), std::string::npos);

  IvPBox query = makePointBox(3, {3, 4, 5});
  bool covered = false;
  EXPECT_DOUBLE_EQ(pdmap->evalPoint(&query, &covered), 24);
  EXPECT_TRUE(covered);
  EXPECT_EQ(pdmap->getIX(&query), 1);
  EXPECT_DOUBLE_EQ(pdmap->getGrid()->getCheapBound(&query), 24);
}

// Covers IvP function behavior: weights copies and translates domain for helm objective functions.
TEST(IvPFunctionTest, WeightsCopiesAndTranslatesDomainForHelmObjectiveFunctions)
{
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(makeTwoPieceCourseMap().release()));
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 10);
  ipf->setPWT(20);
  ipf->setPWT(-1);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 20);
  ipf->setContextStr("bhv=waypt_survey");
  EXPECT_EQ(ipf->getContextStr(), "bhv=waypt_survey");
  EXPECT_EQ(ipf->getVarName(0), "course");

  std::unique_ptr<IvPFunction> copy(ipf->copy());
  EXPECT_EQ(copy->getContextStr(), "bhv=waypt_survey");
  EXPECT_DOUBLE_EQ(copy->getPWT(), 20);
  copy->getPDMap()->applyScalar(100);
  EXPECT_NE(copy->getValMinUtil(), ipf->getValMinUtil());

  IvPDomain expanded;
  expanded.addDomain("speed", 0, 5, 6);
  expanded.addDomain("course", 0, 4, 5);
  expanded.addDomain("depth", 0, 10, 11);
  EXPECT_TRUE(ipf->transDomain(expanded));
  EXPECT_EQ(ipf->getDim(), 3);
  EXPECT_EQ(ipf->getVarName(1), "course");

  IvPDomain missing;
  missing.addDomain("speed", 0, 5, 6);
  EXPECT_FALSE(copy->transDomain(missing));
}

// Covers IvP function behavior: failed trans domain leaves function unchanged.
TEST(IvPFunctionTest, FailedTransDomainLeavesFunctionUnchanged)
{
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(makeTwoPieceCourseMap().release()));
  const IvPDomain original_domain = ipf->getPDMap()->getDomain();
  const double original_value = evalPDMapAtIndex(*ipf->getPDMap(), 4);

  IvPDomain smaller;
  smaller.addDomain("speed", 0, 5, 6);
  ASSERT_FALSE(ipf->transDomain(smaller));

  EXPECT_EQ(ipf->getPDMap()->getDomain(), original_domain);
  EXPECT_EQ(ipf->getDim(), 1);
  EXPECT_EQ(ipf->getVarName(0), "course");
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*ipf->getPDMap(), 4), original_value);
}

// Covers IvP function behavior: reports grid config and utility range through function wrapper.
TEST(IvPFunctionTest, ReportsGridConfigAndUtilityRangeThroughFunctionWrapper)
{
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(makeTwoPieceCourseMap().release()));
  EXPECT_TRUE(ipf->valid());
  EXPECT_TRUE(ipf->freeOfNan());
  EXPECT_EQ(ipf->size(), 2);
  EXPECT_EQ(ipf->getVarName(99), "");
  EXPECT_DOUBLE_EQ(ipf->getValMinUtil(), 10);
  EXPECT_DOUBLE_EQ(ipf->getValMaxUtil(), 24);
  EXPECT_EQ(ipf->getGridConfig(), "");

  ipf->getPDMap()->updateGrid();
  EXPECT_NE(ipf->getGridConfig().find("dim=1"), std::string::npos);

  IvPDomain same = ipf->getPDMap()->getDomain();
  EXPECT_TRUE(ipf->transDomain(same));
  EXPECT_EQ(ipf->getDim(), 1);
}

// Covers IvP function behavior: wraps empty maps with conservative defaults.
TEST(IvPFunctionTest, WrapsEmptyMapsWithConservativeDefaults)
{
  IvPDomain domain = makeCourseOnlyDomain();
  IvPFunction ipf(new PDMap(0, domain, 1));

  EXPECT_EQ(ipf.size(), 0);
  EXPECT_EQ(ipf.getDim(), 1);
  EXPECT_EQ(ipf.getVarName(0), "course");
  EXPECT_TRUE(ipf.freeOfNan());
  EXPECT_FALSE(ipf.valid());
  EXPECT_DOUBLE_EQ(ipf.getValMinUtil(), 0);
  EXPECT_DOUBLE_EQ(ipf.getValMaxUtil(), 0);
  EXPECT_EQ(ipf.getGridConfig(), "");
}

// Covers IvP function behavior: copy after grid update preserves grid and independence.
TEST(IvPFunctionTest, CopyAfterGridUpdatePreservesGridAndIndependence)
{
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(makeTwoPieceCourseMap().release()));
  ipf->setPWT(0);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 0);

  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(ipf->getPDMap()->setGelBox(gel));
  ipf->getPDMap()->updateGrid();
  ASSERT_NE(ipf->getGridConfig().find("dim=1"), std::string::npos);

  std::unique_ptr<IvPFunction> copy(ipf->copy());
  EXPECT_DOUBLE_EQ(copy->getPWT(), 0);
  EXPECT_EQ(copy->getGridConfig(), ipf->getGridConfig());
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*copy->getPDMap(), 4), 24);

  ipf->getPDMap()->applyScalar(100);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*copy->getPDMap(), 4), 24);
  EXPECT_DOUBLE_EQ(evalPDMapAtIndex(*ipf->getPDMap(), 4), 124);
}

// Covers IvP function behavior: copy context and priority remain independent.
TEST(IvPFunctionTest, CopyContextAndPriorityRemainIndependent)
{
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(makeTwoPieceCourseMap().release()));
  ipf->setContextStr("bhv=station_keep,run=alpha");
  ipf->setPWT(5);

  std::unique_ptr<IvPFunction> copy(ipf->copy());
  ASSERT_NE(copy.get(), nullptr);
  EXPECT_EQ(copy->getContextStr(), "bhv=station_keep,run=alpha");
  EXPECT_DOUBLE_EQ(copy->getPWT(), 5);

  ipf->setContextStr("bhv=waypoint,run=bravo");
  ipf->setPWT(-3);
  copy->setPWT(0);

  EXPECT_EQ(ipf->getContextStr(), "bhv=waypoint,run=bravo");
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 5);
  EXPECT_EQ(copy->getContextStr(), "bhv=station_keep,run=alpha");
  EXPECT_DOUBLE_EQ(copy->getPWT(), 0);
}

// Covers IvP function behavior: trans domain rebuilds existing grid for expanded helm domain.
TEST(IvPFunctionTest, TransDomainRebuildsExistingGridForExpandedHelmDomain)
{
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(makeTwoPieceCourseMap().release()));
  IvPBox gel(1);
  gel.setPTS(0, 0, 2);
  ASSERT_TRUE(ipf->getPDMap()->setGelBox(gel));
  ipf->getPDMap()->updateGrid();

  IvPDomain expanded;
  expanded.addDomain("speed", 0, 5, 6);
  expanded.addDomain("course", 0, 4, 5);
  expanded.addDomain("depth", 0, 10, 11);
  ASSERT_TRUE(ipf->transDomain(expanded));

  EXPECT_EQ(ipf->getDim(), 3);
  EXPECT_EQ(ipf->getVarName(1), "course");
  EXPECT_NE(ipf->getGridConfig().find("dim=3"), std::string::npos);

  IvPBox query = makePointBox(3, {3, 4, 5});
  bool covered = false;
  EXPECT_DOUBLE_EQ(ipf->getPDMap()->evalPoint(&query, &covered), 24);
  EXPECT_TRUE(covered);
}

// Covers IvP function behavior: free of NaN reflects underlying map state.
TEST(IvPFunctionTest, FreeOfNanReflectsUnderlyingMapState)
{
  IvPDomain domain = makeCourseOnlyDomain();
  PDMap* pdmap = new PDMap(1, domain, 1);
  pdmap->bx(0) = new IvPBox(intervalBox(0, 4, 0));
  pdmap->bx(0)->wt(0) = std::numeric_limits<double>::quiet_NaN();

  IvPFunction ipf(pdmap);
  EXPECT_FALSE(ipf.freeOfNan());
  EXPECT_TRUE(ipf.valid());
  EXPECT_EQ(ipf.size(), 1);
}
