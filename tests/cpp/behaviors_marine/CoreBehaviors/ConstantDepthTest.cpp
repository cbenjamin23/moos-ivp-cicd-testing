#include <gtest/gtest.h>

#include <memory>

#include "BHV_ConstantDepth.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV constant depth behavior: builds depth objective and posts mismatch telemetry.
TEST(BHVConstantDepthTest, BuildsDepthObjectiveAndPostsMismatchTelemetry)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 12.25);

  BHV_ConstantDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "20"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "1"));
  ASSERT_TRUE(behavior.setParam("basewidth", "8"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "15"));
  ASSERT_TRUE(behavior.setParam("depth_mismatch_var", "DEPTH_ERR"));
  EXPECT_FALSE(behavior.setParam("depth_mismatch_var", "BAD VAR"));
  EXPECT_FALSE(behavior.setParam("depth", "deep"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 20), evalOneDimPoint(*ipf, 5));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DEPTH_ERR", mismatch));
  EXPECT_FALSE(mismatch.is_string());
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 7.75);
}

// Covers BHV constant depth behavior: clips negative depth and ZAIC widths from mission config.
TEST(BHVConstantDepthTest, ClipsNegativeDepthAndZaicWidthsFromMissionConfig)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 3.5);

  BHV_ConstantDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "-5"));
  ASSERT_TRUE(behavior.setParam("basewidth", "-8"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "-2"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "250"));
  ASSERT_TRUE(behavior.setParam("depth_mismatch_var", "DEPTH_ERR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 0), evalOneDimPoint(*ipf, 20));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DEPTH_ERR", mismatch));
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 3.5);
}

// Covers BHV constant depth behavior: warns on missing nav depth and does not post mismatch.
TEST(BHVConstantDepthTest, WarnsOnMissingNavDepthAndDoesNotPostMismatch)
{
  BHV_ConstantDepth behavior(makeDepthDomain());

  ASSERT_TRUE(behavior.setParam("depth", "18"));
  ASSERT_TRUE(behavior.setParam("depth_mismatch_var", "DEPTH_ERR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("No ownship DEPTH"), std::string::npos);

  VarDataPair mismatch;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "DEPTH_ERR", mismatch));
}

// Covers BHV constant depth behavior: fails without depth dimension in helm domain.
TEST(BHVConstantDepthTest, FailsWithoutDepthDimensionInHelmDomain)
{
  BHV_ConstantDepth behavior(makeCourseSpeedDomain());

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("No 'depth' variable"), std::string::npos);
}

// Covers BHV constant depth behavior: mission style delta depth config applies shape and priority.
TEST(BHVConstantDepthTest, MissionStyleDeltaDepthConfigAppliesShapeAndPriority)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 45);

  BHV_ConstantDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  // Mirrors ivp/missions/s4_delta/delta.bhv constant depth block.
  ASSERT_TRUE(behavior.setParam("pwt", "100"));
  ASSERT_TRUE(behavior.setParam("duration", "no-time-limit"));
  ASSERT_TRUE(behavior.setParam("updates", "DEPTH_VALUE"));
  ASSERT_TRUE(behavior.setParam("depth", "50"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "8"));
  ASSERT_TRUE(behavior.setParam("basewidth", "12"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "10"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_DOUBLE_EQ(ipf->getPWT(), 100);
  EXPECT_GT(evalOneDimPoint(*ipf, 50), evalOneDimPoint(*ipf, 35));
}
