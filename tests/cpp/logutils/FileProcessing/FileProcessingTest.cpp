#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "ALogScanner.h"
#include "ALogSorter.h"
#include "ScanReport.h"
#include "SplitHandler.h"
#include "TestFileUtils.h"

namespace {

std::string joinLines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

TempFile writeTempFile(const std::string& stem,
                       const std::vector<std::string>& lines,
                       const std::string& suffix = ".alog")
{
  return TempFile(stem, joinLines(lines), suffix);
}

std::vector<std::string> readLines(const std::string& path)
{
  std::ifstream in(path.c_str());
  std::vector<std::string> lines;
  std::string line;
  while(std::getline(in, line))
    lines.push_back(line);
  return lines;
}

std::string readText(const std::string& path)
{
  std::ifstream in(path.c_str());
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

ALogEntry entry(double time, const std::string& var, const std::string& source,
                const std::string& aux, const std::string& value)
{
  ALogEntry result;
  result.set(time, var, source, aux, value);
  return result;
}

ALogEntry numericEntry(double time, const std::string& var, const std::string& source,
                       double value)
{
  ALogEntry result;
  result.set(time, var, source, "", value);
  return result;
}

void expectOrder(ScanReport& report, const std::vector<std::string>& names)
{
  ASSERT_EQ(report.size(), names.size());
  for(unsigned int i = 0; i < names.size(); ++i)
    EXPECT_EQ(report.getVarName(i), names[i]) << "index " << i;
}

}  // namespace

// Covers scan report behavior: aggregates counts chars sources and time bounds by variable.
TEST(ScanReportTest, AggregatesCountsCharsSourcesAndTimeBoundsByVariable)
{
  ScanReport report;
  report.addLine(10.0, "NAV_X", "pNodeReporter", "1.2");
  report.addLine(12.0, "NAV_X", "pLogger", "1.4");
  report.addLine(11.0, "HELM_MODE", "pHelmIvP", "SURVEY");

  EXPECT_EQ(report.size(), 2u);
  EXPECT_TRUE(report.containsVar("NAV_X"));
  EXPECT_FALSE(report.containsVar("NAV_Y"));
  EXPECT_EQ(report.getVarIndex("NAV_X"), 0);
  EXPECT_EQ(report.getVarIndex("missing"), -1);
  EXPECT_EQ(report.getVarName(99), "");
  EXPECT_EQ(report.getVarSources("NAV_X"), "pNodeReporter,pLogger");
  EXPECT_DOUBLE_EQ(report.getVarFirstTime("NAV_X"), 10.0);
  EXPECT_DOUBLE_EQ(report.getVarLastTime("NAV_X"), 12.0);
  EXPECT_EQ(report.getVarCount("NAV_X"), 2u);
  EXPECT_EQ(report.getVarChars("NAV_X"),
            static_cast<unsigned int>(std::string("NAV_X").size() +
                                      std::string("pNodeReporter").size() +
                                      std::string("1.2").size() +
                                      std::string("NAV_X").size() +
                                      std::string("pLogger").size() +
                                      std::string("1.4").size()));
  EXPECT_EQ(report.getMaxLines(), 2u);
  EXPECT_EQ(report.getVarNameMaxLength(), std::string("HELM_MODE").size());
  EXPECT_DOUBLE_EQ(report.getMinStartTime(), 10.0);
  EXPECT_DOUBLE_EQ(report.getMaxStartTime(), 11.0);
  EXPECT_DOUBLE_EQ(report.getMaxStopTime(), 12.0);
  EXPECT_GT(report.getDataRate(), 0.0);
}

// Covers scan report behavior: supports rate only aggregation without variable rows.
TEST(ScanReportTest, SupportsRateOnlyAggregationWithoutVariableRows)
{
  ScanReport report;
  report.addLineRateOnly(entry(5.0, "NODE_REPORT", "pNodeReporter", "",
                               "NAME=abe,X=1,Y=2"));
  report.addLineRateOnly(entry(9.0, "NODE_REPORT", "pNodeReporter", "",
                               "NAME=abe,X=3,Y=4"));

  EXPECT_EQ(report.size(), 0u);
  EXPECT_EQ(report.getMaxLines(), 0u);
  EXPECT_DOUBLE_EQ(report.getMinStartTime(), 0.0);
  EXPECT_GT(report.getDataRate(), 0.0);
}

// Covers scan report behavior: sorts rows by each column used by alogscan output.
TEST(ScanReportTest, SortsRowsByEachColumnUsedByAlogscanOutput)
{
  ScanReport report;
  report.addLine(10.0, "ZETA", "pZulu", "long-value");
  report.addLine(3.0, "ALPHA", "pAlpha", "x");
  report.addLine(7.0, "BRAVO", "pBravo", "yy");
  report.addLine(8.0, "BRAVO", "pBravo", "zz");

  report.sort("byvars_ascending");
  expectOrder(report, {"ALPHA", "BRAVO", "ZETA"});

  report.sort("byvars_descending");
  expectOrder(report, {"ZETA", "BRAVO", "ALPHA"});

  report.sort("bylines_descending");
  EXPECT_EQ(report.getVarName(0), "BRAVO");

  report.sort("bystarttime_ascending");
  expectOrder(report, {"ALPHA", "BRAVO", "ZETA"});

  report.sort("bystoptime_descending");
  expectOrder(report, {"ZETA", "BRAVO", "ALPHA"});

  report.sort("bysrc_ascending");
  expectOrder(report, {"ALPHA", "BRAVO", "ZETA"});

  report.sort("bychars_descending");
  EXPECT_EQ(report.getVarName(0), "BRAVO");
}

// Covers scan report behavior: fills application statistics from variable sources.
TEST(ScanReportTest, FillsApplicationStatisticsFromVariableSources)
{
  ScanReport report;
  report.addLine(1.0, "NAV_X", "pNodeReporter", "1");
  report.addLine(2.0, "NAV_Y", "pNodeReporter", "2");
  report.addLine(3.0, "DESIRED_HEADING", "pHelmIvP", "90");

  report.fillAppStats();

  std::vector<std::string> sources = report.getAllSources();
  std::set<std::string> source_set(sources.begin(), sources.end());
  EXPECT_EQ(source_set, std::set<std::string>({"pHelmIvP", "pNodeReporter"}));
  EXPECT_EQ(report.getLinesBySource("pNodeReporter"), 2u);
  EXPECT_EQ(report.getLinesBySource("pHelmIvP"), 1u);
  EXPECT_NEAR(report.getLinesPctBySource("pNodeReporter"), 2.0 / 3.0, 1e-9);
  EXPECT_NEAR(report.getLinesPctBySource("pHelmIvP"), 1.0 / 3.0, 1e-9);
  EXPECT_GT(report.getCharsBySource("pNodeReporter"), report.getCharsBySource("pHelmIvP"));
  EXPECT_GT(report.getCharsPctBySource("pNodeReporter"), report.getCharsPctBySource("pHelmIvP"));
}

// Covers a log scanner behavior: scans alog file with helm aux behavior sources.
TEST(ALogScannerTest, ScansAlogFileWithHelmAuxBehaviorSources)
{
  TempFile file = writeTempFile("scan", {
      "%% LOGSTART 1000.000",
      "0.000 NAV_X pNodeReporter 12.5",
      "0.100 DESIRED_HEADING pHelmIvP:iter=7:waypt_survey 90",
      "0.200 DESIRED_SPEED pHelmIvP:iter=7:waypt_survey 1.5",
      "continuation text from DB_VARSUMMARY",
      "0.300 APP_LOG pHelmIvP iter=7,log=active"});

  ALogScanner scanner;
  scanner.setVerbose(false);
  ASSERT_TRUE(scanner.openALogFile(file.path()));
  ScanReport report = scanner.scan();

  EXPECT_TRUE(report.containsVar("NAV_X"));
  EXPECT_TRUE(report.containsVar("DESIRED_HEADING"));
  EXPECT_EQ(report.getVarSources("DESIRED_HEADING"), "pHelmIvP:waypt_survey");
  EXPECT_EQ(report.getVarCount("DESIRED_SPEED"), 1u);
  EXPECT_FALSE(report.containsVar("continuation"));

  ALogScanner no_aux_scanner;
  no_aux_scanner.setVerbose(false);
  no_aux_scanner.setUseFullSource(false);
  ASSERT_TRUE(no_aux_scanner.openALogFile(file.path()));
  ScanReport no_aux = no_aux_scanner.scan();
  EXPECT_EQ(no_aux.getVarSources("DESIRED_HEADING"), "pHelmIvP");
}

// Covers a log scanner behavior: scans rate only and rejects missing files.
TEST(ALogScannerTest, ScansRateOnlyAndRejectsMissingFiles)
{
  TempFile file = writeTempFile("rate_scan", {
      "0.000 NAV_X pNodeReporter 12.5",
      "1.000 NAV_Y pNodeReporter 14.5"});

  ALogScanner scanner;
  scanner.setVerbose(false);
  ASSERT_TRUE(scanner.openALogFile(file.path()));
  ScanReport report = scanner.scanRateOnly();
  EXPECT_EQ(report.size(), 0u);
  EXPECT_GT(report.getDataRate(), 0.0);

  ALogScanner missing;
  missing.setVerbose(false);
  EXPECT_FALSE(missing.openALogFile("/definitely/not/present.alog"));
}

// Covers a log sorter behavior: sorts out of order entries and counts front insertion warnings.
TEST(ALogSorterTest, SortsOutOfOrderEntriesAndCountsFrontInsertionWarnings)
{
  ALogSorter sorter;
  EXPECT_FALSE(sorter.addEntry(numericEntry(10, "NAV_X", "pNodeReporter", 10)));
  EXPECT_FALSE(sorter.addEntry(numericEntry(12, "NAV_X", "pNodeReporter", 12)));
  EXPECT_TRUE(sorter.addEntry(numericEntry(11, "NAV_X", "pNodeReporter", 11)));
  EXPECT_TRUE(sorter.addEntry(numericEntry(5, "NAV_X", "pNodeReporter", 5)));

  EXPECT_EQ(sorter.size(), 4u);
  EXPECT_EQ(sorter.sortWarnings(), 1u);
  EXPECT_DOUBLE_EQ(sorter.popEntry().time(), 5);
  EXPECT_DOUBLE_EQ(sorter.popEntry().time(), 10);
  EXPECT_DOUBLE_EQ(sorter.popEntry().time(), 11);
  EXPECT_DOUBLE_EQ(sorter.popEntry().time(), 12);
  EXPECT_EQ(sorter.size(), 0u);
}

// Covers a log sorter behavior: removes duplicate entries at same timestamp by default.
TEST(ALogSorterTest, RemovesDuplicateEntriesAtSameTimestampByDefault)
{
  ALogEntry first = entry(1.0, "APP_LOG", "pHelmIvP", "", "iter=1,log=a");
  ALogEntry duplicate = first;
  ALogEntry distinct = entry(1.0, "APP_LOG", "pHelmIvP", "", "iter=1,log=b");

  ALogSorter sorter;
  sorter.addEntry(first);
  sorter.addEntry(duplicate);
  sorter.addEntry(distinct);

  EXPECT_EQ(sorter.popEntry().getStringVal(), "iter=1,log=a");
  EXPECT_EQ(sorter.size(), 1u);
  EXPECT_EQ(sorter.popEntry().getStringVal(), "iter=1,log=b");
}

// Covers a log sorter behavior: can keep duplicates and force continuation order.
TEST(ALogSorterTest, CanKeepDuplicatesAndForceContinuationOrder)
{
  ALogSorter sorter;
  sorter.checkForDuplicates(false);
  sorter.addEntry(entry(2.0, "DB_VARSUMMARY", "MOOSDB", "", "first"));
  sorter.addEntry(entry(0.0, "DB_VARSUMMARY", "MOOSDB", "", "continuation"), true);
  sorter.addEntry(entry(2.0, "DB_VARSUMMARY", "MOOSDB", "", "first"));

  EXPECT_EQ(sorter.size(), 3u);
  EXPECT_EQ(sorter.popEntry().getStringVal(), "first");
  EXPECT_EQ(sorter.popEntry().getStringVal(), "continuation");
  EXPECT_EQ(sorter.popEntry().getStringVal(), "first");
}

// Covers split handler behavior: validates alog suffix whitespace and readable file.
TEST(SplitHandlerTest, ValidatesAlogSuffixWhitespaceAndReadableFile)
{
  SplitHandler missing("/definitely/not/present.alog");
  EXPECT_FALSE(missing.handlePreCheckALogFile());

  TempFile txt = writeTempFile("not_alog", {"0 NAV_X pNodeReporter 1"}, ".txt");
  SplitHandler wrong_suffix(txt.path());
  EXPECT_FALSE(wrong_suffix.handlePreCheckALogFile());

  TempFile spaced = writeTempFile("bad name", {"0 NAV_X pNodeReporter 1"});
  SplitHandler whitespace(spaced.path());
  EXPECT_FALSE(whitespace.handlePreCheckALogFile());

  TempFile good = writeTempFile("valid_split_precheck", {"0 NAV_X pNodeReporter 1"});
  SplitHandler handler(good.path());
  EXPECT_TRUE(handler.handlePreCheckALogFile());
  EXPECT_TRUE(handler.handlePreCheckALogFile());
}

// Covers split handler behavior: splits mission alog into klog files summary visuals and detached fields.
TEST(SplitHandlerTest, SplitsMissionAlogIntoKlogFilesSummaryVisualsAndDetachedFields)
{
  // This fixture mixes the log lines SplitHandler sees in real mission alogs:
  // helm IPFs, app logs, visual posts, node reports, overview fields, and JSON.
  TempFile file = writeTempFile("split", {
      "%% LOGSTART 1000.000",
      "0.000 DB_TIME MOOSDB_alpha 0.0",
      "0.050 NODE_REPORT_LOCAL pNodeReporter NAME=alpha,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,HDG=90",
      "0.100 IVPHELM_ITER pHelmIvP 445",
      "0.200 BHV_IPF pHelmIvP P,waypt_return^445,1,1,H,16,445:waypt_return,2,35,1,100,D,",
      "0.300 APP_LOG pHelmIvP iter=3,log=helm active",
      "0.400 VIEW_POINT pMarineViewer x=1,y=2,label=alpha",
      "0.500 NODE_REPORT pNodeReporter NAME=alpha,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,SPD=1.5,HDG=90,UTC_TIME=1000",
      "0.600 APP_OVERVIEW pExampleApp temp=98.5,fuel=14.5,age=11.9,errs=11",
      "0.700 JSON_REPORT pExampleApp {\"temp\":12.5,\"fuel\":3}",
      "0.800 BAD/VAR pExampleApp 1.0"});
  TempDir dir("split_out_parent");
  // The split directory must be fresh; existing split directories can be
  // treated as prior cached output by the log tooling.
  const std::string out_dir = dir.filePath("split_out");

  SplitHandler handler(file.path());
  handler.setVerbose(false);
  handler.setProgress(false);
  handler.setDirectory(out_dir);
  handler.setMaxFilePtrCache(20);
  // Detached pairs turn composite app payload fields into their own klogs.
  EXPECT_TRUE(handler.addDetachedPair("APP_OVERVIEW:fuel:errs"));
  EXPECT_TRUE(handler.addDetachedPair("JSON_REPORT", "temp"));
  EXPECT_FALSE(handler.addDetachedPair("BAD VAR", "fuel"));

  EXPECT_TRUE(handler.handle());

  EXPECT_EQ(readLines(out_dir + "/NAV_X.klog").size(), 0u);
  EXPECT_EQ(readLines(out_dir + "/DB_TIME.klog"),
            std::vector<std::string>({"0.000 DB_TIME MOOSDB_alpha 0.0"}));
  EXPECT_EQ(readLines(out_dir + "/BHV_IPF_waypt_return.klog").size(), 1u);
  EXPECT_EQ(readLines(out_dir + "/APP_LOG_pHelmIvP.klog").size(), 1u);
  EXPECT_EQ(readLines(out_dir + "/APP_OVERVIEW:FUEL.klog"),
            std::vector<std::string>({"0.600  APP_OVERVIEW:FUEL  pExampleApp  14.5"}));
  EXPECT_EQ(readLines(out_dir + "/APP_OVERVIEW:ERRS.klog"),
            std::vector<std::string>({"0.600  APP_OVERVIEW:ERRS  pExampleApp  11"}));
  EXPECT_EQ(readLines(out_dir + "/JSON_REPORT:TEMP.klog"),
            std::vector<std::string>({"0.700  JSON_REPORT:TEMP  pExampleApp  12.5"}));
  // File names sanitize '/' in variable names so BAD/VAR lands in BAD_VAR.klog.
  EXPECT_EQ(readLines(out_dir + "/BAD_VAR.klog"),
            std::vector<std::string>({"0.800 BAD/VAR pExampleApp 1.0"}));

  const std::vector<std::string> visuals = readLines(out_dir + "/VISUALS.klog");
  ASSERT_EQ(visuals.size(), 2u);
  EXPECT_EQ(visuals[0], "0.400 VIEW_POINT pMarineViewer x=1,y=2,label=alpha");
  EXPECT_NE(visuals[1].find("VIEW_VESSEL"), std::string::npos);
  EXPECT_NE(visuals[1].find("label=alpha"), std::string::npos);
  EXPECT_NE(visuals[1].find("type=kayak"), std::string::npos);
  EXPECT_NE(visuals[1].find("dur=3"), std::string::npos);

  const std::string summary = readText(out_dir + "/summary.klog");
  EXPECT_NE(summary.find("logstart=1000.000"), std::string::npos);
  EXPECT_NE(summary.find("logtmin=0.000"), std::string::npos);
  EXPECT_NE(summary.find("logtmax=0.800"), std::string::npos);
  EXPECT_NE(summary.find("vname=alpha"), std::string::npos);
  EXPECT_NE(summary.find("vtype=kayak"), std::string::npos);
  EXPECT_NE(summary.find("vcolor=red"), std::string::npos);
  EXPECT_NE(summary.find("vlength=4.2"), std::string::npos);
  EXPECT_NE(summary.find("bhvs=waypt_return"), std::string::npos);
  EXPECT_NE(summary.find("applogging_apps=pHelmIvP"), std::string::npos);
  EXPECT_NE(summary.find("var=VISUALS, type=string, srcs=pMarineViewer:pNodeReporter"),
            std::string::npos);
}

// Covers split handler behavior: can disable NODE_REPORT view vessel generation.
TEST(SplitHandlerTest, CanDisableNodeReportViewVesselGeneration)
{
  TempFile file = writeTempFile("split_no_vessel", {
      "%% LOGSTART 1000.000",
      "0.000 NODE_REPORT pNodeReporter NAME=alpha,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,HDG=90",
      "0.100 VIEW_POINT pMarineViewer x=1,y=2,label=alpha"});
  TempDir dir("split_no_vessel_parent");
  const std::string out_dir = dir.filePath("split_no_vessel_out");

  SplitHandler handler(file.path());
  handler.setDirectory(out_dir);
  handler.setViewVessels(false);
  ASSERT_TRUE(handler.handle());

  EXPECT_EQ(readLines(out_dir + "/VISUALS.klog"),
            std::vector<std::string>({"0.100 VIEW_POINT pMarineViewer x=1,y=2,label=alpha"}));
  EXPECT_EQ(readLines(out_dir + "/NODE_REPORT.klog"),
            std::vector<std::string>({"0.000 NODE_REPORT pNodeReporter NAME=alpha,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,HDG=90"}));
}
