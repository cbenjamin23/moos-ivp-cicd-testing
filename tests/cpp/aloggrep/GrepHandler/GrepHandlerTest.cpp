#include <gtest/gtest.h>

#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "GrepHandler.h"
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

std::string RunToStdout(
    TempDir& temp,
    const std::vector<std::string>& lines,
    const std::vector<std::string>& keys,
    const std::function<void(GrepHandler&)>& configure =
        std::function<void(GrepHandler&)>())
{
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines(lines));

  GrepHandler handler;
  if(configure)
    configure(handler);

  EXPECT_TRUE(handler.setALogFile(input_path));
  for(const std::string& key : keys)
    handler.addKey(key);

  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handle());
  return testing::internal::GetCapturedStdout();
}

}  // namespace

// Coverage note: this suite exercises GrepHandler through its stable file and
// stdout behavior. The intentionally unpinned path is the interactive overwrite
// prompt; overwrite behavior is covered through the force flag instead.

// Covers default filtering: comments are retained, exact variable matches are
// retained, and unmatched, bad, gap, and APPCAST lines are dropped.
TEST(GrepHandlerTest, DefaultRetainsCommentsAndExactVariableMatches)
{
  TempDir temp("aloggrep_default");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";
  const std::string nav_y = "1.100 NAV_Y pNodeReporter 20";
  const std::string bad = "DB_VARSUMMARY continuation text";
  const std::string gap = "1.200 NAV_X_GAP pLogger 0.5";
  const std::string appcast = "1.300 APPCAST pHelmIvP status=ok";

  const std::string output =
      RunToStdout(temp, {header, nav_x, nav_y, bad, gap, appcast}, {"NAV_X"});

  EXPECT_EQ(output, WithNewlines({header, nav_x}));
}

// Covers default behavior with no grep keys: only retained comments survive.
TEST(GrepHandlerTest, NoKeysRetainsOnlyComments)
{
  TempDir temp("aloggrep_no_keys");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";

  const std::string output = RunToStdout(temp, {header, nav_x}, {});

  EXPECT_EQ(output, WithNewlines({header}));
}

// Covers retention toggles for comments, bad lines, gap/length variables, and
// APPCAST entries.
TEST(GrepHandlerTest, RetentionTogglesControlSpecialLineClasses)
{
  TempDir temp("aloggrep_toggles");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string bad = "DB_VARSUMMARY continuation text";
  const std::string gap = "1.200 NAV_X_GAP pLogger 0.5";
  const std::string appcast = "1.300 APPCAST pHelmIvP status=ok";

  const std::string output = RunToStdout(
      temp, {header, bad, gap, appcast}, {"NAV_X_GAP", "APPCAST"},
      [](GrepHandler& handler) {
        handler.setCommentsRetained(false);
        handler.setBadLinesRetained(true);
        handler.setGapLinesRetained(true);
        handler.setAppCastRetained(true);
      });

  EXPECT_EQ(output, WithNewlines({bad, gap, appcast}));
}

// Covers the filter order for special variables: explicit keys alone do not
// bypass the default APPCAST and gap/length suppression.
TEST(GrepHandlerTest, ExplicitSpecialVariableKeysStillNeedRetentionToggles)
{
  TempDir temp("aloggrep_special_keys");
  const std::string gap = "1.200 NAV_X_GAP pLogger 0.5";
  const std::string appcast = "1.300 APPCAST pHelmIvP status=ok";

  const std::string output =
      RunToStdout(temp, {gap, appcast}, {"NAV_X_GAP", "APPCAST"});

  EXPECT_EQ(output, "");
}

// Covers the DB_VARSUMMARY compatibility path: requesting DB_VARSUMMARY also
// retains following non-timestamp continuation lines.
TEST(GrepHandlerTest, DbVarSummaryKeyRetainsContinuationBadLines)
{
  TempDir temp("aloggrep_db_varsummary");
  const std::string summary = "1.000 DB_VARSUMMARY pLogger vars=NAV_X,NAV_Y";
  const std::string continuation = "NAV_X double pNodeReporter";
  const std::string other = "1.100 NAV_X pNodeReporter 10";

  const std::string output =
      RunToStdout(temp, {summary, continuation, other}, {"DB_VARSUMMARY"});

  EXPECT_EQ(output, WithNewlines({summary, continuation}));
}

// Covers source matching using the source name without auxiliary suffixes.
TEST(GrepHandlerTest, MatchesSourceNameWithoutAuxSuffix)
{
  TempDir temp("aloggrep_source");
  const std::string echo = "1.000 NAV_X pEchoVar 10";
  const std::string helm = "1.100 DESIRED_HEADING pHelmIvP:helm 90";
  const std::string other = "1.200 NAV_Y pNodeReporter 20";

  const std::string output =
      RunToStdout(temp, {echo, helm, other}, {"pHelmIvP"});

  EXPECT_EQ(output, WithNewlines({helm}));
}

// Covers wildcard matching against source names.
TEST(GrepHandlerTest, PrefixMatchesSourceNames)
{
  TempDir temp("aloggrep_source_prefix");
  const std::string echo = "1.000 NAV_X pEchoVar 10";
  const std::string helm = "1.100 DESIRED_HEADING pHelmIvP 90";
  const std::string other = "1.200 NAV_Y pNodeReporter 20";

  const std::string output =
      RunToStdout(temp, {echo, helm, other}, {"pHelm*"});

  EXPECT_EQ(output, WithNewlines({helm}));
}

// Covers repeated key insertion and wildcard matching: a later wildcard form
// upgrades an existing exact key into a prefix match.
TEST(GrepHandlerTest, DuplicateKeyCanBeUpgradedToPrefixMatch)
{
  TempDir temp("aloggrep_prefix");
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";
  const std::string nav_x_extra = "1.100 NAV_X_EXTRA pNodeReporter 11";
  const std::string nav_y = "1.200 NAV_Y pNodeReporter 20";

  const std::string output =
      RunToStdout(temp, {nav_x, nav_x_extra, nav_y}, {"NAV_X", "NAV_X*"});

  EXPECT_EQ(output, WithNewlines({nav_x, nav_x_extra}));
}

// Covers value-only format mode, including comment suppression and duplicate
// timestamp suppression in formatted output.
TEST(GrepHandlerTest, ValueFormatSuppressesCommentsAndDuplicateTimestamps)
{
  TempDir temp("aloggrep_format_val");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string first = "1.000 NAV_X pNodeReporter 10";
  const std::string duplicate_time = "1.000 NAV_X pNodeReporter 11";
  const std::string second = "2.000 NAV_X pNodeReporter x=1, y=2";

  const std::string output = RunToStdout(
      temp, {header, first, duplicate_time, second}, {"NAV_X"},
      [](GrepHandler& handler) { EXPECT_TRUE(handler.setFormat("val")); });

  EXPECT_EQ(output, WithNewlines({"10", "x=1, y=2"}));
}

// Covers formatted time/variable/value columns with caller-selected separators.
TEST(GrepHandlerTest, TimeVarValueFormatUsesConfiguredColumnSeparator)
{
  TempDir temp("aloggrep_format_tvv");
  const std::string line = "1.250 NAV_X pNodeReporter 10";

  const std::string output = RunToStdout(
      temp, {line}, {"NAV_X"}, [](GrepHandler& handler) {
        handler.setColSep(',');
        EXPECT_TRUE(handler.setFormat("time:var:val"));
      });

  EXPECT_EQ(output, "1.250,NAV_X,10\n");
}

// Covers invalid column separators: unsupported separators are ignored and the
// previously configured separator remains active.
TEST(GrepHandlerTest, InvalidColumnSeparatorIsIgnored)
{
  TempDir temp("aloggrep_bad_separator");
  const std::string line = "1.250 NAV_X pNodeReporter 10";

  const std::string output = RunToStdout(
      temp, {line}, {"NAV_X"}, [](GrepHandler& handler) {
        handler.setColSep(',');
        handler.setColSep('|');
        EXPECT_TRUE(handler.setFormat("time:var:val"));
      });

  EXPECT_EQ(output, "1.250,NAV_X,10\n");
}

// Covers source-only format: matching uses source-without-aux, while formatted
// output preserves the full source field.
TEST(GrepHandlerTest, SourceFormatOutputsFullSourceField)
{
  TempDir temp("aloggrep_format_src");
  const std::string line = "1.100 DESIRED_HEADING pHelmIvP:helm 90";

  const std::string output = RunToStdout(
      temp, {line}, {"pHelmIvP"},
      [](GrepHandler& handler) { EXPECT_TRUE(handler.setFormat("src")); });

  EXPECT_EQ(output, "pHelmIvP:helm\n");
}

// Covers formatted output's global duplicate timestamp suppression across
// different retained variables.
TEST(GrepHandlerTest, FormattedOutputSuppressesRepeatedTimestampsAcrossVariables)
{
  TempDir temp("aloggrep_format_same_time");
  const std::string nav_x = "1.000 NAV_X pNodeReporter 10";
  const std::string nav_y_same_time = "1.000 NAV_Y pNodeReporter 20";
  const std::string nav_y_later = "2.000 NAV_Y pNodeReporter 21";

  const std::string output = RunToStdout(
      temp, {nav_x, nav_y_same_time, nav_y_later}, {"NAV_*"},
      [](GrepHandler& handler) { EXPECT_TRUE(handler.setFormat("time:var:val")); });

  EXPECT_EQ(output, WithNewlines({"1.000 NAV_X 10", "2.000 NAV_Y 21"}));
}

// Covers subpattern extraction in formatted value mode with and without key
// retention.
TEST(GrepHandlerTest, SubpatternsExtractRequestedValueComponents)
{
  TempDir temp("aloggrep_subpat");
  const std::string line =
      "1.000 NODE_REPORT pNodeReporter X=10, Y=20, SPD=3.5";

  const std::string bare_output = RunToStdout(
      temp, {line}, {"NODE_REPORT"}, [](GrepHandler& handler) {
        EXPECT_TRUE(handler.setFormat("val"));
        handler.addSubPattern("x");
        handler.addSubPattern("spd");
      });
  EXPECT_EQ(bare_output, "10 3.5\n");

  TempDir keep_key_temp("aloggrep_subpat_keepkey");
  const std::string keep_key_output = RunToStdout(
      keep_key_temp, {line}, {"NODE_REPORT"}, [](GrepHandler& handler) {
        handler.setKeepKey(true);
        EXPECT_TRUE(handler.setFormat("val"));
        handler.addSubPattern("x");
        handler.addSubPattern("spd");
      });
  EXPECT_EQ(keep_key_output, "x=10, spd=3.5\n");
}

// Covers first-only mode: comments are suppressed and processing stops after
// the first retained data line.
TEST(GrepHandlerTest, FirstOnlyOutputsOnlyFirstMatch)
{
  TempDir temp("aloggrep_first");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string first = "1.000 NAV_X pNodeReporter 10";
  const std::string second = "2.000 NAV_X pNodeReporter 20";

  const std::string output = RunToStdout(
      temp, {header, first, second}, {"NAV_X"},
      [](GrepHandler& handler) { handler.setFirstOnly(true); });

  EXPECT_EQ(output, WithNewlines({first}));
}

// Covers final-only mode: only the final retained line is emitted.
TEST(GrepHandlerTest, FinalOnlyOutputsOnlyLastRetainedLine)
{
  TempDir temp("aloggrep_final");
  const std::string first = "1.000 NAV_X pNodeReporter 10";
  const std::string other = "1.500 NAV_Y pNodeReporter 15";
  const std::string second = "2.000 NAV_X pNodeReporter 20";

  const std::string output = RunToStdout(
      temp, {first, other, second}, {"NAV_X"},
      [](GrepHandler& handler) { handler.setFinalOnly(true); });

  EXPECT_EQ(output, WithNewlines({second}));
}

// Covers final-only mode when no matching data line is observed: retained
// comments can still become the final retained line.
TEST(GrepHandlerTest, FinalOnlyWithoutDataMatchesCanReturnComment)
{
  TempDir temp("aloggrep_final_no_match");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string nav_y = "1.000 NAV_Y pNodeReporter 10";

  const std::string output = RunToStdout(
      temp, {header, nav_y}, {"NAV_X"},
      [](GrepHandler& handler) { handler.setFinalOnly(true); });

  EXPECT_EQ(output, WithNewlines({header}));
}

// Covers sorted output for retained entries.
TEST(GrepHandlerTest, SortEntriesOutputsRetainedLinesByTimestamp)
{
  TempDir temp("aloggrep_sort");
  const std::string late = "3.000 NAV_X pNodeReporter 30";
  const std::string early = "1.000 NAV_X pNodeReporter 10";
  const std::string mid = "2.000 NAV_X pNodeReporter 20";

  const std::string output = RunToStdout(
      temp, {late, early, mid}, {"NAV_X"}, [](GrepHandler& handler) {
        handler.setCommentsRetained(false);
        handler.setSortEntries(true);
      });

  EXPECT_EQ(output, WithNewlines({early, mid, late}));
}

// Covers duplicate removal when sorting is enabled.
TEST(GrepHandlerTest, SortWithRemoveDupsDropsExactDuplicateEntries)
{
  TempDir temp("aloggrep_dups");
  const std::string duplicate = "1.000 NAV_X pNodeReporter 10";
  const std::string same_time_different_value = "1.000 NAV_X pNodeReporter 11";
  const std::string later = "2.000 NAV_X pNodeReporter 20";

  const std::string output = RunToStdout(
      temp, {duplicate, later, duplicate, same_time_different_value}, {"NAV_X"},
      [](GrepHandler& handler) {
        handler.setCommentsRetained(false);
        handler.setSortEntries(true);
        handler.setRemoveDups(true);
      });

  EXPECT_EQ(output,
            WithNewlines({duplicate, same_time_different_value, later}));
}

// Covers output-file mode with forced overwrite.
TEST(GrepHandlerTest, WritesFilteredOutputFileWhenOutputPathIsProvided)
{
  TempDir temp("aloggrep_output");
  const std::string keep = "1.000 NAV_X pNodeReporter 10";
  const std::string drop = "1.100 NAV_Y pNodeReporter 20";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep, drop}));
  const std::string output_path = temp.writeFile("output.alog", "stale\n");

  GrepHandler handler;
  handler.setFileOverWrite(true);
  ASSERT_TRUE(handler.setALogFile(input_path));
  ASSERT_TRUE(handler.setALogFile(output_path));
  handler.addKey("NAV_X");

  ASSERT_TRUE(handler.handle());

  EXPECT_EQ(ReadFile(output_path), WithNewlines({keep}));
}

// Covers vname substitution in the requested output path.
TEST(GrepHandlerTest, OutputPathCanSubstituteVNameFromDbTimeSource)
{
  TempDir temp("aloggrep_vehicle_name");
  const std::string db_time = "0.100 DB_TIME MOOSDB_alpha 1234567890";
  const std::string keep = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({db_time, keep}));
  const std::string templated_output_path = temp.filePath("grep_vname.alog");
  const std::string resolved_output_path = temp.filePath("grep_alpha.alog");

  GrepHandler handler;
  ASSERT_TRUE(handler.setALogFile(input_path));
  testing::internal::CaptureStdout();
  const bool output_opened = handler.setALogFile(templated_output_path);
  testing::internal::GetCapturedStdout();
  ASSERT_TRUE(output_opened);
  handler.addKey("NAV_X");

  ASSERT_TRUE(handler.handle());

  EXPECT_EQ(ReadFile(resolved_output_path), WithNewlines({keep}));
  EXPECT_FALSE(std::ifstream(templated_output_path.c_str()).is_open());
}

// Covers vname output paths when no DB_TIME source provides a vehicle name:
// the requested path is used unchanged.
TEST(GrepHandlerTest, OutputPathKeepsVNameTokenWhenNoDbTimeSourceExists)
{
  TempDir temp("aloggrep_vehicle_missing");
  const std::string keep = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep}));
  const std::string output_path = temp.filePath("grep_vname.alog");

  GrepHandler handler;
  ASSERT_TRUE(handler.setALogFile(input_path));
  testing::internal::CaptureStdout();
  const bool output_opened = handler.setALogFile(output_path);
  testing::internal::GetCapturedStdout();
  ASSERT_TRUE(output_opened);
  handler.addKey("NAV_X");

  ASSERT_TRUE(handler.handle());

  EXPECT_EQ(ReadFile(output_path), WithNewlines({keep}));
}

// Covers input/output validation in setALogFile without entering the
// interactive overwrite path.
TEST(GrepHandlerTest, RejectsMissingInputSameOutputAndUnwritableOutputPath)
{
  TempDir temp("aloggrep_set_file");
  const std::string input_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");

  GrepHandler missing_input;
  EXPECT_FALSE(missing_input.setALogFile(""));
  EXPECT_FALSE(missing_input.setALogFile(temp.filePath("missing.alog")));

  GrepHandler handler;
  ASSERT_TRUE(handler.setALogFile(input_path));
  EXPECT_FALSE(handler.setALogFile(input_path));
  EXPECT_FALSE(handler.setALogFile(temp.filePath("missing_dir/output.alog")));

  GrepHandler two_files;
  ASSERT_TRUE(two_files.setALogFile(input_path));
  ASSERT_TRUE(two_files.setALogFile(temp.filePath("output.alog")));
  EXPECT_FALSE(two_files.setALogFile(temp.filePath("extra.alog")));
}

// Covers handle validation when no input file has been specified.
TEST(GrepHandlerTest, HandleFailsWithoutInputFile)
{
  GrepHandler handler;

  testing::internal::CaptureStdout();
  const bool handled = handler.handle();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(handled);
  EXPECT_NE(output.find("No input alog file given"), std::string::npos);
}

// Covers format validation.
TEST(GrepHandlerTest, RejectsEmptyAndUnknownFormats)
{
  GrepHandler handler;

  EXPECT_FALSE(handler.setFormat(""));
  EXPECT_FALSE(handler.setFormat("time:bogus"));
  EXPECT_TRUE(handler.setFormat("time:src"));
}

// Covers summary report accounting after a handled output-file run.
TEST(GrepHandlerTest, PrintReportSummarizesRetainedAndRemovedLines)
{
  TempDir temp("aloggrep_report");
  const std::string keep_a = "1.000 NAV_X pNodeReporter 10";
  const std::string drop_a = "1.100 NAV_Y pNodeReporter 20";
  const std::string keep_b = "1.200 NAV_X pNodeReporter 11";
  const std::string drop_b = "1.300 NAV_Z pNodeReporter 30";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep_a, drop_a, keep_b, drop_b}));
  const std::string output_path = temp.filePath("output.alog");

  GrepHandler handler;
  ASSERT_TRUE(handler.setALogFile(input_path));
  ASSERT_TRUE(handler.setALogFile(output_path));
  handler.addKey("NAV_X");
  ASSERT_TRUE(handler.handle());

  testing::internal::CaptureStdout();
  handler.printReport();
  const std::string report = testing::internal::GetCapturedStdout();

  EXPECT_NE(report.find("Total lines retained: 2 (50.00%)"),
            std::string::npos);
  EXPECT_NE(report.find("Total lines excluded: 2 (50.00%)"),
            std::string::npos);
  EXPECT_NE(report.find("Variables retained: (1) NAV_X"), std::string::npos);
}

// Covers explicit report suppression.
TEST(GrepHandlerTest, MakeReportFalseSuppressesSummary)
{
  TempDir temp("aloggrep_report_suppressed");
  const std::string keep = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({keep}));

  GrepHandler handler;
  handler.setMakeReport(false);
  ASSERT_TRUE(handler.setALogFile(input_path));
  handler.addKey("NAV_X");

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.handle());
  handler.printReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, WithNewlines({keep}));
}

// Covers report accounting for double-percent alog header lines: they are
// emitted but do not count as retained data lines.
TEST(GrepHandlerTest, PrintReportDoesNotCountDoublePercentHeaders)
{
  TempDir temp("aloggrep_report_headers");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string keep = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({header, keep}));
  const std::string output_path = temp.filePath("output.alog");

  GrepHandler handler;
  ASSERT_TRUE(handler.setALogFile(input_path));
  ASSERT_TRUE(handler.setALogFile(output_path));
  handler.addKey("NAV_X");
  ASSERT_TRUE(handler.handle());

  testing::internal::CaptureStdout();
  handler.printReport();
  const std::string report = testing::internal::GetCapturedStdout();

  EXPECT_EQ(ReadFile(output_path), WithNewlines({header, keep}));
  EXPECT_NE(report.find("Total lines retained: 1 (100.00%)"),
            std::string::npos);
  EXPECT_NE(report.find("Total lines excluded: 0 (0.00%)"),
            std::string::npos);
}
