#include <gtest/gtest.h>

#include "FColorMap.h"

// Covers f color map behavior: initializes standard map used by viewers.
TEST(FColorMapTest, InitializesStandardMapUsedByViewers)
{
  FColorMap map;
  EXPECT_EQ(map.size(), 64);

  EXPECT_DOUBLE_EQ(map.getRVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.5625);

  EXPECT_DOUBLE_EQ(map.getRVal(31), 0.5);
  EXPECT_DOUBLE_EQ(map.getGVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(31), 0.5);

  EXPECT_DOUBLE_EQ(map.getRVal(63), 0.5);
  EXPECT_DOUBLE_EQ(map.getGVal(63), 0.0);
  EXPECT_DOUBLE_EQ(map.getBVal(63), 0.0);

  EXPECT_DOUBLE_EQ(map.getRVal(64), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(64), 0.0);
  EXPECT_DOUBLE_EQ(map.getBVal(64), 0.0);
}

// Covers f color map behavior: interpolates by index and clips out of range inputs.
TEST(FColorMapTest, InterpolatesByIndexAndClipsOutOfRangeInputs)
{
  FColorMap map;

  EXPECT_DOUBLE_EQ(map.getIRVal(-1), map.getRVal(0));
  EXPECT_DOUBLE_EQ(map.getIGVal(-1), map.getGVal(0));
  EXPECT_DOUBLE_EQ(map.getIBVal(-1), map.getBVal(0));

  EXPECT_DOUBLE_EQ(map.getIRVal(0.5), map.getRVal(31));
  EXPECT_DOUBLE_EQ(map.getIGVal(0.5), map.getGVal(31));
  EXPECT_DOUBLE_EQ(map.getIBVal(0.5), map.getBVal(31));

  EXPECT_DOUBLE_EQ(map.getIRVal(2), map.getRVal(63));
  EXPECT_DOUBLE_EQ(map.getIGVal(2), map.getGVal(63));
  EXPECT_DOUBLE_EQ(map.getIBVal(2), map.getBVal(63));
}

// Covers f color map behavior: pins interpolated index rounding used by field viewers.
TEST(FColorMapTest, PinsInterpolatedIndexRoundingUsedByFieldViewers)
{
  FColorMap map;

  EXPECT_DOUBLE_EQ(map.getIRVal(0.01), map.getRVal(0));
  EXPECT_DOUBLE_EQ(map.getIGVal(0.01), map.getGVal(0));
  EXPECT_DOUBLE_EQ(map.getIBVal(0.01), map.getBVal(0));

  EXPECT_DOUBLE_EQ(map.getIRVal(1.0 / 64.0), map.getRVal(0));
  EXPECT_DOUBLE_EQ(map.getIRVal(2.0 / 64.0), map.getRVal(1));
  EXPECT_DOUBLE_EQ(map.getIRVal(63.0 / 64.0), map.getRVal(62));
  EXPECT_DOUBLE_EQ(map.getIRVal(1.0), map.getRVal(63));
}

// Covers f color map behavior: selects named maps and falls back to standard.
TEST(FColorMapTest, SelectsNamedMapsAndFallsBackToStandard)
{
  FColorMap map;
  map.setType("copper");
  EXPECT_EQ(map.size(), 64);
  EXPECT_DOUBLE_EQ(map.getRVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(63), 0.7812);
  EXPECT_DOUBLE_EQ(map.getBVal(63), 0.4975);

  map.setType("bone");
  EXPECT_EQ(map.size(), 64);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.0052);
  EXPECT_DOUBLE_EQ(map.getRVal(63), 1.0);

  map.setType("unknown");
  EXPECT_EQ(map.size(), 64);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.5625);
}

// Covers f color map behavior: applies white plateau around midpoint.
TEST(FColorMapTest, AppliesWhitePlateauAroundMidpoint)
{
  FColorMap map;
  map.applyMidWhite(0.05, 0.05);

  EXPECT_DOUBLE_EQ(map.getRVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getGVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(31), 1.0);

  EXPECT_DOUBLE_EQ(map.getRVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.5625);
}

// Covers f color map behavior: rebuilds maps and ignores invalid white ranges.
TEST(FColorMapTest, RebuildsMapsAndIgnoresInvalidWhiteRanges)
{
  FColorMap map;
  map.addRGB(0.1, 0.2, 0.3);
  EXPECT_EQ(map.size(), 65);

  map.setCopperMap();
  EXPECT_EQ(map.size(), 64);
  EXPECT_DOUBLE_EQ(map.getRVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(63), 0.7812);

  map.applyMidWhite(-0.1, 0.2);
  EXPECT_DOUBLE_EQ(map.getRVal(31), 0.6151);
  map.applyMidWhite(0.2, -0.1);
  EXPECT_DOUBLE_EQ(map.getRVal(31), 0.6151);

  map.setStandardMap();
  EXPECT_DOUBLE_EQ(map.getRVal(31), 0.5);
  EXPECT_DOUBLE_EQ(map.getGVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(31), 0.5);
}

// Covers f color map behavior: pins case sensitive map selection and viewer endpoint colors.
TEST(FColorMapTest, PinsCaseSensitiveMapSelectionAndViewerEndpointColors)
{
  FColorMap map;

  map.setType("Copper");
  EXPECT_DOUBLE_EQ(map.getRVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.5625);

  map.setType(" bone ");
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.5625);

  map.setType("bone");
  EXPECT_DOUBLE_EQ(map.getRVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(0), 0.0);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 0.0052);
  EXPECT_DOUBLE_EQ(map.getRVal(63), 1.0);
  EXPECT_DOUBLE_EQ(map.getGVal(63), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(63), 1.0);
}

// Covers f color map behavior: pins white plateau width and reapplication behavior.
TEST(FColorMapTest, PinsWhitePlateauWidthAndReapplicationBehavior)
{
  FColorMap map;
  map.applyMidWhite(0.0, 0.0);
  EXPECT_DOUBLE_EQ(map.getRVal(32), 0.5625);
  EXPECT_DOUBLE_EQ(map.getGVal(32), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(32), 0.4375);

  map.applyMidWhite(0.02, 0.20);
  EXPECT_DOUBLE_EQ(map.getRVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getGVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(31), 1.0);
  EXPECT_DOUBLE_EQ(map.getRVal(32), 1.0);
  EXPECT_DOUBLE_EQ(map.getGVal(32), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(32), 1.0);
  EXPECT_DOUBLE_EQ(map.getRVal(20), 0.0);
  EXPECT_DOUBLE_EQ(map.getGVal(20), 0.8125);
  EXPECT_DOUBLE_EQ(map.getBVal(20), 1.0);

  map.applyMidWhite(2.0, 0.0);
  EXPECT_DOUBLE_EQ(map.getRVal(0), 1.0);
  EXPECT_DOUBLE_EQ(map.getGVal(0), 1.0);
  EXPECT_DOUBLE_EQ(map.getBVal(0), 1.0);
}

// Covers f color map behavior: leaves manually added rgb values unclamped.
TEST(FColorMapTest, LeavesManuallyAddedRgbValuesUnclamped)
{
  FColorMap map;
  map.addRGB(-0.5, 1.5, 2.25);

  ASSERT_EQ(map.size(), 65);
  EXPECT_DOUBLE_EQ(map.getRVal(64), -0.5);
  EXPECT_DOUBLE_EQ(map.getGVal(64), 1.5);
  EXPECT_DOUBLE_EQ(map.getBVal(64), 2.25);
  EXPECT_DOUBLE_EQ(map.getIRVal(1.0), map.getRVal(64));
  EXPECT_DOUBLE_EQ(map.getIGVal(1.0), map.getGVal(64));
  EXPECT_DOUBLE_EQ(map.getIBVal(1.0), map.getBVal(64));
}
