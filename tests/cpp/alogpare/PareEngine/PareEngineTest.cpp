#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "PareEngine.h"
#include "TestFileUtils.h"

namespace {

class TestablePareEngine : public PareEngine {
 public:
  using PareEngine::passOneFindTimeStamps;
  using PareEngine::varOnHitList;
  using PareEngine::varOnMarkList;
  using PareEngine::varOnPareList;
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

}  // namespace

// Source audit note: this suite covers PareEngine's deterministic variable
// pattern matching, two-pass timestamp selection, stdout/file output, default
// lists, and report behavior against app_alogpare. Missing input exits by
// design and is not used as an ordinary component assertion.

// Covers output-file validation, including stdout mode via an empty path.
TEST(PareEngineTest, ValidatesOutputPathAndAcceptsStdoutMode)
{
  TempDir temp("alogpare_output_validation");
  PareEngine engine;

  EXPECT_TRUE(engine.setALogFileOut(""));
  EXPECT_TRUE(engine.setALogFileOut(temp.filePath("out.alog")));
  EXPECT_FALSE(engine.setALogFileOut(temp.filePath("missing/out.alog")));
}

// Covers exact, prefix, suffix, and contains wildcard list matching.
TEST(PareEngineTest, MatchesVariableListsWithAllWildcardForms)
{
  TestablePareEngine engine;
  ASSERT_TRUE(engine.addMarkListVars("MARK_EXACT,MARK_PREFIX*,*MARK_SUFFIX,*MID*"));
  ASSERT_TRUE(engine.addHitListVars("HIT_EXACT,HIT_PREFIX*,*HIT_SUFFIX,*DROP*"));
  ASSERT_TRUE(engine.addPareListVars("PARE_EXACT,PARE_PREFIX*,*PARE_SUFFIX,*VIEW*"));

  EXPECT_TRUE(engine.varOnMarkList("MARK_EXACT"));
  EXPECT_TRUE(engine.varOnMarkList("MARK_PREFIX_A"));
  EXPECT_TRUE(engine.varOnMarkList("A_MARK_SUFFIX"));
  EXPECT_TRUE(engine.varOnMarkList("A_MID_B"));
  EXPECT_FALSE(engine.varOnMarkList("MARKER"));

  EXPECT_TRUE(engine.varOnHitList("HIT_EXACT"));
  EXPECT_TRUE(engine.varOnHitList("HIT_PREFIX_A"));
  EXPECT_TRUE(engine.varOnHitList("A_HIT_SUFFIX"));
  EXPECT_TRUE(engine.varOnHitList("A_DROP_B"));
  EXPECT_FALSE(engine.varOnHitList("KEEP"));

  EXPECT_TRUE(engine.varOnPareList("PARE_EXACT"));
  EXPECT_TRUE(engine.varOnPareList("PARE_PREFIX_A"));
  EXPECT_TRUE(engine.varOnPareList("A_PARE_SUFFIX"));
  EXPECT_TRUE(engine.varOnPareList("VIEW_POINT"));
  EXPECT_FALSE(engine.varOnPareList("NAV_X"));
}

// Covers default hit and pare lists used by alogpare.
TEST(PareEngineTest, DefaultListsMatchDocumentedHighVolumeVariables)
{
  TestablePareEngine engine;
  engine.defaultHitList();
  engine.defaultPareList();

  EXPECT_TRUE(engine.varOnHitList("NAV_X_ITER_GAP"));
  EXPECT_TRUE(engine.varOnHitList("PSHARE_INPUT_SUMMARY"));
  EXPECT_TRUE(engine.varOnHitList("NODE_REPORT_LOCAL"));
  // The upstream default string has a leading space before DB_QOS after
  // comma-splitting, so the exact DB_QOS variable is currently not matched.
  EXPECT_FALSE(engine.varOnHitList("DB_QOS"));
  EXPECT_TRUE(engine.varOnHitList(" DB_QOS"));
  EXPECT_FALSE(engine.varOnHitList("NAV_X"));

  EXPECT_TRUE(engine.varOnPareList("BHV_IPF"));
  EXPECT_TRUE(engine.varOnPareList("VIEW_SEGLIST"));
  EXPECT_TRUE(engine.varOnPareList("VIEW_POINT"));
  EXPECT_TRUE(engine.varOnPareList("CONTACTS_RECAP"));
  EXPECT_FALSE(engine.varOnPareList("DESIRED_HEADING"));
}

// Covers pass one timestamp discovery and verbose report output.
TEST(PareEngineTest, PassOneCollectsMarkTimestampsAndCommunityName)
{
  TempDir temp("alogpare_pass_one");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "0.100 DB_TIME MOOSDB_alpha 123456",
                    "1.000 MARK pLogger true",
                    "2.000 NAV_X pNodeReporter 10",
                    "3.000 MARK pLogger true"}));

  TestablePareEngine engine;
  engine.setVerbose(true);
  ASSERT_TRUE(engine.setALogFileIn(input));
  ASSERT_TRUE(engine.addMarkListVars("MARK"));

  testing::internal::CaptureStdout();
  engine.passOneFindTimeStamps();
  engine.printReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "Total Pare Events: 2"));
  EXPECT_TRUE(Contains(output, "[0]: 1"));
  EXPECT_TRUE(Contains(output, "[1]: 3"));
}

// Covers two-pass file paring: comments and non-pare vars are kept, hit-list
// vars are dropped, pare-list vars are kept only near mark timestamps, and
// active=false view erasures are preserved.
TEST(PareEngineTest, PareTheFileKeepsOnlyRelevantWindowedLines)
{
  TempDir temp("alogpare_file");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "0.000 MARK pLogger true",
                    "0.500 VIEW_POINT pViewer label=near",
                    "1.000 DROP_ME pNoise value",
                    "2.000 NAV_X pNodeReporter 10",
                    "10.000 VIEW_POINT pViewer label=far",
                    "11.000 VIEW_POINT pViewer label=erase,active=false"}));
  const std::string output = temp.filePath("pared.alog");

  PareEngine engine;
  engine.setPareWindow(2.0);
  ASSERT_TRUE(engine.setALogFileIn(input));
  ASSERT_TRUE(engine.setALogFileOut(output));
  ASSERT_TRUE(engine.addMarkListVars("MARK"));
  ASSERT_TRUE(engine.addHitListVars("DROP_ME"));
  ASSERT_TRUE(engine.addPareListVars("VIEW_POINT"));

  testing::internal::CaptureStdout();
  engine.pareTheFile();
  const std::string report = testing::internal::GetCapturedStdout();
  const std::string contents = ReadFile(output);

  EXPECT_TRUE(Contains(report, "lines pared"));
  EXPECT_TRUE(Contains(contents, "%% LOGSTART"));
  EXPECT_TRUE(Contains(contents, "MARK"));
  EXPECT_TRUE(Contains(contents, "label=near"));
  EXPECT_TRUE(Contains(contents, "NAV_X"));
  EXPECT_TRUE(Contains(contents, "active=false"));
  EXPECT_FALSE(Contains(contents, "DROP_ME"));
  EXPECT_FALSE(Contains(contents, "label=far\n"));
}

// Covers stdout output mode when no output file is provided.
TEST(PareEngineTest, EmptyOutputPathWritesParedLogToStdout)
{
  TempDir temp("alogpare_stdout");
  const std::string input = temp.writeFile(
      "input.alog",
      WithNewlines({"0.000 MARK pLogger true",
                    "0.500 VIEW_POINT pViewer label=near"}));

  PareEngine engine;
  engine.setPareWindow(1.0);
  ASSERT_TRUE(engine.setALogFileIn(input));
  ASSERT_TRUE(engine.setALogFileOut(""));
  ASSERT_TRUE(engine.addMarkListVars("MARK"));
  ASSERT_TRUE(engine.addPareListVars("VIEW_POINT"));

  testing::internal::CaptureStdout();
  engine.pareTheFile();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "MARK"));
  EXPECT_TRUE(Contains(output, "VIEW_POINT"));
}
