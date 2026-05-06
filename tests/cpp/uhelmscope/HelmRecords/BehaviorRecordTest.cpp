#include <gtest/gtest.h>

#include <string>

#include "BehaviorRecord.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers uHelmScope BehaviorRecord value/state
// formatting directly. HelmScope's live mail handling and appcast display stay
// outside CTest.

TEST(BehaviorRecordTest, DefaultsSummarizeAsAlways)
{
  BehaviorRecord record;

  EXPECT_DOUBLE_EQ(record.getTimeStamp(), 0);
  EXPECT_EQ(record.getSummary(), " [always]");
}

TEST(BehaviorRecordTest, SettersRoundTripStoredFields)
{
  BehaviorRecord record;
  record.setName("waypoint");
  record.setIPFCount("7");
  record.setPriority("100");
  record.setTimeCPU("0.02");
  record.setPieces("3");
  record.setUpdates("2/3");
  record.setOriginal("BHV_Waypoint");

  EXPECT_EQ(record.getName(), "waypoint");
  EXPECT_EQ(record.getIPFCount(), "7");
  EXPECT_EQ(record.getPriority(), "100");
  EXPECT_EQ(record.getTimeCPU(), "0.02");
  EXPECT_EQ(record.getPieces(), "3");
  EXPECT_EQ(record.getUpdates(), "2/3");
  EXPECT_EQ(record.getOriginal(), "BHV_Waypoint");
}

TEST(BehaviorRecordTest, ZeroTimestampSummarizesAsAlways)
{
  BehaviorRecord record;
  record.setName("loiter");
  record.setTimeStamp(0);

  EXPECT_EQ(record.getSummary(100), "loiter [always]");
}

TEST(BehaviorRecordTest, PositiveTimestampUsesElapsedWhenUtcIsLater)
{
  BehaviorRecord record;
  record.setName("waypoint");
  record.setTimeStamp(10);

  EXPECT_TRUE(Contains(record.getSummary(12.5), "waypoint [2.50]"));
}

TEST(BehaviorRecordTest, PositiveTimestampUsesTimestampWhenUtcIsEarlier)
{
  BehaviorRecord record;
  record.setName("waypoint");
  record.setTimeStamp(10);

  EXPECT_TRUE(Contains(record.getSummary(9), "waypoint [10.00]"));
}

TEST(BehaviorRecordTest, SummaryIncludesOnlyConfiguredMetrics)
{
  BehaviorRecord record;
  record.setName("station");
  record.setPriority("80");
  record.setPieces("5");
  record.setTimeCPU("0.03");
  record.setUpdates("1/2");

  const std::string summary = record.getSummary();

  EXPECT_TRUE(Contains(summary, "(pwt=80)"));
  EXPECT_TRUE(Contains(summary, "(pcs=5)"));
  EXPECT_TRUE(Contains(summary, "(cpu=0.03)"));
  EXPECT_TRUE(Contains(summary, "(upd=1/2)"));
}
