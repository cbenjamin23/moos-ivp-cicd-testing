#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "BuildUtils.h"
#include "Compactor.h"
#include "FunctionEncoder.h"
#include "IvPBox.h"
#include "IvPDomain.h"
#include "IvPFunction.h"
#include "IvPProblem.h"
#include "IvPProblem_v2.h"
#include "IvPProblem_v3.h"
#include "PDMap.h"
#include "PopulatorIPP.h"
#include "TestFileUtils.h"

namespace {

IvPDomain oneDimDomain(const std::string& var, unsigned int points)
{
  IvPDomain domain;
  domain.addDomain(var, 0, points - 1, points);
  return domain;
}

IvPDomain courseSpeedDomain()
{
  IvPDomain domain;
  domain.addDomain("course", 0, 4, 5);
  domain.addDomain("speed", 0, 3, 4);
  return domain;
}

IvPBox oneDimLinearBox(int low, int high, double slope, double intercept)
{
  IvPBox box(1, 1);
  box.setPTS(0, low, high);
  box.wt(0) = slope;
  box.wt(1) = intercept;
  return box;
}

std::unique_ptr<IvPFunction> oneDimFunction(const std::string& var,
                                            unsigned int points,
                                            double slope,
                                            double intercept,
                                            double pwt = 1.0)
{
  IvPDomain domain = oneDimDomain(var, points);
  std::unique_ptr<PDMap> pdmap(new PDMap(1, domain, 1));
  pdmap->bx(0) = new IvPBox(oneDimLinearBox(0, points - 1, slope, intercept));
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  ipf->setPWT(pwt);
  ipf->setContextStr("bhv=test_" + var);
  return ipf;
}

std::unique_ptr<IvPFunction> twoPieceCourseFunction()
{
  IvPDomain domain = oneDimDomain("course", 5);
  std::unique_ptr<PDMap> pdmap(new PDMap(2, domain, 1));
  pdmap->bx(0) = new IvPBox(oneDimLinearBox(0, 2, 1, 0));
  pdmap->bx(1) = new IvPBox(oneDimLinearBox(3, 4, 1, 0));
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  ipf->setPWT(1);
  return ipf;
}

class RemovingCompactor : public Compactor {
 public:
  bool compact(IvPBox* box) override { return box->pt(0, 0) >= 3; }
  double maxVal(IvPBox* box, bool* ok) override
  {
    *ok = true;
    return box->maxVal();
  }
};

}  // namespace

// Covers problem behavior: admits positive priority functions and applies weighting.
TEST(ProblemTest, AdmitsPositivePriorityFunctionsAndAppliesWeighting)
{
  IvPProblem problem;
  problem.setDomain(oneDimDomain("course", 5));
  problem.setThresh(130);
  EXPECT_DOUBLE_EQ(problem.getThresh(), 100);
  problem.setThresh(-10);
  EXPECT_DOUBLE_EQ(problem.getThresh(), 0);
  problem.setEpsilon(4);
  problem.setEpsilon(-1);
  EXPECT_DOUBLE_EQ(problem.getEpsilon(), 4);

  std::unique_ptr<IvPFunction> rejected = oneDimFunction("course", 5, 1, 0, 0);
  problem.addOF(rejected.get());
  EXPECT_EQ(problem.getOFNUM(), 0);

  std::unique_ptr<IvPFunction> accepted = oneDimFunction("course", 5, 50, 0, 2);
  problem.addOF(accepted.release());
  ASSERT_EQ(problem.getOFNUM(), 1);
  ASSERT_NE(problem.getOF(0), nullptr);
  EXPECT_DOUBLE_EQ(problem.getOF(0)->getPWT(), 2);
  EXPECT_NEAR(problem.getOF(0)->getValMinUtil(), 0, 1e-9);
  EXPECT_NEAR(problem.getOF(0)->getValMaxUtil(), 200, 1e-9);
  EXPECT_EQ(problem.getOF(-1), nullptr);
  EXPECT_EQ(problem.getOF(1), nullptr);
}

// Covers problem behavior: sorts objective functions and reports piece averages.
TEST(ProblemTest, SortsObjectiveFunctionsAndReportsPieceAverages)
{
  IvPProblem problem;
  problem.setDomain(oneDimDomain("course", 5));
  problem.addOF(oneDimFunction("course", 5, 1, 0, 5).release());
  problem.addOF(oneDimFunction("course", 5, -1, 4, 1).release());
  ASSERT_EQ(problem.getOFNUM(), 2);
  EXPECT_DOUBLE_EQ(problem.getPieceAvg(), 1);

  problem.sortOFs(false);
  EXPECT_DOUBLE_EQ(problem.getOF(0)->getPWT(), 1);
  problem.sortOFs(true);
  EXPECT_DOUBLE_EQ(problem.getOF(0)->getPWT(), 5);
}

// Covers problem behavior: rejects unaligned domains and empty solves.
TEST(ProblemTest, RejectsUnalignedDomainsAndEmptySolves)
{
  IvPProblem empty;
  EXPECT_FALSE(empty.solve());

  IvPProblem problem;
  problem.setDomain(oneDimDomain("course", 5));
  problem.addOF(oneDimFunction("speed", 4, 1, 0).release());
  ASSERT_EQ(problem.getOFNUM(), 1);
  EXPECT_FALSE(problem.alignOFs());
  EXPECT_EQ(problem.getGridConfig(-1), "");
  EXPECT_EQ(problem.getGridConfig(99), "");
}

// Covers IvP problem behavior: aligns one dimensional behavior functions to helm domain and solves.
TEST(IvPProblemTest, AlignsOneDimensionalBehaviorFunctionsToHelmDomainAndSolves)
{
  IvPProblem problem;
  problem.setDomain(courseSpeedDomain());
  problem.addOF(oneDimFunction("course", 5, 1, 0).release());
  problem.addOF(oneDimFunction("speed", 4, 2, 0).release());
  ASSERT_EQ(problem.getOFNUM(), 2);
  ASSERT_TRUE(problem.alignOFs());
  EXPECT_EQ(problem.getDim(), 2);

  ASSERT_TRUE(problem.solve());
  bool ok = false;
  EXPECT_DOUBLE_EQ(problem.getResult("course", &ok), 4);
  EXPECT_TRUE(ok);
  EXPECT_DOUBLE_EQ(problem.getResult("speed", &ok), 3);
  EXPECT_TRUE(ok);
  EXPECT_DOUBLE_EQ(problem.getResult("depth", &ok), 0);
  EXPECT_FALSE(ok);
  EXPECT_NEAR(problem.getMaxWT(), 10, 1e-9);
  EXPECT_NEAR(problem.getResultVal(), 10, 1e-9);
  EXPECT_GT(problem.getLeafsVisited(), 0);
  EXPECT_NE(problem.getGridConfig(0).find("dim=2"), std::string::npos);
}

// Covers IvP problem behavior: pre compacts and removes violating pieces before search.
TEST(IvPProblemTest, PreCompactsAndRemovesViolatingPiecesBeforeSearch)
{
  RemovingCompactor compactor;
  IvPProblem problem(&compactor);
  problem.setDomain(oneDimDomain("course", 5));
  problem.addOF(twoPieceCourseFunction().release());
  ASSERT_EQ(problem.getOFNUM(), 1);
  ASSERT_EQ(problem.getOF(0)->size(), 2);

  problem.preCompact();
  ASSERT_EQ(problem.getOF(0)->size(), 1);
  EXPECT_EQ(problem.getOF(0)->getPDMap()->getBox(0)->pt(0, 0), 0);
  EXPECT_TRUE(problem.solve());

  bool ok = false;
  EXPECT_DOUBLE_EQ(problem.getResult("course", &ok), 2);
  EXPECT_TRUE(ok);
}

// Covers IvP problem behavior: uses initial solution only when all functions cover the point.
TEST(IvPProblemTest, UsesInitialSolutionOnlyWhenAllFunctionsCoverThePoint)
{
  IvPProblem problem;
  problem.setDomain(oneDimDomain("course", 5));
  problem.addOF(oneDimFunction("course", 5, 1, 0).release());

  IvPBox non_point(1);
  non_point.setPTS(0, 0, 4);
  problem.processInitSol(&non_point);
  EXPECT_EQ(problem.getMaxBox(), nullptr);

  IvPBox point(1);
  point.setPTS(0, 3, 3);
  problem.processInitSol(&point);
  ASSERT_NE(problem.getMaxBox(), nullptr);
  EXPECT_DOUBLE_EQ(problem.getMaxWT(), 3);
  EXPECT_DOUBLE_EQ(problem.getResultVal(), 0);
  bool ok = false;
  EXPECT_DOUBLE_EQ(problem.getResult("course", &ok), 3);
  EXPECT_TRUE(ok);
}

// Covers IvP problem behavior: initial solution helpers use existing function grids.
TEST(IvPProblemTest, InitialSolutionHelpersUseExistingFunctionGrids)
{
  IvPProblem problem;
  problem.setDomain(oneDimDomain("course", 5));
  std::unique_ptr<IvPFunction> low = oneDimFunction("course", 5, -1, 4, 1);
  std::unique_ptr<IvPFunction> high = oneDimFunction("course", 5, 1, 0, 3);
  low->getPDMap()->updateGrid();
  high->getPDMap()->updateGrid();
  problem.addOF(low.release());
  problem.addOF(high.release());

  problem.initialSolution1();
  ASSERT_NE(problem.getMaxBox(), nullptr);
  bool ok = false;
  EXPECT_DOUBLE_EQ(problem.getResult("course", &ok), 4);
  EXPECT_TRUE(ok);

  problem.initialSolution2();
  EXPECT_DOUBLE_EQ(problem.getResult("course", &ok), 4);
  EXPECT_TRUE(ok);
}

// Covers IvP problem variant behavior: solves same problem across legacy algorithms.
TEST(IvPProblemVariantTest, SolvesSameProblemAcrossLegacyAlgorithms)
{
  IvPProblem_v2 v2;
  v2.setDomain(oneDimDomain("course", 5));
  v2.addOF(oneDimFunction("course", 5, 1, 0).release());
  ASSERT_TRUE(v2.solve());
  bool ok = false;
  EXPECT_DOUBLE_EQ(v2.getResult("course", &ok), 4);
  EXPECT_TRUE(ok);

  IvPProblem_v3 v3;
  v3.setDomain(oneDimDomain("course", 5));
  v3.addOF(oneDimFunction("course", 5, 1, 0).release());
  ASSERT_TRUE(v3.solve());
  EXPECT_DOUBLE_EQ(v3.getResult("course", &ok), 4);
  EXPECT_TRUE(ok);
}

// Covers populator ipp behavior: populates encoded problem files and overrides grid size.
TEST(PopulatorIPPTest, PopulatesEncodedProblemFilesAndOverridesGridSize)
{
  std::unique_ptr<IvPFunction> ipf = oneDimFunction("course", 5, 1, 0);
  const std::string encoded = IvPFunctionToString(ipf.get());
  TempFile file("valid_ipp",
                "# minimal pHelmIvP-style problem file\n"
                "domain = " + domainToString(oneDimDomain("course", 5)) + "\n"
                "ipfs = 1\n"
                "ipf = " + encoded + "\n");

  PopulatorIPP populator;
  populator.setVerbose(false);
  populator.setGridOverrideSize(2);
  ASSERT_TRUE(populator.populate(file.path(), 3));
  ASSERT_NE(populator.getIvPProblem(), nullptr);
  EXPECT_EQ(populator.getIvPProblem()->getOFNUM(), 1);
  EXPECT_NE(populator.getIvPProblem()->getGridConfig(0).find("sz[0]:2"),
            std::string::npos);
  EXPECT_TRUE(populator.getIvPProblem()->solve());

  bool ok = false;
  EXPECT_DOUBLE_EQ(populator.getIvPProblem()->getResult("course", &ok), 4);
  EXPECT_TRUE(ok);
}

// Covers populator ipp behavior: rejects missing files and malformed lines.
TEST(PopulatorIPPTest, RejectsMissingFilesAndMalformedLines)
{
  PopulatorIPP missing;
  missing.setVerbose(false);
  EXPECT_FALSE(missing.populate("/tmp/moos_ivp_cicd_missing.ipp"));
  EXPECT_EQ(missing.getIvPProblem(), nullptr);

  TempFile file("bad_ipp",
                "domain = course:0:4:5\n"
                "not_a_field = nope\n");

  PopulatorIPP malformed;
  malformed.setVerbose(false);
  EXPECT_FALSE(malformed.populate(file.path()));
  EXPECT_EQ(malformed.getIvPProblem(), nullptr);
}
