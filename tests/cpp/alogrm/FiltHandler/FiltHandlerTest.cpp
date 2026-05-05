#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "FiltHandler.h"
#include "TestFileUtils.h"

namespace {

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

std::string ReadFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  EXPECT_TRUE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

std::string FilterToStdout(TempDir& temp,
                           const std::vector<std::string>& lines,
                           const std::vector<std::string>& keys)
{
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines(lines));

  FiltHandler handler;
  for(const std::string& key : keys)
    EXPECT_TRUE(handler.setParam("newkey", key));
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle(input_path));
  return testing::internal::GetCapturedStdout();
}

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers FiltHandler's deterministic removal,
// type filtering, output fallback, validation, and reporting paths against
// app_alogrm. The command-line parser remains outside these component tests.

// Covers parameter validation and boolean parsing.
TEST(FiltHandlerTest, SetsSupportedParametersAndRejectsUnknowns)
{
  FiltHandler handler;

  EXPECT_TRUE(handler.setParam("newkey", "NAV_X"));
  EXPECT_TRUE(handler.setParam("nostrings", "true"));
  EXPECT_TRUE(handler.setParam("nonumbers", "false"));
  EXPECT_TRUE(handler.setParam("clean", "true"));
  EXPECT_TRUE(handler.setParam("file_overwrite", "true"));
  EXPECT_FALSE(handler.setParam("nostrings", "maybe"));
  EXPECT_FALSE(handler.setParam("unknown", "true"));
}

// Covers inverse grep behavior for exact variable and source matches.
TEST(FiltHandlerTest, RemovesExactVariableAndSourceMatches)
{
  TempDir temp("alogrm_exact");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";
  const std::string helm = "1.100 DESIRED_HEADING pHelmIvP:helm 90";
  const std::string nav_y = "1.200 NAV_Y pNodeReporter 20";

  EXPECT_EQ(FilterToStdout(temp, {header, nav_x, helm, nav_y},
                           {"NAV_X", "pHelmIvP"}),
            WithNewlines({header, nav_y}));
}

// Covers wildcard key removal.
TEST(FiltHandlerTest, RemovesWildcardVariableMatches)
{
  TempDir temp("alogrm_wildcard");
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";
  const std::string nav_y = "1.100 NAV_Y pNodeReporter 20";
  const std::string desired = "1.200 DESIRED_HEADING pHelmIvP 90";

  EXPECT_EQ(FilterToStdout(temp, {nav_x, nav_y, desired}, {"NAV_*"}),
            WithNewlines({desired}));
}

// Covers wildcard matching against source names as well as variable names.
TEST(FiltHandlerTest, RemovesWildcardSourceMatches)
{
  TempDir temp("alogrm_source_wildcard");
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";
  const std::string desired = "1.100 DESIRED_HEADING pHelmIvP:helm 90";
  const std::string mode = "1.200 MODE pHelmIvP:mode ACTIVE";

  EXPECT_EQ(FilterToStdout(temp, {nav_x, desired, mode}, {"pHelm*"}),
            WithNewlines({nav_x}));
}

// Covers default retention when no removal criteria are configured.
TEST(FiltHandlerTest, NoKeysRetainsCommentsBadLinesAndData)
{
  TempDir temp("alogrm_no_keys");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string bad_line = "NAV_X double pNodeReporter";
  const std::string data = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({header, bad_line, data}));

  FiltHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle(input_path));
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            WithNewlines({header, bad_line, data}));
}

// Covers clean mode and DB_VARSUMMARY continuation removal.
TEST(FiltHandlerTest, CleanAndDbVarSummaryRemoveBadLines)
{
  TempDir temp("alogrm_bad_lines");
  const std::string summary = "1.000 DB_VARSUMMARY pLogger vars=NAV_X";
  const std::string continuation = "NAV_X double pNodeReporter";
  const std::string keep = "1.100 NAV_X pNodeReporter 10";

  FiltHandler clean;
  ASSERT_TRUE(clean.setParam("clean", "true"));
  const std::string clean_path =
      temp.writeFile("clean.alog", WithNewlines({continuation, keep}));
  testing::internal::CaptureStdout();
  EXPECT_TRUE(clean.handle(clean_path));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), WithNewlines({keep}));

  EXPECT_EQ(FilterToStdout(temp, {summary, continuation, keep},
                           {"DB_VARSUMMARY"}),
            WithNewlines({keep}));
}

// Covers type-based removal of strings and numbers.
TEST(FiltHandlerTest, RemovesStringOrNumericDataByMode)
{
  TempDir temp("alogrm_types");
  const std::string num = "1.000 NAV_X pNodeReporter 10";
  const std::string str = "1.100 MODE pHelmIvP DRIVE";

  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({num, str}));

  FiltHandler no_strings;
  ASSERT_TRUE(no_strings.setParam("nostrings", "true"));
  testing::internal::CaptureStdout();
  EXPECT_TRUE(no_strings.handle(input_path));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), WithNewlines({num}));

  const std::string input_path2 =
      temp.writeFile("input2.alog", WithNewlines({num, str}));
  FiltHandler no_numbers;
  ASSERT_TRUE(no_numbers.setParam("nonumbers", "true"));
  testing::internal::CaptureStdout();
  EXPECT_TRUE(no_numbers.handle(input_path2));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), WithNewlines({str}));
}

// Covers dynamic variable-key promotion after type-based removal.
TEST(FiltHandlerTest, TypeRemovalAlsoRemovesLaterRowsForSameVariable)
{
  TempDir temp("alogrm_type_key_promotion");
  const std::string mode_string = "1.000 MODE pHelmIvP DRIVE";
  const std::string mode_number = "1.100 MODE pHelmIvP 7";
  const std::string nav_x = "1.200 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog",
                     WithNewlines({mode_string, mode_number, nav_x}));

  FiltHandler no_strings;
  ASSERT_TRUE(no_strings.setParam("nostrings", "true"));
  testing::internal::CaptureStdout();
  EXPECT_TRUE(no_strings.handle(input_path));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), WithNewlines({nav_x}));
}

// Covers output file mode, existing-file rejection, and forced overwrite.
TEST(FiltHandlerTest, OutputFileRequiresForceBeforeOverwrite)
{
  TempDir temp("alogrm_output");
  const std::string keep = "1.000 NAV_Y pNodeReporter 20";
  const std::string drop = "1.100 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep, drop}));
  const std::string output_path = temp.writeFile("output.alog", "stale\n");

  FiltHandler rejects_existing;
  EXPECT_FALSE(rejects_existing.handle(input_path, output_path));

  FiltHandler force;
  ASSERT_TRUE(force.setParam("file_overwrite", "true"));
  ASSERT_TRUE(force.setParam("newkey", "NAV_X"));
  ASSERT_TRUE(force.handle(input_path, output_path));
  EXPECT_EQ(ReadFile(output_path), WithNewlines({keep}));
}

// Covers output open failure: filtering falls back to stdout and still reports
// success when the target cannot be opened for writing.
TEST(FiltHandlerTest, OutputOpenFailureFallsBackToStdout)
{
  TempDir temp("alogrm_output_fallback");
  const std::string keep = "1.000 NAV_Y pNodeReporter 20";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep}));

  FiltHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle(input_path, temp.filePath("missing/out.alog")));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), WithNewlines({keep}));
}

// Covers validation for missing input and identical input/output paths.
TEST(FiltHandlerTest, RejectsMissingInputAndSameOutputPath)
{
  TempDir temp("alogrm_validation");
  const std::string input_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");

  FiltHandler missing;
  EXPECT_FALSE(missing.handle(temp.filePath("missing.alog")));

  FiltHandler same;
  EXPECT_FALSE(same.handle(input_path, input_path));
}

// Covers report accounting after a mixed retain/remove run.
TEST(FiltHandlerTest, PrintReportSummarizesRetainedAndDeletedLines)
{
  TempDir temp("alogrm_report");
  const std::string keep = "1.000 NAV_Y pNodeReporter 20";
  const std::string drop = "1.100 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep, drop}));
  const std::string output_path = temp.filePath("output.alog");

  FiltHandler handler;
  ASSERT_TRUE(handler.setParam("newkey", "NAV_X"));
  ASSERT_TRUE(handler.handle(input_path, output_path));

  testing::internal::CaptureStdout();
  handler.printReport();
  const std::string report = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(report, "Total lines retained: 1 (50.00%)"));
  EXPECT_TRUE(Contains(report, "Total lines deleted: 1 (50.00%)"));
  EXPECT_TRUE(Contains(report, "Variables retained: (1) NAV_Y"));
}
