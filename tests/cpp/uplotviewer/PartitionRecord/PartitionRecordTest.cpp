#include <gtest/gtest.h>

#include "PartitionRecord.h"

// Source audit note: this suite covers uPlotViewer PartitionRecord's pure
// histogram/stat state: initialization guards, range filtering, running
// averages, partition boundaries, stored values, color/name setters, and the
// non-zero-low partition-index behavior. GUI/viewer rendering remains outside
// CTest.

TEST(PartitionRecordTest, DefaultsUseSinglePartitionAndLightBlueColor)
{
  PartitionRecord record;

  EXPECT_EQ(record.getPartitions(), 1u);
  EXPECT_EQ(record.getTotalEntries(), 0u);
  EXPECT_DOUBLE_EQ(record.getLowVal(), 0);
  EXPECT_DOUBLE_EQ(record.getHighVal(), 0);
  EXPECT_DOUBLE_EQ(record.getAverage(), 0);
  EXPECT_EQ(record.getEntryCount(0), 0u);
  EXPECT_EQ(record.getValue(0), 0);
  EXPECT_TRUE(record.getColorPack().set());
}

TEST(PartitionRecordTest, InitRejectsInvalidRangeAndZeroPartitions)
{
  PartitionRecord record;
  record.init(0, 10, 5);

  record.init(10, 0, 5);
  EXPECT_EQ(record.getPartitions(), 5u);
  EXPECT_DOUBLE_EQ(record.getLowVal(), 0);
  EXPECT_DOUBLE_EQ(record.getHighVal(), 10);

  record.init(0, 20, 0);
  EXPECT_EQ(record.getPartitions(), 5u);
  EXPECT_DOUBLE_EQ(record.getHighVal(), 10);
}

TEST(PartitionRecordTest, InitClearsEntriesAndResetsPartitionsButKeepsAverage)
{
  PartitionRecord record;
  record.init(0, 10, 5);
  record.addValue(2);
  record.addValue(4);
  ASSERT_DOUBLE_EQ(record.getAverage(), 3);

  record.init(0, 20, 4);

  EXPECT_EQ(record.getPartitions(), 4u);
  EXPECT_EQ(record.getTotalEntries(), 0u);
  EXPECT_DOUBLE_EQ(record.getAverage(), 3);
  EXPECT_EQ(record.getEntryCount(0), 0u);
}

TEST(PartitionRecordTest, AddsValuesAndUpdatesRunningAverage)
{
  PartitionRecord record;
  record.init(0, 10, 5);

  record.addValue(1);
  record.addValue(3);
  record.addValue(5);

  EXPECT_EQ(record.getTotalEntries(), 3u);
  EXPECT_DOUBLE_EQ(record.getAverage(), 3);
  EXPECT_DOUBLE_EQ(record.getValue(0), 1);
  EXPECT_DOUBLE_EQ(record.getValue(1), 3);
  EXPECT_DOUBLE_EQ(record.getValue(2), 5);
}

TEST(PartitionRecordTest, OutOfRangeValuesDoNotAffectEntriesOrAverage)
{
  PartitionRecord record;
  record.init(0, 10, 5);
  record.addValue(4);
  record.addValue(-0.1);
  record.addValue(10.1);

  EXPECT_EQ(record.getTotalEntries(), 1u);
  EXPECT_DOUBLE_EQ(record.getAverage(), 4);
  EXPECT_EQ(record.getEntryCount(2), 1u);
}

TEST(PartitionRecordTest, PartitionsValuesByDeltaForZeroBasedRange)
{
  PartitionRecord record;
  record.init(0, 10, 5);

  record.addValue(0);
  record.addValue(1.99);
  record.addValue(2);
  record.addValue(9.99);

  EXPECT_EQ(record.getEntryCount(0), 2u);
  EXPECT_EQ(record.getEntryCount(1), 1u);
  EXPECT_EQ(record.getEntryCount(4), 1u);
}

TEST(PartitionRecordTest, HighBoundaryIsStoredButNotCountedInAnyPartition)
{
  PartitionRecord record;
  record.init(0, 10, 5);

  record.addValue(10);

  EXPECT_EQ(record.getTotalEntries(), 1u);
  EXPECT_DOUBLE_EQ(record.getAverage(), 10);
  for(unsigned int ix = 0; ix < record.getPartitions(); ++ix)
    EXPECT_EQ(record.getEntryCount(ix), 0u);
}

TEST(PartitionRecordTest, NonZeroLowRangeUsesRawValueForPartitionIndex)
{
  PartitionRecord record;
  record.init(10, 20, 5);

  record.addValue(10);
  record.addValue(11);

  EXPECT_EQ(record.getTotalEntries(), 2u);
  for(unsigned int ix = 0; ix < record.getPartitions(); ++ix)
    EXPECT_EQ(record.getEntryCount(ix), 0u);
}

TEST(PartitionRecordTest, InvalidIndexAccessReturnsZero)
{
  PartitionRecord record;
  record.init(0, 10, 5);
  record.addValue(1);

  EXPECT_EQ(record.getEntryCount(99), 0u);
  EXPECT_DOUBLE_EQ(record.getValue(99), 0);
}

TEST(PartitionRecordTest, SettersUpdateVarNameAndColor)
{
  PartitionRecord record;

  record.setVarName("NAV_X");
  record.setColor("red");

  EXPECT_EQ(record.getVarName(), "NAV_X");
  EXPECT_TRUE(record.getColorPack().set());
  EXPECT_NE(record.getColorPack().red(), record.getColorPack().blu());
}
