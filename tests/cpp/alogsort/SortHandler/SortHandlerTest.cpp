#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "SortHandler.h"
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

std::string SortToStdout(TempDir& temp, const std::vector<std::string>& lines)
{
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines(lines));

  SortHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleSort(input_path));
  return testing::internal::GetCapturedStdout();
}

}  // namespace

// Coverage note: this suite exercises deterministic SortHandler behavior
// through synthetic .alog files. The intentionally unpinned path is the
// interactive overwrite prompt; forced overwrite covers the non-interactive CI
// route.

// Covers default configuration and cache-size setter.
TEST(SortHandlerTest, ExposesAndUpdatesCacheSize)
{
  SortHandler handler;

  EXPECT_EQ(handler.getCacheSize(), 1000u);
  handler.setCacheSize(2);
  EXPECT_EQ(handler.getCacheSize(), 2u);
}

// Covers sorting to stdout while preserving header comments.
TEST(SortHandlerTest, SortsEntriesToStdoutPreservingHeaders)
{
  TempDir temp("alogsort_stdout");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string late = "3.000 NAV_X pNodeReporter 30";
  const std::string early = "1.000 NAV_X pNodeReporter 10";
  const std::string mid = "2.000 NAV_Y pNodeReporter 20";

  EXPECT_EQ(SortToStdout(temp, {header, late, early, mid}),
            WithNewlines({header, early, mid, late}));
}

// Covers empty-file sorting.
TEST(SortHandlerTest, SortsEmptyInputToEmptyStdout)
{
  TempDir temp("alogsort_empty");

  EXPECT_EQ(SortToStdout(temp, {}), "");
}

// Covers current comment behavior: comments are emitted when encountered and
// are not held in the sorter with data entries.
TEST(SortHandlerTest, EmitsCommentsBeforeSortedDataEntries)
{
  TempDir temp("alogsort_mid_comment");
  const std::string late = "3.000 NAV_X pNodeReporter 30";
  const std::string comment = "%% MIDLOG COMMENT";
  const std::string early = "1.000 NAV_X pNodeReporter 10";

  EXPECT_EQ(SortToStdout(temp, {late, comment, early}),
            WithNewlines({comment, early, late}));
}

// Covers output-file mode and forced overwrite.
TEST(SortHandlerTest, SortsEntriesToOutputFileWithForceOverwrite)
{
  TempDir temp("alogsort_output");
  const std::string late = "3.000 NAV_X pNodeReporter 30";
  const std::string early = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({late, early}));
  const std::string output_path = temp.writeFile("output.alog", "stale\n");

  SortHandler handler;
  handler.setFileOverWrite(true);
  ASSERT_TRUE(handler.handleSort(input_path, output_path));

  EXPECT_EQ(ReadFile(output_path), WithNewlines({early, late}));
}

// Covers input/output validation that does not enter the interactive prompt.
TEST(SortHandlerTest, RejectsMissingInputAndSameOutputPath)
{
  TempDir temp("alogsort_validation");
  const std::string input_path =
      temp.writeFile("input.alog", "1.000 NAV_X pNodeReporter 10\n");

  SortHandler missing_input;
  EXPECT_FALSE(missing_input.handleSort(temp.filePath("missing.alog")));

  SortHandler same_output;
  EXPECT_FALSE(same_output.handleSort(input_path, input_path));
}

// Covers output open failure: the handler falls back to stdout and still
// reports success.
TEST(SortHandlerTest, OutputOpenFailureFallsBackToStdout)
{
  TempDir temp("alogsort_output_fallback");
  const std::string line = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path = temp.writeFile("input.alog", line + "\n");

  SortHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleSort(input_path, temp.filePath("missing/out.alog")));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, WithNewlines({line}));
}

// Covers the current atof timestamp path for malformed non-comment entries.
TEST(SortHandlerTest, MalformedTimestampSortsAsZero)
{
  TempDir temp("alogsort_bad_timestamp");
  const std::string positive = "1.000 NAV_X pNodeReporter 10";
  const std::string bad_time = "abc NAV_BAD pNodeReporter 0";
  const std::string negative = "-1.000 NAV_NEG pNodeReporter -1";

  EXPECT_EQ(SortToStdout(temp, {positive, bad_time, negative}),
            WithNewlines({negative, bad_time, positive}));
}

// Covers duplicate handling from ALogSorter: exact duplicate raw entries are
// removed, but same-timestamp entries with different payloads are retained.
TEST(SortHandlerTest, SortDropsExactDuplicatesOnly)
{
  TempDir temp("alogsort_duplicates");
  const std::string duplicate = "1.000 NAV_X pNodeReporter 10";
  const std::string same_time = "1.000 NAV_X pNodeReporter 11";
  const std::string later = "2.000 NAV_X pNodeReporter 20";

  EXPECT_EQ(SortToStdout(temp, {duplicate, later, duplicate, same_time}),
            WithNewlines({duplicate, same_time, later}));
}

// Covers cache configuration edge behavior: a zero cache streams each entry as
// it is read, so later earlier timestamps cannot be reordered.
TEST(SortHandlerTest, ZeroCacheStreamsEntriesInInputOrder)
{
  TempDir temp("alogsort_zero_cache");
  const std::string late = "2.000 NAV_X pNodeReporter 20";
  const std::string early = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({late, early}));

  SortHandler handler;
  handler.setCacheSize(0);
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleSort(input_path));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, WithNewlines({late, early}));
}

// Covers check mode for ordered logs: comments are ignored and no diagnostic is
// emitted.
TEST(SortHandlerTest, CheckOrderedLogIgnoresComments)
{
  TempDir temp("alogsort_check_ordered");
  const std::string input_path = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "1.000 NAV_X pNodeReporter 10",
                    "2.000 NAV_X pNodeReporter 20"}));

  SortHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleCheck(input_path));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// Covers check mode over an empty readable file.
TEST(SortHandlerTest, CheckEmptyLogSucceedsSilently)
{
  TempDir temp("alogsort_check_empty");
  const std::string input_path = temp.writeFile("input.alog", "");

  SortHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleCheck(input_path));
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// Covers check mode for out-of-order logs. The current handler reports the
// pair but still returns true.
TEST(SortHandlerTest, CheckOutOfOrderLogReportsPairButReturnsTrue)
{
  TempDir temp("alogsort_check_unordered");
  const std::string late = "2.000 NAV_X pNodeReporter 20";
  const std::string early = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({late, early}));

  SortHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleCheck(input_path));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("Out-of-order pair detected"), std::string::npos);
  EXPECT_NE(output.find(late), std::string::npos);
  EXPECT_NE(output.find(early), std::string::npos);
}

// Covers check mode's current atof timestamp path for malformed entries.
TEST(SortHandlerTest, CheckMalformedTimestampAsZeroCanReportOutOfOrder)
{
  TempDir temp("alogsort_check_bad_timestamp");
  const std::string positive = "1.000 NAV_X pNodeReporter 10";
  const std::string bad_time = "abc NAV_BAD pNodeReporter 0";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({positive, bad_time}));

  SortHandler handler;
  testing::internal::CaptureStdout();
  EXPECT_TRUE(handler.handleCheck(input_path));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("Out-of-order pair detected"), std::string::npos);
  EXPECT_NE(output.find(bad_time), std::string::npos);
}

// Covers check mode input validation.
TEST(SortHandlerTest, CheckRejectsMissingInput)
{
  TempDir temp("alogsort_check_missing");
  SortHandler handler;

  testing::internal::CaptureStdout();
  EXPECT_FALSE(handler.handleCheck(temp.filePath("missing.alog")));
  EXPECT_NE(testing::internal::GetCapturedStdout().find("input not found"),
            std::string::npos);
}

// Covers report counters after sorting a file requiring one re-sort. The
// current handler does not update total line count.
TEST(SortHandlerTest, PrintReportIncludesCurrentCounters)
{
  TempDir temp("alogsort_report");
  const std::string late = "2.000 NAV_X pNodeReporter 20";
  const std::string early = "1.000 NAV_X pNodeReporter 10";
  const std::string input_path =
      temp.writeFile("input.alog", WithNewlines({late, early}));
  const std::string output_path = temp.filePath("output.alog");

  SortHandler handler;
  ASSERT_TRUE(handler.handleSort(input_path, output_path));

  testing::internal::CaptureStdout();
  handler.printReport();
  const std::string report = testing::internal::GetCapturedStdout();

  EXPECT_NE(report.find("Total lines: 0"), std::string::npos);
  EXPECT_NE(report.find("Cache size : 1000"), std::string::npos);
  EXPECT_NE(report.find("Re-Sorts :   1"), std::string::npos);
}
