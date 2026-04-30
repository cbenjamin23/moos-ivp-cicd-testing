#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "ALogEntry.h"
#include "ALogDataBroker.h"
#include "BuildUtils.h"
#include "CPAEvent.h"
#include "EncounterPlot.h"
#include "HelmPlot.h"
#include "IPF_Plot.h"
#include "IvPDomain.h"
#include "FunctionEncoder.h"
#include "ModelAppLogScope.h"
#include "ModelHelmScope.h"
#include "ModelTaskDiary.h"
#include "ModelVarScope.h"
#include "NumericAssertions.h"
#include "Populator_EncounterPlot.h"
#include "Populator_HelmPlots.h"
#include "Populator_IPF_Plot.h"
#include "Populator_TaskDiary.h"
#include "Populator_VPlugPlots.h"
#include "TestFileUtils.h"
#include "TaskDiary.h"
#include "VPlugPlot.h"
#include "ZAIC_PEAK.h"

namespace {

ALogEntry stringEntry(double time, const std::string& var, const std::string& source,
                      const std::string& aux, const std::string& value,
                      const std::string& node = "")
{
  ALogEntry entry;
  entry.set(time, var, source, aux, value);
  entry.setNode(node);
  return entry;
}

std::string joinLines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

TempFile writeTempAlog(const std::string& stem, const std::vector<std::string>& lines)
{
  return TempFile(stem, joinLines(lines), ".alog");
}

IvPDomain courseSpeedDomain()
{
  IvPDomain domain;
  domain.addDomain("course", 0, 359, 360);
  domain.addDomain("speed", 0, 5, 51);
  return domain;
}

std::unique_ptr<IvPFunction> makePeakFunction(const std::string& var,
                                              const std::string& context,
                                              double summit,
                                              double pwt)
{
  ZAIC_PEAK zaic(courseSpeedDomain(), var);
  EXPECT_TRUE(zaic.setParams(summit, 10, 40, 0, 0, 100));
  std::unique_ptr<IvPFunction> ipf(zaic.extractIvPFunction());
  EXPECT_NE(ipf, nullptr);
  if(ipf) {
    ipf->setContextStr(context);
    ipf->setPWT(pwt);
  }
  return ipf;
}

std::string helmSummary(unsigned int iter, const std::string& active,
                        const std::string& running, const std::string& completed,
                        const std::string& decision)
{
  // Compact IVPHELM_SUMMARY payloads are what HelmReport/HelmPlot consume in
  // real alogs, including the dollar-delimited behavior lists.
  return "iter=" + std::to_string(iter) +
         ",ofnum=1,warnings=0,solve_time=0.01,create_time=0.02,"
         "utc_time=1000." + std::to_string(iter) +
         ",ivpdomain=\"course,0,359,360:speed,0,5,51\"" +
         ",var=course:" + decision +
         ",var=speed:1.5,halted=false,modes=MODE@ACTIVE:LOITERING,"
         "active_bhvs=" + active +
         ",running_bhvs=" + running +
         ",completed_bhvs=" + completed;
}

}  // namespace

// Covers task diary behavior: groups mission task posts by task and matches winning result.
TEST(TaskDiaryTest, GroupsMissionTaskPostsByTaskAndMatchesWinningResult)
{
  TaskDiary diary;
  const std::string task = "type=survey,id=alpha,region=west";
  diary.addALogEntry(stringEntry(105.0, "MISSION_TASK", "pMarineViewer", "", task, "abe"));
  diary.addALogEntry(stringEntry(106.0, "MISSION_TASK", "pMarineViewer", "", task, "ben"));
  diary.addALogEntry(stringEntry(109.0, "TASK_WON", "uFldTaskMonitor", "",
                                 "type=survey,id=alpha,winner=abe,others=ben"));
  diary.addALogEntry(stringEntry(111.0, "MISSION_TASK", "uFldMessageHandler",
                                 "shoreside+pShare", "type=sample,id=bravo", "abe"));

  diary.processAllEntries(100.0);

  ASSERT_EQ(diary.size(), 2u);
  std::vector<TaskDiaryEntry> entries = diary.getDiaryEntries();
  EXPECT_DOUBLE_EQ(entries[0].getTaskTime(), 5.0);
  EXPECT_DOUBLE_EQ(entries[0].getResultTime(), 9.0);
  EXPECT_EQ(entries[0].getSourceApp(), "pMarineViewer");
  EXPECT_EQ(entries[0].getSourceNode(), "shore");
  EXPECT_EQ(entries[0].getDestNodesAll(), "abe,ben");
  EXPECT_EQ(entries[0].getResultInfo(), "[abe] ben");

  EXPECT_DOUBLE_EQ(entries[1].getTaskTime(), 11.0);
  EXPECT_EQ(entries[1].getSourceApp(), "pShare");
  EXPECT_EQ(entries[1].getSourceNode(), "shoreside");
  EXPECT_EQ(entries[1].getDestNodesAll(), "abe");
  EXPECT_EQ(entries[1].getResultInfo(), "");

  EXPECT_EQ(diary.getDiaryEntriesUpToTime(5.0).size(), 1u);
  EXPECT_EQ(diary.getDiaryEntriesPastTime(5.0).size(), 1u);
}

// Covers task diary behavior: formats diary entries with separators and wrapped mission details.
TEST(TaskDiaryTest, FormatsDiaryEntriesWithSeparatorsAndWrappedMissionDetails)
{
  TaskDiary diary;
  diary.addALogEntry(stringEntry(10.0, "MISSION_TASK", "pMarineViewer", "",
                                 "type=survey,id=alpha,region={0,0:100,0:100,100:0,100},"
                                 "priority=high,notes=\"hold,station\"", "abe"));
  diary.addALogEntry(stringEntry(15.0, "TASK_WON", "uFldTaskMonitor", "",
                                 "type=survey,id=alpha,winner=abe,others=ben#cal"));
  diary.processAllEntries(10.0);

  std::vector<std::string> lines =
      diary.formattedLines(diary.getDiaryEntries(), true, true);
  ASSERT_GT(lines.size(), 3u);
  std::string joined;
  for(const std::string& line : lines)
    joined += line + "\n";
  EXPECT_NE(joined.find("Mission Task"), std::string::npos);
  EXPECT_NE(joined.find("pMarineViewer"), std::string::npos);
  EXPECT_NE(joined.find("type=survey"), std::string::npos);
  EXPECT_NE(joined.find("[abe] ben, cal"), std::string::npos);
}

// Covers populator task diary behavior: populates from alog and klog mission task sources.
TEST(PopulatorTaskDiaryTest, PopulatesFromAlogAndKlogMissionTaskSources)
{
  // Alogs represent shoreside tasking through source/destination nodes, while
  // klogs are local vehicle logs and should resolve task posts as self-sourced.
  TempFile alog = writeTempAlog("task_diary", {
      "%% LOGSTART 1000.000",
      "0.000 DB_TIME MOOSDB_abe 1000.000",
      "5.000 MISSION_TASK pMarineViewer type=survey,id=alpha",
      "7.000 TASK_WON uFldTaskMonitor type=survey,id=alpha,winner=abe,others=ben",
      "8.000 NODE_MESSAGE uFldNodeComms src_node=shore,dest_node=abe,var_name=MISSION_TASK,string_val=type=sample:id=bravo"});

  Populator_TaskDiary populator;
  EXPECT_TRUE(populator.addALogFile(alog.path()));
  EXPECT_FALSE(populator.addALogFile(alog.path()));
  EXPECT_TRUE(populator.populateFromALogs());

  TaskDiary diary = populator.getTaskDiary();
  ASSERT_EQ(diary.size(), 1u);
  EXPECT_DOUBLE_EQ(diary.getDiaryEntries()[0].getTaskTime(), 5.0);
  EXPECT_EQ(diary.getDiaryEntries()[0].getDestNodesAll(), "abe");

  TempFile klog = writeTempAlog("task_klog", {
      "1.000 MISSION_TASK pHelmIvP type=survey,id=local",
      "3.000 TASK_WON pHelmIvP type=survey,id=local,winner=abe,others=none"});
  Populator_TaskDiary klog_populator;
  EXPECT_TRUE(klog_populator.addKLogFile(klog.path(), "abe", 2000.0));
  EXPECT_FALSE(klog_populator.addKLogFile(klog.path(), "abe", 2000.0));
  EXPECT_TRUE(klog_populator.populateFromKLogs());
  ASSERT_EQ(klog_populator.getTaskDiary().size(), 1u);
  EXPECT_EQ(klog_populator.getTaskDiary().getDiaryEntries()[0].getSourceNode(), "self");
}

// Covers helm plot behavior: stores helm summaries and navigates by time and iteration.
TEST(HelmPlotTest, StoresHelmSummariesAndNavigatesByTimeAndIteration)
{
  HelmPlot plot;
  plot.setVehiType("KAYAK");
  plot.setVehiLength(4.2);

  ASSERT_TRUE(plot.addEntry(10.0, helmSummary(
      4, "waypt_survey$1000.4$100$4$0.01$ok$1$survey$leg1",
      "loiter$1000.4$ok", "stationkeep$1000.0$done", "90")));
  ASSERT_TRUE(plot.addEntry(12.0, helmSummary(
      6, "waypt_survey$1000.6$90$5$0.02$ok$1$survey$leg2",
      "loiter$1000.6$ok", "none$0$n/a", "95")));
  // Helm timelines are monotonic; an older timestamp after a newer one is
  // rejected instead of being inserted into the middle.
  EXPECT_FALSE(plot.addEntry(11.0, helmSummary(7, "a$0$0$0$0$n/a$0", "", "", "100")));

  EXPECT_EQ(plot.getVType(), "kayak");
  EXPECT_DOUBLE_EQ(plot.getVLength(), 4.2);
  EXPECT_EQ(plot.size(), 2u);
  EXPECT_TRUE(plot.containsTime(11.0));
  EXPECT_FALSE(plot.containsTime(9.9));
  EXPECT_EQ(plot.getIterByIndex(0), 4u);
  EXPECT_EQ(plot.getIterByTime(11.0), 4u);
  EXPECT_DOUBLE_EQ(plot.getTimeByIterAdd(10.0, 2), 12.0);
  EXPECT_DOUBLE_EQ(plot.getTimeByIterSub(12.0, 2), 10.0);
  EXPECT_NE(plot.getValueByIndex("active", 0).find("waypt_survey"), std::string::npos);
  EXPECT_NE(plot.getValueByTime("running", 11.0).find("loiter"), std::string::npos);
  EXPECT_NE(plot.getValueByIndex("completed", 0).find("stationkeep"), std::string::npos);
  EXPECT_NE(plot.getValueByIndex("decision", 1).find("var=course:95"), std::string::npos);
  EXPECT_EQ(plot.getValueByIndex("unknown-query", 0), "unknown");
}

// Covers populator helm plots behavior: populates only pHelmIvP summary entries.
TEST(PopulatorHelmPlotsTest, PopulatesOnlyIvPHelmSummaryEntries)
{
  std::vector<ALogEntry> entries = {
      stringEntry(1.0, "NAV_X", "pNodeReporter", "", "12.5"),
      stringEntry(2.0, "IVPHELM_SUMMARY", "pHelmIvP", "",
                  helmSummary(1, "waypt$1000$100$2$0.01$ok$1", "", "", "180")),
      stringEntry(3.0, "IVPHELM_SUMMARY", "pHelmIvP", "",
                  helmSummary(2, "loiter$1001$100$3$0.01$ok$1", "", "", "181"))};

  Populator_HelmPlots populator;
  EXPECT_TRUE(populator.populateFromEntries(entries));
  HelmPlot plot = populator.getHelmPlot();
  ASSERT_EQ(plot.size(), 2u);
  EXPECT_EQ(plot.getIterByIndex(1), 2u);
  EXPECT_EQ(plot.getValueByTime("iter", 2.5), "1");

  Populator_HelmPlots empty;
  EXPECT_FALSE(empty.populateFromEntries({}));
}

// Covers IPF plot behavior: indexes IvP function strings by time and helm iteration.
TEST(IPFPlotTest, IndexesIvPFunctionStringsByTimeAndHelmIteration)
{
  IvPDomain domain = courseSpeedDomain();
  IPF_Plot plot;
  plot.setVName("abe");
  plot.setSource("waypt_survey");
  plot.setIvPDomain(domain);

  EXPECT_TRUE(plot.addEntry(10.0, "encoded-ipf-a", 4, 12, 75.0, domain, "survey", "leg1"));
  EXPECT_TRUE(plot.addEntry(12.0, "encoded-ipf-b", 6, 10, 50.0, domain, "survey", "leg2"));
  EXPECT_FALSE(plot.addEntry(11.0, "out-of-order-time", 7, 8, 40.0, domain));
  EXPECT_FALSE(plot.addEntry(13.0, "out-of-order-iter", 5, 8, 40.0, domain));

  EXPECT_EQ(plot.getVName(), "abe");
  EXPECT_EQ(plot.getSource(), "waypt_survey");
  EXPECT_EQ(plot.size(), 2);
  EXPECT_DOUBLE_EQ(plot.getMinTime(), 10.0);
  EXPECT_DOUBLE_EQ(plot.getMaxTime(), 12.0);
  EXPECT_EQ(plot.getTimeByIndex(99), 12.0);
  EXPECT_EQ(plot.getIPFByIndex(99), "encoded-ipf-b");
  EXPECT_EQ(plot.getIPFByTime(11.0), "encoded-ipf-a");
  EXPECT_EQ(plot.getIPFByTime(9.0), "");
  // Current IPF_Plot lookup treats the exact upper boundary as outside the
  // active interval, even though getTimeByIndex(99) clamps to that value.
  EXPECT_EQ(plot.getIPFByTime(12.0), "");
  EXPECT_EQ(plot.getIPFByHelmIteration(6), "encoded-ipf-b");
  EXPECT_EQ(plot.getIPFByHelmIteration(5), "");
  EXPECT_EQ(plot.getPcsByHelmIteration(4), 12u);
  EXPECT_DOUBLE_EQ(plot.getPwtByHelmIteration(6), 50.0);
  EXPECT_EQ(plot.getDomainByHelmIter(4).size(), 2u);
  EXPECT_EQ(plot.getDomainByHelmIter(99).size(), 0u);
  EXPECT_EQ(plot.getHelmIterByTime(11.5), 4u);
  EXPECT_DOUBLE_EQ(plot.getTimeByHelmIter(6), 12.0);
  EXPECT_DOUBLE_EQ(plot.getTimeByHelmIter(99), 0.0);

  plot.applySkew(-10.0);
  EXPECT_DOUBLE_EQ(plot.getMinTime(), 0.0);
  EXPECT_DOUBLE_EQ(plot.getMaxTime(), 2.0);
}

// Covers populator IPF plot behavior: demuxes BHV IPF packets into behavior specific plots.
TEST(PopulatorIPFPlotTest, DemuxesBhvIpfPacketsIntoBehaviorSpecificPlots)
{
  // Context strings follow the helm form iter:behavior:mode-info. The populator
  // uses them to tag the vehicle-specific plot as abe_waypt_return.
  std::unique_ptr<IvPFunction> ipf =
      makePeakFunction("course", "17:waypt_return:MODE@SURVEY:LEG=1", 90, 62);
  ASSERT_NE(ipf, nullptr);
  const std::string encoded = IvPFunctionToString(ipf.get());
  ASSERT_FALSE(encoded.empty());
  std::vector<std::string> packets =
      IvPFunctionToVector(encoded, "waypt_return^17", 90);
  ASSERT_GT(packets.size(), 1u);

  // BHV_IPF entries are fragmented on real logs, so this fixture feeds the
  // packet stream through the same demux path instead of handing over an IPF.
  std::vector<ALogEntry> entries;
  for(std::size_t i = 0; i < packets.size(); ++i)
    entries.push_back(stringEntry(20.0 + i, "BHV_IPF", "pHelmIvP", "", packets[i]));
  entries.push_back(stringEntry(30.0, "NAV_X", "pNodeReporter", "", "12"));

  Populator_IPF_Plot populator;
  EXPECT_FALSE(populator.setIvPDomain("course,0,359"));
  ASSERT_TRUE(populator.setIvPDomain(domainToString(courseSpeedDomain())));
  populator.setVName("abe");
  EXPECT_TRUE(populator.populateFromEntries(entries));

  ASSERT_EQ(populator.size(), 1);
  EXPECT_EQ(populator.getTagIPF(0), "abe_waypt_return");
  EXPECT_EQ(populator.getTagIPF(9), "");

  IPF_Plot plot = populator.getPlotIPF(0);
  ASSERT_EQ(plot.size(), 1);
  EXPECT_EQ(plot.getSource(), "waypt_return");
  EXPECT_EQ(plot.getVName(), "abe");
  EXPECT_DOUBLE_EQ(plot.getTimeByIndex(0), 20.0);
  EXPECT_EQ(plot.getPcsByHelmIteration(17), ipf->size());
  // FunctionEncoder stores the helm priority as the integer PWT seen on the
  // BHV_IPF packet path.
  EXPECT_NEAR(plot.getPwtByHelmIteration(17), 62.0, kGeomTol);
  EXPECT_EQ(plot.getDomainByHelmIter(17), ipf->getPDMap()->getDomain());
}

// Covers VPlug plot behavior: bins visual posts and carries forward prior shapes.
TEST(VPlugPlotTest, BinsVisualPostsAndCarriesForwardPriorShapes)
{
  VPlugPlot plot;
  plot.setVehiName("abe");
  plot.setBinVal(1.0);

  EXPECT_TRUE(plot.addEvent("VIEW_POINT", "x=1,y=2,label=alpha", 10.0));
  EXPECT_TRUE(plot.addEvent("VIEW_POLYGON", "pts={0,0:10,0:10,10:0,10},label=op_region", 10.5));
  EXPECT_TRUE(plot.addEvent("VIEW_CIRCLE", "x=5,y=6,radius=10,label=obs", 12.0));
  EXPECT_TRUE(plot.addEvent("VIEW_RANGE_PULSE", "x=1,y=2,rad=50,duration=4,label=pulse", 13.5));

  EXPECT_EQ(plot.getVehiName(), "abe");
  EXPECT_EQ(plot.size(), 3u);
  EXPECT_DOUBLE_EQ(plot.getMinTime(), 10.0);
  EXPECT_DOUBLE_EQ(plot.getMaxTime(), 13.5);

  VPlug_GeoShapes first = plot.getVPlugByIndex(0);
  EXPECT_EQ(first.sizePoints(), 1u);
  EXPECT_EQ(first.sizePolygons(), 1u);
  EXPECT_EQ(first.sizeSegLists(), 0u);

  VPlug_GeoShapes carried = plot.getVPlugByTime(12.5);
  EXPECT_EQ(carried.sizePoints(), 1u);
  EXPECT_EQ(carried.sizePolygons(), 1u);
  EXPECT_EQ(carried.sizeCircles(), 1u);
  EXPECT_EQ(carried.sizeRangePulses(), 0u);

  EXPECT_EQ(plot.getVPlugByTime(10.0).sizePoints(), 0u);
  EXPECT_EQ(plot.getVPlugByIndex(99).sizePoints(), 0u);

  plot.applySkew(-10.0);
  EXPECT_DOUBLE_EQ(plot.getMinTime(), 0.0);
  EXPECT_DOUBLE_EQ(plot.getMaxTime(), 3.5);
}

// Covers populator VPlug plots behavior: populates visual entries and ignores non visual variables.
TEST(PopulatorVPlugPlotsTest, PopulatesVisualEntriesAndIgnoresNonVisualVariables)
{
  std::vector<ALogEntry> entries = {
      stringEntry(1.0, "VIEW_POINT", "pMarineViewer", "", "x=1,y=2,label=alpha"),
      stringEntry(2.0, "NAV_X", "pNodeReporter", "", "12.5"),
      stringEntry(3.0, "VIEW_CIRCLE", "pObstacleMgr", "", "x=5,y=6,radius=10,label=obs")};

  Populator_VPlugPlots populator;
  populator.setVQual("max");
  EXPECT_TRUE(populator.populateFromEntry(entries[0]));
  EXPECT_TRUE(populator.populateFromEntry(entries[1]));
  EXPECT_TRUE(populator.populateFromEntry(entries[2]));

  VPlugPlot plot = populator.getVPlugPlot();
  EXPECT_EQ(plot.size(), 3u);
  EXPECT_EQ(plot.getVPlugByIndex(0).sizePoints(), 1u);
  EXPECT_EQ(plot.getVPlugByIndex(1).sizePoints(), 1u);
  EXPECT_EQ(plot.getVPlugByIndex(2).sizeCircles(), 1u);
}

// Covers encounter plot behavior: tracks CPA efficiency ranges contacts and averages.
TEST(EncounterPlotTest, TracksCpaEfficiencyRangesContactsAndAverages)
{
  CPAEvent first("abe", "ben", 8.0);
  first.setEFF(91.0);
  first.setID(7);
  CPAEvent second("abe", "cal", 18.0);
  second.setEFF(87.0);
  second.setID(8);

  EncounterPlot plot;
  EXPECT_TRUE(plot.empty());
  EXPECT_TRUE(plot.addEncounter(10.0, first));
  EXPECT_TRUE(plot.addEncounter(12.0, second));
  EXPECT_FALSE(plot.addEncounter(11.0, CPAEvent("abe", "dan", 5.0)));

  EXPECT_FALSE(plot.empty());
  EXPECT_EQ(plot.getOwnship(), "abe");
  EXPECT_EQ(plot.size(), 2u);
  EXPECT_DOUBLE_EQ(plot.getValueCPAByIndex(0), 8.0);
  EXPECT_DOUBLE_EQ(plot.getValueEffByIndex(1), 87.0);
  EXPECT_EQ(plot.getValueIDByIndex(1), 8);
  EXPECT_EQ(plot.getValueContactByIndex(0), "ben");
  EXPECT_EQ(plot.getValueContactByIndex(99), "unknown");
  EXPECT_DOUBLE_EQ(plot.getValueCPAByTime(11.0), 8.0);
  EXPECT_DOUBLE_EQ(plot.getValueEffByTime(99.0), 87.0);
  EXPECT_DOUBLE_EQ(plot.getMinCPA(), 8.0);
  EXPECT_DOUBLE_EQ(plot.getMaxCPA(), 18.0);
  EXPECT_DOUBLE_EQ(plot.getMinEFF(), 87.0);
  EXPECT_DOUBLE_EQ(plot.getMaxEFF(), 91.0);
  EXPECT_DOUBLE_EQ(plot.getMeanCPA(), 13.0);
  EXPECT_DOUBLE_EQ(plot.getMeanEFF(), 89.0);

  plot.setCollisionRange(15.0);
  EXPECT_DOUBLE_EQ(plot.getCollisionRange(), 15.0);
  EXPECT_DOUBLE_EQ(plot.getNearMissRange(), 15.0);
  plot.setEncounterRange(12.0);
  EXPECT_DOUBLE_EQ(plot.getCollisionRange(), 12.0);
  EXPECT_DOUBLE_EQ(plot.getNearMissRange(), 12.0);
  EXPECT_DOUBLE_EQ(plot.getEncounterRange(), 12.0);
  plot.setNearMissRange(-1.0);
  EXPECT_DOUBLE_EQ(plot.getNearMissRange(), 12.0);

  plot.applySkew(-10.0);
  EXPECT_DOUBLE_EQ(plot.getMinTime(), 0.0);
  EXPECT_DOUBLE_EQ(plot.getMaxTime(), 2.0);
}

// Covers populator encounter plot behavior: populates encounter summaries and detection ranges.
TEST(PopulatorEncounterPlotTest, PopulatesEncounterSummariesAndDetectionRanges)
{
  std::vector<ALogEntry> entries = {
      stringEntry(1.0, "COLLISION_DETECT_PARAMS", "pContactMgr", "",
                  "collision_range=4,near_miss_range=9,encounter_range=40"),
      stringEntry(2.0, "ENCOUNTER_SUMMARY", "pContactMgr", "",
                  "v1=abe,v2=ben,cpa=8,eff=91,x=10,y=20,id=7,alpha=30,beta=210"),
      stringEntry(3.0, "ENCOUNTER_SUMMARY", "pContactMgr", "",
                  "v1=abe,v2=cal,cpa=18,eff=87,x=30,y=40,id=8")};

  Populator_EncounterPlot populator;
  EXPECT_TRUE(populator.populateFromEntries(entries));
  EncounterPlot plot = populator.getEncounterPlot();
  ASSERT_EQ(plot.size(), 2u);
  EXPECT_DOUBLE_EQ(plot.getCollisionRange(), 4.0);
  EXPECT_DOUBLE_EQ(plot.getNearMissRange(), 9.0);
  EXPECT_DOUBLE_EQ(plot.getEncounterRange(), 40.0);
  EXPECT_EQ(plot.getValueContactByIndex(1), "cal");

  Populator_EncounterPlot empty;
  EXPECT_FALSE(empty.populateFromEntries({}));
}

// Covers a log data broker behavior: splits indexes and retrieves mission like alog data.
TEST(ALogDataBrokerTest, SplitsIndexesAndRetrievesMissionLikeAlogData)
{
  TempFile alog = writeTempAlog("broker", {
      "%% LOGSTART 1000.000",
      "0.000 DB_TIME MOOSDB_abe 1000.000",
      "0.010 NODE_REPORT_LOCAL pNodeReporter NAME=abe,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,SPD=1.5,HDG=90",
      "0.100 NAV_X pNodeReporter 12.5",
      "0.200 NAV_X pNodeReporter 13.5",
      "0.300 HELM_MODE pHelmIvP SURVEY",
      "0.400 APP_LOG pHelmIvP iter=4,log=helm active!@#posted DESIRED_HEADING",
      "0.500 VIEW_POINT pMarineViewer x=1,y=2,label=alpha",
      "0.600 REGION_INFO pMarineViewer poly=op_region,pts={0,0:10,0:10,10:0,10}"});

  ALogDataBroker broker;
  broker.setViewVessels(false);
  broker.setProgress(false);
  broker.addALogFile(alog.path());

  // Broker setup is order-dependent: each step fills state used by the next
  // cache/query layer.
  ASSERT_TRUE(broker.checkALogFiles());
  ASSERT_TRUE(broker.splitALogFiles());
  ASSERT_TRUE(broker.setTimingInfo());
  broker.cacheMasterIndices();
  broker.cacheAppLogIndices();

  EXPECT_EQ(broker.sizeALogs(), 1u);
  EXPECT_EQ(broker.getVNameFromAix(0), "abe");
  EXPECT_EQ(broker.getVTypeFromAix(0), "kayak");
  EXPECT_EQ(broker.getVColorFromAix(0), "red");
  EXPECT_DOUBLE_EQ(broker.getVLengthFromAix(0), 4.2);
  EXPECT_DOUBLE_EQ(broker.getLogStartFromAix(0), 1000.0);
  EXPECT_DOUBLE_EQ(broker.getGlobalMinTime(), 0.0);
  EXPECT_DOUBLE_EQ(broker.getGlobalLogStart(), 1000.0);
  EXPECT_EQ(broker.getAixFromVName("abe"), 0u);
  EXPECT_EQ(broker.getAixFromVName("missing"), broker.sizeALogs());

  std::vector<std::string> num_vars = broker.getVarsInALog(0, true);
  EXPECT_NE(std::find(num_vars.begin(), num_vars.end(), "NAV_X,pNodeReporter"),
            num_vars.end());
  std::vector<std::string> str_vars = broker.getVarsInALog(0, false);
  EXPECT_NE(std::find(str_vars.begin(), str_vars.end(), "HELM_MODE,pHelmIvP"),
            str_vars.end());

  unsigned int nav_mix = broker.getMixFromVNameVarName("abe", "NAV_X");
  ASSERT_LT(nav_mix, broker.sizeMix());
  EXPECT_EQ(broker.getVNameFromMix(nav_mix), "abe");
  EXPECT_EQ(broker.getVarNameFromMix(nav_mix), "NAV_X");
  EXPECT_EQ(broker.getVarTypeFromMix(nav_mix), "double");
  EXPECT_EQ(broker.getVarSourceFromMix(nav_mix), "pNodeReporter");
  LogPlot nav_plot = broker.getLogPlot(nav_mix);
  ASSERT_EQ(nav_plot.size(), 2u);
  EXPECT_DOUBLE_EQ(nav_plot.getValueByTime(0.15), 12.5);

  unsigned int mode_mix = broker.getMixFromVNameVarName("abe", "HELM_MODE");
  ASSERT_LT(mode_mix, broker.sizeMix());
  VarPlot mode_plot = broker.getVarPlot(mode_mix, true);
  ASSERT_EQ(mode_plot.size(), 1u);
  EXPECT_EQ(mode_plot.getEntryByTime(0.3), "SURVEY");
  EXPECT_EQ(mode_plot.getSourceByIndex(0), "pHelmIvP");

  ASSERT_EQ(broker.sizeALix(), 1u);
  EXPECT_EQ(broker.getVNameFromALix(0), "abe");
  EXPECT_EQ(broker.getAppNameFromALix(0), "pHelmIvP");
  AppLogPlot app_plot = broker.getAppLogPlot(0);
  ASSERT_EQ(app_plot.size(), 1u);
  EXPECT_EQ(app_plot.getEntryByIndex(0).getLine(1), "posted DESIRED_HEADING");

  VPlugPlot visual_plot = broker.getVPlugPlot(0);
  EXPECT_EQ(visual_plot.size(), 1u);
  EXPECT_EQ(visual_plot.getVPlugByIndex(0).sizePoints(), 1u);
  EXPECT_EQ(broker.getRegionInfo(), "poly=op_region,pts={0,0:10,0:10,10:0,10}");
}

// Covers model var scope behavior: presents past and future variable entries from broker.
TEST(ModelVarScopeTest, PresentsPastAndFutureVariableEntriesFromBroker)
{
  TempFile alog = writeTempAlog("var_scope", {
      "%% LOGSTART 1000.000",
      "0.000 DB_TIME MOOSDB_abe 1000.000",
      "0.010 NODE_REPORT_LOCAL pNodeReporter NAME=abe,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,SPD=1.5,HDG=90",
      "0.100 NAV_X pNodeReporter 12.5",
      "0.200 DESIRED_HEADING pHelmIvP:11:waypt 90",
      "0.300 NAV_X pNodeReporter 13.5"});

  ALogDataBroker broker;
  broker.setViewVessels(false);
  broker.setProgress(false);
  broker.addALogFile(alog.path());
  ASSERT_TRUE(broker.checkALogFiles());
  ASSERT_TRUE(broker.splitALogFiles());
  ASSERT_TRUE(broker.setTimingInfo());
  broker.cacheMasterIndices();

  unsigned int nav_mix = broker.getMixFromVNameVarName("abe", "NAV_X");
  unsigned int hdg_mix = broker.getMixFromVNameVarName("abe", "DESIRED_HEADING");
  ASSERT_LT(nav_mix, broker.sizeMix());
  ASSERT_LT(hdg_mix, broker.sizeMix());

  ModelVarScope model;
  model.setDataBroker(broker, nav_mix);
  model.setTime(0.15);

  std::vector<std::string> past = model.getPastEntries();
  std::vector<std::string> soon = model.getSoonEntries();
  ASSERT_EQ(past.size(), 1u);
  ASSERT_EQ(soon.size(), 1u);
  EXPECT_NE(past[0].find("NAV_X"), std::string::npos);
  EXPECT_NE(past[0].find("12.5"), std::string::npos);
  EXPECT_NE(soon[0].find("13.5"), std::string::npos);

  model.addVarPlot(hdg_mix);
  model.setShowVName(true);
  model.setShowSrcAux(true);
  model.reformat();
  std::string joined;
  for(const std::string& line : model.getSoonEntries())
    joined += line + "\n";
  EXPECT_NE(joined.find("DESIRED_HEADING"), std::string::npos);
  EXPECT_NE(joined.find("abe"), std::string::npos);
  EXPECT_NE(joined.find("11:waypt"), std::string::npos);

  model.setTime(0.35);
  EXPECT_EQ(model.getSoonEntries().size(), 0u);
  EXPECT_EQ(model.getPastEntries().size(), 3u);

  model.delVarPlot(nav_mix);
  joined.clear();
  for(const std::string& line : model.getPastEntries())
    joined += line + "\n";
  EXPECT_EQ(joined.find("NAV_X"), std::string::npos);
  EXPECT_NE(joined.find("DESIRED_HEADING"), std::string::npos);
}

// Covers model helm scope behavior: presents helm timeline decisions and behavior messages.
TEST(ModelHelmScopeTest, PresentsHelmTimelineDecisionsAndBehaviorMessages)
{
  // LOGSTART is zero so the absolute-looking 1000.xxx helm timestamps remain
  // unskewed when ModelHelmScope joins HelmPlot and VarPlot data.
  TempFile alog = writeTempAlog("helm_scope", {
      "%% LOGSTART 0.000",
      "0.000 DB_TIME MOOSDB_abe 0.000",
      "0.010 NODE_REPORT_LOCAL pNodeReporter NAME=abe,TYPE=kayak,COLOR=red,LENGTH=4.2,X=1,Y=2,SPD=1.5,HDG=90",
      "1000.100 IVPHELM_SUMMARY pHelmIvP " + helmSummary(
          10,
          "waypt_survey$1000.1$100$4$0.01$ok$1$survey$leg1",
          "loiter$1000.1$ok",
          "stationkeep$999.9$done",
          "90"),
      "1000.200 IVPHELM_SUMMARY pHelmIvP " + helmSummary(
          11,
          "waypt_survey$1000.2$90$5$0.02$ok$1$survey$leg2",
          "loiter$1000.2$ok",
          "none$0$n/a",
          "95"),
      "1000.210 IVPHELM_STATE pHelmIvP DRIVE",
      "1000.220 MODE pHelmIvP ACTIVE:LOITERING",
      "1000.230 IVPHELM_MODESET pHelmIvP MODE=ACTIVE",
      "1000.240 BHV_WARNING pHelmIvP:11:waypt_survey waypt_survey: slow turn",
      "1000.250 BHV_ERROR pHelmIvP:11:waypt_survey waypt_survey: no points",
      "1000.260 IVPHELM_LIFE_EVENT pHelmIvP iter=11,bname=waypt_survey,btype=BHV_Waypoint,event=spawn,seed=helm_startup",
      // VarPlot does not carry a value beyond its final sample; repeat DRIVE so
      // helm state remains queryable at the inspection time below.
      "1000.300 IVPHELM_STATE pHelmIvP DRIVE",
      // A later summary keeps HelmPlot::containsTime() true while inspecting
      // iteration 11 at 1000.27.
      "1000.300 IVPHELM_SUMMARY pHelmIvP " + helmSummary(
          12,
          "stationkeep$1000.3$40$2$0.01$ok$1$hold",
          "waypt_survey$1000.3$ok",
          "loiter$1000.3$done",
          "100")});

  ALogDataBroker broker;
  broker.setViewVessels(false);
  broker.setProgress(false);
  broker.addALogFile(alog.path());
  ASSERT_TRUE(broker.checkALogFiles());
  ASSERT_TRUE(broker.splitALogFiles());
  ASSERT_TRUE(broker.setTimingInfo());
  broker.cacheMasterIndices();

  ModelHelmScope model;
  model.setDataBroker(broker, "abe");
  model.setTime(1000.27);

  EXPECT_EQ(model.getCurrIter(), "11");
  EXPECT_EQ(model.getCurrMode(), "MODE@ACTIVE:LOITERING");
  // Pin current formatter behavior: the course field keeps its leading "var=".
  EXPECT_EQ(model.getCurrDecision(), "var=course=95, speed=1.5");
  EXPECT_EQ(model.getCurrHelmState(), "DRIVE");
  EXPECT_EQ(model.getVPlotSize("helm_state"), 2u);
  EXPECT_TRUE(model.getHeadersBHV());
  model.toggleHeadersBHV();
  EXPECT_FALSE(model.getHeadersBHV());

  auto contains = [](const std::vector<std::string>& lines,
                     const std::string& needle) {
    return std::any_of(lines.begin(), lines.end(), [&](const std::string& line) {
      return line.find(needle) != std::string::npos;
    });
  };

  EXPECT_TRUE(contains(model.getActiveList(), "waypt_survey"));
  EXPECT_TRUE(contains(model.getNonActiveList("running"), "loiter"));
  EXPECT_TRUE(contains(model.getWarnings(), "slow turn"));
  EXPECT_TRUE(contains(model.getErrors(), "no points"));
  EXPECT_TRUE(contains(model.getModes(), "ACTIVE:LOITERING"));
  EXPECT_TRUE(contains(model.getLifeEvents(), "waypt_survey"));

  model.incrementIter(-1);
  EXPECT_EQ(model.getCurrIter(), "10");
}

// Covers model app log scope behavior: filters truncates wraps and separates current and future output.
TEST(ModelAppLogScopeTest, FiltersTruncatesWrapsAndSeparatesCurrentAndFutureOutput)
{
  AppLogPlot plot;
  plot.setAppName("pHelmIvP");
  plot.addAppLogEntry(1.0, stringToAppLogEntry("iter=1,log=helm start!@#ignored line"));
  plot.addAppLogEntry(2.0, stringToAppLogEntry(
      "iter=2,log=decision trace includes size and waypoint behavior details that are long enough to wrap cleanly"));
  plot.addAppLogEntry(3.0, stringToAppLogEntry("iter=3,log=future size line"));

  ModelAppLogScope model;
  model.setAppLogPlot(plot);
  model.setTime(2.0);
  model.setGrepStr1("size");
  model.setGrepApply1(true);
  model.setShowSeparator(true);

  std::vector<std::string> current = model.getLinesUpToNow();
  EXPECT_NE(std::find(current.begin(), current.end(),
                      "----------------- App Iteration ----- 2"), current.end());
  EXPECT_TRUE(std::any_of(current.begin(), current.end(), [](const std::string& line) {
    return line.find("decision trace includes size") != std::string::npos;
  }));
  EXPECT_TRUE(std::none_of(current.begin(), current.end(), [](const std::string& line) {
    return line.find("ignored line") != std::string::npos;
  }));

  std::vector<std::string> future = model.getLinesPastNow();
  EXPECT_TRUE(std::any_of(future.begin(), future.end(), [](const std::string& line) {
    return line.find("future size line") != std::string::npos;
  }));

  model.setWrapVal(true);
  std::vector<std::string> wrapped = model.getLinesUpToNow();
  EXPECT_GE(wrapped.size(), current.size());

  model.setWrapVal(false);
  model.setTruncateVal(true);
  std::vector<std::string> truncated = model.getLinesUpToNow();
  EXPECT_TRUE(std::any_of(truncated.begin(), truncated.end(), [](const std::string& line) {
    return line.size() <= 80 && line.find("decision trace") != std::string::npos;
  }));
}

// Covers model task diary behavior: presents past and future task diary entries.
TEST(ModelTaskDiaryTest, PresentsPastAndFutureTaskDiaryEntries)
{
  TaskDiary diary;
  diary.addALogEntry(stringEntry(10.0, "MISSION_TASK", "pMarineViewer", "",
                                 "type=survey,id=alpha", "abe"));
  diary.addALogEntry(stringEntry(12.0, "TASK_WON", "uFldTaskMonitor", "",
                                 "type=survey,id=alpha,winner=abe,others=ben"));
  diary.addALogEntry(stringEntry(20.0, "MISSION_TASK", "pMarineViewer", "",
                                 "type=sample,id=bravo,notes=\"long,wrapped,field\"", "ben"));
  diary.processAllEntries(10.0);

  ModelTaskDiary model;
  model.setTaskDiary(diary);
  model.setTime(5.0);
  model.setShowSeparator(true);

  std::vector<std::string> past = model.getLinesUpToNow();
  std::string past_joined;
  for(const std::string& line : past)
    past_joined += line + "\n";
  EXPECT_NE(past_joined.find("type=survey"), std::string::npos);
  EXPECT_NE(past_joined.find("[abe] ben"), std::string::npos);

  std::vector<std::string> future = model.getLinesPastNow();
  std::string future_joined;
  for(const std::string& line : future)
    future_joined += line + "\n";
  EXPECT_NE(future_joined.find("type=sample"), std::string::npos);

  model.setWrapVal(true);
  std::vector<std::string> wrapped_future = model.getLinesPastNow();
  std::string wrapped_joined;
  for(const std::string& line : wrapped_future)
    wrapped_joined += line + "\n";
  EXPECT_NE(wrapped_joined.find("type=sample"), std::string::npos);
}
