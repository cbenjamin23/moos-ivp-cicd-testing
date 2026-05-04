#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "BHVFile.h"
#include "MOOSFile.h"
#include "PatchApplicator.h"
#include "Populator_BHVFile.h"
#include "Populator_MOOSFile.h"
#include "TestFileUtils.h"

namespace {

bool HasLine(const std::vector<std::string>& lines, const std::string& expected)
{
  for(const std::string& line : lines) {
    if(line == expected)
      return true;
  }
  return false;
}

unsigned int CountLine(const std::vector<std::string>& lines,
                       const std::string& expected)
{
  unsigned int count = 0;
  for(const std::string& line : lines) {
    if(line == expected)
      ++count;
  }
  return count;
}

std::string ReadFile(const std::string& path)
{
  std::ifstream in(path.c_str());
  EXPECT_TRUE(in.is_open());
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

MOOSFile ReadMoosFile(const std::string& path)
{
  Populator_MOOSFile populator;
  EXPECT_TRUE(populator.setFileMOOS(path));
  EXPECT_TRUE(populator.populate());
  return populator.getMOOSFile();
}

BHVFile ReadBhvFile(const std::string& path)
{
  Populator_BHVFile populator;
  EXPECT_TRUE(populator.setFileBHV(path));
  EXPECT_TRUE(populator.populate());
  return populator.getBHVFile();
}

}  // namespace

// Covers nspatch MOOS line patches: patch lines replace existing parameters
// while preserving indentation and append new parameters to the target block.
TEST(NspatchMoosFileTest, PatchLinesReplaceAndAppendProcessConfigValues)
{
  TempDir temp("nspatch_moos_lines");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ServerHost = localhost\n"
      "ServerPort = 9000\n"
      "\n"
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "  CommsTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ServerPort = 9200\n"
      "pFoo::AppTick = 8\n"
      "pFoo::NewParam = enabled\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "ServerPort = 9200"));
  EXPECT_TRUE(HasLine(lines, "  AppTick = 8"));
  EXPECT_TRUE(HasLine(lines, "  CommsTick = 4"));
  EXPECT_TRUE(HasLine(lines, "NewParam = enabled"));
  EXPECT_FALSE(HasLine(lines, "ServerPort = 9000"));
  EXPECT_FALSE(HasLine(lines, "  AppTick = 4"));
}

// Covers nspatch MOOS global patches: ordinary global key/value lines replace
// matching global stem lines and append missing global lines.
TEST(NspatchMoosFileTest, GlobalPatchLinesReplaceAndAppendRootValues)
{
  TempDir temp("nspatch_moos_global");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ServerHost = localhost\n"
      "ServerPort = 9000\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ServerPort = 9100\n"
      "Community = shoreside\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "ServerHost = localhost"));
  EXPECT_TRUE(HasLine(lines, "ServerPort = 9100"));
  EXPECT_TRUE(HasLine(lines, "Community = shoreside"));
  EXPECT_FALSE(HasLine(lines, "ServerPort = 9000"));
}

// Covers nspatch MOOS repeated-key behavior: line patches replace every line
// with the same left-hand key, which is why repeated keys need full-block
// patches when the caller intends to preserve multiple values.
TEST(NspatchMoosFileTest, LinePatchReplacesEveryRepeatedProcessConfigKey)
{
  TempDir temp("nspatch_moos_repeated_key");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pEval\n"
      "{\n"
      "  pass_flag = RESULT=alpha\n"
      "  pass_flag = RESULT=bravo\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "pEval::pass_flag = RESULT=patched\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_EQ(CountLine(lines, "  pass_flag = RESULT=patched"), 2u);
  EXPECT_FALSE(HasLine(lines, "  pass_flag = RESULT=alpha"));
  EXPECT_FALSE(HasLine(lines, "  pass_flag = RESULT=bravo"));
}

// Covers nspatch MOOS missing-block line patches: a patch-line alone updates
// only blocks already present in the stem and does not create a visible block.
TEST(NspatchMoosFileTest, LinePatchForMissingProcessConfigDoesNotCreateBlock)
{
  TempDir temp("nspatch_moos_missing_line_target");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ServerHost = localhost\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "pNew::AppTick = 4\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_FALSE(HasLine(lines, "ProcessConfig = pNew"));
  EXPECT_FALSE(HasLine(lines, "AppTick = 4"));
}

// Covers nspatch MOOS repeated-key safety: full-block patches preserve repeated
// keys exactly, unlike line patches that replace all matching keys.
TEST(NspatchMoosFileTest, FullBlockPatchPreservesRepeatedProcessConfigKeys)
{
  TempDir temp("nspatch_moos_repeated_full_block");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pEval\n"
      "{\n"
      "  pass_flag = RESULT=old\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ProcessConfig = pEval\n"
      "{\n"
      "  pass_flag = RESULT=alpha\n"
      "  pass_flag = RESULT=bravo\n"
      "}\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  pass_flag = RESULT=alpha"));
  EXPECT_TRUE(HasLine(lines, "  pass_flag = RESULT=bravo"));
  EXPECT_FALSE(HasLine(lines, "  pass_flag = RESULT=old"));
}

// Covers nspatch MOOS patch ordering: line patches are applied after full-block
// patches even when the line patch appears earlier in the patch input.
TEST(NspatchMoosFileTest, LinePatchWinsOverFullBlockRegardlessInputOrder)
{
  TempDir temp("nspatch_moos_line_block_order");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "  CommsTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "pFoo::AppTick = 12\n"
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 8\n"
      "  CommsTick = 8\n"
      "}\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  AppTick = 12"));
  EXPECT_TRUE(HasLine(lines, "  CommsTick = 8"));
  EXPECT_FALSE(HasLine(lines, "  AppTick = 8"));
}

// Covers nspatch MOOS parser tolerance: ProcessConfig keywords are parsed
// case-insensitively and emitted in canonical form.
TEST(NspatchMoosFileTest, MixedCaseProcessConfigKeywordIsParsed)
{
  TempDir temp("nspatch_moos_case");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "pRoCeSsCoNfIg = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");

  const std::vector<std::string> lines = ReadMoosFile(stem_path).getLines();

  EXPECT_TRUE(HasLine(lines, "ProcessConfig = pFoo"));
  EXPECT_TRUE(HasLine(lines, "  AppTick = 4"));
}

// Covers nspatch MOOS parser strictness: whitespace before the :: patch
// qualifier becomes part of the app key, so the line does not target pFoo.
TEST(NspatchMoosFileTest, WhitespaceBeforePatchQualifierDoesNotMatchBlock)
{
  TempDir temp("nspatch_moos_patch_whitespace");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "pFoo ::AppTick = 8\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  AppTick = 4"));
  EXPECT_FALSE(HasLine(lines, "AppTick = 8"));
  EXPECT_FALSE(HasLine(lines, "  AppTick = 8"));
}

// Covers nspatch MOOS patch key matching: app names in patch qualifiers are
// case-sensitive.
TEST(NspatchMoosFileTest, PatchQualifierAppNameIsCaseSensitive)
{
  TempDir temp("nspatch_moos_app_case");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "pfoo::AppTick = 8\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  AppTick = 4"));
  EXPECT_FALSE(HasLine(lines, "  AppTick = 8"));
}

// Covers nspatch MOOS permissive parsing: an unterminated ProcessConfig block is
// still represented as a complete emitted block.
TEST(NspatchMoosFileTest, UnterminatedProcessConfigBlockIsEmittedWithClosingBrace)
{
  TempDir temp("nspatch_moos_unclosed");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n");

  const std::vector<std::string> lines = ReadMoosFile(stem_path).getLines();

  EXPECT_TRUE(HasLine(lines, "ProcessConfig = pFoo"));
  EXPECT_TRUE(HasLine(lines, "  AppTick = 4"));
  EXPECT_TRUE(HasLine(lines, "}"));
}

// Covers nspatch MOOS ANTLER patches: launch lines match by the app name before
// the console options, not by the entire Run line.
TEST(NspatchMoosFileTest, AntlerRunPatchMatchesAppNameBeforeConsoleOptions)
{
  TempDir temp("nspatch_moos_antler");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = ANTLER\n"
      "{\n"
      "  Run = pFoo @ NewConsole = false\n"
      "  Run = pBar @ NewConsole = false\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ANTLER::Run = pFoo @ NewConsole = true\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  Run = pFoo @ NewConsole = true"));
  EXPECT_TRUE(HasLine(lines, "  Run = pBar @ NewConsole = false"));
  EXPECT_FALSE(HasLine(lines, "  Run = pFoo @ NewConsole = false"));
}

// Covers nspatch MOOS full-block patches: a ProcessConfig block in the patch
// replaces the matching stem block instead of merging individual keys.
TEST(NspatchMoosFileTest, ProcessConfigBlockPatchReplacesWholeBlock)
{
  TempDir temp("nspatch_moos_block");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "  CommsTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 10\n"
      "  Mode = patched\n"
      "}\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  AppTick = 10"));
  EXPECT_TRUE(HasLine(lines, "  Mode = patched"));
  EXPECT_FALSE(HasLine(lines, "  AppTick = 4"));
  EXPECT_FALSE(HasLine(lines, "  CommsTick = 4"));
}

// Covers nspatch MOOS block insertion: a ProcessConfig block absent from the
// stem is appended as a new config block in the target.
TEST(NspatchMoosFileTest, ProcessConfigPatchAddsMissingBlock)
{
  TempDir temp("nspatch_moos_new_block");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ServerHost = localhost\n"
      "ServerPort = 9000\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ProcessConfig = pNew\n"
      "{\n"
      "  AppTick = 2\n"
      "  mode = fresh\n"
      "}\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "// pNew Config Block"));
  EXPECT_TRUE(HasLine(lines, "ProcessConfig = pNew"));
  EXPECT_TRUE(HasLine(lines, "  AppTick = 2"));
  EXPECT_TRUE(HasLine(lines, "  mode = fresh"));
}

// Covers nspatch MOOS full-block preservation: comments and blank lines inside
// a replacement ProcessConfig block are preserved.
TEST(NspatchMoosFileTest, ProcessConfigBlockPatchPreservesCommentsAndBlankLines)
{
  TempDir temp("nspatch_moos_block_format");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  // timing\n"
      "\n"
      "  AppTick = 8\n"
      "}\n");

  MOOSFile target = ReadMoosFile(patch_path).applyToStemFile(ReadMoosFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  // timing"));
  EXPECT_TRUE(HasLine(lines, ""));
  EXPECT_TRUE(HasLine(lines, "  AppTick = 8"));
}

// Covers nspatch BHV named behavior line patches. Behavior blocks are keyed by
// type and name, so patches can target one behavior without replacing all
// blocks of the same behavior type.
TEST(NspatchBhvFileTest, NamedBehaviorPatchReplacesOnlyTargetBehavior)
{
  TempDir temp("nspatch_bhv_named");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  pwt = 50\n"
      "  speed = 2.0\n"
      "}\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = loiter\n"
      "  pwt = 25\n"
      "  speed = 1.0\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Waypoint@transit::speed = 3.5\n"
      "BHV_Waypoint@transit::condition = RETURN = false\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  name = transit"));
  EXPECT_TRUE(HasLine(lines, "  speed = 3.5"));
  EXPECT_TRUE(HasLine(lines, "condition = RETURN = false"));
  EXPECT_TRUE(HasLine(lines, "  name = loiter"));
  EXPECT_TRUE(HasLine(lines, "  speed = 1.0"));
  EXPECT_FALSE(HasLine(lines, "  speed = 2.0"));
}

// Covers nspatch BHV key selection: an unqualified behavior-type patch does not
// affect behavior blocks that have been keyed by their configured name.
TEST(NspatchBhvFileTest, UnqualifiedBehaviorPatchDoesNotTargetNamedBehavior)
{
  TempDir temp("nspatch_bhv_unqualified_named");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  speed = 2.0\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Waypoint::speed = 3.5\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  name = transit"));
  EXPECT_TRUE(HasLine(lines, "  speed = 2.0"));
  EXPECT_FALSE(HasLine(lines, "  speed = 3.5"));
}

// Covers nspatch BHV nameless behavior patches: behavior blocks without a name
// remain keyed by behavior type and can be patched with that unqualified key.
TEST(NspatchBhvFileTest, UnqualifiedBehaviorPatchTargetsNamelessBehavior)
{
  TempDir temp("nspatch_bhv_unqualified_nameless");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer::duration = 20\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  duration = 20"));
  EXPECT_FALSE(HasLine(lines, "  duration = 10"));
}

// Covers nspatch BHV repeated-key behavior: line patches replace every matching
// left-hand key in the target behavior block.
TEST(NspatchBhvFileTest, LinePatchReplacesEveryRepeatedBehaviorKey)
{
  TempDir temp("nspatch_bhv_repeated_key");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  condition = DEPLOY = true\n"
      "  condition = RETURN = false\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Waypoint@transit::condition = MODE = SURVEY\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_EQ(CountLine(lines, "  condition = MODE = SURVEY"), 2u);
  EXPECT_FALSE(HasLine(lines, "  condition = DEPLOY = true"));
  EXPECT_FALSE(HasLine(lines, "  condition = RETURN = false"));
}

// Covers nspatch BHV repeated-key safety: full-block patches preserve repeated
// keys exactly, unlike line patches that replace all matching keys.
TEST(NspatchBhvFileTest, FullBlockPatchPreservesRepeatedBehaviorKeys)
{
  TempDir temp("nspatch_bhv_repeated_full_block");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  condition = OLD = true\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  condition = DEPLOY = true\n"
      "  condition = RETURN = false\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  condition = DEPLOY = true"));
  EXPECT_TRUE(HasLine(lines, "  condition = RETURN = false"));
  EXPECT_FALSE(HasLine(lines, "  condition = OLD = true"));
}

// Covers nspatch BHV patch ordering: line patches are applied after full-block
// patches even when the line patch appears earlier in the patch input.
TEST(NspatchBhvFileTest, LinePatchWinsOverFullBlockRegardlessInputOrder)
{
  TempDir temp("nspatch_bhv_line_block_order");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer@timer::duration = 30\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 20\n"
      "  condition = DEPLOY = true\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  duration = 30"));
  EXPECT_TRUE(HasLine(lines, "  condition = DEPLOY = true"));
  EXPECT_FALSE(HasLine(lines, "  duration = 20"));
}

// Covers nspatch BHV duplicate names: duplicate behavior names share one key
// and collapse to the last parsed block.
TEST(NspatchBhvFileTest, DuplicateBehaviorNamesCollapseToLastParsedBlock)
{
  TempDir temp("nspatch_bhv_duplicate_names");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 20\n"
      "}\n");

  const std::vector<std::string> lines = ReadBhvFile(stem_path).getLines();

  EXPECT_EQ(CountLine(lines, "Behavior = BHV_Timer"), 1u);
  EXPECT_EQ(CountLine(lines, "  duration = 20"), 1u);
  EXPECT_FALSE(HasLine(lines, "  duration = 10"));
}

// Covers nspatch BHV key selection: a named patch does not target a behavior
// block that never provided a name.
TEST(NspatchBhvFileTest, NamedPatchDoesNotTargetNamelessBehavior)
{
  TempDir temp("nspatch_bhv_named_patch_nameless");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer@timer::duration = 20\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  duration = 10"));
  EXPECT_FALSE(HasLine(lines, "  duration = 20"));
}

// Covers nspatch BHV behavior keys: configured behavior names are normalized to
// lowercase for the internal patch key.
TEST(NspatchBhvFileTest, BehaviorNameKeyIsLowercasedFromConfig)
{
  TempDir temp("nspatch_bhv_name_lowercase");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = Transit\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer@transit::duration = 20\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  name = Transit"));
  EXPECT_TRUE(HasLine(lines, "  duration = 20"));
  EXPECT_FALSE(HasLine(lines, "  duration = 10"));
}

// Covers nspatch BHV patch key matching: patch qualifiers are case-sensitive
// after behavior names have been normalized to lowercase.
TEST(NspatchBhvFileTest, MixedCaseBehaviorPatchQualifierDoesNotMatchLowercaseKey)
{
  TempDir temp("nspatch_bhv_patch_case");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = Transit\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer@Transit::duration = 20\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  name = Transit"));
  EXPECT_TRUE(HasLine(lines, "  duration = 10"));
  EXPECT_FALSE(HasLine(lines, "  duration = 20"));
}

// Covers nspatch BHV parser tolerance: Behavior keywords are parsed
// case-insensitively and emitted in canonical form.
TEST(NspatchBhvFileTest, MixedCaseBehaviorKeywordIsParsed)
{
  TempDir temp("nspatch_bhv_case");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "bEhAvIoR = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");

  const std::vector<std::string> lines = ReadBhvFile(stem_path).getLines();

  EXPECT_TRUE(HasLine(lines, "Behavior = BHV_Timer"));
  EXPECT_TRUE(HasLine(lines, "  name = timer"));
  EXPECT_TRUE(HasLine(lines, "  duration = 10"));
}

// Covers nspatch BHV permissive parsing: an unterminated behavior block is still
// represented as a complete emitted block.
TEST(NspatchBhvFileTest, UnterminatedBehaviorBlockIsEmittedWithClosingBrace)
{
  TempDir temp("nspatch_bhv_unclosed");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n");

  const std::vector<std::string> lines = ReadBhvFile(stem_path).getLines();

  EXPECT_TRUE(HasLine(lines, "Behavior = BHV_Timer"));
  EXPECT_TRUE(HasLine(lines, "  duration = 10"));
  EXPECT_TRUE(HasLine(lines, "}"));
}

// Covers nspatch BHV missing behavior insertion: a full behavior block absent
// from the stem is appended as a new behavior block.
TEST(NspatchBhvFileTest, BehaviorBlockPatchAddsMissingNamedBehavior)
{
  TempDir temp("nspatch_bhv_new_block");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = watchdog\n"
      "  duration = 30\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "Behavior = BHV_Timer"));
  EXPECT_TRUE(HasLine(lines, "  name = watchdog"));
  EXPECT_TRUE(HasLine(lines, "  duration = 30"));
}

// Covers nspatch BHV full-block preservation: comments and blank lines inside a
// replacement behavior block are preserved.
TEST(NspatchBhvFileTest, BehaviorBlockPatchPreservesCommentsAndBlankLines)
{
  TempDir temp("nspatch_bhv_block_format");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  // timing\n"
      "\n"
      "  duration = 20\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  // timing"));
  EXPECT_TRUE(HasLine(lines, ""));
  EXPECT_TRUE(HasLine(lines, "  duration = 20"));
}

// Covers nspatch BHV initialize patches: existing initialize values are
// replaced by MOOS variable name and missing initialize lines are appended.
TEST(NspatchBhvFileTest, InitializePatchReplacesExistingAndAddsMissingVariables)
{
  TempDir temp("nspatch_bhv_init");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "initialize DEPLOY=false\n"
      "initialize RETURN=false\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "initialize DEPLOY=true\n"
      "initialize READY=true\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "initialize DEPLOY=true"));
  EXPECT_TRUE(HasLine(lines, "initialize READY=true"));
  EXPECT_TRUE(HasLine(lines, "initialize RETURN=false"));
  EXPECT_FALSE(HasLine(lines, "initialize DEPLOY=false"));
}

// Covers nspatch BHV mode patches: mode blocks in the patch replace the stem
// mode block as a unit.
TEST(NspatchBhvFileTest, ModePatchReplacesWholeModeBlock)
{
  TempDir temp("nspatch_bhv_modes");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "set MODE = ACTIVE {\n"
      "  DEPLOY = true\n"
      "}\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "set MODE = SURVEY {\n"
      "  DEPLOY = true\n"
      "  RETURN = false\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "set MODE = SURVEY {"));
  EXPECT_TRUE(HasLine(lines, "  RETURN = false"));
  EXPECT_FALSE(HasLine(lines, "set MODE = ACTIVE {"));
}

// Covers nspatch BHV mode parsing with multiple mode blocks: a patch mode block
// replaces all stem mode lines, including additional set blocks.
TEST(NspatchBhvFileTest, ModePatchReplacesMultipleStemModeBlocks)
{
  TempDir temp("nspatch_bhv_multi_modes");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "set MODE = ACTIVE {\n"
      "  DEPLOY = true\n"
      "}\n"
      "set SUBMODE = FAST {\n"
      "  SPEED_OK = true\n"
      "}\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "set MODE = SURVEY {\n"
      "  DEPLOY = true\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "set MODE = SURVEY {"));
  EXPECT_FALSE(HasLine(lines, "set MODE = ACTIVE {"));
  EXPECT_FALSE(HasLine(lines, "set SUBMODE = FAST {"));
  EXPECT_FALSE(HasLine(lines, "  SPEED_OK = true"));
}

// Covers nspatch BHV full-block patches: a named behavior block in the patch
// replaces only the matching named behavior block in the stem.
TEST(NspatchBhvFileTest, NamedBehaviorBlockPatchReplacesWholeTargetBlock)
{
  TempDir temp("nspatch_bhv_block");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  pwt = 50\n"
      "  speed = 2.0\n"
      "}\n"
      "\n"
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = loiter\n"
      "  pwt = 25\n"
      "  speed = 1.0\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "Behavior = BHV_Waypoint\n"
      "{\n"
      "  name = transit\n"
      "  condition = RETURN = false\n"
      "  speed = 4.0\n"
      "}\n");

  BHVFile target = ReadBhvFile(patch_path).applyToStemFile(ReadBhvFile(stem_path));
  const std::vector<std::string> lines = target.getLines();

  EXPECT_TRUE(HasLine(lines, "  name = transit"));
  EXPECT_TRUE(HasLine(lines, "  condition = RETURN = false"));
  EXPECT_TRUE(HasLine(lines, "  speed = 4.0"));
  EXPECT_TRUE(HasLine(lines, "  name = loiter"));
  EXPECT_TRUE(HasLine(lines, "  pwt = 25"));
  EXPECT_FALSE(HasLine(lines, "  pwt = 50"));
  EXPECT_FALSE(HasLine(lines, "  speed = 2.0"));
}

// Covers nspatch populators: empty files populate successfully and emit one
// blank line through the current file-buffer path.
TEST(NspatchPopulatorTest, EmptyFilesPopulateAsEmptyPatchObjects)
{
  TempDir temp("nspatch_empty_files");
  const std::string empty_moos_path = temp.writeFile("empty.xmoos", "");
  const std::string empty_bhv_path = temp.writeFile("empty.xbhv", "");

  Populator_MOOSFile moos_populator;
  ASSERT_TRUE(moos_populator.setFileMOOS(empty_moos_path));
  EXPECT_TRUE(moos_populator.populate());
  EXPECT_EQ(moos_populator.getMOOSFile().getLines().size(), 1u);
  EXPECT_TRUE(HasLine(moos_populator.getMOOSFile().getLines(), ""));

  Populator_BHVFile bhv_populator;
  ASSERT_TRUE(bhv_populator.setFileBHV(empty_bhv_path));
  EXPECT_TRUE(bhv_populator.populate());
  EXPECT_EQ(bhv_populator.getBHVFile().getLines().size(), 1u);
  EXPECT_TRUE(HasLine(bhv_populator.getBHVFile().getLines(), ""));
}

// Covers nspatch MOOS populator state management: setFileMOOS clears files
// previously queued with addFileMOOS.
TEST(NspatchPopulatorTest, SetFileMoosClearsPreviouslyQueuedFiles)
{
  TempDir temp("nspatch_setfile_moos");
  const std::string first_path = temp.writeFile(
      "first.xmoos",
      "ServerPort = 9000\n");
  const std::string second_path = temp.writeFile(
      "second.xmoos",
      "Community = alpha\n");

  Populator_MOOSFile populator;
  ASSERT_TRUE(populator.addFileMOOS(first_path));
  ASSERT_TRUE(populator.setFileMOOS(second_path));
  ASSERT_TRUE(populator.populate());
  const std::vector<std::string> lines = populator.getMOOSFile().getLines();

  EXPECT_TRUE(HasLine(lines, "Community = alpha"));
  EXPECT_FALSE(HasLine(lines, "ServerPort = 9000"));
}

// Covers nspatch BHV populator state management: setFileBHV clears files
// previously queued with addFileBHV.
TEST(NspatchPopulatorTest, SetFileBhvClearsPreviouslyQueuedFiles)
{
  TempDir temp("nspatch_setfile_bhv");
  const std::string first_path = temp.writeFile(
      "first.xbhv",
      "initialize DEPLOY=true\n");
  const std::string second_path = temp.writeFile(
      "second.xbhv",
      "initialize RETURN=true\n");

  Populator_BHVFile populator;
  ASSERT_TRUE(populator.addFileBHV(first_path));
  ASSERT_TRUE(populator.setFileBHV(second_path));
  ASSERT_TRUE(populator.populate());
  const std::vector<std::string> lines = populator.getBHVFile().getLines();

  EXPECT_TRUE(HasLine(lines, "initialize RETURN=true"));
  EXPECT_FALSE(HasLine(lines, "initialize DEPLOY=true"));
}

// Covers PatchApplicator end-to-end MOOS patching: multiple xmoos files are
// applied in caller order and the final target file is written to disk.
TEST(NspatchPatchApplicatorTest, AppliesMultipleMoosPatchFilesInCallerOrder)
{
  TempDir temp("nspatch_applicator_moos");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");
  const std::string first_patch_path = temp.writeFile(
      "first.xmoos",
      "pFoo::AppTick = 8\n");
  const std::string second_patch_path = temp.writeFile(
      "second.xmoos",
      "pFoo::AppTick = 12\n");
  const std::string target_path = temp.filePath("target.moos");

  PatchApplicator applicator;
  ASSERT_TRUE(applicator.setStemMoosFile(stem_path));
  ASSERT_TRUE(applicator.addXMoosFile(first_patch_path));
  ASSERT_TRUE(applicator.addXMoosFile(second_patch_path));
  ASSERT_TRUE(applicator.setTargMoosFile(target_path));
  EXPECT_TRUE(applicator.applyPatch());
  // PatchApplicator owns C FILE buffers; flush before reading in-process.
  std::fflush(nullptr);

  const std::string target_text = ReadFile(target_path);
  EXPECT_NE(target_text.find("AppTick = 12"), std::string::npos) << target_text;
  EXPECT_EQ(target_text.find("AppTick = 8"), std::string::npos) << target_text;
  EXPECT_EQ(target_text.find("AppTick = 4"), std::string::npos) << target_text;
}

// Covers PatchApplicator MOOS target suffix handling: .moosx targets are
// accepted and written by the lower-level applicator.
TEST(NspatchPatchApplicatorTest, AppliesMoosPatchAndWritesMoosxTarget)
{
  TempDir temp("nspatch_applicator_moosx");
  const std::string stem_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xmoos",
      "pFoo::AppTick = 9\n");
  const std::string target_path = temp.filePath("target.moosx");

  PatchApplicator applicator;
  ASSERT_TRUE(applicator.setStemMoosFile(stem_path));
  ASSERT_TRUE(applicator.addXMoosFile(patch_path));
  ASSERT_TRUE(applicator.setTargMoosFile(target_path));
  EXPECT_TRUE(applicator.applyPatch());
  // PatchApplicator owns C FILE buffers; flush before reading in-process.
  std::fflush(nullptr);

  const std::string target_text = ReadFile(target_path);
  EXPECT_NE(target_text.find("AppTick = 9"), std::string::npos) << target_text;
  EXPECT_EQ(target_text.find("AppTick = 4"), std::string::npos) << target_text;
}

// Covers PatchApplicator end-to-end BHV patching and target suffix handling:
// .bhvx targets are accepted and written.
TEST(NspatchPatchApplicatorTest, AppliesBhvPatchAndWritesBhvxTarget)
{
  TempDir temp("nspatch_applicator_bhv");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer@timer::duration = 25\n");
  const std::string target_path = temp.filePath("target.bhvx");

  PatchApplicator applicator;
  ASSERT_TRUE(applicator.setStemBhvFile(stem_path));
  ASSERT_TRUE(applicator.addXBhvFile(patch_path));
  ASSERT_TRUE(applicator.setTargBhvFile(target_path));
  EXPECT_TRUE(applicator.applyPatch());
  // PatchApplicator owns C FILE buffers; flush before reading in-process.
  std::fflush(nullptr);

  const std::string target_text = ReadFile(target_path);
  EXPECT_NE(target_text.find("duration = 25"), std::string::npos) << target_text;
  EXPECT_EQ(target_text.find("duration = 10"), std::string::npos) << target_text;
}

// Covers PatchApplicator BHV patch ordering: multiple xbhv files are applied in
// caller order and later line patches win.
TEST(NspatchPatchApplicatorTest, AppliesMultipleBhvPatchFilesInCallerOrder)
{
  TempDir temp("nspatch_applicator_bhv_order");
  const std::string stem_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string first_patch_path = temp.writeFile(
      "first.xbhv",
      "BHV_Timer@timer::duration = 20\n");
  const std::string second_patch_path = temp.writeFile(
      "second.xbhv",
      "BHV_Timer@timer::duration = 30\n");
  const std::string target_path = temp.filePath("target.bhv");

  PatchApplicator applicator;
  ASSERT_TRUE(applicator.setStemBhvFile(stem_path));
  ASSERT_TRUE(applicator.addXBhvFile(first_patch_path));
  ASSERT_TRUE(applicator.addXBhvFile(second_patch_path));
  ASSERT_TRUE(applicator.setTargBhvFile(target_path));
  EXPECT_TRUE(applicator.applyPatch());
  // PatchApplicator owns C FILE buffers; flush before reading in-process.
  std::fflush(nullptr);

  const std::string target_text = ReadFile(target_path);
  EXPECT_NE(target_text.find("duration = 30"), std::string::npos) << target_text;
  EXPECT_EQ(target_text.find("duration = 20"), std::string::npos) << target_text;
  EXPECT_EQ(target_text.find("duration = 10"), std::string::npos) << target_text;
}

// Covers PatchApplicator combined jobs: a single applicator can apply complete
// MOOS and BHV patch jobs in one call.
TEST(NspatchPatchApplicatorTest, AppliesMoosAndBhvJobsInSingleCall)
{
  TempDir temp("nspatch_applicator_both");
  const std::string stem_moos_path = temp.writeFile(
      "stem.moos",
      "ProcessConfig = pFoo\n"
      "{\n"
      "  AppTick = 4\n"
      "}\n");
  const std::string patch_moos_path = temp.writeFile(
      "patch.xmoos",
      "pFoo::AppTick = 7\n");
  const std::string target_moos_path = temp.filePath("target.moos");
  const std::string stem_bhv_path = temp.writeFile(
      "stem.bhv",
      "// header\n"
      "\n"
      "\n"
      "Behavior = BHV_Timer\n"
      "{\n"
      "  name = timer\n"
      "  duration = 10\n"
      "}\n");
  const std::string patch_bhv_path = temp.writeFile(
      "patch.xbhv",
      "BHV_Timer@timer::duration = 14\n");
  const std::string target_bhv_path = temp.filePath("target.bhv");

  PatchApplicator applicator;
  ASSERT_TRUE(applicator.setStemMoosFile(stem_moos_path));
  ASSERT_TRUE(applicator.addXMoosFile(patch_moos_path));
  ASSERT_TRUE(applicator.setTargMoosFile(target_moos_path));
  ASSERT_TRUE(applicator.setStemBhvFile(stem_bhv_path));
  ASSERT_TRUE(applicator.addXBhvFile(patch_bhv_path));
  ASSERT_TRUE(applicator.setTargBhvFile(target_bhv_path));
  EXPECT_TRUE(applicator.applyPatch());
  // PatchApplicator owns C FILE buffers; flush before reading in-process.
  std::fflush(nullptr);

  const std::string moos_text = ReadFile(target_moos_path);
  const std::string bhv_text = ReadFile(target_bhv_path);
  EXPECT_NE(moos_text.find("AppTick = 7"), std::string::npos) << moos_text;
  EXPECT_NE(bhv_text.find("duration = 14"), std::string::npos) << bhv_text;
}

// Covers PatchApplicator validation: missing inputs and unwritable targets are
// rejected before applyPatch is attempted.
TEST(NspatchPatchApplicatorTest, RejectsMissingInputsAndUnwritableTargetPath)
{
  TempDir temp("nspatch_applicator_validation");
  const std::string existing_path = temp.writeFile("stem.moos", "ServerHost = localhost\n");
  const std::string missing_path = temp.filePath("missing.moos");
  const std::string bad_target_path = temp.filePath("missing_dir/target.moos");

  PatchApplicator applicator;
  EXPECT_FALSE(applicator.setStemMoosFile(missing_path));
  EXPECT_FALSE(applicator.addXMoosFile(missing_path));
  EXPECT_TRUE(applicator.setStemMoosFile(existing_path));
  EXPECT_FALSE(applicator.setTargMoosFile(bad_target_path));
}

// Covers PatchApplicator incomplete configuration handling: applyPatch fails
// cleanly when no complete MOOS or BHV patch job has been configured.
TEST(NspatchPatchApplicatorTest, ApplyPatchFailsWithoutCompletePatchJob)
{
  PatchApplicator applicator;

  EXPECT_FALSE(applicator.applyPatch());
}
