#include <gtest/gtest.h>

#include "VelocityFilter.h"

// Covers velocity filter behavior: parses valid config and interpolates by faster vessel.
TEST(VelocityFilterTest, ParsesValidConfigAndInterpolatesByFasterVessel)
{
  VelocityFilter filter = stringToVelocityFilter("min_spd=1,max_spd=5,pct=25");
  ASSERT_TRUE(filter.valid());
  EXPECT_DOUBLE_EQ(filter.getMinSpd(), 1);
  EXPECT_DOUBLE_EQ(filter.getMaxSpd(), 5);
  EXPECT_DOUBLE_EQ(filter.getMinSpdPct(), 25);

  filter.setSpdOS(0.5);
  filter.setSpdCN(0.75);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 25);

  filter.setSpdOS(3);
  filter.setSpdCN(2);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 0.625);

  filter.setSpdOS(2);
  filter.setSpdCN(5);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 100);
}

// Covers velocity filter behavior: rejects incomplete inverted or out of range configs.
TEST(VelocityFilterTest, RejectsIncompleteInvertedOrOutOfRangeConfigs)
{
  EXPECT_FALSE(stringToVelocityFilter("min_spd=5,max_spd=1,pct=50").valid());
  EXPECT_FALSE(stringToVelocityFilter("min_spd=1,max_spd=5,pct=125").valid());
}

// Covers velocity filter behavior: defaults invalid and direct setters expose contact speeds.
TEST(VelocityFilterTest, DefaultsInvalidAndDirectSettersExposeContactSpeeds)
{
  VelocityFilter filter;
  EXPECT_FALSE(filter.valid());
  EXPECT_DOUBLE_EQ(filter.getMinSpd(), -1);
  EXPECT_DOUBLE_EQ(filter.getMaxSpd(), -1);
  EXPECT_DOUBLE_EQ(filter.getMinSpdPct(), -1);
  EXPECT_DOUBLE_EQ(filter.getSpdOS(), -1);
  EXPECT_DOUBLE_EQ(filter.getSpdCN(), -1);
  EXPECT_EQ(filter.getSpec(), "");

  filter.setMinSpd(0);
  filter.setMaxSpd(4);
  filter.setMinSpdPct(50);
  filter.setSpdOS(1.5);
  filter.setSpdCN(3.0);

  EXPECT_TRUE(filter.valid());
  EXPECT_DOUBLE_EQ(filter.getSpdOS(), 1.5);
  EXPECT_DOUBLE_EQ(filter.getSpdCN(), 3.0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 0.875);
}

// Covers velocity filter behavior: handles boundary speeds used by colregs range filtering.
TEST(VelocityFilterTest, HandlesBoundarySpeedsUsedByColregsRangeFiltering)
{
  VelocityFilter filter = stringToVelocityFilter("min_spd=2,max_spd=6,pct=40");
  ASSERT_TRUE(filter.valid());

  filter.setSpdOS(2);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 40);

  filter.setSpdOS(1.99);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 40);

  filter.setSpdOS(6);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 100);

  filter.setSpdOS(3);
  filter.setSpdCN(5);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 0.85);
}

// Covers velocity filter behavior: equal speed bounds are valid and act as immediate full filter.
TEST(VelocityFilterTest, EqualSpeedBoundsAreValidAndActAsImmediateFullFilter)
{
  VelocityFilter filter = stringToVelocityFilter("min_spd=3,max_spd=3,pct=25");
  ASSERT_TRUE(filter.valid());

  filter.setSpdOS(0);
  filter.setSpdCN(2.99);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 25);

  filter.setSpdOS(3);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 100);
}

// Covers velocity filter behavior: parser ignores unknown fields and uses last duplicate values.
TEST(VelocityFilterTest, ParserIgnoresUnknownFieldsAndUsesLastDuplicateValues)
{
  VelocityFilter filter =
      stringToVelocityFilter("min_spd=1,ignored=9,max_spd=5,pct=20,pct=60");
  ASSERT_TRUE(filter.valid());
  EXPECT_DOUBLE_EQ(filter.getMinSpd(), 1);
  EXPECT_DOUBLE_EQ(filter.getMaxSpd(), 5);
  EXPECT_DOUBLE_EQ(filter.getMinSpdPct(), 60);

  filter.setSpdOS(3);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 0.8);
}

// Covers velocity filter behavior: negative pct is accepted but clamped at runtime.
TEST(VelocityFilterTest, NegativePctIsAcceptedButClampedAtRuntime)
{
  VelocityFilter filter = stringToVelocityFilter("min_spd=1,max_spd=5,pct=-25");
  ASSERT_TRUE(filter.valid());
  EXPECT_DOUBLE_EQ(filter.getMinSpdPct(), -25);

  filter.setSpdOS(1);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), -25);

  filter.setSpdOS(2);
  filter.setSpdCN(0);
  EXPECT_DOUBLE_EQ(filter.getFilterPct(), 0.0625);
}

// Covers velocity filter behavior: rejects missing speed fields but pins pct validation quirk.
TEST(VelocityFilterTest, RejectsMissingSpeedFieldsButPinsPctValidationQuirk)
{
  EXPECT_FALSE(stringToVelocityFilter("").valid());
  EXPECT_FALSE(stringToVelocityFilter("min_spd=1,pct=50").valid());
  EXPECT_FALSE(stringToVelocityFilter("max_spd=5,pct=50").valid());
  EXPECT_FALSE(stringToVelocityFilter("min_spd=fast,max_spd=5,pct=50").valid());
  EXPECT_FALSE(stringToVelocityFilter("min_spd=1,max_spd=fast,pct=50").valid());

  VelocityFilter missing_pct = stringToVelocityFilter("min_spd=1,max_spd=5");
  EXPECT_TRUE(missing_pct.valid());
  EXPECT_DOUBLE_EQ(missing_pct.getMinSpdPct(), -1);

  VelocityFilter malformed_pct =
      stringToVelocityFilter("min_spd=1,max_spd=5,pct=much");
  EXPECT_TRUE(malformed_pct.valid());
  EXPECT_DOUBLE_EQ(malformed_pct.getMinSpdPct(), -1);
}
