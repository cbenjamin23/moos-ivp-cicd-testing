#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ScanHandler.h"
#include "TestFileUtils.h"

namespace {

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

std::string WriteAlog(TempDir& temp,
                      const std::string& name,
                      const std::vector<std::string>& lines)
{
  return temp.writeFile(name, WithNewlines(lines));
}

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

std::size_t PositionOf(const std::string& haystack, const std::string& needle)
{
  const std::size_t pos = haystack.find(needle);
  EXPECT_NE(pos, std::string::npos);
  return pos;
}

}  // namespace

// Source audit note: this suite covers ScanHandler's stable parameter,
// parsing, report, sort, color, and rate-only paths against app_alogscan.
// The thin CLI parser and terminal rendering side effects are intentionally
// left outside these direct component tests.

// Covers parameter validation.
TEST(ScanHandlerTest, SetsParametersAndRejectsInvalidValues)
{
  ScanHandler handler;

  EXPECT_TRUE(handler.setParam("proc_colors", "false"));
  EXPECT_TRUE(handler.setParam("use_full_source", "false"));
  EXPECT_TRUE(handler.setParam("sort_style", "varname"));
  EXPECT_FALSE(handler.setParam("proc_colors", "maybe"));
  EXPECT_FALSE(handler.setParam("unknown", "true"));
}

// Covers fixed process colors, sequential assignment, and terminal fallback.
TEST(ScanHandlerTest, AssignsProcessColorsThroughFallback)
{
  ScanHandler handler;

  EXPECT_EQ(handler.procColor("pHelmIvP"), "blue");
  EXPECT_EQ(handler.procColor("pNodeReporter"), "green");
  EXPECT_EQ(handler.procColor("pCustomA"), "red");
  EXPECT_EQ(handler.procColor("pCustomA"), "red");
  EXPECT_EQ(handler.procColor("pCustomB"), "magenta");
  EXPECT_EQ(handler.procColor("pCustomC"), "cyan");
  EXPECT_EQ(handler.procColor("pCustomD"), "yellow");
  EXPECT_EQ(handler.procColor("pCustomE"), "nocolor");
  EXPECT_EQ(handler.procColor("pCustomF"), "nocolor");
}

// Covers scan validation for missing and empty log files.
TEST(ScanHandlerTest, HandleRejectsMissingAndEmptyLogFiles)
{
  TempDir temp("alogscan_validation");
  ScanHandler missing;

  testing::internal::CaptureStdout();
  EXPECT_FALSE(missing.handle(temp.filePath("missing.alog")));
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Unable to find or open"));

  ScanHandler empty;
  const std::string empty_path = temp.writeFile("empty.alog", "");
  testing::internal::CaptureStdout();
  EXPECT_FALSE(empty.handle(empty_path));
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Empty log file"));
}

// Covers rate-only scans over empty files.
TEST(ScanHandlerTest, RateOnlyAcceptsEmptyLogFiles)
{
  TempDir temp("alogscan_rate_empty");
  const std::string empty_path = temp.writeFile("empty.alog", "");

  ScanHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle(empty_path, true));
  handler.dataRateReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "Data Rate (chars/sec): 0"));
}

// Covers variable-stat reporting over grounded .alog entries.
TEST(ScanHandlerTest, VarStatReportListsVariablesSourcesAndTimeRange)
{
  TempDir temp("alogscan_var_report");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"%% LOGSTART 1000.000",
       "1.000 NAV_X pNodeReporter 10",
       "2.000 NAV_Y pNodeReporter 20",
       "3.000 APPCAST pHelmIvP status=ok"});

  ScanHandler handler;
  ASSERT_TRUE(handler.setParam("proc_colors", "false"));
  ASSERT_TRUE(handler.setParam("sort_style", "varname"));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path));
  handler.varStatReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "NAV_X"));
  EXPECT_TRUE(Contains(output, "NAV_Y"));
  EXPECT_TRUE(Contains(output, "APPCAST"));
  EXPECT_TRUE(Contains(output, "pNodeReporter"));
  EXPECT_TRUE(Contains(output, "Total variables: 3"));
}

// Covers ScanReport sort style forwarding through the handler.
TEST(ScanHandlerTest, VarStatReportSortsByVariableName)
{
  TempDir temp("alogscan_sort_vars");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"1.000 NAV_Z pNodeReporter 10",
       "2.000 ALPHA pHelmIvP 20"});

  ScanHandler handler;
  ASSERT_TRUE(handler.setParam("proc_colors", "false"));
  ASSERT_TRUE(handler.setParam("sort_style", "byvars_ascending"));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path));
  handler.varStatReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_LT(PositionOf(output, "ALPHA"), PositionOf(output, "NAV_Z"));
}

// Covers ScanReport source sorting through the handler.
TEST(ScanHandlerTest, VarStatReportSortsBySourceName)
{
  TempDir temp("alogscan_sort_sources");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"1.000 NAV_Z pZulu 10",
       "2.000 NAV_A pAlpha 20"});

  ScanHandler handler;
  ASSERT_TRUE(handler.setParam("proc_colors", "false"));
  ASSERT_TRUE(handler.setParam("sort_style", "bysrc_ascending"));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path));
  handler.varStatReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_LT(PositionOf(output, "NAV_A"), PositionOf(output, "NAV_Z"));
}

// Covers comments and malformed raw lines being ignored by the scanner.
TEST(ScanHandlerTest, VarStatReportIgnoresCommentsAndMalformedEntries)
{
  TempDir temp("alogscan_invalid_lines");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"%% LOGSTART 1000.000",
       "not an alog row",
       "1.000 NAV_X pNodeReporter 10"});

  ScanHandler handler;
  ASSERT_TRUE(handler.setParam("proc_colors", "false"));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path));
  handler.varStatReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "NAV_X"));
  EXPECT_TRUE(Contains(output, "Total variables: 1"));
}

// Covers source formatting when auxiliary source suffixes are disabled.
TEST(ScanHandlerTest, VarStatReportCanUseSourceWithoutAuxSuffix)
{
  TempDir temp("alogscan_source_aux");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"1.000 DESIRED_HEADING pHelmIvP:helm 90"});

  ScanHandler handler;
  ASSERT_TRUE(handler.setParam("proc_colors", "false"));
  ASSERT_TRUE(handler.setParam("use_full_source", "false"));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path));
  handler.varStatReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "pHelmIvP"));
  EXPECT_FALSE(Contains(output, "pHelmIvP:helm"));
}

// Covers app-stat and log-list report surfaces.
TEST(ScanHandlerTest, AppStatAndLogListReportsSummarizeScan)
{
  TempDir temp("alogscan_app_report");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"1.000 NAV_X pNodeReporter 10",
       "2.000 DESIRED_HEADING pHelmIvP 90"});

  ScanHandler handler;
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path));
  handler.appStatReport();
  handler.loglistReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "MOOS Application"));
  EXPECT_TRUE(Contains(output, "pNodeReporter"));
  EXPECT_TRUE(Contains(output, "pHelmIvP"));
  EXPECT_TRUE(Contains(output, "List of Logged variables (2)"));
}

// Covers rate-only scanning and data-rate reporting.
TEST(ScanHandlerTest, RateOnlyScanReportsDataRate)
{
  TempDir temp("alogscan_rate");
  const std::string alog_path = WriteAlog(
      temp, "input.alog",
      {"1.000 NAV_X pNodeReporter 10",
       "3.000 NAV_Y pNodeReporter 20"});

  ScanHandler handler;
  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle(alog_path, true));
  handler.dataRateReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "Data Rate (chars/sec):"));
}
