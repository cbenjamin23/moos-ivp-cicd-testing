#include <gtest/gtest.h>

#include <string>

#include "RandomVariableSet.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Coverage note: this suite covers the rand_var/randvar parser used by
// TS_MOOSApp configuration, including accepted key aliases, default uniform
// type, normal-as-gaussian aliasing, duplicate detection, and stable error
// strings. It avoids only crash-prone upstream behavior outside normal config
// validation.

TEST(RandomVariableSetTest, AddsUniformVariableWithDefaultTypeAndLowercaseKey)
{
  RandomVariableSet set;

  EXPECT_EQ(set.addRandomVar("varname=SPEED,key=AT_POST,min=1,max=3,snap=0.5"),
            "");

  ASSERT_EQ(set.size(), 1u);
  EXPECT_TRUE(set.contains("SPEED"));
  EXPECT_EQ(set.getVarName(0), "SPEED");
  EXPECT_EQ(set.getKeyName(0), "at_post");
  EXPECT_EQ(set.getType(0), "uniform");
  EXPECT_DOUBLE_EQ(set.getMinVal(0), 1);
  EXPECT_DOUBLE_EQ(set.getMaxVal(0), 3);
  EXPECT_EQ(set.getVarName(9), "");
  EXPECT_DOUBLE_EQ(set.getValue(9), 0);
}

TEST(RandomVariableSetTest, AddsGaussianVariableWithNormalAlias)
{
  RandomVariableSet set;

  EXPECT_EQ(set.addRandomVar("type=normal,varname=DEPTH,key=at_reset,min=-5,"
                             "max=5,mu=1.25,sigma=0.5"),
            "");

  ASSERT_EQ(set.size(), 1u);
  EXPECT_EQ(set.getType(0), "gaussian");
  EXPECT_EQ(set.getParams(0), "sigma=0.5, mu=1.25");
  EXPECT_TRUE(Contains(set.getStringSummary(0), "type=gaussian"));
}

TEST(RandomVariableSetTest, AcceptsAppStyleWhitespaceAroundParameters)
{
  RandomVariableSet set;

  EXPECT_EQ(set.addRandomVar("varname=ANG, min=0, max=359, key=at_reset"), "");
  EXPECT_EQ(set.addRandomVar("type=gaussian, varname=RNG, key=at_post,"
                             " min=10, max=20, mu=15, sigma=2"),
            "");

  ASSERT_EQ(set.size(), 2u);
  EXPECT_EQ(set.getVarName(0), "ANG");
  EXPECT_EQ(set.getKeyName(0), "at_reset");
  EXPECT_EQ(set.getType(0), "uniform");
  EXPECT_DOUBLE_EQ(set.getMaxVal(0), 359);
  EXPECT_EQ(set.getVarName(1), "RNG");
  EXPECT_EQ(set.getKeyName(1), "at_post");
  EXPECT_EQ(set.getType(1), "gaussian");
  EXPECT_EQ(set.getParams(1), "sigma=2, mu=15");
}

TEST(RandomVariableSetTest, ResetOnlyUpdatesVariablesForTheRequestedKey)
{
  RandomVariableSet set;
  ASSERT_EQ(set.addRandomVar("varname=A,key=at_start,min=2,max=2"), "");
  ASSERT_EQ(set.addRandomVar("type=gaussian,varname=B,key=at_post,min=7,max=7,"
                             "mu=0,sigma=1"),
            "");

  set.reset("at_start");
  EXPECT_DOUBLE_EQ(set.getValue(0), 2);
  EXPECT_DOUBLE_EQ(set.getValue(1), 0);
  EXPECT_EQ(set.getStringValue(0), "");

  set.reset("at_post");
  EXPECT_DOUBLE_EQ(set.getValue(1), 7);
  EXPECT_FALSE(set.getStringValue(1).empty());
}

TEST(RandomVariableSetTest, RejectsUnknownTypeAndBadParameters)
{
  RandomVariableSet set;

  EXPECT_EQ(set.addRandomVar("type=triangular,varname=X,key=at_post,min=0,max=1"),
            "unknown random variable type: triangular");
  EXPECT_EQ(set.addRandomVar("type=UNIFORM,varname=X,key=at_post,min=0,max=1"),
            "unknown random variable type: UNIFORM");
  EXPECT_EQ(set.addRandomVar("varname=X,key=at_post,min=bad,max=1"),
            "Bad parametery=value: min=bad");
  EXPECT_EQ(set.addRandomVar("type=gaussian,varname=X,key=at_post,min=0,max=1,"
                             "mu=bad,sigma=1"),
            "Bad parameterx=value: mu=bad");
  EXPECT_EQ(set.size(), 0u);
}

TEST(RandomVariableSetTest, RejectsMissingAndInvalidUniformRequiredFields)
{
  RandomVariableSet set;

  EXPECT_EQ(set.addRandomVarUniform("varname=X,min=0,max=1"),
            "key is not specified");
  EXPECT_EQ(set.addRandomVarUniform("varname=X,key=at_stop,min=0,max=1"),
            "unknown random_var key: at_stop");
  EXPECT_EQ(set.addRandomVarUniform("key=at_post,min=0,max=1"),
            "Unset variable name");
  EXPECT_EQ(set.addRandomVarUniform("varname=X,key=at_post,max=1"),
            "Lower value of the range not set");
  EXPECT_EQ(set.addRandomVarUniform("varname=X,key=at_post,min=0"),
            "Upper value of the range not set");
  EXPECT_EQ(set.addRandomVarUniform("varname=X,key=at_post,min=2,max=1"),
            "Minimum value greater than maximum value");
}

TEST(RandomVariableSetTest, RejectsMissingAndInvalidGaussianRequiredFields)
{
  RandomVariableSet set;

  EXPECT_EQ(set.addRandomVarGaussian("varname=X,min=0,max=1,mu=0,sigma=1"),
            "key is not specified");
  EXPECT_EQ(set.addRandomVarGaussian("varname=X,key=at_post,max=1,mu=0,sigma=1"),
            "Lower value of the range not set");
  EXPECT_EQ(set.addRandomVarGaussian("varname=X,key=at_post,min=0,mu=0,sigma=1"),
            "Upper value of the range not set");
  EXPECT_EQ(set.addRandomVarGaussian("varname=X,key=at_post,min=0,max=1,sigma=1"),
            "Mu not set");
  EXPECT_EQ(set.addRandomVarGaussian("varname=X,key=at_post,min=0,max=1,mu=0"),
            "Sigma not set");
}

TEST(RandomVariableSetTest, RejectsDuplicateVariableNamesAcrossTypes)
{
  RandomVariableSet set;

  ASSERT_EQ(set.addRandomVarUniform("varname=X,key=at_post,min=0,max=1"), "");
  EXPECT_EQ(set.addRandomVarUniform("varname=X,key=at_reset,min=2,max=3"),
            "Duplicate random variable");
  EXPECT_EQ(set.addRandomVarGaussian("type=gaussian,varname=X,key=at_start,"
                                     "min=0,max=1,mu=0,sigma=1"),
            "Duplicate random variable");
  EXPECT_EQ(set.size(), 1u);
}
