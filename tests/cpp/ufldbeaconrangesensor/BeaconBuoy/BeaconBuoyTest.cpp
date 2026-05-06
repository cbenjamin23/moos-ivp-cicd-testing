#include <gtest/gtest.h>

#include <string>

#include "BeaconBuoy.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers BeaconBuoy's deterministic state,
// display hints, range/frequency parsing, timestamp monotonicity, counters, and
// spec formatting. BRS_App geodesy, vehicle range calculations, and MOOS
// publications remain harness/runtime territory.

TEST(BeaconBuoyTest, DefaultsUseRenderableBeaconAndPollingFrequency)
{
  BeaconBuoy buoy;

  EXPECT_DOUBLE_EQ(buoy.getX(), 0);
  EXPECT_DOUBLE_EQ(buoy.getY(), 0);
  EXPECT_DOUBLE_EQ(buoy.getTimeStamp(), 0);
  EXPECT_DOUBLE_EQ(buoy.getFrequency(), -1);
  EXPECT_DOUBLE_EQ(buoy.getWidth(), 4);
  EXPECT_EQ(buoy.getShape(), "circle");
  EXPECT_EQ(buoy.getBuoyColor(), "orange");
  EXPECT_EQ(buoy.getFreqSetting(), "poll");
  EXPECT_EQ(buoy.getSpec(), "x=0,y=0,color=orange,type=circle,width=4");
}

TEST(BeaconBuoyTest, BasicSettersAndCountersUpdateState)
{
  BeaconBuoy buoy;
  buoy.setX(12.5);
  buoy.setY(-4.25);
  buoy.setLabel("beacon_alpha");
  buoy.incPingsReceived();
  buoy.incPingsReceived();
  buoy.incPingsReplied();
  buoy.incPingsUnsol();

  EXPECT_DOUBLE_EQ(buoy.getX(), 12.5);
  EXPECT_DOUBLE_EQ(buoy.getY(), -4.25);
  EXPECT_EQ(buoy.getLabel(), "beacon_alpha");
  EXPECT_EQ(buoy.getPingsReceived(), 2u);
  EXPECT_EQ(buoy.getPingsReplied(), 1u);
  EXPECT_EQ(buoy.getPingsUnsol(), 1u);
}

TEST(BeaconBuoyTest, ShapeAcceptsKnownValuesCaseInsensitively)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setShape("DIAMOND"));
  EXPECT_EQ(buoy.getShape(), "diamond");
  EXPECT_TRUE(buoy.setShape("square"));
  EXPECT_TRUE(buoy.setShape("triangle"));
  EXPECT_TRUE(buoy.setShape("efield"));
  EXPECT_TRUE(buoy.setShape("gateway"));
  EXPECT_FALSE(buoy.setShape("hexagon"));
  EXPECT_EQ(buoy.getShape(), "gateway");
}

TEST(BeaconBuoyTest, WidthRejectsInvalidAndNegativeValues)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setWidth("0"));
  EXPECT_DOUBLE_EQ(buoy.getWidth(), 0);
  EXPECT_TRUE(buoy.setWidth("2.5"));
  EXPECT_DOUBLE_EQ(buoy.getWidth(), 2.5);
  EXPECT_FALSE(buoy.setWidth("-0.1"));
  EXPECT_FALSE(buoy.setWidth("wide"));
  EXPECT_DOUBLE_EQ(buoy.getWidth(), 2.5);
}

TEST(BeaconBuoyTest, PushAndPullDistancesAcceptNumericAndUnlimitedAliases)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setPushDist("25.5"));
  EXPECT_DOUBLE_EQ(buoy.getPushDist(), 25.5);
  EXPECT_TRUE(buoy.setPushDist("-7"));
  EXPECT_DOUBLE_EQ(buoy.getPushDist(), -7);
  EXPECT_TRUE(buoy.setPushDist("nolimit"));
  EXPECT_DOUBLE_EQ(buoy.getPushDist(), -1);
  EXPECT_FALSE(buoy.setPushDist("UNLIMITED"));
  EXPECT_DOUBLE_EQ(buoy.getPushDist(), -1);
  EXPECT_FALSE(buoy.setPushDist("far"));
  EXPECT_DOUBLE_EQ(buoy.getPushDist(), -1);

  EXPECT_TRUE(buoy.setPullDist("unlimited"));
  EXPECT_DOUBLE_EQ(buoy.getPullDist(), -1);
  EXPECT_FALSE(buoy.setPullDist("NOLIMIT"));
  EXPECT_DOUBLE_EQ(buoy.getPullDist(), -1);
  EXPECT_TRUE(buoy.setPullDist("-3"));
  EXPECT_DOUBLE_EQ(buoy.getPullDist(), -3);
  EXPECT_FALSE(buoy.setPullDist("near"));
  EXPECT_DOUBLE_EQ(buoy.getPullDist(), -3);
}

TEST(BeaconBuoyTest, ColorSettersAcceptKnownColorsAndRejectUnknownColors)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setBuoyColor("red"));
  EXPECT_EQ(buoy.getBuoyColor(), "red");
  EXPECT_FALSE(buoy.setBuoyColor("not_a_color"));
  EXPECT_EQ(buoy.getBuoyColor(), "red");

  EXPECT_TRUE(buoy.setPulseLineColor("blue"));
  EXPECT_TRUE(buoy.setPulseFillColor("yellow"));
  EXPECT_FALSE(buoy.setPulseLineColor("bad_color"));
  EXPECT_FALSE(buoy.setPulseFillColor("bad_color"));
}

TEST(BeaconBuoyTest, SingleFrequencyConfigProducesFixedFrequency)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setFrequencyRange("5"));

  EXPECT_DOUBLE_EQ(buoy.getFrequency(), 5);
  EXPECT_EQ(buoy.getFreqSetting(), "5.0");
}

TEST(BeaconBuoyTest, NeverFrequencyConfigReturnsToPolling)
{
  BeaconBuoy buoy;
  ASSERT_TRUE(buoy.setFrequencyRange("5"));

  EXPECT_TRUE(buoy.setFrequencyRange("NEVER"));

  EXPECT_DOUBLE_EQ(buoy.getFrequency(), 5);
  EXPECT_EQ(buoy.getFreqSetting(), "5.0");

  EXPECT_TRUE(buoy.setTimeStamp(1));
  EXPECT_DOUBLE_EQ(buoy.getFrequency(), -1);
  EXPECT_EQ(buoy.getFreqSetting(), "poll");
}

TEST(BeaconBuoyTest, FrequencyRangeProducesValueWithinRange)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setFrequencyRange("10:40"));

  EXPECT_GE(buoy.getFrequency(), 10);
  EXPECT_LE(buoy.getFrequency(), 40);
  EXPECT_EQ(buoy.getFreqSetting(), "[10:40]");
}

TEST(BeaconBuoyTest, FrequencyRangeRejectsNonNumericAndNegativeValues)
{
  BeaconBuoy buoy;

  EXPECT_FALSE(buoy.setFrequencyRange("abc"));
  EXPECT_FALSE(buoy.setFrequencyRange("-1"));
  EXPECT_FALSE(buoy.setFrequencyRange("1:bad"));
  EXPECT_FALSE(buoy.setFrequencyRange("-1:2"));

  EXPECT_DOUBLE_EQ(buoy.getFrequency(), -1);
  EXPECT_EQ(buoy.getFreqSetting(), "poll");
}

TEST(BeaconBuoyTest, EmptyHighFrequencyRangeIsParsedAsFixedLowValue)
{
  BeaconBuoy buoy;

  EXPECT_TRUE(buoy.setFrequencyRange("5:"));

  EXPECT_DOUBLE_EQ(buoy.getFrequency(), 5);
  EXPECT_EQ(buoy.getFreqSetting(), "5.0");
}

TEST(BeaconBuoyTest, FrequencyRangeUsesStringComparisonForLowHighOrdering)
{
  BeaconBuoy rejects_numeric_valid_range;
  EXPECT_FALSE(rejects_numeric_valid_range.setFrequencyRange("2:10"));
  EXPECT_EQ(rejects_numeric_valid_range.getFreqSetting(), "poll");

  BeaconBuoy accepts_numeric_reversed_range;
  EXPECT_TRUE(accepts_numeric_reversed_range.setFrequencyRange("10:2"));
  EXPECT_DOUBLE_EQ(accepts_numeric_reversed_range.getFrequency(), -1);
  EXPECT_EQ(accepts_numeric_reversed_range.getFreqSetting(), "poll");
}

TEST(BeaconBuoyTest, TimestampIsMonotonicAndResetsFrequency)
{
  BeaconBuoy buoy;
  ASSERT_TRUE(buoy.setFrequencyRange("5"));

  EXPECT_TRUE(buoy.setTimeStamp(10));
  EXPECT_DOUBLE_EQ(buoy.getTimeStamp(), 10);
  EXPECT_DOUBLE_EQ(buoy.getFrequency(), 5);
  EXPECT_FALSE(buoy.setTimeStamp(9.99));
  EXPECT_DOUBLE_EQ(buoy.getTimeStamp(), 10);
}

TEST(BeaconBuoyTest, SpecIncludesRenderFieldsAndOmitsZeroWidth)
{
  BeaconBuoy buoy;
  buoy.setX(1.25);
  buoy.setY(2.5);
  buoy.setLabel("alpha");
  ASSERT_TRUE(buoy.setBuoyColor("green"));
  ASSERT_TRUE(buoy.setShape("square"));
  ASSERT_TRUE(buoy.setWidth("0"));

  EXPECT_EQ(buoy.getSpec(), "x=1.25,y=2.5,label=alpha,color=green,type=square");
}

TEST(BeaconBuoyTest, FullSpecIncludesFrequencyRangeAndReportRange)
{
  BeaconBuoy buoy;
  ASSERT_TRUE(buoy.setFrequencyRange("5"));

  const std::string spec = buoy.getSpec(true);

  EXPECT_TRUE(Contains(spec, "freq=5"));
  EXPECT_TRUE(Contains(spec, "freq_low=5"));
  EXPECT_TRUE(Contains(spec, "freq_hgh=5"));
  EXPECT_TRUE(Contains(spec, "report_range=100"));
}
