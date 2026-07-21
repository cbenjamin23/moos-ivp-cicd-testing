#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>

#include "RandVarGaussian.h"
#include "RandVarUniform.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

bool IsSnappedTo(double value, double step)
{
  const double quotient = value / step;
  return std::fabs(quotient - std::round(quotient)) < 1e-9;
}

}  // namespace

// Coverage note: these tests stay on the deterministic contracts of the
// uTimerScript random scalar classes. They assert ranges, clamping, snapping,
// summaries, and malformed parameter handling without pinning platform-specific
// PRNG sequences.

TEST(RandVarUniformTest, DefaultsResetIntoUnitRangeWithStringValue)
{
  RandVarUniform variable;

  variable.reset();

  EXPECT_GE(variable.getValue(), 0.0);
  EXPECT_LE(variable.getValue(), 1.0);
  EXPECT_FALSE(variable.getStringValue().empty());
  EXPECT_TRUE(Contains(variable.getStringSummary(), "type=uniform"));
}

TEST(RandVarUniformTest, MinAndMaxCrossoversClampTheRange)
{
  RandVarUniform variable;

  EXPECT_TRUE(variable.setParam("min", 10));
  EXPECT_DOUBLE_EQ(variable.getMinVal(), 10);
  EXPECT_DOUBLE_EQ(variable.getMaxVal(), 10);

  EXPECT_TRUE(variable.setParam("max", 5));
  EXPECT_DOUBLE_EQ(variable.getMinVal(), 5);
  EXPECT_DOUBLE_EQ(variable.getMaxVal(), 5);
}

TEST(RandVarUniformTest, ResetSnapsAndBoundsRandomValues)
{
  RandVarUniform variable;
  ASSERT_TRUE(variable.setParam("min", -1));
  ASSERT_TRUE(variable.setParam("max", 1));
  ASSERT_TRUE(variable.setParam("snap", 0.25));

  for(int i = 0; i < 25; ++i) {
    variable.reset();
    EXPECT_GE(variable.getValue(), -1.0);
    EXPECT_LE(variable.getValue(), 1.0);
    EXPECT_TRUE(IsSnappedTo(variable.getValue(), 0.25));
    EXPECT_FALSE(variable.getStringValue().empty());
  }
}

TEST(RandVarUniformTest, SamplesSpanConfiguredRangeWithUniformMeanAndVariance)
{
  srand(17);
  RandVarUniform variable;
  ASSERT_TRUE(variable.setParam("min", -1));
  ASSERT_TRUE(variable.setParam("max", 1));

  constexpr int sample_count = 2000;
  double sum = 0;
  double square_sum = 0;
  double minimum = std::numeric_limits<double>::infinity();
  double maximum = -std::numeric_limits<double>::infinity();
  for(int i = 0; i < sample_count; ++i) {
    variable.reset();
    const double value = variable.getValue();
    sum += value;
    square_sum += value * value;
    minimum = std::min(minimum, value);
    maximum = std::max(maximum, value);
  }

  const double mean = sum / sample_count;
  const double variance = (square_sum / sample_count) - (mean * mean);
  EXPECT_NEAR(mean, 0, 0.08);
  EXPECT_GT(variance, 0.27);
  EXPECT_LT(variance, 0.40);
  EXPECT_LT(minimum, -0.9);
  EXPECT_GT(maximum, 0.9);
}

TEST(RandVarUniformTest, EqualRangeSetsNumericValueButLeavesPreviousStringUntouched)
{
  RandVarUniform variable;
  ASSERT_TRUE(variable.setParam("min", 0));
  ASSERT_TRUE(variable.setParam("max", 1));
  variable.reset();
  const std::string previous_value = variable.getStringValue();
  ASSERT_FALSE(previous_value.empty());

  ASSERT_TRUE(variable.setParam("min", 3));
  ASSERT_TRUE(variable.setParam("max", 3));
  variable.reset();

  EXPECT_DOUBLE_EQ(variable.getValue(), 3);
  EXPECT_EQ(variable.getStringValue(), previous_value);
}

TEST(RandVarUniformTest, RejectsUnknownParameterAndIgnoresNonPositiveSnap)
{
  RandVarUniform variable;
  ASSERT_TRUE(variable.setParam("min", 0));
  ASSERT_TRUE(variable.setParam("max", 1));
  ASSERT_TRUE(variable.setParam("snap", 0.5));
  ASSERT_TRUE(variable.setParam("snap", 0));
  EXPECT_FALSE(variable.setParam("bogus", 1));

  for(int i = 0; i < 10; ++i) {
    variable.reset();
    EXPECT_TRUE(IsSnappedTo(variable.getValue(), 0.5));
  }
}

TEST(RandVarGaussianTest, ParametersSummaryAndParamsReflectConfiguredValues)
{
  RandVarGaussian variable;

  EXPECT_TRUE(variable.setParam("min", -5));
  EXPECT_TRUE(variable.setParam("max", 5));
  EXPECT_TRUE(variable.setParam("mu", 1.5));
  EXPECT_TRUE(variable.setParam("sigma", 0.25));
  EXPECT_FALSE(variable.setParam("unknown", 1));

  const std::string summary = variable.getStringSummary();
  EXPECT_TRUE(Contains(summary, "sigma=0.25"));
  EXPECT_TRUE(Contains(summary, "mu=1.5"));
  EXPECT_TRUE(Contains(summary, "type=gaussian"));
  EXPECT_EQ(variable.getParams(), "sigma=0.25, mu=1.5");
}

TEST(RandVarGaussianTest, ResetClampsToConfiguredBoundsAndProducesStringValue)
{
  RandVarGaussian variable;
  ASSERT_TRUE(variable.setParam("min", 7));
  ASSERT_TRUE(variable.setParam("max", 7));
  ASSERT_TRUE(variable.setParam("mu", 0));
  ASSERT_TRUE(variable.setParam("sigma", 1));

  variable.reset();

  EXPECT_DOUBLE_EQ(variable.getValue(), 7);
  EXPECT_FALSE(variable.getStringValue().empty());
}

TEST(RandVarGaussianTest, ResetSnapsValuesInsideRange)
{
  RandVarGaussian variable;
  ASSERT_TRUE(variable.setParam("min", -2));
  ASSERT_TRUE(variable.setParam("max", 2));
  ASSERT_TRUE(variable.setParam("mu", 0));
  ASSERT_TRUE(variable.setParam("sigma", 0.1));
  ASSERT_TRUE(variable.setParam("snap", 0.05));

  for(int i = 0; i < 25; ++i) {
    variable.reset();
    EXPECT_GE(variable.getValue(), -2.0);
    EXPECT_LE(variable.getValue(), 2.0);
    EXPECT_TRUE(IsSnappedTo(variable.getValue(), 0.05));
    EXPECT_FALSE(variable.getStringValue().empty());
  }
}

TEST(RandVarGaussianTest, SamplesHaveConfiguredMeanAndGaussianVariance)
{
  srand(23);
  RandVarGaussian variable;
  ASSERT_TRUE(variable.setParam("min", -5));
  ASSERT_TRUE(variable.setParam("max", 5));
  ASSERT_TRUE(variable.setParam("mu", 1.5));
  ASSERT_TRUE(variable.setParam("sigma", 0.5));

  constexpr int sample_count = 2000;
  double sum = 0;
  double square_sum = 0;
  for(int i = 0; i < sample_count; ++i) {
    variable.reset();
    const double value = variable.getValue();
    sum += value;
    square_sum += value * value;
  }

  const double mean = sum / sample_count;
  const double variance = (square_sum / sample_count) - (mean * mean);
  EXPECT_NEAR(mean, 1.5, 0.08);
  EXPECT_GT(variance, 0.18);
  EXPECT_LT(variance, 0.32);
}
