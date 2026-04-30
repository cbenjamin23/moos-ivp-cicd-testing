#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ALogEntry.h"
#include "AppLogEntry.h"
#include "AppLogPlot.h"
#include "Populator_AppLogPlot.h"

namespace {

ALogEntry MakeAlogEntry(double time,
                        const std::string& var,
                        const std::string& src,
                        const std::string& val)
{
  ALogEntry entry;
  entry.set(time, var, src, "", val);
  return entry;
}

}  // namespace

// Covers app log entry behavior: parses canonical app log payload from helm and apps.
TEST(AppLogEntryTest, ParsesCanonicalAppLogPayloadFromHelmAndApps)
{
  // APP_LOG payloads use !@# as the line separator inside the single alog value.
  AppLogEntry entry = stringToAppLogEntry(
      "iter=23,log=BHV_WAYPOINT active!@#objective function built!@#posted DESIRED_HEADING");

  EXPECT_TRUE(entry.valid());
  EXPECT_EQ(entry.getIteration(), 23u);
  EXPECT_EQ(entry.size(), 3u);
  EXPECT_EQ(entry.getLine(0), "BHV_WAYPOINT active");
  EXPECT_EQ(entry.getLine(1), "objective function built");
  EXPECT_EQ(entry.getLine(2), "posted DESIRED_HEADING");
  EXPECT_EQ(entry.getLine(99), "");
  EXPECT_EQ(entry.getLines(true),
            std::vector<std::string>({"[23] BHV_WAYPOINT active",
                                      "[23] objective function built",
                                      "[23] posted DESIRED_HEADING"}));
}

// Covers app log entry behavior: keeps comma rich log text after log field.
TEST(AppLogEntryTest, KeepsCommaRichLogTextAfterLogField)
{
  AppLogEntry entry = stringToAppLogEntry(
      "iter=7,log=mode=SURVEY,helm=active!@#condition=\"x>10,y<20\"!@#post=VIEW_POINT");

  EXPECT_EQ(entry.getIteration(), 7u);
  ASSERT_EQ(entry.size(), 3u);
  EXPECT_EQ(entry.getLine(0), "mode=SURVEY,helm=active");
  EXPECT_EQ(entry.getLine(1), "condition=\"x>10,y<20\"");
  EXPECT_EQ(entry.getLine(2), "post=VIEW_POINT");
}

// Covers app log entry behavior: treats everything after log as application text.
TEST(AppLogEntryTest, TreatsEverythingAfterLogAsApplicationText)
{
  AppLogEntry comma_rich = stringToAppLogEntry(
      "iter=8,log=mode=SURVEY,helm=active,decision=hold station");
  EXPECT_EQ(comma_rich.getIteration(), 8u);
  ASSERT_EQ(comma_rich.size(), 1u);
  EXPECT_EQ(comma_rich.getLine(0),
            "mode=SURVEY,helm=active,decision=hold station");

  AppLogEntry log_first = stringToAppLogEntry(
      "log=first line,iter=99,this remains app text!@#second line");
  EXPECT_EQ(log_first.getIteration(), 0u);
  ASSERT_EQ(log_first.size(), 2u);
  EXPECT_EQ(log_first.getLine(0),
            "first line,iter=99,this remains app text");
  EXPECT_EQ(log_first.getLine(1), "second line");
}

// Covers app log entry behavior: pins iter parsing case whitespace and duplicate semantics.
TEST(AppLogEntryTest, PinsIterParsingCaseWhitespaceAndDuplicateSemantics)
{
  // Pin current parser behavior: iter values use C-style conversion, lowercase
  // keys are required, and a later duplicate iter field wins.
  AppLogEntry decimal = stringToAppLogEntry("iter=12.9,log=decimal iter");
  EXPECT_EQ(decimal.getIteration(), 12u);
  EXPECT_EQ(decimal.getLine(0), "decimal iter");

  AppLogEntry text_iter = stringToAppLogEntry("iter=abc,log=falls back to zero");
  EXPECT_EQ(text_iter.getIteration(), 0u);
  EXPECT_EQ(text_iter.getLine(0), "falls back to zero");

  AppLogEntry duplicate = stringToAppLogEntry("iter=1,iter=42,log=last iter wins");
  EXPECT_EQ(duplicate.getIteration(), 42u);
  EXPECT_EQ(duplicate.getLine(0), "last iter wins");

  AppLogEntry uppercase = stringToAppLogEntry("ITER=3,log=case sensitive");
  EXPECT_EQ(uppercase.getIteration(), 0u);
  EXPECT_EQ(uppercase.size(), 0u);

  AppLogEntry leading_space = stringToAppLogEntry(" iter=3,log=space sensitive");
  EXPECT_EQ(leading_space.getIteration(), 3u);
  EXPECT_EQ(leading_space.getLine(0), "space sensitive");
}

// Covers app log entry behavior: rejects bad parameters and negative iterations as empty null entries.
TEST(AppLogEntryTest, RejectsBadParametersAndNegativeIterationsAsEmptyNullEntries)
{
  AppLogEntry bad_param = stringToAppLogEntry("iter=1,bad=value,log=line");
  EXPECT_EQ(bad_param.getIteration(), 0u);
  EXPECT_EQ(bad_param.size(), 0u);

  AppLogEntry negative = stringToAppLogEntry("iter=-1,log=line");
  EXPECT_EQ(negative.getIteration(), 0u);
  EXPECT_EQ(negative.size(), 0u);
}

// Covers app log entry behavior: removes terminal colors from MOOS app log lines.
TEST(AppLogEntryTest, RemovesTerminalColorsFromMoosAppLogLines)
{
  AppLogEntry entry = stringToAppLogEntry(
      std::string("iter=4,log=\033[91mBHV_AVOID active\033[0m!@#") +
      "\033[32mVIEW_SEGLIST posted\033[0m");

  EXPECT_EQ(entry.getIteration(), 4u);
  ASSERT_EQ(entry.size(), 2u);
  EXPECT_EQ(entry.getLine(0), "BHV_AVOID active");
  EXPECT_EQ(entry.getLine(1), "VIEW_SEGLIST posted");
}

// Covers app log entry behavior: truncates and wraps long application log lines.
TEST(AppLogEntryTest, TruncatesAndWrapsLongApplicationLogLines)
{
  const std::string long_line =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
      "abcdefghijklmnopqrstuvwxyz";

  AppLogEntry entry = stringToAppLogEntry("iter=2,log=" + long_line);
  EXPECT_EQ(entry.size(), 1u);
  EXPECT_EQ(entry.getTruncLine(0).size(), 80u);
  EXPECT_EQ(entry.getTruncLine(99), "");

  std::vector<std::string> trunc_lines = entry.getTruncLines();
  ASSERT_EQ(trunc_lines.size(), 1u);
  EXPECT_EQ(trunc_lines[0], entry.getTruncLine(0));

  std::vector<std::string> wrapped = entry.getWrapLines();
  ASSERT_GE(wrapped.size(), 2u);
  for(const std::string& line : wrapped)
    EXPECT_LE(line.size(), 70u);
}

// Covers app log entry behavior: pins truncate and wrap boundary lengths.
TEST(AppLogEntryTest, PinsTruncateAndWrapBoundaryLengths)
{
  // The viewer model depends on two separate limits: 80 chars for truncation
  // and 70 chars for wrapped display lines.
  const std::string seventy(70, 'a');
  const std::string eighty(80, 'b');
  const std::string eighty_one(81, 'c');
  AppLogEntry entry =
      stringToAppLogEntry("iter=6,log=" + seventy + "!@#" + eighty + "!@#" + eighty_one);

  ASSERT_EQ(entry.size(), 3u);
  EXPECT_EQ(entry.getTruncLine(1), eighty);
  EXPECT_EQ(entry.getTruncLine(2), std::string(80, 'c'));

  std::vector<std::string> wrapped = entry.getWrapLines();
  ASSERT_EQ(wrapped.size(), 5u);
  EXPECT_EQ(wrapped[0], seventy);
  EXPECT_EQ(wrapped[1], std::string(70, 'b'));
  EXPECT_EQ(wrapped[2], std::string(10, 'b'));
  EXPECT_EQ(wrapped[3], std::string(70, 'c'));
  EXPECT_EQ(wrapped[4], std::string(11, 'c'));
}

// Covers app log plot behavior: stores and retrieves entries by index and time.
TEST(AppLogPlotTest, StoresAndRetrievesEntriesByIndexAndTime)
{
  AppLogEntry first = stringToAppLogEntry("iter=1,log=helm started");
  AppLogEntry second = stringToAppLogEntry("iter=2,log=bhv waypoint active");
  AppLogEntry third = stringToAppLogEntry("iter=3,log=bhv loiter active");

  AppLogPlot plot;
  plot.setVName("abe");
  plot.setAppName("pHelmIvP");
  plot.addAppLogEntry(10, first);
  plot.addAppLogEntry(20, second);
  plot.addAppLogEntry(30, third);

  EXPECT_EQ(plot.getAppName(), "pHelmIvP");
  EXPECT_EQ(plot.size(), 3u);
  EXPECT_FALSE(plot.containsTime(9.9));
  EXPECT_TRUE(plot.containsTime(10));
  EXPECT_TRUE(plot.containsTime(25));
  EXPECT_FALSE(plot.containsTime(30.1));
  EXPECT_EQ(plot.getEntryByIndex(1).getIteration(), 2u);
  EXPECT_EQ(plot.getEntryByIndex(99).size(), 0u);
  EXPECT_EQ(plot.getEntryByTime(25).getIteration(), 2u);
  EXPECT_EQ(plot.getEntryByTime(9).size(), 0u);
}

// Covers app log plot behavior: windows entries around current time with fifty entry limit.
TEST(AppLogPlotTest, WindowsEntriesAroundCurrentTimeWithFiftyEntryLimit)
{
  AppLogPlot plot;
  plot.setAppName("pHelmIvP");
  for(unsigned int i = 0; i < 60; ++i) {
    AppLogEntry entry = stringToAppLogEntry("iter=" + std::to_string(i) + ",log=line");
    plot.addAppLogEntry(static_cast<double>(i), entry);
  }

  std::vector<AppLogEntry> up_to = plot.getEntriesUpToTime(55);
  // AppLogScope intentionally caps each side of the current-time window at 51
  // entries, matching the viewer's bounded scroll context.
  ASSERT_EQ(up_to.size(), 51u);
  EXPECT_EQ(up_to.front().getIteration(), 5u);
  EXPECT_EQ(up_to.back().getIteration(), 55u);

  std::vector<AppLogEntry> past = plot.getEntriesPastTime(8);
  ASSERT_EQ(past.size(), 49u);
  EXPECT_EQ(past.front().getIteration(), 9u);
  EXPECT_EQ(past.back().getIteration(), 57u);

  EXPECT_TRUE(plot.getEntriesUpToTime(-1).empty());
  EXPECT_TRUE(plot.getEntriesPastTime(99).empty());
}

// Covers app log plot behavior: pins inclusive time bounds and duplicate timestamp lookup.
TEST(AppLogPlotTest, PinsInclusiveTimeBoundsAndDuplicateTimestampLookup)
{
  AppLogPlot plot;
  plot.setAppName("pHelmIvP");
  plot.addAppLogEntry(10.0, stringToAppLogEntry("iter=10,log=start"));
  plot.addAppLogEntry(20.0, stringToAppLogEntry("iter=20,log=first at duplicate time"));
  plot.addAppLogEntry(20.0, stringToAppLogEntry("iter=21,log=second at duplicate time"));
  plot.addAppLogEntry(30.0, stringToAppLogEntry("iter=30,log=end"));

  EXPECT_TRUE(plot.containsTime(10.0));
  EXPECT_TRUE(plot.containsTime(30.0));
  EXPECT_EQ(plot.getEntryByTime(10.0).getIteration(), 10u);
  EXPECT_EQ(plot.getEntryByTime(19.999).getIteration(), 10u);
  // Duplicate timestamps return the most recently inserted entry at that time.
  EXPECT_EQ(plot.getEntryByTime(20.0).getIteration(), 21u);
  EXPECT_EQ(plot.getEntryByTime(29.999).getIteration(), 21u);
  EXPECT_EQ(plot.getEntryByTime(30.0).getIteration(), 30u);

  std::vector<AppLogEntry> up_to_start = plot.getEntriesUpToTime(10.0);
  ASSERT_EQ(up_to_start.size(), 1u);
  EXPECT_EQ(up_to_start.front().getIteration(), 10u);
  EXPECT_TRUE(plot.getEntriesPastTime(30.0).empty());
}

// Covers app log plot behavior: pins viewer window limits at early middle and late times.
TEST(AppLogPlotTest, PinsViewerWindowLimitsAtEarlyMiddleAndLateTimes)
{
  AppLogPlot plot;
  for(unsigned int i = 0; i < 101; ++i)
    plot.addAppLogEntry(i * 0.5,
                        stringToAppLogEntry("iter=" + std::to_string(i) +
                                            ",log=app iteration"));

  std::vector<AppLogEntry> early = plot.getEntriesUpToTime(1.25);
  ASSERT_EQ(early.size(), 3u);
  EXPECT_EQ(early.front().getIteration(), 0u);
  EXPECT_EQ(early.back().getIteration(), 2u);

  std::vector<AppLogEntry> middle_past = plot.getEntriesPastTime(25.0);
  ASSERT_EQ(middle_past.size(), 49u);
  EXPECT_EQ(middle_past.front().getIteration(), 51u);
  EXPECT_EQ(middle_past.back().getIteration(), 99u);

  std::vector<AppLogEntry> late_up_to = plot.getEntriesUpToTime(50.0);
  ASSERT_EQ(late_up_to.size(), 51u);
  EXPECT_EQ(late_up_to.front().getIteration(), 50u);
  EXPECT_EQ(late_up_to.back().getIteration(), 100u);
  EXPECT_TRUE(plot.getEntriesPastTime(50.0).empty());
}

// Covers populator app log plot behavior: populates from alog entries for single application source.
TEST(PopulatorAppLogPlotTest, PopulatesFromAlogEntriesForSingleApplicationSource)
{
  std::vector<ALogEntry> entries;

  ALogEntry ignored;
  ignored.set(1.0, "NAV_X", "pNodeReporter", "", 10.0);
  entries.push_back(ignored);

  ALogEntry first;
  first.set(2.0, "APP_LOG", "pHelmIvP", "", "iter=1,log=helm start!@#build active set");
  entries.push_back(first);

  ALogEntry second;
  second.set(3.0, "APP_LOG", "pHelmIvP", "", "iter=2,log=waypoint complete");
  entries.push_back(second);

  Populator_AppLogPlot populator;
  EXPECT_TRUE(populator.populateFromEntries(entries));

  AppLogPlot plot = populator.getAppLogPlot();
  EXPECT_EQ(plot.getAppName(), "pHelmIvP");
  EXPECT_EQ(plot.size(), 2u);
  EXPECT_EQ(plot.getEntryByTime(2.5).getIteration(), 1u);
  EXPECT_EQ(plot.getEntryByTime(3.0).getLine(0), "waypoint complete");
}

// Covers populator app log plot behavior: populates from split klog style app log entries.
TEST(PopulatorAppLogPlotTest, PopulatesFromSplitKlogStyleAppLogEntries)
{
  std::vector<ALogEntry> entries;
  entries.push_back(MakeAlogEntry(
      100.0, "APP_LOG_pHelmIvP", "pHelmIvP",
      "iter=0,log=split-file variable names are ignored by this populator"));
  entries.push_back(MakeAlogEntry(
      101.0, "APP_LOG", "pHelmIvP",
      "iter=1,log=behavior=BHV_WAYPOINT,mode=SURVEY!@#post=VIEW_POINT"));
  entries.push_back(MakeAlogEntry(
      102.0, "APP_LOG", "pHelmIvP",
      "iter=2,log=behavior=BHV_RETURN,mode=RETURNING"));

  Populator_AppLogPlot populator;
  EXPECT_TRUE(populator.populateFromEntries(entries));

  AppLogPlot plot = populator.getAppLogPlot();
  EXPECT_EQ(plot.getAppName(), "pHelmIvP");
  ASSERT_EQ(plot.size(), 2u);
  EXPECT_EQ(plot.getEntryByTime(101.5).getIteration(), 1u);
  EXPECT_EQ(plot.getEntryByTime(101.5).getLine(0),
            "behavior=BHV_WAYPOINT,mode=SURVEY");
  EXPECT_EQ(plot.getEntryByTime(102.0).getLine(0),
            "behavior=BHV_RETURN,mode=RETURNING");
}

// Covers populator app log plot behavior: keeps malformed app log payloads as empty entries.
TEST(PopulatorAppLogPlotTest, KeepsMalformedAppLogPayloadsAsEmptyEntries)
{
  std::vector<ALogEntry> entries;
  entries.push_back(MakeAlogEntry(10.0, "APP_LOG", "pMarinePID",
                                  "iter=1,log=pid begins"));
  entries.push_back(MakeAlogEntry(11.0, "APP_LOG", "pMarinePID",
                                  "iter=-1,log=bad iter"));
  entries.push_back(MakeAlogEntry(12.0, "APP_LOG", "pMarinePID",
                                  "iter=2,unknown=value,log=bad param"));

  Populator_AppLogPlot populator;
  EXPECT_TRUE(populator.populateFromEntries(entries));

  AppLogPlot plot = populator.getAppLogPlot();
  ASSERT_EQ(plot.size(), 3u);
  EXPECT_EQ(plot.getEntryByIndex(0).getIteration(), 1u);
  EXPECT_EQ(plot.getEntryByIndex(1).getIteration(), 0u);
  EXPECT_EQ(plot.getEntryByIndex(1).size(), 0u);
  EXPECT_EQ(plot.getEntryByIndex(2).getIteration(), 0u);
  EXPECT_EQ(plot.getEntryByIndex(2).size(), 0u);
}

// Covers populator app log plot behavior: succeeds with only non app log entries but leaves empty plot.
TEST(PopulatorAppLogPlotTest, SucceedsWithOnlyNonAppLogEntriesButLeavesEmptyPlot)
{
  std::vector<ALogEntry> entries;
  entries.push_back(MakeAlogEntry(1.0, "NAV_X", "pNodeReporter", "14.1"));
  entries.push_back(MakeAlogEntry(2.0, "IVPHELM_STATE", "pHelmIvP", "DRIVE"));

  Populator_AppLogPlot populator;
  EXPECT_TRUE(populator.populateFromEntries(entries));

  AppLogPlot plot = populator.getAppLogPlot();
  EXPECT_EQ(plot.getAppName(), "");
  EXPECT_EQ(plot.size(), 0u);
  EXPECT_FALSE(plot.containsTime(1.5));
}

// Covers populator app log plot behavior: rejects empty input and mismatched application sources.
TEST(PopulatorAppLogPlotTest, RejectsEmptyInputAndMismatchedApplicationSources)
{
  Populator_AppLogPlot empty;
  EXPECT_FALSE(empty.populateFromEntries({}));

  ALogEntry helm;
  helm.set(1.0, "APP_LOG", "pHelmIvP", "", "iter=1,log=helm");
  ALogEntry pid;
  pid.set(2.0, "APP_LOG", "pMarinePID", "", "iter=2,log=pid");

  Populator_AppLogPlot mixed;
  EXPECT_FALSE(mixed.populateFromEntries({helm, pid}));
}
