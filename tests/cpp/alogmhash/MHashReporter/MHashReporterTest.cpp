#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "MHashReporter.h"
#include "TestFileUtils.h"

namespace {

class TestableMHashReporter : public MHashReporter {
 public:
  using MHashReporter::getMaxOdoMHash;
  using MHashReporter::getXHash;
  void setStateForXHash(const std::string& vname, double duration, double odometry)
  {
    m_vname = vname;
    m_curr_time = duration;
    m_odometry = odometry;
  }
  void setMHashIn(const std::string& value) { m_mhash_in = value; }
};

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

// Source audit note: this suite covers MHashReporter's deterministic alog
// validation, odometry accumulation, report modes, node-name extraction, xhash
// formatting, and mhash selection against app_alogmhash.

// Covers missing alog handling.
TEST(MHashReporterTest, RejectsMissingAndUnsetAlogFile)
{
  TempDir temp("alogmhash_missing");

  MHashReporter missing;
  EXPECT_FALSE(missing.setALogFile(temp.filePath("missing.alog")));

  MHashReporter unset;
  testing::internal::CaptureStdout();
  EXPECT_FALSE(unset.handle());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(),
                       "No alog file provided"));
}

// Covers all report fields over odometry grouped by mission hash.
TEST(MHashReporterTest, ReportAllUsesMHashWithGreatestOdometry)
{
  TempDir temp("alogmhash_all");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 123.456",
                    "0.000 DB_UPTIME MOOSDB_abe 0",
                    "0.000 MISSION_HASH pLogger mhash=H1,utc=123.456",
                    "0.000 NAV_X pNodeReporter 0",
                    "0.000 NAV_Y pNodeReporter 0",
                    "1.000 NAV_X pNodeReporter 3",
                    "2.000 NAV_Y pNodeReporter 4",
                    "2.100 MISSION_HASH pLogger mhash=H2,utc=124.000",
                    "3.000 NAV_X pNodeReporter 6"}));

  MHashReporter reporter;
  reporter.setReportAll();
  ASSERT_TRUE(reporter.setALogFile(alog));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(reporter.handle());
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "mhash=H1"));
  EXPECT_TRUE(Contains(output, "odo=10"));
  EXPECT_TRUE(Contains(output, "duration=3"));
  EXPECT_TRUE(Contains(output, "name=abe"));
  EXPECT_TRUE(Contains(output, "utc=123"));
  EXPECT_TRUE(Contains(output, "xhash=ABE-003S-010M"));
}

// Covers individual report modes and terse newline suppression.
TEST(MHashReporterTest, IndividualReportModesUseParsedState)
{
  TempDir temp("alogmhash_modes");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 10.250",
                    "0.000 DB_UPTIME MOOSDB_shoreside 0",
                    "0.000 MISSION_HASH pLogger mhash=SHORE,utc=10.250",
                    "2.000 DB_UPTIME MOOSDB_shoreside 2"}));

  MHashReporter name;
  name.setReportName();
  ASSERT_TRUE(name.setALogFile(alog));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(name.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "shore\n");

  MHashReporter utc;
  utc.setReportUTC();
  utc.setTerse();
  ASSERT_TRUE(utc.setALogFile(alog));
  testing::internal::CaptureStdout();
  ASSERT_TRUE(utc.handle());
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "10.25");
}

// Covers direct xhash formatting from reporter state.
TEST(MHashReporterTest, DirectXHashFormattingUsesReporterState)
{
  TestableMHashReporter reporter;
  reporter.setStateForXHash("abe", 12.4, 7.5);

  EXPECT_EQ(reporter.getXHash(), "ABE-012S-008M");
}

// Covers xhash padding and the current duration clamp quirk: the local
// duration variable is clamped, but the formatted string still uses curr_time.
TEST(MHashReporterTest, XHashPadsValuesAndPinsDurationClampQuirk)
{
  TestableMHashReporter reporter;
  reporter.setStateForXHash("ben", 1001, 1001);

  EXPECT_EQ(reporter.getXHash(), "BEN-1001S-999M");
}
