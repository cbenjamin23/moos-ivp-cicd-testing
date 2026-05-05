#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "ALogCatHandler.h"
#include "TestFileUtils.h"

namespace {

class TestableALogCatHandler : public ALogCatHandler {
 public:
  using ALogCatHandler::preCheck;
};

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

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

std::string WriteAlog(TempDir& temp,
                      const std::string& name,
                      double logstart,
                      const std::vector<std::string>& data_lines)
{
  std::vector<std::string> lines = {
      "%% LOGSTART " + std::to_string(logstart)};
  lines.insert(lines.end(), data_lines.begin(), data_lines.end());
  return temp.writeFile(name, WithNewlines(lines));
}

}  // namespace

// Source audit note: this suite covers ALogCatHandler's deterministic input
// validation, LOGSTART ordering, overlap checks, forced overwrite, and
// concatenation paths against app_alogcat. The thin CLI parser is intentionally
// left to executable smoke coverage.

// Covers input file list validation.
TEST(ALogCatHandlerTest, AddsOnlyUniqueWhitespaceFreeAlogFiles)
{
  ALogCatHandler handler;

  EXPECT_FALSE(handler.addALogFile("input.txt"));
  EXPECT_FALSE(handler.addALogFile("bad name.alog"));
  EXPECT_TRUE(handler.addALogFile("alpha.alog"));
  EXPECT_FALSE(handler.addALogFile("alpha.alog"));
  EXPECT_TRUE(handler.addALogFile("bravo.alog"));
  EXPECT_EQ(handler.size(), 2u);
}

// Covers precheck failures for incomplete jobs.
TEST(ALogCatHandlerTest, PreCheckRejectsMissingInputsAndOutput)
{
  TempDir temp("alogcat_precheck");
  const std::string alpha =
      WriteAlog(temp, "alpha.alog", 1000.0, {"1.000 NAV_X pNodeReporter 10"});

  TestableALogCatHandler too_few;
  ASSERT_TRUE(too_few.addALogFile(alpha));
  EXPECT_FALSE(too_few.preCheck());

  TestableALogCatHandler no_output;
  ASSERT_TRUE(no_output.addALogFile(alpha));
  ASSERT_TRUE(no_output.addALogFile(temp.filePath("missing.alog")));
  EXPECT_FALSE(no_output.preCheck());
}

// Covers missing output validation once input files are otherwise valid.
TEST(ALogCatHandlerTest, PreCheckRejectsMissingOutputWithValidInputs)
{
  TempDir temp("alogcat_no_output");
  const std::string alpha =
      WriteAlog(temp, "alpha.alog", 1000.0, {"1.000 NAV_X pNodeReporter 10"});
  const std::string bravo =
      WriteAlog(temp, "bravo.alog", 1010.0, {"1.000 NAV_Y pNodeReporter 20"});

  TestableALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(alpha));
  ASSERT_TRUE(handler.addALogFile(bravo));

  EXPECT_FALSE(handler.preCheck());
}

// Covers output writability validation after input files pass precheck.
TEST(ALogCatHandlerTest, PreCheckRejectsUnwritableOutputPath)
{
  TempDir temp("alogcat_bad_output");
  const std::string alpha =
      WriteAlog(temp, "alpha.alog", 1000.0, {"1.000 NAV_X pNodeReporter 10"});
  const std::string bravo =
      WriteAlog(temp, "bravo.alog", 1010.0, {"1.000 NAV_Y pNodeReporter 20"});

  TestableALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(alpha));
  ASSERT_TRUE(handler.addALogFile(bravo));
  handler.setNewALogFile(temp.filePath("missing/out.alog"));

  EXPECT_FALSE(handler.preCheck());
}

// Covers existing output rejection unless force is enabled.
TEST(ALogCatHandlerTest, PreCheckRequiresForceForExistingOutput)
{
  TempDir temp("alogcat_existing_output");
  const std::string alpha =
      WriteAlog(temp, "alpha.alog", 1000.0, {"1.000 NAV_X pNodeReporter 10"});
  const std::string bravo =
      WriteAlog(temp, "bravo.alog", 1010.0, {"1.000 NAV_Y pNodeReporter 20"});
  const std::string output = temp.writeFile("out.alog", "stale\n");

  TestableALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(alpha));
  ASSERT_TRUE(handler.addALogFile(bravo));
  handler.setNewALogFile(output);
  EXPECT_FALSE(handler.preCheck());

  TestableALogCatHandler forced;
  forced.setForceOverwrite();
  ASSERT_TRUE(forced.addALogFile(alpha));
  ASSERT_TRUE(forced.addALogFile(bravo));
  forced.setNewALogFile(output);
  EXPECT_TRUE(forced.preCheck());
}

// Covers overlap detection in UTC data windows.
TEST(ALogCatHandlerTest, PreCheckRejectsOverlappingAlogWindows)
{
  TempDir temp("alogcat_overlap");
  const std::string alpha =
      WriteAlog(temp, "alpha.alog", 1000.0,
                {"1.000 NAV_X pNodeReporter 10",
                 "4.000 NAV_X pNodeReporter 40"});
  const std::string bravo =
      WriteAlog(temp, "bravo.alog", 1000.5, {"1.000 NAV_Y pNodeReporter 20"});

  TestableALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(alpha));
  ASSERT_TRUE(handler.addALogFile(bravo));
  handler.setNewALogFile(temp.filePath("out.alog"));

  EXPECT_FALSE(handler.preCheck());
}

// Covers the strict overlap rule: touching UTC data windows are allowed.
TEST(ALogCatHandlerTest, PreCheckAllowsTouchingAlogWindows)
{
  TempDir temp("alogcat_touching_windows");
  const std::string alpha =
      WriteAlog(temp, "alpha.alog", 1000.0,
                {"1.000 NAV_X pNodeReporter 10",
                 "2.000 NAV_X pNodeReporter 20"});
  const std::string bravo =
      WriteAlog(temp, "bravo.alog", 1001.0, {"1.000 NAV_Y pNodeReporter 30"});

  TestableALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(alpha));
  ASSERT_TRUE(handler.addALogFile(bravo));
  handler.setNewALogFile(temp.filePath("out.alog"));

  EXPECT_TRUE(handler.preCheck());
}

// Covers successful concatenation and timestamp shifting by LOGSTART delta.
TEST(ALogCatHandlerTest, ProcessConcatenatesAndShiftsLaterLogs)
{
  TempDir temp("alogcat_process");
  const std::string alpha = WriteAlog(
      temp, "alpha.alog", 1000.0,
      {"1.000 NAV_X pNodeReporter 10",
       "2.000 NAV_Y pNodeReporter 20"});
  const std::string bravo = WriteAlog(
      temp, "bravo.alog", 1005.0,
      {"1.000 NAV_X pNodeReporter 30",
       "bad timestamp line",
       "-1.000 NAV_NEG pNodeReporter -1"});
  const std::string output = temp.filePath("out.alog");

  ALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(alpha));
  ASSERT_TRUE(handler.addALogFile(bravo));
  handler.setNewALogFile(output);

  ASSERT_TRUE(handler.process());
  fflush(nullptr);

  EXPECT_EQ(ReadFile(output),
            WithNewlines({"%% LOGSTART 1000.000000",
                          "1.000 NAV_X pNodeReporter 10",
                          "2.000 NAV_Y pNodeReporter 20",
                          "6.000 NAV_X pNodeReporter 30"}));
}

// Covers sorting by UTC data end time before concatenation.
TEST(ALogCatHandlerTest, ProcessSortsInputsBeforeConcatenating)
{
  TempDir temp("alogcat_process_sort");
  const std::string late =
      WriteAlog(temp, "late.alog", 1010.0, {"1.000 LATE pLogger 2"});
  const std::string early =
      WriteAlog(temp, "early.alog", 1000.0,
                {"1.000 EARLY pLogger 1",
                 "2.000 EARLY pLogger 1"});
  const std::string output = temp.filePath("out.alog");

  ALogCatHandler handler;
  ASSERT_TRUE(handler.addALogFile(late));
  ASSERT_TRUE(handler.addALogFile(early));
  handler.setNewALogFile(output);

  ASSERT_TRUE(handler.process());
  fflush(nullptr);
  const std::string contents = ReadFile(output);

  EXPECT_TRUE(Contains(contents, "%% LOGSTART 1000.000000\n"));
  EXPECT_TRUE(Contains(contents, "11.000 LATE pLogger 2\n"));
}
