#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "AOF.h"
#include "FColorMap.h"
#include "FunctionEncoder.h"
#include "IPF_Bundle.h"
#include "IPF_BundleSeries.h"
#include "IPF_Entry.h"
#include "IPF_Utils.h"
#include "IPFViewUtils.h"
#include "IvPBuildTestUtils.h"
#include "IvPBox.h"
#include "IvPDomain.h"
#include "IvPFunction.h"
#include "NumericAssertions.h"
#include "PDMap.h"
#include "Quad3D.h"
#include "QuadSet.h"
#include "QuadSet1D.h"
#include "ZAIC_PEAK.h"

namespace {

IvPDomain smallCourseSpeedDomain()
{
  IvPDomain domain;
  domain.addDomain("course", 0, 3, 4);
  domain.addDomain("speed", 0, 2, 3);
  return domain;
}

IvPDomain smallSpeedCourseDomain()
{
  IvPDomain domain;
  domain.addDomain("speed", 0, 2, 3);
  domain.addDomain("course", 0, 3, 4);
  return domain;
}

IvPDomain smallDepthDomain()
{
  IvPDomain domain;
  domain.addDomain("depth", 0, 4, 5);
  return domain;
}

std::unique_ptr<IvPFunction> makeLinear2DIPF(const IvPDomain& domain,
                                             double priority = 1.0,
                                             const std::string& context = "")
{
  std::unique_ptr<PDMap> pdmap(new PDMap(1, domain, 1));
  std::unique_ptr<IvPBox> box(new IvPBox(2, 1));
  box->setPTS(0, 0, domain.getVarPoints(0) - 1);
  box->setPTS(1, 0, domain.getVarPoints(1) - 1);
  box->wt(0) = 10;
  box->wt(1) = 100;
  box->wt(2) = 5;
  pdmap->bx(0) = box.release();

  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  ipf->setPWT(priority);
  ipf->setContextStr(context);
  return ipf;
}

Quad3D makeViewerQuad(double base = 0)
{
  Quad3D quad;
  quad.setLLX(0);
  quad.setHLX(2);
  quad.setHHX(2);
  quad.setLHX(0);
  quad.setLLY(0);
  quad.setHLY(0);
  quad.setHHY(2);
  quad.setLHY(2);
  quad.setLLZ(base + 1);
  quad.setHLZ(base + 3);
  quad.setHHZ(base + 7);
  quad.setLHZ(base + 5);
  quad.setLLR(0.2);
  quad.setHLR(0.4);
  quad.setHHR(0.8);
  quad.setLHR(0.6);
  quad.setLLG(1);
  quad.setHLG(2);
  quad.setHHG(4);
  quad.setLHG(3);
  quad.setLLB(10);
  quad.setHLB(20);
  quad.setHHB(40);
  quad.setLHB(30);
  return quad;
}

QuadSet makeViewerQuadSet()
{
  QuadSet set;
  set.setIvPDomain(smallCourseSpeedDomain());
  set.addQuad3D(makeViewerQuad());
  Quad3D high = makeViewerQuad(10);
  high.setLLX(2);
  high.setHLX(3);
  high.setHHX(3);
  high.setLHX(2);
  high.setLLY(1);
  high.setHLY(1);
  high.setHHY(2);
  high.setLHY(2);
  set.addQuad3D(high);
  set.resetMinMaxVals();
  return set;
}

class LinearViewerAOF : public AOF {
 public:
  explicit LinearViewerAOF(const IvPDomain& domain) : AOF(domain) {}

  double evalBox(const IvPBox* box) const override
  {
    return (2 * extract("course", box)) + (3 * extract("speed", box)) + 1;
  }
};

std::unique_ptr<IvPFunction> makePiecewiseOneDimIPF(double priority,
                                                    const std::string& context)
{
  std::unique_ptr<PDMap> pdmap(new PDMap(2, smallDepthDomain(), 1));
  std::unique_ptr<IvPBox> low(new IvPBox(1, 1));
  low->setPTS(0, 0, 2);
  low->wt(0) = 1;
  low->wt(1) = 0;
  pdmap->bx(0) = low.release();

  std::unique_ptr<IvPBox> high(new IvPBox(1, 1));
  high->setPTS(0, 3, 4);
  high->wt(0) = -1;
  high->wt(1) = 10;
  pdmap->bx(1) = high.release();

  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  ipf->setPWT(priority);
  ipf->setContextStr(context);
  return ipf;
}

std::string makeEncodedCourseIPF(const std::string& source,
                                 unsigned int iteration,
                                 double summit,
                                 double priority)
{
  ZAIC_PEAK zaic(makeCourseSpeedDomain(), "course");
  EXPECT_TRUE(zaic.setParams(summit, 5, 60, 0, 0, 100));
  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  EXPECT_NE(ipf, nullptr);
  ipf->setPWT(priority);
  ipf->setContextStr(std::to_string(iteration) + ":" + source);
  return IvPFunctionToString(ipf.get());
}

std::string makeEncodedDepthIPF(const std::string& source,
                                unsigned int iteration,
                                double summit,
                                double priority)
{
  IvPDomain domain;
  domain.addDomain("depth", 0, 100, 101);
  ZAIC_PEAK zaic(domain, "depth");
  EXPECT_TRUE(zaic.setParams(summit, 2, 20, 0, 0, 100));
  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  EXPECT_NE(ipf, nullptr);
  ipf->setPWT(priority);
  ipf->setContextStr(std::to_string(iteration) + ":" + source);
  return IvPFunctionToString(ipf.get());
}

std::string makeEncodedSpeedIPF(const std::string& source,
                                unsigned int iteration,
                                double summit,
                                double priority)
{
  ZAIC_PEAK zaic(makeCourseSpeedDomain(), "speed");
  EXPECT_TRUE(zaic.setParams(summit, 1, 4, 0, 0, 100));
  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  EXPECT_NE(ipf, nullptr);
  ipf->setPWT(priority);
  ipf->setContextStr(std::to_string(iteration) + ":" + source);
  return IvPFunctionToString(ipf.get());
}

std::string encodeLinear2DWithContext(const IvPDomain& domain,
                                      const std::string& context,
                                      double priority = 1.0)
{
  std::unique_ptr<IvPFunction> ipf =
      makeLinear2DIPF(domain, priority, context);
  std::string encoded = IvPFunctionToString(ipf.get());
  EXPECT_FALSE(encoded.empty());
  return encoded;
}

}  // namespace

// Covers IPF entry behavior: decodes helm posted IPF strings and builds viewer quad set.
TEST(IPFEntryTest, DecodesHelmPostedIpfStringsAndBuildsViewerQuadSet)
{
  const std::string encoded = makeEncodedCourseIPF("waypt_survey", 42, 90, 75);
  IPF_Entry entry(encoded);

  EXPECT_EQ(entry.getIPFString(), encoded);
  EXPECT_EQ(entry.getPieces(), 0u);
  EXPECT_DOUBLE_EQ(entry.getPriority(), 0);

  std::unique_ptr<IvPFunction> decoded(entry.getIvPFunction());
  ASSERT_NE(decoded, nullptr);
  EXPECT_TRUE(decoded->valid());
  EXPECT_EQ(decoded->getContextStr(), "42:waypt_survey");
  EXPECT_DOUBLE_EQ(decoded->getPWT(), 75);
  EXPECT_GT(entry.getPieces(), 0u);
  EXPECT_DOUBLE_EQ(entry.getPriority(), 75);

  QuadSet qset = entry.getQuadSet(makeCourseSpeedDomain());
  EXPECT_GT(qset.size(), 0u);
  EXPECT_TRUE(qset.getDomain().hasOnlyDomain("course", "speed"));
  EXPECT_EQ(entry.getDomain().getVarName(0), "course");
  EXPECT_GE(qset.getMaxVal(), qset.getMinVal());
}

// Covers IPF entry behavior: empty function string keeps defaults and empty quad set.
TEST(IPFEntryTest, EmptyFunctionStringKeepsDefaultsAndEmptyQuadSet)
{
  IPF_Entry entry("");
  EXPECT_EQ(entry.getIPFString(), "");
  EXPECT_EQ(entry.getPieces(), 0u);
  EXPECT_DOUBLE_EQ(entry.getPriority(), 0);
  EXPECT_EQ(entry.getDomain().size(), 0u);
  EXPECT_EQ(entry.getIvPFunction(), nullptr);
  EXPECT_EQ(entry.getQuadSet(makeCourseSpeedDomain()).size(), 0u);
}

// Covers IPF entry behavior: malformed function string is retained but never builds state.
TEST(IPFEntryTest, MalformedFunctionStringIsRetainedButNeverBuildsState)
{
  IPF_Entry entry("not_an_encoded_ipf");

  EXPECT_EQ(entry.getIPFString(), "not_an_encoded_ipf");
  EXPECT_EQ(entry.getPieces(), 0u);
  EXPECT_DOUBLE_EQ(entry.getPriority(), 0);
  EXPECT_EQ(entry.getDomain().size(), 0u);
}

// Covers IPF entry behavior: first quad set request caches dense or sparse choice.
TEST(IPFEntryTest, FirstQuadSetRequestCachesDenseOrSparseChoice)
{
  const std::string encoded = makeEncodedCourseIPF("waypt_survey", 12, 90, 55);

  IPF_Entry sparse_first(encoded);
  QuadSet sparse = sparse_first.getQuadSet(makeCourseSpeedDomain(), false);
  QuadSet later_dense = sparse_first.getQuadSet(makeCourseSpeedDomain(), true);
  ASSERT_GT(sparse.size(), 0u);
  EXPECT_EQ(later_dense.size(), sparse.size());
  EXPECT_DOUBLE_EQ(later_dense.getMaxVal(), sparse.getMaxVal());

  IPF_Entry dense_first(encoded);
  QuadSet dense = dense_first.getQuadSet(makeCourseSpeedDomain(), true);
  QuadSet later_sparse = dense_first.getQuadSet(makeCourseSpeedDomain(), false);
  ASSERT_GT(dense.size(), 0u);
  EXPECT_EQ(later_sparse.size(), dense.size());
  EXPECT_DOUBLE_EQ(later_sparse.getMaxVal(), dense.getMaxVal());
}

// Covers IPF entry behavior: IvP function decode returns fresh functions and leaves domain lazy.
TEST(IPFEntryTest, IvPFunctionDecodeReturnsFreshFunctionsAndLeavesDomainLazy)
{
  const std::string encoded =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "5:linear", 7);
  IPF_Entry entry(encoded);

  std::unique_ptr<IvPFunction> first(entry.getIvPFunction());
  std::unique_ptr<IvPFunction> second(entry.getIvPFunction());
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  EXPECT_NE(first.get(), second.get());
  EXPECT_EQ(first->getContextStr(), "5:linear");
  EXPECT_EQ(second->getContextStr(), "5:linear");
  EXPECT_DOUBLE_EQ(first->getPWT(), 7);
  EXPECT_DOUBLE_EQ(second->getPWT(), 7);
  EXPECT_GT(entry.getPieces(), 0u);
  EXPECT_DOUBLE_EQ(entry.getPriority(), 7);
  EXPECT_EQ(entry.getDomain().size(), 0u);

  QuadSet qset = entry.getQuadSet(smallCourseSpeedDomain());
  EXPECT_GT(qset.size(), 0u);
  EXPECT_TRUE(entry.getDomain().hasOnlyDomain("course", "speed"));
}

// Covers IPF entry behavior: one dimensional entry populates metadata but no two d quad set.
TEST(IPFEntryTest, OneDimensionalEntryPopulatesMetadataButNoTwoDQuadSet)
{
  IPF_Entry entry(makeEncodedDepthIPF("depth_hold", 6, 30, 11));

  QuadSet qset = entry.getQuadSet(makeCourseSpeedDomain());
  EXPECT_EQ(qset.size(), 0u);
  EXPECT_GT(entry.getPieces(), 0u);
  EXPECT_DOUBLE_EQ(entry.getPriority(), 11);
  EXPECT_TRUE(entry.getDomain().hasOnlyDomain("depth"));
}

// Covers IPF bundle behavior: keeps one function per behavior source within helm iteration.
TEST(IPFBundleTest, KeepsOneFunctionPerBehaviorSourceWithinHelmIteration)
{
  const std::string first = makeEncodedCourseIPF("waypt_survey", 10, 45, 40);
  const std::string duplicate_source =
      makeEncodedCourseIPF("waypt_survey", 10, 90, 80);
  const std::string other_source = makeEncodedSpeedIPF("speed_hold", 10, 3, 25);
  const std::string wrong_iteration =
      makeEncodedCourseIPF("loiter", 11, 270, 60);

  IPF_Bundle bundle;
  bundle.addIPF(first);
  bundle.addIPF(duplicate_source);
  bundle.addIPF(other_source);
  bundle.addIPF(wrong_iteration);

  ASSERT_EQ(bundle.size(), 2u);
  EXPECT_EQ(bundle.getSource(0), "waypt_survey");
  EXPECT_EQ(bundle.getSource(1), "speed_hold");
  EXPECT_EQ(bundle.getSource(99), "");
  EXPECT_EQ(bundle.getIPFString(0), first);
  EXPECT_EQ(bundle.getIPFString(99), "");

  QuadSet waypt = bundle.getQuadSet("waypt_survey");
  QuadSet speed = bundle.getQuadSet("speed_hold");
  EXPECT_GT(waypt.size(), 0u);
  EXPECT_GT(speed.size(), 0u);
  EXPECT_EQ(bundle.getQuadSet("missing").size(), 0u);
  EXPECT_DOUBLE_EQ(bundle.getPriority("waypt_survey"), 40);
  EXPECT_DOUBLE_EQ(bundle.getPriority("speed_hold"), 25);
  EXPECT_DOUBLE_EQ(bundle.getPriority("missing"), 0);
  EXPECT_GT(bundle.getPieces("waypt_survey"), 0u);
  EXPECT_TRUE(bundle.getDomain().hasOnlyDomain("course", "speed"));
}

// Covers IPF bundle behavior: separates collective heading speed surface from depth only source.
TEST(IPFBundleTest, SeparatesCollectiveHeadingSpeedSurfaceFromDepthOnlySource)
{
  IPF_Bundle bundle;
  bundle.addIPF(makeEncodedCourseIPF("waypt_survey", 7, 120, 50));
  bundle.addIPF(makeEncodedSpeedIPF("speed_hold", 7, 2, 25));
  bundle.addIPF(makeEncodedDepthIPF("constant_depth", 7, 25, 30));

  ASSERT_EQ(bundle.size(), 3u);
  EXPECT_EQ(bundle.getDomain().size(), 3u);
  EXPECT_TRUE(bundle.getDomain().hasDomain("course"));
  EXPECT_TRUE(bundle.getDomain().hasDomain("speed"));
  EXPECT_TRUE(bundle.getDomain().hasDomain("depth"));

  QuadSet hdgspd = bundle.getCollectiveQuadSet("collective-hdgspd");
  QuadSet depth = bundle.getCollectiveQuadSet("collective-depth");
  QuadSet unknown = bundle.getCollectiveQuadSet("unknown");

  EXPECT_GT(hdgspd.size(), 0u);
  EXPECT_EQ(depth.size(), 0u);
  EXPECT_EQ(unknown.size(), 0u);
  EXPECT_EQ(bundle.getDomain("constant_depth").size(), 1u);
}

// Covers IPF bundle behavior: default collective quad set means heading speed.
TEST(IPFBundleTest, DefaultCollectiveQuadSetMeansHeadingSpeed)
{
  IPF_Bundle bundle;
  bundle.addIPF(makeEncodedCourseIPF("course_hold", 8, 90, 20));
  bundle.addIPF(makeEncodedSpeedIPF("speed_hold", 8, 2, 10));
  bundle.addIPF(makeEncodedDepthIPF("depth_hold", 8, 30, 5));

  QuadSet implicit = bundle.getCollectiveQuadSet();
  QuadSet explicit_hdgspd = bundle.getCollectiveQuadSet("collective-hdgspd");
  QuadSet depth = bundle.getCollectiveQuadSet("collective-depth");

  ASSERT_GT(implicit.size(), 0u);
  EXPECT_EQ(implicit.size(), explicit_hdgspd.size());
  EXPECT_DOUBLE_EQ(implicit.getMinVal(), explicit_hdgspd.getMinVal());
  EXPECT_DOUBLE_EQ(implicit.getMaxVal(), explicit_hdgspd.getMaxVal());
  EXPECT_EQ(depth.size(), 0u);
}

// Covers IPF bundle behavior: index and string accessors handle missing entries.
TEST(IPFBundleTest, IndexAndStringAccessorsHandleMissingEntries)
{
  IPF_Bundle bundle;
  const std::string course = makeEncodedCourseIPF("course_one", 3, 45, 10);
  bundle.addIPF(course);

  EXPECT_EQ(bundle.getSource(0), "course_one");
  EXPECT_EQ(bundle.getSource(9), "");
  EXPECT_EQ(bundle.getIPFString(0), course);
  EXPECT_EQ(bundle.getIPFString(9), "");
  EXPECT_EQ(bundle.getQuadSet(9).size(), 0u);
  EXPECT_EQ(bundle.getQuadSet("missing").size(), 0u);
  EXPECT_DOUBLE_EQ(bundle.getPriority("missing"), 0);
  EXPECT_EQ(bundle.getPieces("missing"), 0u);
  EXPECT_EQ(bundle.getDomain("missing").size(), 0u);

  std::vector<std::string> strings = bundle.getIPFStrings();
  ASSERT_EQ(strings.size(), 1u);
  EXPECT_EQ(strings[0], course);
}

// Covers IPF bundle behavior: entry metadata populates lazily after quad set decode.
TEST(IPFBundleTest, EntryMetadataPopulatesLazilyAfterQuadSetDecode)
{
  IPF_Bundle bundle;
  bundle.addIPF(encodeLinear2DWithContext(smallCourseSpeedDomain(),
                                          "3:course_speed", 10));

  EXPECT_DOUBLE_EQ(bundle.getPriority("course_speed"), 0);
  EXPECT_EQ(bundle.getPieces("course_speed"), 0u);
  EXPECT_EQ(bundle.getDomain("course_speed").size(), 0u);
  EXPECT_TRUE(bundle.getDomain().hasOnlyDomain("course", "speed"));

  QuadSet qset = bundle.getQuadSet("course_speed");
  ASSERT_GT(qset.size(), 0u);
  EXPECT_DOUBLE_EQ(bundle.getPriority("course_speed"), 10);
  EXPECT_GT(bundle.getPieces("course_speed"), 0u);
  EXPECT_TRUE(bundle.getDomain("course_speed").hasOnlyDomain("course", "speed"));
  EXPECT_TRUE(bundle.getDomain().hasOnlyDomain("course", "speed"));
}

// Covers IPF bundle behavior: index quad set access populates metadata like source access.
TEST(IPFBundleTest, IndexQuadSetAccessPopulatesMetadataLikeSourceAccess)
{
  IPF_Bundle bundle;
  bundle.addIPF(encodeLinear2DWithContext(smallCourseSpeedDomain(),
                                          "4:indexed", 12));

  EXPECT_DOUBLE_EQ(bundle.getPriority("indexed"), 0);
  EXPECT_EQ(bundle.getPieces("indexed"), 0u);
  EXPECT_EQ(bundle.getDomain("indexed").size(), 0u);

  QuadSet qset = bundle.getQuadSet(0);
  ASSERT_GT(qset.size(), 0u);
  EXPECT_DOUBLE_EQ(bundle.getPriority("indexed"), 12);
  EXPECT_GT(bundle.getPieces("indexed"), 0u);
  EXPECT_TRUE(bundle.getDomain("indexed").hasOnlyDomain("course", "speed"));
  EXPECT_EQ(bundle.getQuadSet(99).size(), 0u);
}

// Covers IPF bundle behavior: first quad set request caches subsequent dense or sparse requests.
TEST(IPFBundleTest, FirstQuadSetRequestCachesSubsequentDenseOrSparseRequests)
{
  IPF_Bundle bundle;
  bundle.addIPF(makeEncodedCourseIPF("waypt_survey", 9, 60, 30));
  bundle.addIPF(makeEncodedSpeedIPF("speed_hold", 9, 2, 20));

  QuadSet sparse = bundle.getQuadSet("waypt_survey", false);
  QuadSet dense = bundle.getQuadSet("waypt_survey", true);
  ASSERT_GT(sparse.size(), 0u);
  ASSERT_GT(dense.size(), 0u);
  EXPECT_EQ(dense.size(), sparse.size());
  EXPECT_DOUBLE_EQ(dense.getMaxVal(), sparse.getMaxVal());
  EXPECT_TRUE(dense.getDomain().hasOnlyDomain("course", "speed"));
}

// Covers IPF bundle behavior: context without colon becomes iteration zero empty source.
TEST(IPFBundleTest, ContextWithoutColonBecomesIterationZeroEmptySource)
{
  IPF_Bundle bundle;
  const std::string first =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "no_colon_context", 3);
  const std::string duplicate_empty_source =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "0:", 5);
  const std::string same_iteration_named =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "0:named_source", 7);
  const std::string different_iteration =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "1:ignored_source", 9);

  bundle.addIPF(first);
  bundle.addIPF(duplicate_empty_source);
  bundle.addIPF(same_iteration_named);
  bundle.addIPF(different_iteration);

  ASSERT_EQ(bundle.size(), 2u);
  EXPECT_EQ(bundle.getSource(0), "");
  EXPECT_EQ(bundle.getSource(1), "named_source");
  EXPECT_EQ(bundle.getIPFString(0), first);
  EXPECT_EQ(bundle.getIPFString(1), same_iteration_named);
  EXPECT_TRUE(bundle.getDomain().hasOnlyDomain("course", "speed"));
}

// Covers IPF bundle behavior: context parsing keeps text after first colon as source.
TEST(IPFBundleTest, ContextParsingKeepsTextAfterFirstColonAsSource)
{
  IPF_Bundle bundle;
  const std::string colon_source =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "12:bhv:submode", 4);
  const std::string negative_iter =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "-1:negative", 6);
  const std::string non_numeric_iter =
      encodeLinear2DWithContext(smallCourseSpeedDomain(), "abc:nonnumeric", 8);

  bundle.addIPF(colon_source);
  bundle.addIPF(negative_iter);
  bundle.addIPF(non_numeric_iter);

  ASSERT_EQ(bundle.size(), 1u);
  EXPECT_EQ(bundle.getSource(0), "bhv:submode");
  EXPECT_EQ(bundle.getIPFString(0), colon_source);
  EXPECT_GT(bundle.getQuadSet("bhv:submode").size(), 0u);

  IPF_Bundle zero_iter_bundle;
  zero_iter_bundle.addIPF(non_numeric_iter);
  zero_iter_bundle.addIPF(encodeLinear2DWithContext(smallCourseSpeedDomain(),
                                                    "0:zero", 10));
  ASSERT_EQ(zero_iter_bundle.size(), 2u);
  EXPECT_EQ(zero_iter_bundle.getSource(0), "nonnumeric");
  EXPECT_EQ(zero_iter_bundle.getSource(1), "zero");
}

// Covers IPF bundle series behavior: indexes bundles by iteration and trims front and back.
TEST(IPFBundleSeriesTest, IndexesBundlesByIterationAndTrimsFrontAndBack)
{
  IPF_BundleSeries series;
  EXPECT_EQ(series.size(), 0u);
  EXPECT_EQ(series.getFirstSource(), "");

  series.addIPF(makeEncodedCourseIPF("waypt_survey", 4, 45, 20));
  series.addIPF(makeEncodedSpeedIPF("speed_hold", 4, 3, 15));
  series.addIPF(makeEncodedCourseIPF("station_keep", 6, 90, 30));
  series.addIPF(makeEncodedSpeedIPF("station_speed", 6, 2, 10));
  series.addIPF(makeEncodedDepthIPF("constant_depth", 8, 30, 40));

  EXPECT_EQ(series.size(), 3u);
  EXPECT_EQ(series.getMinIteration(), 4u);
  EXPECT_EQ(series.getMaxIteration(), 8u);
  EXPECT_EQ(series.getTotalFunctions(4), 2u);
  EXPECT_EQ(series.getTotalFunctions(5), 0u);
  EXPECT_EQ(series.size(), 4u);
  EXPECT_GT(series.getQuadSet(4, "waypt_survey").size(), 0u);
  EXPECT_GT(series.getQuadSet(6, "station_keep").size(), 0u);
  EXPECT_DOUBLE_EQ(series.getPriority(6, "station_keep"), 30);
  EXPECT_EQ(series.getTotalFunctions(8), 1u);
  EXPECT_EQ(series.getDomain(8, "constant_depth").size(), 0u);
  EXPECT_EQ(series.getIPFStrings(4).size(), 2u);
  EXPECT_EQ(series.getAllSources().size(), 5u);
  EXPECT_EQ(series.getFirstSource(), "waypt_survey");

  series.popFront();
  EXPECT_EQ(series.getMinIteration(), 5u);
  EXPECT_EQ(series.size(), 3u);
  series.popBack();
  EXPECT_EQ(series.getMaxIteration(), 7u);
  EXPECT_EQ(series.size(), 2u);
  series.popBack(99);
  EXPECT_EQ(series.size(), 0u);
  EXPECT_EQ(series.getMinIteration(), 0u);
  EXPECT_EQ(series.getMaxIteration(), 0u);
}

// Covers IPF bundle series behavior: delegates collective quad sets and clears sources.
TEST(IPFBundleSeriesTest, DelegatesCollectiveQuadSetsAndClearsSources)
{
  IPF_BundleSeries series;
  series.addIPF(makeEncodedCourseIPF("waypt_survey", 4, 45, 20));
  series.addIPF(makeEncodedSpeedIPF("speed_hold", 4, 3, 15));
  series.addIPF(makeEncodedCourseIPF("station_keep", 6, 90, 30));
  series.addIPF(makeEncodedSpeedIPF("station_speed", 6, 2, 10));

  EXPECT_GT(series.getCollectiveQuadSet(4, "collective-hdgspd").size(), 0u);
  QuadSet dense = series.getQuadSet(6, "station_keep", true);
  QuadSet sparse = series.getQuadSet(6, "station_keep", false);
  ASSERT_GT(dense.size(), 0u);
  EXPECT_EQ(sparse.size(), dense.size());
  EXPECT_DOUBLE_EQ(sparse.getMaxVal(), dense.getMaxVal());
  EXPECT_DOUBLE_EQ(series.getPriority(6, "station_keep"), 30);
  EXPECT_GT(series.getPieces(6, "station_keep"), 0u);
  EXPECT_EQ(series.getCollectiveQuadSet(99, "collective-hdgspd").size(), 0u);
  EXPECT_EQ(series.getTotalFunctions(99), 0u);
  EXPECT_EQ(series.size(), 3u);

  series.clear();
  EXPECT_EQ(series.size(), 0u);
  EXPECT_EQ(series.getFirstSource(), "waypt_survey");
}

// Covers IPF bundle series behavior: empty collective type differs from explicit heading speed.
TEST(IPFBundleSeriesTest, EmptyCollectiveTypeDiffersFromExplicitHeadingSpeed)
{
  IPF_BundleSeries series;
  series.addIPF(makeEncodedCourseIPF("course_hold", 5, 90, 20));
  series.addIPF(makeEncodedSpeedIPF("speed_hold", 5, 2, 10));
  series.addIPF(makeEncodedDepthIPF("depth_hold", 5, 30, 5));

  QuadSet implicit = series.getCollectiveQuadSet(5);
  QuadSet explicit_hdgspd = series.getCollectiveQuadSet(5, "collective-hdgspd");

  EXPECT_EQ(implicit.size(), 0u);
  EXPECT_GT(explicit_hdgspd.size(), 0u);
  EXPECT_EQ(series.getCollectiveQuadSet(5, "collective-depth").size(), 0u);
}

// Covers IPF bundle series behavior: pop noops and oversized pops preserve window rules.
TEST(IPFBundleSeriesTest, PopNoopsAndOversizedPopsPreserveWindowRules)
{
  IPF_BundleSeries series;
  series.popFront();
  series.popBack();
  EXPECT_EQ(series.size(), 0u);
  EXPECT_EQ(series.getMinIteration(), 0u);
  EXPECT_EQ(series.getMaxIteration(), 0u);

  series.addIPF(makeEncodedCourseIPF("early", 2, 45, 10));
  series.addIPF(makeEncodedCourseIPF("late", 5, 90, 20));
  ASSERT_EQ(series.size(), 2u);
  series.popFront(0);
  series.popBack(0);
  EXPECT_EQ(series.size(), 2u);
  EXPECT_EQ(series.getMinIteration(), 2u);
  EXPECT_EQ(series.getMaxIteration(), 5u);

  series.popFront(2);
  EXPECT_EQ(series.size(), 0u);
  EXPECT_EQ(series.getMinIteration(), 0u);
  EXPECT_EQ(series.getMaxIteration(), 0u);

  series.addIPF(makeEncodedCourseIPF("again", 9, 120, 30));
  ASSERT_EQ(series.size(), 1u);
  series.popBack(99);
  EXPECT_EQ(series.size(), 0u);
  EXPECT_EQ(series.getMinIteration(), 0u);
  EXPECT_EQ(series.getMaxIteration(), 0u);
}

// Covers IPF bundle series behavior: pop front and back traverse missing iteration gaps.
TEST(IPFBundleSeriesTest, PopFrontAndBackTraverseMissingIterationGaps)
{
  IPF_BundleSeries series;
  series.addIPF(makeEncodedCourseIPF("early", 2, 45, 10));
  series.addIPF(makeEncodedCourseIPF("middle", 5, 90, 20));
  series.addIPF(makeEncodedCourseIPF("late", 9, 135, 30));

  ASSERT_EQ(series.size(), 3u);
  EXPECT_EQ(series.getMinIteration(), 2u);
  EXPECT_EQ(series.getMaxIteration(), 9u);

  series.popFront();
  EXPECT_EQ(series.size(), 2u);
  EXPECT_EQ(series.getMinIteration(), 3u);
  EXPECT_EQ(series.getMaxIteration(), 9u);

  series.popFront();
  EXPECT_EQ(series.size(), 1u);
  EXPECT_EQ(series.getMinIteration(), 6u);
  EXPECT_EQ(series.getMaxIteration(), 9u);

  series.addIPF(makeEncodedCourseIPF("new_late", 12, 180, 40));
  ASSERT_EQ(series.size(), 2u);
  EXPECT_EQ(series.getMaxIteration(), 12u);
  series.popBack();
  EXPECT_EQ(series.size(), 1u);
  EXPECT_EQ(series.getMaxIteration(), 11u);
}

// Covers IPF bundle series behavior: missing iteration accessors insert empty bundles.
TEST(IPFBundleSeriesTest, MissingIterationAccessorsInsertEmptyBundles)
{
  IPF_BundleSeries series;
  series.addIPF(makeEncodedCourseIPF("waypt_survey", 4, 45, 20));
  ASSERT_EQ(series.size(), 1u);
  EXPECT_EQ(series.getMinIteration(), 4u);
  EXPECT_EQ(series.getMaxIteration(), 4u);

  EXPECT_EQ(series.getDomain(99).size(), 0u);
  EXPECT_EQ(series.getDomain(100, "missing").size(), 0u);
  EXPECT_DOUBLE_EQ(series.getPriority(101, "missing"), 0);
  EXPECT_EQ(series.getPieces(102, "missing"), 0u);
  EXPECT_EQ(series.getQuadSet(103, "missing").size(), 0u);
  EXPECT_EQ(series.getCollectiveQuadSet(104, "collective-hdgspd").size(), 0u);
  EXPECT_TRUE(series.getIPFStrings(105).empty());
  EXPECT_EQ(series.getTotalFunctions(106), 0u);

  EXPECT_EQ(series.size(), 9u);
  EXPECT_EQ(series.getMinIteration(), 4u);
  EXPECT_EQ(series.getMaxIteration(), 4u);
}

// Covers IPF bundle series behavior: tracks unique sources in first seen order across iterations.
TEST(IPFBundleSeriesTest, TracksUniqueSourcesInFirstSeenOrderAcrossIterations)
{
  IPF_BundleSeries series;
  series.addIPF(makeEncodedCourseIPF("alpha", 1, 45, 10));
  series.addIPF(makeEncodedSpeedIPF("bravo", 1, 2, 20));
  series.addIPF(makeEncodedCourseIPF("alpha", 2, 90, 30));
  series.addIPF(makeEncodedDepthIPF("charlie", 3, 30, 40));

  std::vector<std::string> sources = series.getAllSources();
  ASSERT_EQ(sources.size(), 3u);
  EXPECT_EQ(sources[0], "alpha");
  EXPECT_EQ(sources[1], "bravo");
  EXPECT_EQ(sources[2], "charlie");
  EXPECT_EQ(series.getFirstSource(), "alpha");
}

// Covers IPF bundle series behavior: whole iteration domain is eager but source domain is lazy.
TEST(IPFBundleSeriesTest, WholeIterationDomainIsEagerButSourceDomainIsLazy)
{
  IPF_BundleSeries series;
  series.addIPF(encodeLinear2DWithContext(smallCourseSpeedDomain(),
                                          "12:course_speed", 8));
  series.addIPF(makeEncodedDepthIPF("depth_hold", 12, 25, 5));

  IvPDomain whole = series.getDomain(12);
  EXPECT_EQ(whole.size(), 3u);
  EXPECT_TRUE(whole.hasDomain("course"));
  EXPECT_TRUE(whole.hasDomain("speed"));
  EXPECT_TRUE(whole.hasDomain("depth"));
  EXPECT_EQ(series.getDomain(12, "course_speed").size(), 0u);
  EXPECT_EQ(series.getDomain(12, "depth_hold").size(), 0u);

  EXPECT_GT(series.getQuadSet(12, "course_speed").size(), 0u);
  EXPECT_TRUE(series.getDomain(12, "course_speed")
                  .hasOnlyDomain("course", "speed"));
  EXPECT_EQ(series.getQuadSet(12, "depth_hold").size(), 0u);
  EXPECT_TRUE(series.getDomain(12, "depth_hold").hasOnlyDomain("depth"));
}

// Covers IPF view utils behavior: expands one dimensional heading speed functions and preserves priority.
TEST(IPFViewUtilsTest, ExpandsOneDimensionalHeadingSpeedFunctionsAndPreservesPriority)
{
  std::unique_ptr<IvPFunction> course(StringToIvPFunction(
      makeEncodedCourseIPF("course_only", 1, 90, 33)));
  ASSERT_NE(course, nullptr);
  IvPFunction* expanded_course = expandHdgSpdIPF(course.release(),
                                                 makeCourseSpeedDomain());
  std::unique_ptr<IvPFunction> owned_course(expanded_course);
  ASSERT_NE(owned_course, nullptr);
  EXPECT_EQ(owned_course->getDim(), 2);
  EXPECT_TRUE(owned_course->getPDMap()->getDomain().hasOnlyDomain("course", "speed"));
  EXPECT_DOUBLE_EQ(owned_course->getPWT(), 33);

  std::unique_ptr<IvPFunction> speed(StringToIvPFunction(
      makeEncodedSpeedIPF("speed_only", 1, 2, 44)));
  ASSERT_NE(speed, nullptr);
  IvPFunction* expanded_speed = expandHdgSpdIPF(speed.release(),
                                                makeCourseSpeedDomain());
  std::unique_ptr<IvPFunction> owned_speed(expanded_speed);
  ASSERT_NE(owned_speed, nullptr);
  EXPECT_EQ(owned_speed->getDim(), 2);
  EXPECT_TRUE(owned_speed->getPDMap()->getDomain().hasOnlyDomain("course", "speed"));
  EXPECT_DOUBLE_EQ(owned_speed->getPWT(), 44);
}

// Covers IPF view utils behavior: expand leaves null two dimensional and unexpandable functions.
TEST(IPFViewUtilsTest, ExpandLeavesNullTwoDimensionalAndUnexpandableFunctions)
{
  EXPECT_EQ(expandHdgSpdIPF(nullptr, makeCourseSpeedDomain()), nullptr);

  std::unique_ptr<IvPFunction> two_dim =
      makeLinear2DIPF(smallCourseSpeedDomain(), 8, "2:linear");
  IvPFunction* two_dim_raw = two_dim.get();
  EXPECT_EQ(expandHdgSpdIPF(two_dim_raw, makeCourseSpeedDomain()), two_dim_raw);

  std::unique_ptr<IvPFunction> depth(StringToIvPFunction(
      makeEncodedDepthIPF("depth_only", 2, 20, 9)));
  ASSERT_NE(depth, nullptr);
  IvPFunction* depth_raw = depth.get();
  EXPECT_EQ(expandHdgSpdIPF(depth_raw, makeCourseSpeedDomain()), depth_raw);
}

// Covers IPF view utils behavior: one dimensional expansion requires complementary domain.
TEST(IPFViewUtilsTest, OneDimensionalExpansionRequiresComplementaryDomain)
{
  std::unique_ptr<IvPFunction> course(StringToIvPFunction(
      makeEncodedCourseIPF("course_only", 1, 90, 12)));
  ASSERT_NE(course, nullptr);
  IvPFunction* course_raw = course.get();
  EXPECT_EQ(expandHdgSpdIPF(course_raw, smallDepthDomain()), course_raw);
  EXPECT_EQ(course->getDim(), 1);
  EXPECT_DOUBLE_EQ(course->getPWT(), 12);

  std::unique_ptr<IvPFunction> speed(StringToIvPFunction(
      makeEncodedSpeedIPF("speed_only", 1, 2, 14)));
  ASSERT_NE(speed, nullptr);
  IvPFunction* speed_raw = speed.get();
  EXPECT_EQ(expandHdgSpdIPF(speed_raw, smallDepthDomain()), speed_raw);
  EXPECT_EQ(speed->getDim(), 1);
  EXPECT_DOUBLE_EQ(speed->getPWT(), 14);
}

// Covers IPF utils behavior: build quads from cache covers patch and course wrap bridge.
TEST(IPFUtilsTest, BuildQuadsFromCacheCoversPatchAndCourseWrapBridge)
{
  std::vector<std::vector<double> > vals = {
      {0, 1, 2},
      {10, 11, 12},
      {20, 21, 22},
      {30, 31, 32}};

  std::vector<Quad3D> quads = buildQuadsFromCache(vals);
  ASSERT_EQ(quads.size(), 8u);
  EXPECT_DOUBLE_EQ(quads[0].getLLZ(), 0);
  EXPECT_DOUBLE_EQ(quads[0].getHLZ(), 10);
  EXPECT_DOUBLE_EQ(quads[0].getHHZ(), 11);
  EXPECT_DOUBLE_EQ(quads[0].getLHZ(), 1);

  const Quad3D& bridge = quads.back();
  EXPECT_DOUBLE_EQ(bridge.getLLX(), 2);
  EXPECT_DOUBLE_EQ(bridge.getHLX(), 0);
  EXPECT_DOUBLE_EQ(bridge.getLLZ(), 31);
  EXPECT_DOUBLE_EQ(bridge.getHLZ(), 1);

  std::vector<Quad3D> patched = buildQuadsFromCache(vals, 2);
  ASSERT_EQ(patched.size(), 4u);
  EXPECT_DOUBLE_EQ(patched[0].getHHX(), 2);
  EXPECT_DOUBLE_EQ(patched[0].getHHY(), 1);
  EXPECT_TRUE(buildQuadsFromCache(std::vector<std::vector<double> >()).empty());
}

// Covers IPF utils behavior: build quads from cache clamps oversized patch corners.
TEST(IPFUtilsTest, BuildQuadsFromCacheClampsOversizedPatchCorners)
{
  std::vector<std::vector<double> > vals = {
      {0, 1, 2, 3},
      {10, 11, 12, 13},
      {20, 21, 22, 23},
      {30, 31, 32, 33},
      {40, 41, 42, 43}};

  std::vector<Quad3D> quads = buildQuadsFromCache(vals, 3);
  ASSERT_EQ(quads.size(), 5u);
  EXPECT_DOUBLE_EQ(quads[0].getHLX(), 3);
  EXPECT_DOUBLE_EQ(quads[0].getHHY(), 2);
  EXPECT_DOUBLE_EQ(quads[0].getHHZ(), 32);
  EXPECT_DOUBLE_EQ(quads[1].getLLX(), 3);
  EXPECT_DOUBLE_EQ(quads[1].getHLX(), 3);
  EXPECT_DOUBLE_EQ(quads[1].getLLY(), 0);
  EXPECT_DOUBLE_EQ(quads[1].getHHY(), 2);
  EXPECT_DOUBLE_EQ(quads[1].getHHZ(), 32);
  EXPECT_DOUBLE_EQ(quads.back().getLLX(), 3);
  EXPECT_DOUBLE_EQ(quads.back().getHLX(), 0);
}

// Covers IPF utils behavior: build quads from cache honors patch stride across interior and bridge.
TEST(IPFUtilsTest, BuildQuadsFromCacheHonorsPatchStrideAcrossInteriorAndBridge)
{
  std::vector<std::vector<double> > vals = {
      {0, 1, 2, 3, 4},
      {10, 11, 12, 13, 14},
      {20, 21, 22, 23, 24},
      {30, 31, 32, 33, 34},
      {40, 41, 42, 43, 44},
      {50, 51, 52, 53, 54}};

  std::vector<Quad3D> quads = buildQuadsFromCache(vals, 2);
  ASSERT_EQ(quads.size(), 10u);
  EXPECT_DOUBLE_EQ(quads[0].getLLX(), 0);
  EXPECT_DOUBLE_EQ(quads[0].getHHX(), 2);
  EXPECT_DOUBLE_EQ(quads[0].getHHY(), 2);
  EXPECT_DOUBLE_EQ(quads[0].getHHZ(), 22);

  EXPECT_DOUBLE_EQ(quads[5].getLLX(), 4);
  EXPECT_DOUBLE_EQ(quads[5].getHLX(), 4);
  EXPECT_DOUBLE_EQ(quads[5].getLLY(), 2);
  EXPECT_DOUBLE_EQ(quads[5].getHHY(), 3);
  EXPECT_DOUBLE_EQ(quads[5].getHHZ(), 43);

  EXPECT_DOUBLE_EQ(quads[6].getLLX(), 4);
  EXPECT_DOUBLE_EQ(quads[6].getHLX(), 0);
  EXPECT_DOUBLE_EQ(quads[6].getLLZ(), 50);
  EXPECT_DOUBLE_EQ(quads[6].getHLZ(), 0);
  EXPECT_DOUBLE_EQ(quads.back().getLHZ(), 54);
  EXPECT_DOUBLE_EQ(quads.back().getHHZ(), 4);
}

// Covers IPF utils behavior: sparse and dense IPF builders preserve viewer surface values.
TEST(IPFUtilsTest, SparseAndDenseIPFBuildersPreserveViewerSurfaceValues)
{
  std::unique_ptr<IvPFunction> sparse_ipf =
      makeLinear2DIPF(smallCourseSpeedDomain());
  QuadSet sparse = buildQuadSet2DFromIPF(sparse_ipf.get());
  ASSERT_EQ(sparse.size(), 1u);
  EXPECT_DOUBLE_EQ(sparse.getQuad(0).getLLZ(), 5);
  EXPECT_DOUBLE_EQ(sparse.getQuad(0).getHLZ(), 35);
  EXPECT_DOUBLE_EQ(sparse.getQuad(0).getLHZ(), 205);
  EXPECT_DOUBLE_EQ(sparse.getQuad(0).getHHZ(), 235);
  EXPECT_DOUBLE_EQ(sparse.getMaxPoint("course"), 3);
  EXPECT_DOUBLE_EQ(sparse.getMaxPoint("speed"), 2);

  std::unique_ptr<IvPFunction> dense_ipf =
      makeLinear2DIPF(smallSpeedCourseDomain(), 2);
  QuadSet dense = buildQuadSetDense2DFromIPF(dense_ipf.get());
  ASSERT_EQ(dense.size(), 8u);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getLLZ(), 10);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getHLZ(), 210);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getLHZ(), 30);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getHHZ(), 230);
  EXPECT_DOUBLE_EQ(dense.getMinVal(), 10);
  EXPECT_DOUBLE_EQ(dense.getMaxVal(), 650);
}

// Covers IPF utils behavior: sparse builder uses raw piece values while dense builder applies priority.
TEST(IPFUtilsTest, SparseBuilderUsesRawPieceValuesWhileDenseBuilderAppliesPriority)
{
  std::unique_ptr<IvPFunction> sparse_ipf =
      makeLinear2DIPF(smallCourseSpeedDomain(), 3);
  QuadSet sparse = buildQuadSet2DFromIPF(sparse_ipf.get());
  ASSERT_EQ(sparse.size(), 1u);
  EXPECT_DOUBLE_EQ(sparse.getQuad(0).getLLZ(), 5);
  EXPECT_DOUBLE_EQ(sparse.getMaxVal(), 235);

  std::unique_ptr<IvPFunction> dense_ipf =
      makeLinear2DIPF(smallCourseSpeedDomain(), 3);
  QuadSet dense = buildQuadSetDense2DFromIPF(dense_ipf.get());
  ASSERT_GT(dense.size(), 0u);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getLLZ(), 15);
  EXPECT_DOUBLE_EQ(dense.getMaxVal(), 705);
}

// Covers IPF utils behavior: sparse builder preserves multiple partial pieces.
TEST(IPFUtilsTest, SparseBuilderPreservesMultiplePartialPieces)
{
  IvPDomain domain = smallCourseSpeedDomain();
  std::unique_ptr<PDMap> pdmap(new PDMap(2, domain, 1));

  std::unique_ptr<IvPBox> low(new IvPBox(2, 1));
  low->setPTS(0, 0, 1);
  low->setPTS(1, 0, 1);
  low->wt(0) = 2;
  low->wt(1) = 20;
  low->wt(2) = 1;
  pdmap->bx(0) = low.release();

  std::unique_ptr<IvPBox> high(new IvPBox(2, 1));
  high->setPTS(0, 2, 3);
  high->setPTS(1, 1, 2);
  high->wt(0) = -5;
  high->wt(1) = 50;
  high->wt(2) = 10;
  pdmap->bx(1) = high.release();

  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  QuadSet set = buildQuadSet2DFromIPF(ipf.get());
  ASSERT_EQ(set.size(), 2u);

  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLX(), 0);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHHY(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHLZ(), 3);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHHZ(), 23);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLHZ(), 21);

  EXPECT_DOUBLE_EQ(set.getQuad(1).getLLX(), 2);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHLX(), 3);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getLLY(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHY(), 2);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getLLZ(), 50);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHLZ(), 45);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHZ(), 95);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getLHZ(), 100);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 100);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("course"), 2);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("speed"), 2);
}

// Covers IPF utils behavior: builder guards reject null wrong dimension and non linear pieces.
TEST(IPFUtilsTest, BuilderGuardsRejectNullWrongDimensionAndNonLinearPieces)
{
  EXPECT_EQ(buildQuadSetFromIPF(nullptr).size(), 0u);
  EXPECT_EQ(buildQuadSetDense2DFromIPF(nullptr).size(), 0u);
  EXPECT_EQ(buildQuadSet2DFromIPF(nullptr).size(), 0u);
  EXPECT_TRUE(buildQuadSet1DFromIPF(nullptr, "none").isEmptyND());

  std::unique_ptr<IvPFunction> one_dim(StringToIvPFunction(
      makeEncodedDepthIPF("constant_depth", 1, 20, 10)));
  ASSERT_NE(one_dim, nullptr);
  EXPECT_EQ(buildQuadSetFromIPF(one_dim.get()).size(), 0u);

  IvPDomain domain = smallCourseSpeedDomain();
  std::unique_ptr<PDMap> pdmap(new PDMap(1, domain, 2));
  std::unique_ptr<IvPBox> box(new IvPBox(2, 2));
  box->setPTS(0, 0, 3);
  box->setPTS(1, 0, 2);
  box->wt(0) = 1;
  box->wt(1) = 1;
  pdmap->bx(0) = box.release();
  std::unique_ptr<IvPFunction> quadratic(new IvPFunction(pdmap.release()));
  EXPECT_EQ(buildQuadSet2DFromIPF(quadratic.get()).size(), 0u);
}

// Covers IPF utils behavior: public builder dispatches sparse dense and one dimensional paths.
TEST(IPFUtilsTest, PublicBuilderDispatchesSparseDenseAndOneDimensionalPaths)
{
  std::unique_ptr<IvPFunction> sparse_ipf =
      makeLinear2DIPF(smallCourseSpeedDomain());
  QuadSet sparse = buildQuadSetFromIPF(sparse_ipf.get(), false);
  ASSERT_EQ(sparse.size(), 1u);
  EXPECT_DOUBLE_EQ(sparse.getMaxVal(), 235);

  std::unique_ptr<IvPFunction> dense_ipf =
      makeLinear2DIPF(smallCourseSpeedDomain());
  QuadSet dense = buildQuadSetFromIPF(dense_ipf.get(), true);
  ASSERT_EQ(dense.size(), 8u);
  EXPECT_DOUBLE_EQ(dense.getMaxVal(), 235);

  EXPECT_TRUE(buildQuadSet1DFromIPF(dense_ipf.get(), "two_dim").isEmptyND());
  std::unique_ptr<IvPFunction> one_dim = makePiecewiseOneDimIPF(1, "1:depth");
  QuadSet1D one_dim_set = buildQuadSet1DFromIPF(one_dim.get(), "depth");
  ASSERT_FALSE(one_dim_set.isEmptyND());
  EXPECT_TRUE(one_dim_set.getDomain().hasOnlyDomain("depth"));
}

// Covers IPF utils behavior: dense AOF builder samples domain values and honors patch size.
TEST(IPFUtilsTest, DenseAOFBuilderSamplesDomainValuesAndHonorsPatchSize)
{
  LinearViewerAOF aof(smallCourseSpeedDomain());
  QuadSet set = buildQuadSetDense2DFromAOF(&aof, 2);
  ASSERT_EQ(set.size(), 4u);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHLZ(), 5);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLHZ(), 4);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHHZ(), 8);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 13);

  EXPECT_EQ(buildQuadSetDense2DFromAOF(nullptr).size(), 0u);
  LinearViewerAOF wrong_dim(smallDepthDomain());
  EXPECT_EQ(buildQuadSetDense2DFromAOF(&wrong_dim).size(), 0u);
}

// Covers IPF utils behavior: dense AOF builder clamps oversized patch like cache builder.
TEST(IPFUtilsTest, DenseAOFBuilderClampsOversizedPatchLikeCacheBuilder)
{
  LinearViewerAOF aof(smallCourseSpeedDomain());
  QuadSet set = buildQuadSetDense2DFromAOF(&aof, 99);

  ASSERT_EQ(set.size(), 3u);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLX(), 0);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHLX(), 2);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHHY(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHHZ(), 8);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getLLX(), 2);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHLX(), 0);
  EXPECT_DOUBLE_EQ(set.getQuad(2).getLHZ(), 13);
  EXPECT_DOUBLE_EQ(set.getQuad(2).getHHZ(), 7);
}

// Covers IPF utils behavior: dense builders handle non heading speed two dimensional domains.
TEST(IPFUtilsTest, DenseBuildersHandleNonHeadingSpeedTwoDimensionalDomains)
{
  IvPDomain xy_domain;
  xy_domain.addDomain("x", 0, 2, 3);
  xy_domain.addDomain("y", 0, 1, 2);
  std::unique_ptr<IvPFunction> ipf = makeLinear2DIPF(xy_domain, 3);

  QuadSet dense = buildQuadSetDense2DFromIPF(ipf.get());
  ASSERT_EQ(dense.size(), 3u);
  EXPECT_TRUE(dense.getDomain().hasOnlyDomain("x", "y"));
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getLLZ(), 15);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getHLZ(), 45);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getLHZ(), 15);
  EXPECT_DOUBLE_EQ(dense.getQuad(0).getHHZ(), 45);
  EXPECT_DOUBLE_EQ(dense.getMinVal(), 15);
  EXPECT_DOUBLE_EQ(dense.getMaxVal(), 375);
  EXPECT_DOUBLE_EQ(dense.getMaxPoint("x"), 1);
  EXPECT_DOUBLE_EQ(dense.getMaxPoint("y"), 1);
  EXPECT_DOUBLE_EQ(dense.getMaxPoint("course"), 0);
  EXPECT_EQ(dense.getMaxPointQIX("course"), 1u);
  EXPECT_EQ(dense.getMaxPointQIX("speed"), 1u);
  EXPECT_EQ(dense.getMaxPointQIX("x"), 0u);
}

// Covers IPF utils behavior: cache builder handles degenerate rows and columns.
TEST(IPFUtilsTest, CacheBuilderHandlesDegenerateRowsAndColumns)
{
  std::vector<std::vector<double> > one_speed = {
      {0},
      {10},
      {20}};
  EXPECT_TRUE(buildQuadsFromCache(one_speed).empty());

  std::vector<std::vector<double> > one_course = {
      {0, 1, 2}};
  std::vector<Quad3D> bridge_only = buildQuadsFromCache(one_course);
  ASSERT_EQ(bridge_only.size(), 2u);
  EXPECT_DOUBLE_EQ(bridge_only[0].getLLX(), -1);
  EXPECT_DOUBLE_EQ(bridge_only[0].getHLX(), 0);
  EXPECT_DOUBLE_EQ(bridge_only[0].getLLZ(), 0);
  EXPECT_DOUBLE_EQ(bridge_only[0].getHLZ(), 0);
  EXPECT_DOUBLE_EQ(bridge_only[1].getLHZ(), 2);
  EXPECT_DOUBLE_EQ(bridge_only[1].getHHZ(), 2);
}

// Covers quad set behavior: empty set and out of range access use conservative defaults.
TEST(QuadSetTest, EmptySetAndOutOfRangeAccessUseConservativeDefaults)
{
  QuadSet set;
  EXPECT_EQ(set.size(), 0u);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 0);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 0);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("course"), 0);
  EXPECT_EQ(set.getMaxPointQIX("course"), 0u);
  EXPECT_DOUBLE_EQ(set.getQuad(99).getAvgVal(), 0);
}

// Covers quad set behavior: reset min max tracks every corner and keeps first tie.
TEST(QuadSetTest, ResetMinMaxTracksEveryCornerAndKeepsFirstTie)
{
  QuadSet set;
  set.setIvPDomain(smallCourseSpeedDomain());
  Quad3D quad = makeViewerQuad();
  quad.setLLZ(4);
  quad.setLHZ(20);
  quad.setHLZ(-3);
  quad.setHHZ(18);
  set.addQuad3D(quad);
  set.resetMinMaxVals();

  EXPECT_DOUBLE_EQ(set.getMinVal(), -3);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 20);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("course"), 0);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("speed"), 2);

  Quad3D tie = makeViewerQuad();
  tie.setLLZ(20);
  tie.setHLZ(20);
  tie.setHHZ(20);
  tie.setLHZ(20);
  set.addQuad3D(tie);
  set.resetMinMaxVals();

  EXPECT_DOUBLE_EQ(set.getMaxVal(), 20);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("course"), 0);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("speed"), 2);
}

// Covers quad set behavior: max point returns zero when peak index exceeds domain bounds.
TEST(QuadSetTest, MaxPointReturnsZeroWhenPeakIndexExceedsDomainBounds)
{
  QuadSet set;
  set.setIvPDomain(smallCourseSpeedDomain());

  Quad3D quad = makeViewerQuad();
  quad.setHHX(99);
  quad.setHHY(99);
  quad.setHHZ(50);
  set.addQuad3D(quad);
  set.resetMinMaxVals();

  EXPECT_DOUBLE_EQ(set.getMaxVal(), 50);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("course"), 0);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("speed"), 0);
  EXPECT_EQ(set.getMaxPointQIX("course"), 99u);
  EXPECT_EQ(set.getMaxPointQIX("speed"), 99u);
}

// Covers quad set behavior: tracks min max normalizes and sums matching surfaces.
TEST(QuadSetTest, TracksMinMaxNormalizesAndSumsMatchingSurfaces)
{
  QuadSet set = makeViewerQuadSet();
  EXPECT_EQ(set.size(), 2u);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("course"), 3);
  EXPECT_DOUBLE_EQ(set.getMaxPoint("speed"), 2);

  QuadSet accumulator;
  accumulator.addQuadSet(set);
  ASSERT_EQ(accumulator.size(), 2u);
  accumulator.addQuadSet(set);
  EXPECT_DOUBLE_EQ(accumulator.getQuad(0).getLLZ(), 2);
  EXPECT_DOUBLE_EQ(accumulator.getQuad(1).getHHZ(), 34);
  EXPECT_DOUBLE_EQ(accumulator.getMaxVal(), 34);

  QuadSet mismatch;
  mismatch.addQuad3D(makeViewerQuad(100));
  mismatch.resetMinMaxVals();
  accumulator.addQuadSet(mismatch);
  EXPECT_EQ(accumulator.size(), 2u);
  EXPECT_DOUBLE_EQ(accumulator.getMaxVal(), 34);

  accumulator.normalize(10, 90);
  EXPECT_DOUBLE_EQ(accumulator.getMinVal(), 10);
  EXPECT_DOUBLE_EQ(accumulator.getMaxVal(), 100);
}

// Covers quad set behavior: adding empty set does not change existing surface.
TEST(QuadSetTest, AddingEmptySetDoesNotChangeExistingSurface)
{
  QuadSet set = makeViewerQuadSet();
  QuadSet empty;

  set.addQuadSet(empty);

  EXPECT_EQ(set.size(), 2u);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHZ(), 17);
}

// Covers quad set behavior: same sized different domain aggregation still adds by index.
TEST(QuadSetTest, SameSizedDifferentDomainAggregationStillAddsByIndex)
{
  QuadSet course_speed = makeViewerQuadSet();

  QuadSet depth_time;
  IvPDomain depth_time_domain;
  depth_time_domain.addDomain("depth", 0, 10, 2);
  depth_time_domain.addDomain("time", 0, 10, 2);
  depth_time.setIvPDomain(depth_time_domain);
  depth_time.addQuad3D(makeViewerQuad(100));
  depth_time.addQuad3D(makeViewerQuad(200));
  depth_time.resetMinMaxVals();

  course_speed.addQuadSet(depth_time);

  EXPECT_TRUE(course_speed.getDomain().hasOnlyDomain("course", "speed"));
  EXPECT_DOUBLE_EQ(course_speed.getQuad(0).getLLZ(), 102);
  EXPECT_DOUBLE_EQ(course_speed.getQuad(1).getHHZ(), 224);
  EXPECT_DOUBLE_EQ(course_speed.getMaxVal(), 224);
}

// Covers quad set behavior: degenerate normalization and color ranges leave stable values.
TEST(QuadSetTest, DegenerateNormalizationAndColorRangesLeaveStableValues)
{
  QuadSet flat;
  flat.setIvPDomain(smallCourseSpeedDomain());
  Quad3D quad;
  quad.setLLZ(5);
  quad.setHLZ(5);
  quad.setHHZ(5);
  quad.setLHZ(5);
  flat.addQuad3D(quad);
  flat.resetMinMaxVals();
  flat.normalize(0, 100);
  EXPECT_DOUBLE_EQ(flat.getMinVal(), 5);
  EXPECT_DOUBLE_EQ(flat.getMaxVal(), 5);

  FColorMap map;
  flat.applyColorMap(map, 10, 10);
  EXPECT_DOUBLE_EQ(flat.getQuad(0).getLLR(), map.getIRVal(0));
  flat.applyColorMap(map, 10, 0);
  EXPECT_DOUBLE_EQ(flat.getQuad(0).getLLR(), map.getIRVal(0));

  QuadSet empty;
  empty.normalize(0, 100);
  empty.setBase(4);
  empty.applyScale(2);
  empty.applyBase(2);
  empty.applyColorMap(map, 0, 1);
  EXPECT_EQ(empty.size(), 0u);
  EXPECT_DOUBLE_EQ(empty.getMinVal(), 4);
  EXPECT_DOUBLE_EQ(empty.getMaxVal(), 4);
}

// Covers quad set behavior: non positive target range normalization leaves surface unchanged.
TEST(QuadSetTest, NonPositiveTargetRangeNormalizationLeavesSurfaceUnchanged)
{
  QuadSet set = makeViewerQuadSet();
  set.normalize(100, 0);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHZ(), 17);

  set.normalize(100, -5);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 1);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHZ(), 17);
}

// Covers quad set behavior: explicit color map range overrides cached min max.
TEST(QuadSetTest, ExplicitColorMapRangeOverridesCachedMinMax)
{
  QuadSet set = makeViewerQuadSet();
  FColorMap map;

  set.applyColorMap(map, 1, 33);

  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLR(), map.getIRVal(0));
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLG(), map.getIGVal(0));
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHR(), map.getIRVal(0.5));
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHG(), map.getIGVal(0.5));
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHB(), map.getIBVal(0.5));
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);
}

// Covers quad set behavior: color map base and transform delegation reach stored quads.
TEST(QuadSetTest, ColorMapBaseAndTransformDelegationReachStoredQuads)
{
  QuadSet set = makeViewerQuadSet();
  FColorMap map;
  set.applyColorMap(map);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLR(), map.getIRVal(0));
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHR(), map.getIRVal(1));

  set.setBase(-5);
  EXPECT_DOUBLE_EQ(set.getMinVal(), -5);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 11);

  set.interpolate(1);
  set.applyScale(2);
  set.applyBase(3);
  set.applyColorIntensity(0.25);
  set.applyTranslation(10, 20);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), -7);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getZinLOW(0), -5);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLR(), map.getIRVal(0) * 0.25);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLX(), 10);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLY(), 10);
}

// Covers quad set behavior: direct scale and base do not refresh cached extrema until reset.
TEST(QuadSetTest, DirectScaleAndBaseDoNotRefreshCachedExtremaUntilReset)
{
  QuadSet set = makeViewerQuadSet();
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);

  set.applyScale(2);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), 2);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHZ(), 34);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);

  set.applyBase(-4);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLZ(), -2);
  EXPECT_DOUBLE_EQ(set.getQuad(1).getHHZ(), 30);
  EXPECT_DOUBLE_EQ(set.getMinVal(), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 17);

  set.resetMinMaxVals();
  EXPECT_DOUBLE_EQ(set.getMinVal(), -2);
  EXPECT_DOUBLE_EQ(set.getMaxVal(), 30);
}

// Covers quad set behavior: automatic translation only applies to two dimensional domains.
TEST(QuadSetTest, AutomaticTranslationOnlyAppliesToTwoDimensionalDomains)
{
  QuadSet set = makeViewerQuadSet();
  set.applyTranslation();
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLX(), -2);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLY(), -2);

  QuadSet one_dim;
  one_dim.setIvPDomain(smallDepthDomain());
  one_dim.addQuad3D(makeViewerQuad());
  one_dim.applyTranslation();
  EXPECT_DOUBLE_EQ(one_dim.getQuad(0).getLLX(), 0);
  EXPECT_DOUBLE_EQ(one_dim.getQuad(0).getLLY(), 0);
}

// Covers quad set behavior: invalid polar arguments and wrong dimension leave quads unchanged.
TEST(QuadSetTest, InvalidPolarArgumentsAndWrongDimensionLeaveQuadsUnchanged)
{
  QuadSet set = makeViewerQuadSet();
  set.applyPolar(10, 0);
  set.applyPolar(10, 3);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getLLX(), 0);
  EXPECT_DOUBLE_EQ(set.getQuad(0).getHHY(), 2);

  QuadSet one_dim;
  one_dim.setIvPDomain(smallDepthDomain());
  one_dim.addQuad3D(makeViewerQuad());
  one_dim.applyPolar(10, 1);
  EXPECT_DOUBLE_EQ(one_dim.getQuad(0).getLLX(), 0);
  EXPECT_DOUBLE_EQ(one_dim.getQuad(0).getHHY(), 2);
}

// Covers quad set behavior: polar projection delegates through domain point counts.
TEST(QuadSetTest, PolarProjectionDelegatesThroughDomainPointCounts)
{
  QuadSet set;
  set.setIvPDomain(smallCourseSpeedDomain());
  Quad3D quad;
  quad.setLLX(0);
  quad.setHLX(1);
  quad.setHHX(2);
  quad.setLHX(3);
  quad.setLLY(1);
  quad.setHLY(1);
  quad.setHHY(2);
  quad.setLHY(2);
  quad.setLLZ(1);
  quad.setHLZ(2);
  quad.setHHZ(3);
  quad.setLHZ(4);
  quad.interpolate(0.5);
  set.addQuad3D(quad);
  set.applyPolar(10, 1);

  Quad3D projected = set.getQuad(0);
  EXPECT_NEAR(projected.getLLX(), 0, kGeomTol);
  EXPECT_NEAR(projected.getLLY(), 10, kGeomTol);
  EXPECT_NEAR(projected.getHLX(), 10, kGeomTol);
  EXPECT_NEAR(projected.getHLY(), 0, kGeomTol);
  EXPECT_NEAR(projected.getHHX(), 0, kGeomTol);
  EXPECT_NEAR(projected.getHHY(), -20, kGeomTol);
  EXPECT_NEAR(projected.getLHX(), -20, kGeomTol);
  EXPECT_NEAR(projected.getLHY(), 0, kGeomTol);
  EXPECT_NEAR(projected.getXinLOW(0), std::sqrt(50.0), kGeomTol);
  EXPECT_NEAR(projected.getYinLOW(0), std::sqrt(50.0), kGeomTol);
  EXPECT_NEAR(projected.getXinHGH(0), -std::sqrt(200.0), kGeomTol);
  EXPECT_NEAR(projected.getYinHGH(0), -std::sqrt(200.0), kGeomTol);
}

// Covers quad set behavior: polar projection dim two uses second domain point count.
TEST(QuadSetTest, PolarProjectionDimTwoUsesSecondDomainPointCount)
{
  QuadSet set;
  set.setIvPDomain(smallCourseSpeedDomain());
  Quad3D quad;
  quad.setLLX(1);
  quad.setHLX(1);
  quad.setHHX(2);
  quad.setLHX(2);
  quad.setLLY(0);
  quad.setHLY(1);
  quad.setHHY(2);
  quad.setLHY(3);
  quad.setLLZ(1);
  quad.setHLZ(2);
  quad.setHHZ(3);
  quad.setLHZ(4);
  set.addQuad3D(quad);
  set.applyPolar(10, 2);

  Quad3D projected = set.getQuad(0);
  EXPECT_NEAR(projected.getLLX(), 10, kGeomTol);
  EXPECT_NEAR(projected.getLLY(), 0, kGeomTol);
  EXPECT_NEAR(projected.getHLX(), -5, kGeomTol);
  EXPECT_NEAR(projected.getHLY(), 8.660254037844386, kGeomTol);
  EXPECT_NEAR(projected.getHHX(), -10, kGeomTol);
  EXPECT_NEAR(projected.getHHY(), -17.32050807568877, kGeomTol);
  EXPECT_NEAR(projected.getLHX(), 20, kGeomTol);
  EXPECT_NEAR(projected.getLHY(), 0, kGeomTol);
}

// Covers quad3 d behavior: interpolates interior samples and returns zero out of range.
TEST(Quad3DTest, InterpolatesInteriorSamplesAndReturnsZeroOutOfRange)
{
  Quad3D quad = makeViewerQuad();
  EXPECT_DOUBLE_EQ(quad.getXinLOW(0), 0);
  EXPECT_DOUBLE_EQ(quad.getBinHGH(0), 0);

  quad.interpolate(0.5);
  ASSERT_EQ(quad.getInPtsSize(), 3u);
  EXPECT_DOUBLE_EQ(quad.getXinLOW(1), 1);
  EXPECT_DOUBLE_EQ(quad.getYinLOW(1), 0);
  EXPECT_DOUBLE_EQ(quad.getZinLOW(1), 2);
  EXPECT_DOUBLE_EQ(quad.getRinLOW(1), 0.3);
  EXPECT_DOUBLE_EQ(quad.getGinLOW(1), 1.5);
  EXPECT_DOUBLE_EQ(quad.getBinLOW(1), 15);
  EXPECT_DOUBLE_EQ(quad.getXinHGH(1), 1);
  EXPECT_DOUBLE_EQ(quad.getYinHGH(1), 2);
  EXPECT_DOUBLE_EQ(quad.getZinHGH(1), 6);
  EXPECT_DOUBLE_EQ(quad.getRinHGH(1), 0.7);
  EXPECT_DOUBLE_EQ(quad.getGinHGH(1), 3.5);
  EXPECT_DOUBLE_EQ(quad.getBinHGH(1), 35);
  EXPECT_DOUBLE_EQ(quad.getXinLOW(99), 0);
  EXPECT_DOUBLE_EQ(quad.getBinHGH(99), 0);
}

// Covers quad3 d behavior: defaults all geometry height and color channels to zero.
TEST(Quad3DTest, DefaultsAllGeometryHeightAndColorChannelsToZero)
{
  Quad3D quad;

  EXPECT_DOUBLE_EQ(quad.getLLX(), 0);
  EXPECT_DOUBLE_EQ(quad.getHLX(), 0);
  EXPECT_DOUBLE_EQ(quad.getHHY(), 0);
  EXPECT_DOUBLE_EQ(quad.getLHY(), 0);
  EXPECT_DOUBLE_EQ(quad.getLLZ(), 0);
  EXPECT_DOUBLE_EQ(quad.getHLZ(), 0);
  EXPECT_DOUBLE_EQ(quad.getHHZ(), 0);
  EXPECT_DOUBLE_EQ(quad.getLHZ(), 0);
  EXPECT_DOUBLE_EQ(quad.getLLR(), 0);
  EXPECT_DOUBLE_EQ(quad.getHLG(), 0);
  EXPECT_DOUBLE_EQ(quad.getHHB(), 0);
  EXPECT_DOUBLE_EQ(quad.getLHR(), 0);
  EXPECT_DOUBLE_EQ(quad.getAvgVal(), 0);
  EXPECT_EQ(quad.getInPtsSize(), 0u);
}

// Covers quad3 d behavior: reinterpolate replaces samples and direct height adds affect average.
TEST(Quad3DTest, ReinterpolateReplacesSamplesAndDirectHeightAddsAffectAverage)
{
  Quad3D quad = makeViewerQuad();
  quad.interpolate(0.5);
  ASSERT_EQ(quad.getInPtsSize(), 3u);
  quad.interpolate(1.5);
  ASSERT_EQ(quad.getInPtsSize(), 1u);
  EXPECT_DOUBLE_EQ(quad.getXinLOW(0), 1.5);
  EXPECT_DOUBLE_EQ(quad.getZinLOW(0), 2.5);
  EXPECT_DOUBLE_EQ(quad.getZinHGH(0), 6.5);

  quad.addLLZ(10);
  quad.addHLZ(20);
  quad.addHHZ(30);
  quad.addLHZ(40);
  EXPECT_DOUBLE_EQ(quad.getLLZ(), 11);
  EXPECT_DOUBLE_EQ(quad.getHLZ(), 23);
  EXPECT_DOUBLE_EQ(quad.getHHZ(), 37);
  EXPECT_DOUBLE_EQ(quad.getLHZ(), 45);
  EXPECT_DOUBLE_EQ(quad.getAvgVal(), 29);
}

// Covers quad3 d behavior: large positive interpolation delta clears interior samples.
TEST(Quad3DTest, LargePositiveInterpolationDeltaClearsInteriorSamples)
{
  Quad3D quad = makeViewerQuad();
  quad.interpolate(0.5);
  ASSERT_GT(quad.getInPtsSize(), 0u);

  quad.interpolate(2);
  EXPECT_EQ(quad.getInPtsSize(), 0u);
  EXPECT_DOUBLE_EQ(quad.getXinLOW(0), 0);
  EXPECT_DOUBLE_EQ(quad.getZinHGH(0), 0);
}

// Covers quad3 d behavior: interpolated samples follow scale base and color intensity.
TEST(Quad3DTest, InterpolatedSamplesFollowScaleBaseAndColorIntensity)
{
  Quad3D quad = makeViewerQuad();
  quad.interpolate(1);
  ASSERT_EQ(quad.getInPtsSize(), 1u);

  quad.applyScale(2);
  quad.applyBase(-1);
  quad.applyColorIntensity(0.5);

  EXPECT_DOUBLE_EQ(quad.getZinLOW(0), 3);
  EXPECT_DOUBLE_EQ(quad.getZinHGH(0), 11);
  EXPECT_NEAR(quad.getRinLOW(0), 0.15, kGeomTol);
  EXPECT_NEAR(quad.getRinHGH(0), 0.35, kGeomTol);
  EXPECT_NEAR(quad.getGinLOW(0), 0.75, kGeomTol);
  EXPECT_NEAR(quad.getGinHGH(0), 1.75, kGeomTol);
  EXPECT_NEAR(quad.getBinLOW(0), 7.5, kGeomTol);
  EXPECT_NEAR(quad.getBinHGH(0), 17.5, kGeomTol);
}

// Covers quad3 d behavior: invalid polar arguments leave geometry and interpolants unchanged.
TEST(Quad3DTest, InvalidPolarArgumentsLeaveGeometryAndInterpolantsUnchanged)
{
  Quad3D quad = makeViewerQuad();
  quad.interpolate(1);
  quad.applyPolar(0, 1, 4);
  quad.applyPolar(5, 0, 4);
  quad.applyPolar(5, 3, 4);
  quad.applyPolar(5, 1, 0);

  EXPECT_DOUBLE_EQ(quad.getLLX(), 0);
  EXPECT_DOUBLE_EQ(quad.getHLX(), 2);
  EXPECT_DOUBLE_EQ(quad.getHHY(), 2);
  EXPECT_DOUBLE_EQ(quad.getXinLOW(0), 1);
  EXPECT_DOUBLE_EQ(quad.getYinHGH(0), 2);
}

// Covers quad3 d behavior: valid polar dim two projects using y as angle and x as radius.
TEST(Quad3DTest, ValidPolarDimTwoProjectsUsingYAsAngleAndXAsRadius)
{
  Quad3D quad;
  quad.setLLX(1);
  quad.setHLX(1);
  quad.setHHX(2);
  quad.setLHX(2);
  quad.setLLY(0);
  quad.setHLY(1);
  quad.setHHY(2);
  quad.setLHY(3);
  quad.applyPolar(10, 2, 4);

  EXPECT_NEAR(quad.getLLX(), 10, kGeomTol);
  EXPECT_NEAR(quad.getLLY(), 0, kGeomTol);
  EXPECT_NEAR(quad.getHLX(), 0, kGeomTol);
  EXPECT_NEAR(quad.getHLY(), 10, kGeomTol);
  EXPECT_NEAR(quad.getHHX(), -20, kGeomTol);
  EXPECT_NEAR(quad.getHHY(), 0, kGeomTol);
  EXPECT_NEAR(quad.getLHX(), 0, kGeomTol);
  EXPECT_NEAR(quad.getLHY(), -20, kGeomTol);
}

// Covers quad3 d behavior: polar dim two does not transform interpolated interior samples.
TEST(Quad3DTest, PolarDimTwoDoesNotTransformInterpolatedInteriorSamples)
{
  Quad3D quad = makeViewerQuad();
  quad.interpolate(1);
  ASSERT_EQ(quad.getInPtsSize(), 1u);
  quad.applyPolar(10, 2, 4);

  EXPECT_NEAR(quad.getLLX(), 0, kGeomTol);
  EXPECT_NEAR(quad.getLLY(), 0, kGeomTol);
  EXPECT_NEAR(quad.getHLX(), 20, kGeomTol);
  EXPECT_NEAR(quad.getHLY(), 0, kGeomTol);
  EXPECT_DOUBLE_EQ(quad.getXinLOW(0), 1);
  EXPECT_DOUBLE_EQ(quad.getYinLOW(0), 0);
  EXPECT_DOUBLE_EQ(quad.getXinHGH(0), 1);
  EXPECT_DOUBLE_EQ(quad.getYinHGH(0), 2);
}

// Covers quad3 d behavior: applies viewer transforms and pins translation quirk.
TEST(Quad3DTest, AppliesViewerTransformsAndPinsTranslationQuirk)
{
  Quad3D quad;
  quad.setLLX(1);
  quad.setHLX(2);
  quad.setHHX(3);
  quad.setLHX(4);
  quad.setLLY(10);
  quad.setHLY(20);
  quad.setHHY(30);
  quad.setLHY(40);
  quad.setLLZ(5);
  quad.setHLZ(6);
  quad.setHHZ(7);
  quad.setLHZ(8);
  quad.setLLR(0.5);
  quad.setHLG(0.4);
  quad.setHHB(0.3);

  EXPECT_DOUBLE_EQ(quad.getAvgVal(), 6.5);

  quad.applyScale(2);
  quad.applyBase(1);
  EXPECT_DOUBLE_EQ(quad.getLLZ(), 11);
  EXPECT_DOUBLE_EQ(quad.getHHZ(), 15);

  quad.applyColorIntensity(0.5);
  EXPECT_DOUBLE_EQ(quad.getLLR(), 0.25);
  EXPECT_DOUBLE_EQ(quad.getHLG(), 0.2);
  EXPECT_DOUBLE_EQ(quad.getHHB(), 0.15);

  quad.expand(2);
  EXPECT_DOUBLE_EQ(quad.getLLX(), 2);
  EXPECT_DOUBLE_EQ(quad.getLLY(), 11);

  quad.applyTranslation(3, 7);
  EXPECT_DOUBLE_EQ(quad.getLLX(), 5);
  EXPECT_DOUBLE_EQ(quad.getHLX(), 6);
  EXPECT_DOUBLE_EQ(quad.getLLY(), 14);
  EXPECT_DOUBLE_EQ(quad.getHLY(), 24);
}

// Covers quad3 d behavior: expand clamps negative and positive deltas.
TEST(Quad3DTest, ExpandClampsNegativeAndPositiveDeltas)
{
  Quad3D quad = makeViewerQuad();
  quad.expand(-2);
  EXPECT_DOUBLE_EQ(quad.getLLX(), -1);
  EXPECT_DOUBLE_EQ(quad.getHLX(), 1);
  EXPECT_DOUBLE_EQ(quad.getLLY(), -1);
  EXPECT_DOUBLE_EQ(quad.getHHY(), 1);

  quad.expand(0.25);
  EXPECT_DOUBLE_EQ(quad.getLLX(), -0.75);
  EXPECT_DOUBLE_EQ(quad.getHLX(), 1.25);
  EXPECT_DOUBLE_EQ(quad.getLLY(), -0.75);
  EXPECT_DOUBLE_EQ(quad.getHHY(), 1.25);

  quad.expand(2);
  EXPECT_DOUBLE_EQ(quad.getLLX(), 0.25);
  EXPECT_DOUBLE_EQ(quad.getHLX(), 2.25);
  EXPECT_DOUBLE_EQ(quad.getLLY(), 0.25);
  EXPECT_DOUBLE_EQ(quad.getHHY(), 2.25);
}

// Covers quad set1 d behavior: builds one dimensional viewer series from helm IPF.
TEST(QuadSet1DTest, BuildsOneDimensionalViewerSeriesFromHelmIpf)
{
  const std::string encoded = makeEncodedCourseIPF("waypt_survey", 22, 180, 65);
  std::unique_ptr<IvPFunction> ipf(StringToIvPFunction(encoded));
  ASSERT_NE(ipf, nullptr);

  QuadSet1D qset;
  ASSERT_TRUE(qset.applyIPF(ipf.get(), "waypt_survey"));

  EXPECT_FALSE(qset.isEmpty1D());
  EXPECT_FALSE(qset.isEmptyND());
  EXPECT_DOUBLE_EQ(qset.getPriorityWt(), 65);
  EXPECT_EQ(qset.getSource(), "waypt_survey");
  EXPECT_EQ(qset.size1DFs(), 1u);
  EXPECT_GT(qset.size1D(), 0u);
  EXPECT_EQ(qset.getDomainPts().size(), qset.getRangeVals().size());
  EXPECT_GT(qset.getRangeValMax(), 0);
  EXPECT_LT(qset.getDomainIXMax(), qset.getDomainPts().size());

  QuadSet1D aggregate;
  aggregate.addQuadSet1D(qset);
  aggregate.addQuadSet1D(qset);
  EXPECT_EQ(aggregate.size1DFs(), 2u);
  EXPECT_EQ(aggregate.getSource(1), "waypt_survey");
  EXPECT_TRUE(aggregate.getSource(99).empty());
}

// Covers quad set1 d behavior: piecewise functions expose weighted edges and max index.
TEST(QuadSet1DTest, PiecewiseFunctionsExposeWeightedEdgesAndMaxIndex)
{
  std::unique_ptr<IvPFunction> ipf = makePiecewiseOneDimIPF(2, "12:piecewise");
  QuadSet1D qset = buildQuadSet1DFromIPF(ipf.get(), "piecewise");
  ASSERT_FALSE(qset.isEmptyND());
  EXPECT_TRUE(qset.getDomain().hasOnlyDomain("depth"));
  EXPECT_DOUBLE_EQ(qset.getPriorityWt(), 2);
  EXPECT_EQ(qset.getSource(), "piecewise");

  std::vector<double> domain_pts = qset.getDomainPts();
  std::vector<double> range_vals = qset.getRangeVals();
  std::vector<bool> edge_pts = qset.getDomainPtsX();
  ASSERT_EQ(domain_pts.size(), 5u);
  ASSERT_EQ(range_vals.size(), 5u);
  ASSERT_EQ(edge_pts.size(), 5u);
  EXPECT_DOUBLE_EQ(domain_pts[0], 0);
  EXPECT_DOUBLE_EQ(domain_pts[4], 4);
  EXPECT_DOUBLE_EQ(range_vals[0], 0);
  EXPECT_DOUBLE_EQ(range_vals[1], 2);
  EXPECT_DOUBLE_EQ(range_vals[2], 4);
  EXPECT_DOUBLE_EQ(range_vals[3], 14);
  EXPECT_DOUBLE_EQ(range_vals[4], 12);
  EXPECT_FALSE(edge_pts[0]);
  EXPECT_FALSE(edge_pts[1]);
  EXPECT_TRUE(edge_pts[2]);
  EXPECT_TRUE(edge_pts[3]);
  EXPECT_TRUE(edge_pts[4]);
  EXPECT_DOUBLE_EQ(qset.getRangeValMax(), 14);
  EXPECT_EQ(qset.getDomainIXMax(), 3u);
}

// Covers quad set1 d behavior: negative utilities still track maximum at least negative point.
TEST(QuadSet1DTest, NegativeUtilitiesStillTrackMaximumAtLeastNegativePoint)
{
  std::unique_ptr<PDMap> pdmap(new PDMap(1, smallDepthDomain(), 1));
  std::unique_ptr<IvPBox> box(new IvPBox(1, 1));
  box->setPTS(0, 0, 4);
  box->wt(0) = -1;
  box->wt(1) = -10;
  pdmap->bx(0) = box.release();

  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  ipf->setPWT(2);
  QuadSet1D qset = buildQuadSet1DFromIPF(ipf.get(), "negative");

  ASSERT_FALSE(qset.isEmptyND());
  ASSERT_EQ(qset.getRangeVals().size(), 5u);
  EXPECT_DOUBLE_EQ(qset.getRangeVals()[0], -20);
  EXPECT_DOUBLE_EQ(qset.getRangeVals()[4], -28);
  EXPECT_DOUBLE_EQ(qset.getRangeValMax(), -20);
  EXPECT_EQ(qset.getDomainIXMax(), 0u);
  EXPECT_EQ(qset.getSource(), "negative");
}

// Covers quad set1 d behavior: apply IPF resets prior series and aggregation keeps initial copy.
TEST(QuadSet1DTest, ApplyIPFResetsPriorSeriesAndAggregationKeepsInitialCopy)
{
  std::unique_ptr<IvPFunction> first = makePiecewiseOneDimIPF(2, "1:first");
  std::unique_ptr<IvPFunction> second = makePiecewiseOneDimIPF(3, "1:second");

  QuadSet1D qset;
  ASSERT_TRUE(qset.applyIPF(first.get(), "first"));
  EXPECT_EQ(qset.getSource(), "first");
  ASSERT_TRUE(qset.applyIPF(second.get(), "second"));
  EXPECT_EQ(qset.size1DFs(), 1u);
  EXPECT_EQ(qset.getSource(), "second");
  EXPECT_DOUBLE_EQ(qset.getPriorityWt(), 3);
  EXPECT_DOUBLE_EQ(qset.getRangeValMax(), 21);

  QuadSet1D aggregate;
  QuadSet1D empty;
  aggregate.addQuadSet1D(empty);
  EXPECT_TRUE(aggregate.isEmptyND());
  aggregate.addQuadSet1D(qset);
  ASSERT_EQ(aggregate.size1DFs(), 2u);
  EXPECT_EQ(aggregate.getSource(0), "collective");
  EXPECT_EQ(aggregate.getSource(1), "second");
  EXPECT_EQ(aggregate.getRangeVals(0), qset.getRangeVals());

  QuadSet1D replacement;
  ASSERT_TRUE(replacement.applyIPF(first.get(), "first"));
  aggregate.addQuadSet1D(replacement);
  EXPECT_EQ(aggregate.size1DFs(), 2u);
  EXPECT_EQ(aggregate.getSource(1), "second");
  EXPECT_EQ(aggregate.getRangeVals(0), qset.getRangeVals());
}

// Covers quad set1 d behavior: set domain does not make empty series non empty.
TEST(QuadSet1DTest, SetDomainDoesNotMakeEmptySeriesNonEmpty)
{
  QuadSet1D qset;
  qset.setIvPDomain(smallDepthDomain());
  EXPECT_TRUE(qset.isEmpty1D());
  EXPECT_TRUE(qset.isEmptyND());
  EXPECT_TRUE(qset.getDomain().hasOnlyDomain("depth"));
  EXPECT_EQ(qset.size1DFs(), 0u);
  EXPECT_EQ(qset.size1D(), 0u);
  EXPECT_TRUE(qset.getDomainPts().empty());
}

// Covers quad set1 d behavior: rejects invalid inputs and bounds getter indexes.
TEST(QuadSet1DTest, RejectsInvalidInputsAndBoundsGetterIndexes)
{
  QuadSet1D qset;
  EXPECT_FALSE(qset.applyIPF(nullptr, "null"));
  std::unique_ptr<IvPFunction> two_dim = makeLinear2DIPF(smallCourseSpeedDomain());
  EXPECT_FALSE(qset.applyIPF(two_dim.get(), "two_dim"));
  EXPECT_TRUE(qset.isEmptyND());
  EXPECT_TRUE(qset.getDomainPts(4).empty());
  EXPECT_TRUE(qset.getRangeVals(4).empty());
  EXPECT_TRUE(qset.getDomainPtsX(4).empty());
  EXPECT_DOUBLE_EQ(qset.getRangeValMax(4), 0);
  EXPECT_EQ(qset.getDomainIXMax(4), 0u);
  EXPECT_EQ(qset.getSource(4), "");
}
