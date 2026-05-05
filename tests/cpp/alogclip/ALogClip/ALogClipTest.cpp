#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "ALogClipHandler.h"
#include "ALogClipper.h"
#include "TestFileUtils.h"

namespace {

class TestableALogClipHandler : public ALogClipHandler {
 public:
  using ALogClipHandler::addSuffixToALogFile;
  using ALogClipHandler::preCheck;
  using ALogClipHandler::processFile;
};

std::string JoinLines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(unsigned int i = 0; i < lines.size(); ++i) {
    if(i > 0)
      contents += "\n";
    contents += lines[i];
  }
  return contents;
}

std::string ReadFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  EXPECT_TRUE(in.is_open());
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

bool ClipFile(const std::string& input_path,
              const std::string& output_path,
              double min_time,
              double max_time,
              ALogClipper& clipper)
{
  if(!clipper.openALogFileRead(input_path))
    return false;
  if(!clipper.openALogFileWrite(output_path))
    return false;
  clipper.clip(min_time, max_time);
  return true;
}

}  // namespace

// Coverage note: this suite exercises the deterministic ALogClipper and
// ALogClipHandler edges that are stable enough for CI. The only intentionally
// unpinned paths are the interactive overwrite prompt, the non-batch output
// writability precheck typo, and trailing blank/EOF handling where upstream
// parses an empty final read through timestr[0].

// Covers alogclip line filtering: headers and IVPHELM_DOMAIN survive while
// entries outside the inclusive time window are clipped and counted by side.
TEST(ALogClipperTest, ClipsWindowPreservingHeadersAndHelmDomain)
{
  TempDir temp("alogclip_window");
  const std::string header = "%% LOGSTART 1000.000";
  const std::string front = "0.500 NAV_X pNodeReporter 1";
  const std::string domain =
      "0.750 IVPHELM_DOMAIN pHelmIvP course:0:359:360";
  const std::string min_line = "1.000 NAV_X pNodeReporter 10";
  const std::string mid_line = "2.500 NAV_Y pNodeReporter 20";
  const std::string max_line = "3.000 NAV_SPEED pNodeReporter 2.0";
  const std::string back = "3.001 NAV_X pNodeReporter 11";
  const std::string input_path = temp.writeFile(
      "input.alog",
      JoinLines({header, front, domain, min_line, mid_line, max_line, back}));
  const std::string output_path = temp.filePath("output.alog");

  ALogClipper clipper;
  ASSERT_TRUE(ClipFile(input_path, output_path, 1.0, 3.0, clipper));

  EXPECT_EQ(ReadFile(output_path),
            WithNewlines({header, domain, min_line, mid_line, max_line}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 4u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_front"), 1u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 1u);
  EXPECT_EQ(clipper.getDetails("kept_chars"),
            static_cast<unsigned int>(domain.size() + min_line.size() +
                                      mid_line.size() + max_line.size()));
  EXPECT_EQ(clipper.getDetails("clipped_chars_front"),
            static_cast<unsigned int>(front.size()));
  EXPECT_EQ(clipper.getDetails("clipped_chars_back"),
            static_cast<unsigned int>(back.size()));
  EXPECT_EQ(clipper.getDetails("not_a_detail"), 0u);
}

// Covers alogclip header accounting: header records are copied verbatim but do
// not contribute to kept or clipped line/character counters.
TEST(ALogClipperTest, PreservesHeadersWithoutCountingThem)
{
  TempDir temp("alogclip_headers");
  const std::string header_a = "% LOGSTART 1000.000";
  const std::string header_b = "%% LOG_METADATA source=pLogger";
  const std::string front = "0.999 NAV_X pNodeReporter 1";
  const std::string keep = "1.000 NAV_X pNodeReporter 2";
  const std::string back = "1.001 NAV_X pNodeReporter 3";
  const std::string input_path = temp.writeFile(
      "input.alog", JoinLines({header_a, front, header_b, keep, back}));
  const std::string output_path = temp.filePath("output.alog");

  ALogClipper clipper;
  ASSERT_TRUE(ClipFile(input_path, output_path, 1.0, 1.0, clipper));

  EXPECT_EQ(ReadFile(output_path), WithNewlines({header_a, header_b, keep}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 1u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_front"), 1u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 1u);
  EXPECT_EQ(clipper.getDetails("kept_chars"),
            static_cast<unsigned int>(keep.size()));
  EXPECT_EQ(clipper.getDetails("clipped_chars_front"),
            static_cast<unsigned int>(front.size()));
  EXPECT_EQ(clipper.getDetails("clipped_chars_back"),
            static_cast<unsigned int>(back.size()));
}

// Covers alogclip header-only files: all headers are preserved and data
// counters remain zero.
TEST(ALogClipperTest, PreservesHeaderOnlyFilesWithZeroDataCounters)
{
  TempDir temp("alogclip_header_only");
  const std::string header_a = "% LOGSTART 1000.000";
  const std::string header_b = "%% LOG_METADATA source=pLogger";
  const std::string input_path =
      temp.writeFile("input.alog", JoinLines({header_a, header_b}));
  const std::string output_path = temp.filePath("output.alog");

  ALogClipper clipper;
  ASSERT_TRUE(ClipFile(input_path, output_path, 1.0, 2.0, clipper));

  EXPECT_EQ(ReadFile(output_path), WithNewlines({header_a, header_b}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 0u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_front"), 0u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 0u);
  EXPECT_EQ(clipper.getDetails("kept_chars"), 0u);
}

// Covers alogclip preserved-variable behavior on both sides of the clip window.
TEST(ALogClipperTest, PreservesHelmDomainBeforeAndAfterWindow)
{
  TempDir temp("alogclip_preserve_var");
  const std::string domain_front =
      "0.500 IVPHELM_DOMAIN pHelmIvP course:0:359:360";
  const std::string drop_front = "0.500 NAV_X pNodeReporter 1";
  const std::string keep = "2.000 NAV_X pNodeReporter 2";
  const std::string domain_back =
      "3.500 IVPHELM_DOMAIN pHelmIvP speed:0:5:51";
  const std::string drop_back = "3.500 NAV_X pNodeReporter 3";
  const std::string input_path = temp.writeFile(
      "input.alog",
      JoinLines({domain_front, drop_front, keep, domain_back, drop_back}));
  const std::string output_path = temp.filePath("output.alog");

  ALogClipper clipper;
  ASSERT_TRUE(ClipFile(input_path, output_path, 1.0, 3.0, clipper));

  EXPECT_EQ(ReadFile(output_path),
            WithNewlines({domain_front, keep, domain_back}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 3u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_front"), 1u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 1u);
}

// Covers alogclip timestamp handling: bounds are inclusive and non-numeric
// timestamps follow the current atof-style conversion path.
TEST(ALogClipperTest, InclusiveBoundsAndAtOfTimestampFallback)
{
  TempDir temp("alogclip_bounds");
  const std::string bad_time = "abc NAV_BAD pNodeReporter 1";
  const std::string negative = "-1.000 NAV_NEG pNodeReporter 2";
  const std::string min_line = "1.000 NAV_MIN pNodeReporter 3";
  const std::string max_line = "2.000 NAV_MAX pNodeReporter 4";
  const std::string back = "2.001 NAV_BACK pNodeReporter 5";
  const std::string input_path = temp.writeFile(
      "input.alog",
      JoinLines({bad_time, negative, min_line, max_line, back}));
  const std::string output_path = temp.filePath("output.alog");

  ALogClipper clipper;
  ASSERT_TRUE(ClipFile(input_path, output_path, 1.0, 2.0, clipper));

  EXPECT_EQ(ReadFile(output_path), WithNewlines({min_line, max_line}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 2u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_front"), 2u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 1u);
}

// Covers alogclip stdout mode at the clipper layer when no output file is open.
TEST(ALogClipperTest, ClipWithoutOutputFileWritesToStdout)
{
  TempDir temp("alogclip_clipper_stdout");
  const std::string keep = "2.000 NAV_X pNodeReporter 10";
  const std::string drop = "4.000 NAV_Y pNodeReporter 20";
  const std::string input_path =
      temp.writeFile("input.alog", JoinLines({keep, drop}));

  ALogClipper clipper;
  ASSERT_TRUE(clipper.openALogFileRead(input_path));

  testing::internal::CaptureStdout();
  EXPECT_EQ(clipper.clip(1.0, 3.0), 1u);
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, WithNewlines({keep}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 1u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 1u);
}

// Covers clipper behavior when no input file has been opened.
TEST(ALogClipperTest, ClipWithoutInputReturnsZero)
{
  ALogClipper clipper;

  testing::internal::CaptureStdout();
  EXPECT_EQ(clipper.clip(1.0, 2.0), 0u);
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
  EXPECT_EQ(clipper.getDetails("kept_lines"), 0u);
}

// Covers alogclip counter lifetime: details accumulate across multiple clips
// on the same ALogClipper instance.
TEST(ALogClipperTest, AccumulatesDetailsAcrossSequentialClips)
{
  TempDir temp("alogclip_accumulates");
  const std::string keep_a = "2.000 NAV_X pNodeReporter 10";
  const std::string drop_a = "4.000 NAV_Y pNodeReporter 20";
  const std::string keep_b = "2.000 NAV_SPEED pNodeReporter 2.0";
  const std::string drop_b = "0.500 NAV_DEPTH pNodeReporter 5";
  const std::string input_a =
      temp.writeFile("alpha.alog", JoinLines({keep_a, drop_a}));
  const std::string input_b =
      temp.writeFile("bravo.alog", JoinLines({drop_b, keep_b}));
  const std::string output_a = temp.filePath("alpha_out.alog");
  const std::string output_b = temp.filePath("bravo_out.alog");

  ALogClipper clipper;
  ASSERT_TRUE(ClipFile(input_a, output_a, 1.0, 3.0, clipper));
  ASSERT_TRUE(ClipFile(input_b, output_b, 1.0, 3.0, clipper));

  EXPECT_EQ(ReadFile(output_a), WithNewlines({keep_a}));
  EXPECT_EQ(ReadFile(output_b), WithNewlines({keep_b}));
  EXPECT_EQ(clipper.getDetails("kept_lines"), 2u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_front"), 1u);
  EXPECT_EQ(clipper.getDetails("clipped_lines_back"), 1u);
}

// Covers alogclip file opening validation.
TEST(ALogClipperTest, RejectsMissingInputAndUnwritableOutputPath)
{
  TempDir temp("alogclip_open_failures");
  ALogClipper clipper;

  EXPECT_FALSE(clipper.openALogFileRead(temp.filePath("missing.alog")));
  EXPECT_FALSE(clipper.openALogFileWrite(temp.filePath("missing_dir/out.alog")));
}

// Covers alogclip handler timestamp validation: exactly two ordered timestamps
// are accepted.
TEST(ALogClipHandlerTest, ValidatesOrderedTimestamps)
{
  ALogClipHandler handler;

  EXPECT_TRUE(handler.setTimeStamp(10.0));
  EXPECT_FALSE(handler.setTimeStamp(9.0));
  EXPECT_TRUE(handler.setTimeStamp(12.0));
  EXPECT_FALSE(handler.setTimeStamp(13.0));

  ALogClipHandler equal_window;
  EXPECT_TRUE(equal_window.setTimeStamp(2.0));
  EXPECT_TRUE(equal_window.setTimeStamp(2.0));
  EXPECT_FALSE(equal_window.setTimeStamp(2.0));
}

// Covers alogclip handler suffix normalization.
TEST(ALogClipHandlerTest, NormalizesBatchSuffix)
{
  TestableALogClipHandler handler;

  EXPECT_EQ(handler.addSuffixToALogFile("mission.alog"), "mission_clipped.alog");
  EXPECT_EQ(handler.addSuffixToALogFile("notes.txt"), "notes.txt");

  EXPECT_TRUE(handler.setSuffix("trim"));
  EXPECT_EQ(handler.addSuffixToALogFile("mission.alog"), "mission_trim.alog");

  EXPECT_TRUE(handler.setSuffix("_cut"));
  EXPECT_EQ(handler.addSuffixToALogFile("mission.alog"), "mission_cut.alog");
}

// Covers alogclip handler empty suffix behavior.
TEST(ALogClipHandlerTest, EmptyBatchSuffixUsesBareUnderscore)
{
  TestableALogClipHandler handler;

  EXPECT_TRUE(handler.setSuffix(""));
  EXPECT_EQ(handler.addSuffixToALogFile("mission.alog"), "mission_.alog");
}

// Covers alogclip handler suffix validation: whitespace is rejected without
// changing the prior suffix.
TEST(ALogClipHandlerTest, RejectsWhitespaceBatchSuffix)
{
  TestableALogClipHandler handler;
  ASSERT_TRUE(handler.setSuffix(""));

  EXPECT_FALSE(handler.setSuffix("bad suffix"));
  EXPECT_EQ(handler.addSuffixToALogFile("mission.alog"), "mission_.alog");

  EXPECT_FALSE(handler.setSuffix("bad\tsuffix"));
  EXPECT_EQ(handler.addSuffixToALogFile("mission.alog"), "mission_.alog");
}

// Covers alogclip handler file selection rules in non-batch mode.
TEST(ALogClipHandlerTest, NonBatchAcceptsOnlyAlogInputAndOptionalOutput)
{
  ALogClipHandler handler;

  EXPECT_FALSE(handler.addALogFile("input.txt"));
  EXPECT_FALSE(handler.addALogFile("input.ALOG"));
  EXPECT_TRUE(handler.addALogFile("input.alog"));
  EXPECT_TRUE(handler.addALogFile("output.alog"));
  EXPECT_FALSE(handler.addALogFile("extra.alog"));
}

// Covers alogclip handler's current ignored-file branch for names containing
// the literal marker string checked by addALogFile.
TEST(ALogClipHandlerTest, IgnoresFilesContainingLiteralMSuffixMarker)
{
  TempDir temp("alogclip_literal_suffix");
  const std::string ignored_path =
      temp.writeFile("alpha_m_suffix.alog", "1.000 NAV_X pNodeReporter 1");

  TestableALogClipHandler handler;
  ASSERT_TRUE(handler.setTimeStamp(1.0));
  ASSERT_TRUE(handler.setTimeStamp(2.0));
  EXPECT_TRUE(handler.addALogFile(ignored_path));
  EXPECT_FALSE(handler.preCheck());
}

// Covers alogclip preflight validation for missing timestamps and inputs.
TEST(ALogClipHandlerTest, PreCheckRejectsIncompleteSingleFileJobs)
{
  TempDir temp("alogclip_precheck");

  TestableALogClipHandler missing_times;
  EXPECT_FALSE(missing_times.preCheck());

  TestableALogClipHandler missing_input;
  ASSERT_TRUE(missing_input.setTimeStamp(1.0));
  ASSERT_TRUE(missing_input.setTimeStamp(2.0));
  EXPECT_FALSE(missing_input.preCheck());

  TestableALogClipHandler missing_file;
  ASSERT_TRUE(missing_file.setTimeStamp(1.0));
  ASSERT_TRUE(missing_file.setTimeStamp(2.0));
  ASSERT_TRUE(missing_file.addALogFile(temp.filePath("missing.alog")));
  EXPECT_FALSE(missing_file.preCheck());
}

// Covers alogclip preflight validation for missing batch files.
TEST(ALogClipHandlerTest, PreCheckRejectsIncompleteBatchJobs)
{
  TempDir temp("alogclip_precheck_batch");

  TestableALogClipHandler empty_batch;
  empty_batch.setBatch();
  ASSERT_TRUE(empty_batch.setTimeStamp(1.0));
  ASSERT_TRUE(empty_batch.setTimeStamp(2.0));
  EXPECT_FALSE(empty_batch.preCheck());

  TestableALogClipHandler missing_batch_file;
  missing_batch_file.setBatch();
  ASSERT_TRUE(missing_batch_file.setTimeStamp(1.0));
  ASSERT_TRUE(missing_batch_file.setTimeStamp(2.0));
  ASSERT_TRUE(missing_batch_file.addALogFile(temp.filePath("missing_batch.alog")));
  EXPECT_FALSE(missing_batch_file.preCheck());
}

// Covers alogclip handler processing: single-file mode clips to the explicit
// output file and overwrites it in force mode.
TEST(ALogClipHandlerTest, ProcessesSingleFileToExplicitOutput)
{
  TempDir temp("alogclip_single_process");
  const std::string keep = "2.000 NAV_X pNodeReporter 10";
  const std::string drop = "4.000 NAV_Y pNodeReporter 20";
  const std::string input_path =
      temp.writeFile("input.alog", JoinLines({keep, drop}));
  const std::string output_path = temp.writeFile("output.alog", "stale\n");

  ALogClipHandler handler;
  handler.setForceOverwrite();
  ASSERT_TRUE(handler.setTimeStamp(1.0));
  ASSERT_TRUE(handler.setTimeStamp(3.0));
  ASSERT_TRUE(handler.addALogFile(input_path));
  ASSERT_TRUE(handler.addALogFile(output_path));

  ASSERT_TRUE(handler.process());

  EXPECT_EQ(ReadFile(output_path), WithNewlines({keep}));
}

// Covers alogclip documented stdout mode: omitting an output file clips the
// input and writes kept lines to stdout.
TEST(ALogClipHandlerTest, SingleInputWithoutOutputWritesToStdout)
{
  TempDir temp("alogclip_stdout_process");
  const std::string keep = "2.000 NAV_X pNodeReporter 10";
  const std::string drop = "4.000 NAV_Y pNodeReporter 20";
  const std::string input_path =
      temp.writeFile("input.alog", JoinLines({keep, drop}));

  ALogClipHandler handler;
  ASSERT_TRUE(handler.setTimeStamp(1.0));
  ASSERT_TRUE(handler.setTimeStamp(3.0));
  ASSERT_TRUE(handler.addALogFile(input_path));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(handler.process());
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output, WithNewlines({keep}));
}

// Covers alogclip batch mode: each input file gets a suffixed output using the
// same clipping window.
TEST(ALogClipHandlerTest, ProcessesBatchFilesToSuffixedOutputs)
{
  TempDir temp("alogclip_batch_process");
  const std::string keep_a = "1.500 NAV_X pNodeReporter 10";
  const std::string drop_a = "0.500 NAV_X pNodeReporter 0";
  const std::string keep_b = "2.500 NAV_Y pNodeReporter 20";
  const std::string drop_b = "3.500 NAV_Y pNodeReporter 30";
  const std::string alpha_path =
      temp.writeFile("alpha.alog", JoinLines({drop_a, keep_a}));
  const std::string bravo_path =
      temp.writeFile("bravo.alog", JoinLines({keep_b, drop_b}));

  ALogClipHandler handler;
  handler.setBatch();
  handler.setForceOverwrite();
  ASSERT_TRUE(handler.setSuffix("trim"));
  ASSERT_TRUE(handler.setTimeStamp(1.0));
  ASSERT_TRUE(handler.setTimeStamp(3.0));
  ASSERT_TRUE(handler.addALogFile(alpha_path));
  ASSERT_TRUE(handler.addALogFile(bravo_path));

  ASSERT_TRUE(handler.process());

  EXPECT_EQ(ReadFile(temp.filePath("alpha_trim.alog")), WithNewlines({keep_a}));
  EXPECT_EQ(ReadFile(temp.filePath("bravo_trim.alog")), WithNewlines({keep_b}));
}

// Covers alogclip batch validation: processing stops during precheck when any
// batch input is unreadable, before outputs are created for earlier files.
TEST(ALogClipHandlerTest, BatchProcessRejectsMissingFileBeforeWritingOutputs)
{
  TempDir temp("alogclip_batch_missing");
  const std::string keep = "1.500 NAV_X pNodeReporter 10";
  const std::string alpha_path = temp.writeFile("alpha.alog", keep);
  const std::string missing_path = temp.filePath("missing.alog");

  ALogClipHandler handler;
  handler.setBatch();
  handler.setForceOverwrite();
  ASSERT_TRUE(handler.setSuffix("trim"));
  ASSERT_TRUE(handler.setTimeStamp(1.0));
  ASSERT_TRUE(handler.setTimeStamp(2.0));
  ASSERT_TRUE(handler.addALogFile(alpha_path));
  ASSERT_TRUE(handler.addALogFile(missing_path));

  EXPECT_FALSE(handler.process());
  EXPECT_FALSE(std::ifstream(temp.filePath("alpha_trim.alog")).is_open());
}
