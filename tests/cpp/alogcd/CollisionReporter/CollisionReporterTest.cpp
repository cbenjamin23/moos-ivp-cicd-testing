#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "CollisionReporter.h"
#include "TestFileUtils.h"

namespace {

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

// Source audit note: this suite covers CollisionReporter's deterministic input
// validation, timestamp output, encounter/near/collision counting, terse mode,
// average/worst reporting, and ENCOUNTER_SUMMARY compatibility against
// app_alogcd.

// Covers alog and timestamp-file validation.
TEST(CollisionReporterTest, ValidatesInputAndTimestampOutput)
{
  TempDir temp("alogcd_validation");
  const std::string alog = temp.writeFile("input.alog", "");

  CollisionReporter reporter;
  EXPECT_FALSE(reporter.setALogFile(temp.filePath("missing.alog")));
  EXPECT_TRUE(reporter.setALogFile(alog));
  EXPECT_TRUE(reporter.setTimeStampFile(""));
  EXPECT_TRUE(reporter.setTimeStampFile(temp.filePath("times.csv")));
  EXPECT_FALSE(reporter.setTimeStampFile(temp.filePath("missing/times.csv")));
}

// Covers event counting, averages, worst collision, and timestamp CSV output.
TEST(CollisionReporterTest, CountsCollisionEventsAndWritesTimestamps)
{
  TempDir temp("alogcd_counts");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"%% LOGSTART 1000.000",
                    "not an alog row",
                    "1.000 ENCOUNTER uFldCollisionDetect 10",
                    "2.000 NEAR_MISS uFldCollisionDetect 4",
                    "3.000 COLLISION uFldCollisionDetect 1.5",
                    "4.000 COLLISION uFldCollisionDetect 0.5"}));
  const std::string tstamp = temp.filePath("times.csv");

  CollisionReporter reporter;
  ASSERT_TRUE(reporter.setALogFile(alog));
  ASSERT_TRUE(reporter.setTimeStampFile(tstamp));

  testing::internal::CaptureStdout();
  ASSERT_TRUE(reporter.handle());
  reporter.printReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(reporter.hadEncounters());
  EXPECT_TRUE(reporter.hadCollisions());
  EXPECT_TRUE(Contains(output, "Encounters:  1  (avg 10"));
  EXPECT_TRUE(Contains(output, "Near Misses: 1  (avg 4"));
  EXPECT_TRUE(Contains(output, "Collisions:  2  (avg 1"));
  EXPECT_TRUE(Contains(output, "Collision Worst: 0.5"));
  EXPECT_EQ(ReadFile(tstamp),
            WithNewlines({"2.000,NEAR_MISS,4.00",
                          "3.000,COLLISION,1.50",
                          "4.000,COLLISION,0.50"}));
}

// Covers terse report mode.
TEST(CollisionReporterTest, TerseReportPrintsSlashSeparatedCounts)
{
  TempDir temp("alogcd_terse");
  const std::string alog = temp.writeFile(
      "input.alog",
      WithNewlines({"1.000 ENCOUNTER uFldCollisionDetect 10",
                    "2.000 NEAR_MISS uFldCollisionDetect 4",
                    "3.000 COLLISION uFldCollisionDetect 1"}));

  CollisionReporter reporter;
  reporter.setTerse();
  ASSERT_TRUE(reporter.setALogFile(alog));
  ASSERT_TRUE(reporter.handle());

  testing::internal::CaptureStdout();
  reporter.printReport();
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "1/1/1\n");
}

// Covers ENCOUNTER_SUMMARY counting. The upstream code applies atof directly
// to the payload, so summary CPA contributes as zero unless the payload starts
// with a number.
TEST(CollisionReporterTest, EncounterSummaryCountsWithCStylePayloadCoercion)
{
  TempDir temp("alogcd_summary");
  const std::string alog = temp.writeFile(
      "input.alog",
      "1.000 ENCOUNTER_SUMMARY uFldCollisionDetect cpa=8,v1=abe,v2=ben\n");

  CollisionReporter reporter;
  ASSERT_TRUE(reporter.setALogFile(alog));
  ASSERT_TRUE(reporter.handle());

  testing::internal::CaptureStdout();
  reporter.printReport();
  const std::string output = testing::internal::GetCapturedStdout();

  EXPECT_TRUE(Contains(output, "Encounters:  1  (avg 0"));
  EXPECT_FALSE(reporter.hadCollisions());
}

