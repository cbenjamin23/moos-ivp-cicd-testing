#include <gtest/gtest.h>

#include <list>
#include <string>
#include <vector>

#include "LogPlot.h"
#include "VarPlot.h"
#include "VarPlotEntry.h"

namespace {

std::vector<std::string> valuesFrom(const std::list<VarPlotEntry>& entries)
{
  std::vector<std::string> values;
  for(const VarPlotEntry& entry : entries)
    values.push_back(entry.getVarVal());
  return values;
}

}  // namespace

// Covers log plot behavior: stores ascending numeric series and rejects out of order posts.
TEST(LogPlotTest, StoresAscendingNumericSeriesAndRejectsOutOfOrderPosts)
{
  LogPlot plot;
  plot.setVName("abe");
  plot.setVarName("NAV_SPEED");

  EXPECT_TRUE(plot.empty());
  EXPECT_TRUE(plot.setValue(0, 0.5));
  EXPECT_TRUE(plot.setValue(2, 1.5));
  EXPECT_TRUE(plot.setValue(2, 1.7));
  EXPECT_FALSE(plot.setValue(1, 9.9));

  EXPECT_FALSE(plot.empty());
  EXPECT_EQ(plot.size(), 3u);
  EXPECT_EQ(plot.getVName(), "abe");
  EXPECT_EQ(plot.getVarName(), "NAV_SPEED");
  EXPECT_DOUBLE_EQ(plot.getMinTime(), 0);
  EXPECT_DOUBLE_EQ(plot.getMaxTime(), 2);
  EXPECT_DOUBLE_EQ(plot.getMinVal(), 0.5);
  EXPECT_DOUBLE_EQ(plot.getMaxVal(), 1.7);
  EXPECT_DOUBLE_EQ(plot.getValueByIndex(1), 1.5);
  EXPECT_DOUBLE_EQ(plot.getTimeByIndex(1), 2);
  EXPECT_DOUBLE_EQ(plot.getValueByIndex(99), 0);
  EXPECT_DOUBLE_EQ(plot.getTimeByIndex(99), 0);
}

// Covers log plot behavior: returns stepwise and interpolated values for mission telemetry.
TEST(LogPlotTest, ReturnsStepwiseAndInterpolatedValuesForMissionTelemetry)
{
  LogPlot xplot;
  xplot.setVarName("NAV_X");
  ASSERT_TRUE(xplot.setValue(0, 0));
  ASSERT_TRUE(xplot.setValue(10, 100));
  ASSERT_TRUE(xplot.setValue(20, 150));

  EXPECT_EQ(xplot.getLPIndexByTime(-5), 0u);
  EXPECT_EQ(xplot.getLPIndexByTime(0), 0u);
  EXPECT_EQ(xplot.getLPIndexByTime(9.9), 0u);
  EXPECT_EQ(xplot.getLPIndexByTime(10), 1u);
  EXPECT_EQ(xplot.getLPIndexByTime(99), 2u);

  EXPECT_DOUBLE_EQ(xplot.getValueByTime(-1), 0);
  EXPECT_DOUBLE_EQ(xplot.getValueByTime(5), 0);
  EXPECT_DOUBLE_EQ(xplot.getValueByTime(5, true), 50);
  EXPECT_DOUBLE_EQ(xplot.getValueByTime(15, true), 125);
  EXPECT_DOUBLE_EQ(xplot.getValueByTime(25, true), 150);
}

// Covers log plot behavior: computes median range bounds and average gap.
TEST(LogPlotTest, ComputesMedianRangeBoundsAndAverageGap)
{
  LogPlot plot;
  ASSERT_TRUE(plot.setValue(0, 8));
  ASSERT_TRUE(plot.setValue(3, -2));
  ASSERT_TRUE(plot.setValue(9, 12));
  ASSERT_TRUE(plot.setValue(12, 4));

  EXPECT_DOUBLE_EQ(plot.getMedian(), 8);
  EXPECT_DOUBLE_EQ(plot.getMedian(), 8);
  EXPECT_DOUBLE_EQ(plot.getMinVal(2, 10), -2);
  EXPECT_DOUBLE_EQ(plot.getMaxVal(2, 10), 12);
  EXPECT_DOUBLE_EQ(plot.getMinVal(20, 30), 0);
  EXPECT_DOUBLE_EQ(plot.getMaxVal(20, 30), 0);
  EXPECT_DOUBLE_EQ(plot.getAvgTimeGap(), 3);
  EXPECT_DOUBLE_EQ(plot.getAvgTimeGap(), 3);
}

// Covers log plot behavior: applies skew and value offsets used for log alignment.
TEST(LogPlotTest, AppliesSkewAndValueOffsetsUsedForLogAlignment)
{
  LogPlot plot;
  ASSERT_TRUE(plot.setValue(10, 90));
  ASSERT_TRUE(plot.setValue(20, 180));

  plot.applySkew(-10);
  EXPECT_DOUBLE_EQ(plot.getTimeByIndex(0), 0);
  EXPECT_DOUBLE_EQ(plot.getTimeByIndex(1), 10);
  EXPECT_DOUBLE_EQ(plot.getValueByTime(5, true), 135);

  plot.modValues(5);
  EXPECT_DOUBLE_EQ(plot.getValueByIndex(0), 95);
  EXPECT_DOUBLE_EQ(plot.getValueByIndex(1), 185);
}

// Covers log plot behavior: round trips compact spec for scope and plot transfer.
TEST(LogPlotTest, RoundTripsCompactSpecForScopeAndPlotTransfer)
{
  LogPlot plot;
  plot.setVName("henry");
  plot.setVarName("DESIRED_HEADING");
  ASSERT_TRUE(plot.setValue(0, 90.1254));
  ASSERT_TRUE(plot.setValue(1.25, 91.5));

  EXPECT_EQ(plot.getSpec(2, 1),
            "vname=henry,varname=DESIRED_HEADING,elements=0.00:90.1#1.25:91.5");

  LogPlot parsed;
  EXPECT_TRUE(parsed.setFromSpec(plot.getSpec()));
  EXPECT_EQ(parsed.getVName(), "henry");
  EXPECT_EQ(parsed.getVarName(), "DESIRED_HEADING");
  EXPECT_EQ(parsed.size(), 2u);
  EXPECT_DOUBLE_EQ(parsed.getValueByTime(0), 90.125);
  EXPECT_DOUBLE_EQ(parsed.getValueByTime(1.25), 91.5);

  EXPECT_FALSE(parsed.setFromSpec("vname=abe,elements=0:1"));
  EXPECT_FALSE(parsed.setFromSpec("varname=NAV_X,elements=2:20#1:10"));
  EXPECT_FALSE(parsed.setFromSpec("varname=NAV_X,elements=bad:10"));
}

// Covers var plot behavior: stores string posts with per entry sources and aux channels.
TEST(VarPlotTest, StoresStringPostsWithPerEntrySourcesAndAuxChannels)
{
  VarPlot plot;
  plot.setVName("abe");
  plot.setVarName("HELM_MODE");

  EXPECT_TRUE(plot.setValue(1.0, "DRIVE", "pHelmIvP:iter=4"));
  EXPECT_TRUE(plot.setValue(2.0, "SURVEY", "pHelmIvP:iter=5"));
  EXPECT_FALSE(plot.setValue(1.5, "STALE", "pHelmIvP"));

  EXPECT_EQ(plot.size(), 2u);
  EXPECT_EQ(plot.getVarName(), "HELM_MODE");
  EXPECT_EQ(plot.getEntryByIndex(0), "DRIVE");
  EXPECT_EQ(plot.getSourceByIndex(0), "pHelmIvP");
  EXPECT_EQ(plot.getEntryByIndex(99), "");
  EXPECT_EQ(plot.getSourceByIndex(99), "");
  EXPECT_DOUBLE_EQ(plot.getTStampByIndex(99), 0);
  EXPECT_EQ(plot.getMaxLenSource(), 8u);
  EXPECT_EQ(plot.getMaxLenSrcAux(), 6u);
}

// Covers var plot behavior: looks up stepwise entries and partitions around current time.
TEST(VarPlotTest, LooksUpStepwiseEntriesAndPartitionsAroundCurrentTime)
{
  VarPlot plot;
  ASSERT_TRUE(plot.setValue(1.0, "DEPLOY=false", "pMarineViewer"));
  ASSERT_TRUE(plot.setValue(3.0, "DEPLOY=true", "pMarineViewer"));
  ASSERT_TRUE(plot.setValue(7.0, "RETURN=true", "uTimerScript"));

  EXPECT_FALSE(plot.containsTime(0.9));
  EXPECT_TRUE(plot.containsTime(1.0));
  EXPECT_TRUE(plot.containsTime(5.0));
  EXPECT_FALSE(plot.containsTime(8.0));

  EXPECT_EQ(plot.getEntryByTime(0.9), "");
  EXPECT_EQ(plot.getEntryByTime(5.0), "DEPLOY=true");
  EXPECT_EQ(plot.getSourceByTime(5.0), "pMarineViewer");
  EXPECT_DOUBLE_EQ(plot.getTStampByTime(5.0), 3.0);

  EXPECT_EQ(plot.getEntriesUpToTime(3.0),
            std::vector<std::string>({"DEPLOY=false", "DEPLOY=true"}));
  EXPECT_EQ(plot.getSourcesUpToTime(3.0),
            std::vector<std::string>({"pMarineViewer", "pMarineViewer"}));
  EXPECT_EQ(plot.getTStampsUpToTime(3.0),
            std::vector<double>({1.0, 3.0}));

  EXPECT_EQ(plot.getEntriesPastTime(3.0),
            std::vector<std::string>({"RETURN=true"}));
  EXPECT_EQ(plot.getSourcesPastTime(3.0),
            std::vector<std::string>({"uTimerScript"}));
  EXPECT_EQ(plot.getTStampsPastTime(3.0),
            std::vector<double>({7.0}));
}

// Covers var plot behavior: handles before first and after last partition edges.
TEST(VarPlotTest, HandlesBeforeFirstAndAfterLastPartitionEdges)
{
  VarPlot plot;
  ASSERT_TRUE(plot.setValue(10.0, "alpha", "pA"));
  ASSERT_TRUE(plot.setValue(20.0, "bravo", "pB"));

  EXPECT_TRUE(plot.getEntriesUpToTime(9.9).empty());
  EXPECT_TRUE(plot.getSourcesUpToTime(9.9).empty());
  EXPECT_TRUE(plot.getTStampsUpToTime(9.9).empty());

  EXPECT_EQ(plot.getEntriesPastTime(9.9),
            std::vector<std::string>({"alpha", "bravo"}));
  EXPECT_EQ(plot.getSourcesPastTime(9.9),
            std::vector<std::string>({"pA", "pB"}));
  EXPECT_EQ(plot.getTStampsPastTime(9.9),
            std::vector<double>({10.0, 20.0}));

  EXPECT_EQ(plot.getEntriesUpToTime(25.0),
            std::vector<std::string>({"alpha", "bravo"}));
  EXPECT_TRUE(plot.getEntriesPastTime(25.0).empty());
}

// Covers var plot behavior: applies global source override for scope views.
TEST(VarPlotTest, AppliesGlobalSourceOverrideForScopeViews)
{
  VarPlot plot;
  plot.setSource("pHelmIvP");
  ASSERT_TRUE(plot.setValue(1.0, "iter=1", "ignored:aux"));
  ASSERT_TRUE(plot.setValue(2.0, "iter=2", "also_ignored"));

  EXPECT_EQ(plot.getSourceByIndex(0), "pHelmIvP");
  EXPECT_EQ(plot.getSourceByTime(1.5), "pHelmIvP");
  EXPECT_EQ(plot.getSourcesUpToTime(2.0),
            std::vector<std::string>({"pHelmIvP", "pHelmIvP"}));
  EXPECT_EQ(plot.getSourcesPastTime(0.0),
            std::vector<std::string>({"pHelmIvP", "pHelmIvP"}));
}

// Covers var plot behavior: returns rich entry objects for formatted scope output.
TEST(VarPlotTest, ReturnsRichEntryObjectsForFormattedScopeOutput)
{
  VarPlot plot;
  plot.setVName("alpha");
  plot.setVarName("NODE_REPORT");
  ASSERT_TRUE(plot.setValue(1.0, "NAME=alpha,X=10,Y=20", "pNodeReporter"));
  ASSERT_TRUE(plot.setValue(2.0, "NAME=alpha,X=15,Y=25", "pNodeReporter:summary"));

  std::list<VarPlotEntry> up_to = plot.getVarPlotEntriesUpToTime(2.0);
  ASSERT_EQ(up_to.size(), 2u);
  EXPECT_EQ(valuesFrom(up_to),
            std::vector<std::string>({"NAME=alpha,X=10,Y=20",
                                      "NAME=alpha,X=15,Y=25"}));
  VarPlotEntry second = up_to.back();
  EXPECT_DOUBLE_EQ(second.getTStamp(), 2.0);
  EXPECT_EQ(second.getVName(), "alpha");
  EXPECT_EQ(second.getVarName(), "NODE_REPORT");
  EXPECT_EQ(second.getSource(), "pNodeReporter");
  EXPECT_EQ(second.getVarSrcAux(), "summary");

  std::list<VarPlotEntry> past = plot.getVarPlotEntriesPastTime(1.0);
  ASSERT_EQ(past.size(), 1u);
  EXPECT_EQ(past.front().getVarVal(), "NAME=alpha,X=15,Y=25");
}

// Covers var plot behavior: applies time skew to all string entries.
TEST(VarPlotTest, AppliesTimeSkewToAllStringEntries)
{
  VarPlot plot;
  ASSERT_TRUE(plot.setValue(100.0, "MODE=DRIVE", "pHelmIvP"));
  ASSERT_TRUE(plot.setValue(105.0, "MODE=SURVEY", "pHelmIvP"));

  plot.applySkew(-100);
  EXPECT_DOUBLE_EQ(plot.getTStampByIndex(0), 0);
  EXPECT_DOUBLE_EQ(plot.getTStampByIndex(1), 5);
  EXPECT_EQ(plot.getEntryByTime(4), "MODE=DRIVE");
}

// Covers var plot entry behavior: formats columns once and clears for regeneration.
TEST(VarPlotEntryTest, FormatsColumnsOnceAndClearsForRegeneration)
{
  VarPlotEntry entry(12.3456);
  entry.setVName("abe");
  entry.setVarName("APP_LOG");
  entry.setVarSource("pHelmIvP");
  entry.setVarSrcAux("iter=9");
  entry.setVarVal("iter=9,log=on_run_state");

  entry.format(2, 8, 4, 8, 6, true, true, true, true);
  EXPECT_EQ(entry.getFormatted(),
            "12.346   abe   APP_LOG  pHelmIvP  iter=9  iter=9,log=on_run_state");

  entry.format(1, 1, 1, 1, 1, false, false, false, false);
  EXPECT_EQ(entry.getFormatted(),
            "12.346   abe   APP_LOG  pHelmIvP  iter=9  iter=9,log=on_run_state");

  entry.clearFormat();
  entry.format(1, 8, 4, 8, 6, false, true, false, false);
  EXPECT_EQ(entry.getFormatted(), "12.346  APP_LOG iter=9,log=on_run_state");
}
