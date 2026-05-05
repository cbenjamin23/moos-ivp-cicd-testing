#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "HelmReporter.h"
#include "TestFileUtils.h"

namespace {

std::string WithNewlines(const std::vector<std::string>& lines)
{
  std::string contents;
  for(const std::string& line : lines)
    contents += line + "\n";
  return contents;
}

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers HelmReporter's deterministic alog
// validation, watch-variable output, mode change reporting, life-event capture,
// and report surfaces against app_aloghelm. HelmReport parsing itself is
// covered in the helmivp library family.

// Covers input validation for empty and missing alog paths.
TEST(HelmReporterTest, RejectsEmptyAndMissingAlogPaths)
{
  TempDir temp("aloghelm_validation");
  HelmReporter reporter;
  reporter.setUseColor(false);

  testing::internal::CaptureStdout();
  EXPECT_FALSE(reporter.handle(""));
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Alog file was not specified"));

  testing::internal::CaptureStdout();
  EXPECT_FALSE(reporter.handle(temp.filePath("missing.alog")));
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "Alog file not found"));
}

// Covers watch-variable output and whitespace validation in addWatchVar.
TEST(HelmReporterTest, WatchVariablesEmitMatchingRowsOnly)
{
  TempDir temp("aloghelm_watch");
  const std::string long_line =
      "1.000 NAV_X pNodeReporter 1234567890123456789012345678901234567890";
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    long_line,
                    "2.000 NAV_Y pNodeReporter 20",
                    "3.000 BAD_VAR pLogger ignored"}));

  HelmReporter reporter;
  reporter.setUseColor(false);
  reporter.setVarTrunc(false);
  reporter.addWatchVar("NAV_X");
  reporter.addWatchVar("BAD VAR");
  reporter.addWatchVar("NAV_X");

  testing::internal::CaptureStdout();
  EXPECT_TRUE(reporter.handle(alog));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, long_line));
  EXPECT_FALSE(Contains(output, "NAV_Y"));
  EXPECT_FALSE(Contains(output, "BAD_VAR"));
  EXPECT_TRUE(Contains(output, "lines total"));
}

// Covers default truncation for watch-variable lines.
TEST(HelmReporterTest, WatchVariablesTruncateLongRowsByDefault)
{
  TempDir temp("aloghelm_trunc");
  const std::string long_payload(120, 'x');
  const std::string alog = temp.writeFile(
      "input.alog", "1.000 NAV_X pNodeReporter " + long_payload + "\n");

  HelmReporter reporter;
  reporter.setUseColor(false);
  reporter.addWatchVar("NAV_X");

  testing::internal::CaptureStdout();
  EXPECT_TRUE(reporter.handle(alog));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_EQ(output.find(std::string(100, 'x')), std::string::npos);
  EXPECT_EQ(output.find(long_payload), std::string::npos);
}

// Covers IVPHELM_MODESET tracking and reporting only changed mode values.
TEST(HelmReporterTest, ReportsModeChangesOnlyWhenValueChanges)
{
  TempDir temp("aloghelm_modes");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 IVPHELM_MODESET pHelmIvP MODE#ACTIVE",
                    "2.000 MODE pHelmIvP LOITER",
                    "3.000 MODE pHelmIvP LOITER",
                    "4.000 MODE pHelmIvP RETURN"}));

  HelmReporter reporter;
  reporter.setUseColor(false);
  reporter.reportModeChanges(true);

  testing::internal::CaptureStdout();
  EXPECT_TRUE(reporter.handle(alog));
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "2.000 Mode: LOITER"));
  EXPECT_TRUE(Contains(output, "4.000 Mode: RETURN"));
  EXPECT_EQ(output.find("LOITER"), output.rfind("LOITER"));
}

// Covers life-event ingestion during handle() and report printing afterward.
TEST(HelmReporterTest, CollectsLifeEventsAndPrintsReport)
{
  TempDir temp("aloghelm_life_events");
  const std::string event =
      "time=100.25,iter=4,bname=waypt,btype=BHV_Waypoint,event=spawn,"
      "seed=helm startup";
  const std::string alog = temp.writeFile(
      "input.alog", "1.000 IVPHELM_LIFE_EVENT pHelmIvP " + event + "\n");

  HelmReporter reporter;
  reporter.setUseColor(false);
  reporter.setColorActive(false);
  reporter.reportLifeEvents(true);

  testing::internal::CaptureStdout();
  EXPECT_TRUE(reporter.handle(alog));
  reporter.printReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "1 life events."));
  EXPECT_TRUE(Contains(output, "waypt"));
  EXPECT_TRUE(Contains(output, "BHV_Waypoint"));
  EXPECT_TRUE(Contains(output, "helm startup"));
}
