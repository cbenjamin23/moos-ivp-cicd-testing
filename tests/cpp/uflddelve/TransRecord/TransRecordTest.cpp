#include <gtest/gtest.h>

#include "TransRecord.h"

// Source audit note: this suite covers uFldDelve TransRecord's deterministic
// counters, latest-source tracking, rate-frame validation, rate-window math,
// and pruning boundaries. Delver's live MOOS subscriptions and appcast output
// remain outside CTest.

TEST(TransRecordTest, DefaultsStartEmptyWithNamedVariable)
{
  TransRecord record("NODE_REPORT");

  EXPECT_EQ(record.getVarName(), "NODE_REPORT");
  EXPECT_EQ(record.getLatestSource(), "");
  EXPECT_DOUBLE_EQ(record.getMsgCount(), 0);
  EXPECT_DOUBLE_EQ(record.getCharCount(), 0);
  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 0);
}

TEST(TransRecordTest, RejectsNonPositiveRateFramesWithoutChangingExistingFrame)
{
  TransRecord record;
  record.setCurrTime(10);
  record.addMsg(10, "src", 10);

  EXPECT_FALSE(record.setRateFrame(0));
  EXPECT_FALSE(record.setRateFrame(-1));

  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.1);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 1.0);
}

TEST(TransRecordTest, CountsMessagesAndCharsBeforeCurrentTimeIsSet)
{
  TransRecord record;

  record.addMsg(1, "alpha", 7);
  record.addMsg(2, "bravo");

  EXPECT_DOUBLE_EQ(record.getMsgCount(), 2);
  EXPECT_DOUBLE_EQ(record.getCharCount(), 11);
  EXPECT_EQ(record.getLatestSource(), "bravo");
  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 0);
}

TEST(TransRecordTest, AddsMessagesToRateWindowAfterCurrentTimeIsSet)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(5));
  record.setCurrTime(100);

  record.addMsg(98, "alpha", 10);
  record.addMsg(99, "bravo", 15);

  EXPECT_DOUBLE_EQ(record.getMsgCount(), 2);
  EXPECT_DOUBLE_EQ(record.getCharCount(), 25);
  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.4);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 5.0);
  EXPECT_EQ(record.getLatestSource(), "bravo");
}

TEST(TransRecordTest, AddMsgPrunesEntriesOlderThanRateFrame)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(5));
  record.setCurrTime(100);

  record.addMsg(94.9, "old", 10);
  record.addMsg(96, "kept", 20);

  EXPECT_DOUBLE_EQ(record.getMsgCount(), 2);
  EXPECT_DOUBLE_EQ(record.getCharCount(), 30);
  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.2);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 4.0);
}

TEST(TransRecordTest, BoundaryAtExactlyRateFrameIsKept)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(5));
  record.setCurrTime(100);

  record.addMsg(95, "boundary", 9);

  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.2);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 1.8);
}

TEST(TransRecordTest, SetCurrTimePrunesExistingHistoryOutsideWindow)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(10));
  record.setCurrTime(100);
  record.addMsg(95, "old", 10);
  record.addMsg(99, "new", 20);

  record.setCurrTime(110);

  EXPECT_DOUBLE_EQ(record.getMsgCount(), 2);
  EXPECT_DOUBLE_EQ(record.getCharCount(), 30);
  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 0);
}

TEST(TransRecordTest, SetCurrTimeKeepsExistingHistoryAtExactBoundary)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(10));
  record.setCurrTime(100);
  record.addMsg(99.9, "old", 20);
  record.addMsg(100, "boundary", 10);

  record.setCurrTime(110);

  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.1);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 1.0);
}

TEST(TransRecordTest, OutOfOrderTimestampsCanKeepOlderEntryBehindBoundary)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(10));
  record.setCurrTime(100);
  record.addMsg(100, "boundary", 10);
  record.addMsg(99.9, "old", 20);

  record.setCurrTime(110);

  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.2);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 3.0);
}

TEST(TransRecordTest, RateFrameChangeAffectsRatesWithoutImmediatePrune)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(10));
  record.setCurrTime(100);
  record.addMsg(95, "alpha", 10);
  record.addMsg(99, "bravo", 20);

  ASSERT_TRUE(record.setRateFrame(5));

  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0.4);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 6.0);
}

TEST(TransRecordTest, CurrentTimeZeroNeverRecordsRateHistory)
{
  TransRecord record;
  ASSERT_TRUE(record.setRateFrame(5));
  record.setCurrTime(0);
  record.addMsg(-100, "old", 10);
  record.addMsg(-90, "older", 20);

  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0);
  EXPECT_DOUBLE_EQ(record.getCharRate(), 0);

  record.setCurrTime(0);
  EXPECT_DOUBLE_EQ(record.getMsgRate(), 0);
}

TEST(TransRecordTest, EmptySourceStillBecomesLatestSource)
{
  TransRecord record;
  record.setCurrTime(10);
  record.addMsg(10, "alpha", 1);
  record.addMsg(10, "", 1);

  EXPECT_EQ(record.getLatestSource(), "");
  EXPECT_DOUBLE_EQ(record.getMsgCount(), 2);
}
