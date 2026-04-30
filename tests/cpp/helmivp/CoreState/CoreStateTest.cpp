#include <gtest/gtest.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "BehaviorReport.h"
#include "BehaviorSpec.h"
#include "HelmReport.h"
#include "InfoBuffer.h"
#include "IvPBox.h"
#include "IvPDomain.h"
#include "IvPFunction.h"
#include "ModeEntry.h"
#include "ModeSet.h"
#include "PDMap.h"

namespace {

std::unique_ptr<IvPFunction> makeOnePieceFunction(const std::string& var,
                                                  double pwt = 1.0)
{
  IvPDomain domain;
  domain.addDomain(var, 0, 4, 5);
  std::unique_ptr<PDMap> pdmap(new PDMap(1, domain, 1));
  pdmap->bx(0) = new IvPBox(1, 1);
  pdmap->bx(0)->setPTS(0, 0, 4);
  pdmap->bx(0)->wt(0) = 1;
  pdmap->bx(0)->wt(1) = 0;
  std::unique_ptr<IvPFunction> ipf(new IvPFunction(pdmap.release()));
  ipf->setPWT(pwt);
  return ipf;
}

bool contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Covers mode entry behavior: parses mode head conditions else values and mode prefixes.
TEST(ModeEntryTest, ParsesModeHeadConditionsElseValuesAndModePrefixes)
{
  ModeEntry entry;
  EXPECT_FALSE(entry.setHead("BAD MODE", "SURVEY"));
  ASSERT_TRUE(entry.setHead("MODE", "SURVEY"));
  EXPECT_TRUE(entry.setElseValue("IDLE"));
  EXPECT_FALSE(entry.setElseValue("BAD ELSE"));
  ASSERT_TRUE(entry.addCondition("MODE = TRANSIT"));
  ASSERT_TRUE(entry.addCondition("NAV_X > 10"));
  EXPECT_FALSE(entry.addCondition("NAV_X >"));

  EXPECT_EQ(entry.getModeVarName(), "MODE");
  EXPECT_EQ(entry.getModeVarValue(), "SURVEY");
  EXPECT_EQ(entry.getModeVarElseValue(), "IDLE");
  EXPECT_EQ(entry.getModePrefix(), "TRANSIT");
  EXPECT_EQ(entry.getModeParent(), "TRANSIT");

  std::vector<std::string> vars = entry.getConditionVars();
  EXPECT_NE(std::find(vars.begin(), vars.end(), "MODE"), vars.end());
  EXPECT_NE(std::find(vars.begin(), vars.end(), "NAV_X"), vars.end());

  entry.setVarVal("MODE", "TRANSIT");
  entry.setVarVal("NAV_X", 8);
  EXPECT_FALSE(entry.evalConditions());
  EXPECT_TRUE(entry.evalModeVarConditions());
  entry.setVarVal("NAV_X", 12);
  EXPECT_TRUE(entry.evalConditions());

  entry.clearConditionVarVals();
  EXPECT_FALSE(entry.evalConditions());
  entry.clear();
  EXPECT_EQ(entry.getModeVarName(), "");
  EXPECT_TRUE(entry.getConditionVars().empty());
}

// Covers mode set behavior: evaluates mode hierarchy from info buffer and posts results back.
TEST(ModeSetTest, EvaluatesModeHierarchyFromInfoBufferAndPostsResultsBack)
{
  InfoBuffer buffer;
  buffer.setValue("NAV_X", 12);
  buffer.setValue("BATTERY", 80);

  ModeEntry mode;
  ASSERT_TRUE(mode.setHead("MODE", "SURVEY"));
  ASSERT_TRUE(mode.setElseValue("TRANSIT"));
  ASSERT_TRUE(mode.addCondition("NAV_X > 10"));

  ModeEntry submode;
  ASSERT_TRUE(submode.setHead("SUBMODE", "LOITER"));
  ASSERT_TRUE(submode.addCondition("MODE = SURVEY"));
  ASSERT_TRUE(submode.addCondition("BATTERY > 50"));

  ModeSet modes;
  modes.setInfoBuffer(&buffer);
  modes.addEntry(mode);
  modes.addEntry(submode);
  modes.evaluate();

  EXPECT_EQ(modes.size(), 2u);
  EXPECT_TRUE(contains(modes.getModeSummary(), "MODE@SURVEY"));
  EXPECT_TRUE(contains(modes.getModeSummary(), "SUBMODE@LOITER"));
  EXPECT_TRUE(contains(modes.getStringDescription(), "MODE#---,SURVEY"));
  EXPECT_TRUE(contains(modes.getStringDescription(), "SUBMODE#---,LOITER"));

  bool ok = false;
  EXPECT_EQ(buffer.sQuery("MODE", ok), "SURVEY");
  EXPECT_TRUE(ok);
  EXPECT_EQ(buffer.sQuery("SUBMODE", ok), "LOITER");
  EXPECT_TRUE(ok);
  EXPECT_EQ(modes.getVarDataPairs().size(), 2u);
  EXPECT_EQ(modes.getNonModeLogicConditions()["SURVEY"].size(), 1u);
}

// Covers mode set behavior: posts else value when entry conditions fail.
TEST(ModeSetTest, PostsElseValueWhenEntryConditionsFail)
{
  InfoBuffer buffer;
  buffer.setValue("NAV_X", 5);

  ModeEntry mode;
  ASSERT_TRUE(mode.setHead("MODE", "SURVEY"));
  ASSERT_TRUE(mode.setElseValue("TRANSIT"));
  ASSERT_TRUE(mode.addCondition("NAV_X > 10"));

  ModeSet modes;
  modes.setInfoBuffer(&buffer);
  modes.addEntry(mode);
  modes.evaluate();

  EXPECT_EQ(modes.getModeSummary(), "MODE@TRANSIT");
  bool ok = false;
  EXPECT_EQ(buffer.sQuery("MODE", ok), "TRANSIT");
  EXPECT_TRUE(ok);
}

// Covers behavior report behavior: manages keys priority serialization and NaN detection.
TEST(BehaviorReportTest, ManagesKeysPrioritySerializationAndNanDetection)
{
  BehaviorReport report("waypt_survey", 42);
  EXPECT_TRUE(report.isEmpty());
  report.setPriority(10);
  report.setPriority(-5);
  EXPECT_DOUBLE_EQ(report.getPriority(), 0);

  std::unique_ptr<IvPFunction> rejected = makeOnePieceFunction("course", 0);
  report.addIPF(rejected.get(), "ignored");
  EXPECT_TRUE(report.isEmpty());

  std::unique_ptr<IvPFunction> first = makeOnePieceFunction("course");
  std::unique_ptr<IvPFunction> second = makeOnePieceFunction("speed");
  report.addIPF(first.get(), "leg");
  report.addIPF(second.get(), "leg");
  ASSERT_EQ(report.size(), 2u);
  EXPECT_DOUBLE_EQ(report.getAvgPieces(), 1);

  report.makeKeysUnique();
  EXPECT_EQ(report.getKey(0), "leg_0");
  EXPECT_EQ(report.getKey(1), "leg_1");
  EXPECT_FALSE(report.hasUniqueKey(0));
  EXPECT_FALSE(report.hasUniqueKey(1));
  EXPECT_EQ(report.getIPF("leg_0"), first.get());
  EXPECT_EQ(report.getIPF(9), nullptr);

  report.setIPFStrings();
  EXPECT_TRUE(report.hasIPFString(0));
  EXPECT_TRUE(contains(report.getIPFString(0), "42:waypt_survey:leg_0"));
  EXPECT_FALSE(report.checkForNans());
  second->getPDMap()->bx(0)->wt(0) = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(report.checkForNans());
}

// Covers behavior report behavior: treats single empty key as unique and names it from domain.
TEST(BehaviorReportTest, TreatsSingleEmptyKeyAsUniqueAndNamesItFromDomain)
{
  BehaviorReport report("speed_control", 7);
  std::unique_ptr<IvPFunction> ipf = makeOnePieceFunction("speed");
  report.addIPF(ipf.get());
  ASSERT_EQ(report.size(), 1u);

  report.makeKeysUnique();
  EXPECT_TRUE(report.hasUniqueKey(0));
  report.setIPFStrings();
  EXPECT_TRUE(contains(report.getIPFString(0), "7:speed_control"));
}

// Covers behavior spec behavior: parses template config and filters spawn updates from info buffer.
TEST(BehaviorSpecTest, ParsesTemplateConfigAndFiltersSpawnUpdatesFromInfoBuffer)
{
  InfoBuffer buffer;
  buffer.setValue("CONTACT_INFO", "name=abe,type=contact,x=10,y=20");
  buffer.setValue("CONTACT_INFO", "type=contact,x=30,y=40");
  buffer.setValue("CONTACT_INFO", "name=ben,type=contact,spd=2.5");

  BehaviorSpec spec;
  spec.setInfoBuffer(&buffer);
  spec.setFileName("meta_vehicle.bhv");
  spec.setBehaviorKind("BHV_AvdColregsV22", 94);
  spec.addBehaviorConfig("name=avdc_", 95);
  spec.addBehaviorConfig("updates=CONTACT_INFO", 96);
  spec.addBehaviorConfig("templating=spawn", 97);
  spec.addBehaviorConfig("max_spawnings=3", 98);
  spec.addBehaviorConfig("pwt=500", 99);

  EXPECT_EQ(spec.getFileName(), "meta_vehicle.bhv");
  EXPECT_EQ(spec.getKind(), "BHV_AvdColregsV22");
  EXPECT_EQ(spec.getKindLine(), 94u);
  EXPECT_EQ(spec.getUpdatesVar(), "CONTACT_INFO");
  EXPECT_EQ(spec.getTemplatingType(), "spawn");
  EXPECT_TRUE(spec.templating());
  EXPECT_EQ(spec.getNamePrefix(), "avdc_");
  EXPECT_EQ(spec.getMaxSpawnings(), 3u);

  ASSERT_EQ(spec.size(), 3u);
  EXPECT_EQ(spec.getConfigLine(0), "name=avdc_");
  EXPECT_EQ(spec.getConfigLine(1), "updates=CONTACT_INFO");
  EXPECT_EQ(spec.getConfigLine(2), "pwt=500");
  EXPECT_EQ(spec.getConfigLineNum(2), 99u);
  EXPECT_EQ(spec.getConfigLine(99), "");
  EXPECT_EQ(spec.getConfigLineNum(99), 0u);

  std::vector<std::string> spawn_updates = spec.checkForSpawningStrings();
  ASSERT_EQ(spawn_updates.size(), 2u);
  EXPECT_EQ(spawn_updates[0], "name=abe,type=contact,x=10,y=20");
  EXPECT_EQ(spawn_updates[1], "name=ben,type=contact,spd=2.5");
}

// Covers behavior spec behavior: disallowed templating suppresses spawn updates and clear resets state.
TEST(BehaviorSpecTest, DisallowedTemplatingSuppressesSpawnUpdatesAndClearResetsState)
{
  InfoBuffer buffer;
  buffer.setValue("OBSTACLE_ALERT", "name=obs_01,poly=pts:{0,0:10,0:10,10:0,10}");

  BehaviorSpec spec;
  spec.setInfoBuffer(&buffer);
  ASSERT_TRUE(spec.setTemplatingType("clone"));
  EXPECT_FALSE(spec.setTemplatingType("bad_mode"));
  spec.addBehaviorConfig("updates=OBSTACLE_ALERT", 12);
  spec.addBehaviorConfig("templating=disallow", 13);
  spec.addBehaviorConfig("max_spawnings=2", 14);
  spec.spawnTried();
  spec.spawnMade();

  EXPECT_EQ(spec.getTemplatingType(), "disallow");
  EXPECT_FALSE(spec.templating());
  EXPECT_TRUE(spec.checkForSpawningStrings().empty());
  EXPECT_EQ(spec.getSpawnsTried(), 1u);
  EXPECT_EQ(spec.getSpawnsMade(), 1u);
  EXPECT_EQ(spec.getMaxSpawnings(), 2u);

  spec.clear();
  EXPECT_EQ(spec.getKind(), "");
  EXPECT_EQ(spec.getUpdatesVar(), "");
  EXPECT_EQ(spec.getTemplatingType(), "disallowed");
  EXPECT_FALSE(spec.templating());
  EXPECT_EQ(spec.size(), 0u);
  EXPECT_EQ(spec.getSpawnsTried(), 0u);
  EXPECT_EQ(spec.getSpawnsMade(), 0u);
  EXPECT_TRUE(spec.checkForSpawningStrings().empty());
}

// Covers helm report behavior: tracks decisions behavior states and clear semantics.
TEST(HelmReportTest, TracksDecisionsBehaviorStatesAndClearSemantics)
{
  HelmReport report;
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 6);
  report.setIvPDomain(domain);
  report.addDecision("course", 90);
  report.addDecision("speed", 2.5);
  EXPECT_TRUE(report.hasDecision("course"));
  EXPECT_DOUBLE_EQ(report.getDecision("speed"), 2.5);
  EXPECT_EQ(report.getDecisionSummary(), "var=course:90,var=speed:2.5");

  report.addActiveBHV("waypt", 10, 50, 12, 0.1, "ok", 1, "MODE=SURVEY", "");
  report.addRunningBHV("loiter", 11, "waiting");
  report.addIdleBHV("station", 12, "idle");
  report.addDisabledBHV("avoid", 13, "disabled");
  report.addCompletedBHV("return", 14, "done");
  EXPECT_EQ(report.getActiveBhvsCnt(), 1u);
  EXPECT_EQ(report.getRunningBhvsCnt(), 1u);
  EXPECT_EQ(report.getIdleBhvsCnt(), 1u);
  EXPECT_EQ(report.getDisabledBhvsCnt(), 1u);
  EXPECT_EQ(report.getCompletedBhvsCnt(), 1u);
  EXPECT_TRUE(contains(report.getActiveBehaviors(true), "waypt$10.00$50"));
  EXPECT_TRUE(contains(report.getRunningBehaviors(true), "loiter$11.00$waiting"));
  EXPECT_TRUE(contains(report.getCompletedBehaviors(false), "return"));

  HelmReport changed = report;
  changed.addRunningBHV("new_bhv", 15, "new");
  EXPECT_TRUE(changed.changedBehaviors(report));

  report.clear(false);
  EXPECT_FALSE(report.hasDecision("course"));
  EXPECT_EQ(report.getCompletedBhvsCnt(), 1u);
  report.clear(true);
  EXPECT_EQ(report.getCompletedBhvsCnt(), 0u);
}

// Covers helm report behavior: caps completed behavior history at twenty entries.
TEST(HelmReportTest, CapsCompletedBehaviorHistoryAtTwentyEntries)
{
  HelmReport report;
  for(unsigned int i = 0; i < 25; ++i)
    report.addCompletedBHV("bhv_" + std::to_string(i), 100 + i, "done");

  EXPECT_EQ(report.getCompletedBhvsCnt(), 20u);
  EXPECT_FALSE(contains(report.getCompletedBehaviors(false), "bhv_0"));
  EXPECT_TRUE(contains(report.getCompletedBehaviors(false), "bhv_24"));
  report.setTimeUTC(130);
  EXPECT_TRUE(contains(report.getCompletedBehaviorsTerse(), "bhv_24"));
}

// Covers helm report behavior: serializes domain templating messages and timing for app casts.
TEST(HelmReportTest, SerializesDomainTemplatingMessagesAndTimingForAppCasts)
{
  HelmReport report;
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 26);
  report.setIvPDomain(domain);
  report.setModeSummary("MODE@ACTIVE:SURVEYING");
  report.setTimeUTC(125.5);
  report.setIteration(42);
  report.setOFNUM(3);
  report.setWarningCount(2);
  report.setCreateTime(0.04);
  report.setSolveTime(0.08);
  report.setMaxCreateTime(0.14);
  report.setMaxSolveTime(0.22);
  report.setMaxLoopTime(0.35);
  report.setTotalPcsFormed(18);
  report.setTotalPcsCached(7);
  report.setTemplatingSummary({"BHV_AvdColregsV22:CONTACT_INFO",
                               "BHV_AvoidObstacleV24:OBSTACLE_ALERT"});
  report.setUpdateResults({"avdc_:ok", "obs_:ok"});
  report.addMsg("helm warning detail");

  EXPECT_EQ(report.getDomainString(), "course,0,359,360:speed,0,5,26");
  EXPECT_EQ(report.getModeSummary(), "MODE@ACTIVE:SURVEYING");
  EXPECT_DOUBLE_EQ(report.getLoopTime(), 0.12);
  EXPECT_EQ(report.getTemplatingSummary().size(), 2u);
  EXPECT_EQ(report.getUpdateResults().size(), 2u);
  EXPECT_EQ(report.getMsgs().size(), 1u);

  std::string report_string = report.getReportAsString();
  EXPECT_TRUE(contains(report_string, "iter=42"));
  EXPECT_TRUE(contains(report_string, "ofnum=3"));
  EXPECT_TRUE(contains(report_string, "warnings=2"));

  auto summary = report.formattedSummary(130, true);
  EXPECT_NE(std::find(summary.begin(), summary.end(),
                      "  BHV_AvdColregsV22:CONTACT_INFO"),
            summary.end());
}
