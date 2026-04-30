#include <gtest/gtest.h>

#include <memory>

#include "BHV_ConstantHeading.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV constant heading behavior: computes wrapped heading mismatch and completes inside threshold.
TEST(BHVConstantHeadingTest, ComputesWrappedHeadingMismatchAndCompletesInsideThreshold)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 10.0);

  BHV_ConstantHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading", "350"));
  ASSERT_TRUE(behavior.setParam("heading_mismatch_var", "HDG_ERR"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "5"));
  ASSERT_TRUE(behavior.setParam("basewidth", "80"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "10"));
  EXPECT_FALSE(behavior.setParam("heading", "east"));
  EXPECT_FALSE(behavior.setParam("heading_mismatch_var", "BAD VAR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 350), evalOneDimPoint(*ipf, 90));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HDG_ERR", mismatch));
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 20.0);

  behavior.clearMessages();
  ASSERT_TRUE(behavior.setParam("heading", "11"));
  ASSERT_TRUE(behavior.setParam("complete_thresh", "2"));
  std::unique_ptr<IvPFunction> completed_ipf(behavior.onRunState());
  EXPECT_EQ(completed_ipf, nullptr);

  VarDataPair completed_mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HDG_ERR", completed_mismatch));
  EXPECT_DOUBLE_EQ(completed_mismatch.get_ddata(), 1.0);
}

// Covers BHV constant heading behavior: normalizes mission heading and keeps wrapped utility peak.
TEST(BHVConstantHeadingTest, NormalizesMissionHeadingAndKeepsWrappedUtilityPeak)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 359.0);

  BHV_ConstantHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading", "-1"));
  ASSERT_TRUE(behavior.setParam("heading_mismatch_var", "HDG_ERR"));
  ASSERT_TRUE(behavior.setParam("basewidth", "-10"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "-5"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "250"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 359), evalOneDimPoint(*ipf, 180));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HDG_ERR", mismatch));
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 0.0);
}

// Covers BHV constant heading behavior: warns on missing nav heading but still builds default objective.
TEST(BHVConstantHeadingTest, WarnsOnMissingNavHeadingButStillBuildsDefaultObjective)
{
  BHV_ConstantHeading behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("heading", "90"));
  ASSERT_TRUE(behavior.setParam("heading_mismatch_var", "HDG_ERR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("No ownship HEADING"), std::string::npos);

  VarDataPair mismatch;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "HDG_ERR", mismatch));
}

// Covers BHV constant heading behavior: fails without course dimension in helm domain.
TEST(BHVConstantHeadingTest, FailsWithoutCourseDimensionInHelmDomain)
{
  BHV_ConstantHeading behavior(makeDepthDomain());

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("No 'heading/course' variable"), std::string::npos);
}

// Covers BHV constant heading behavior: mission style bravo turn heading completes at threshold.
TEST(BHVConstantHeadingTest, MissionStyleBravoTurnHeadingCompletesAtThreshold)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 185);

  BHV_ConstantHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  // Mirrors ivp/missions/s2_bravo/bravo.bhv constant heading turn block.
  ASSERT_TRUE(behavior.setParam("pwt", "100"));
  ASSERT_TRUE(behavior.setParam("perpetual", "true"));
  ASSERT_TRUE(behavior.setParam("updates", "TURN_HEADING"));
  ASSERT_TRUE(behavior.setParam("heading", "190"));
  ASSERT_TRUE(behavior.setParam("duration", "no-time-limit"));
  ASSERT_TRUE(behavior.setParam("complete_thresh", "4"));
  ASSERT_TRUE(behavior.setParam("heading_mismatch_var", "DBG_CH"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 100);
  EXPECT_GT(evalOneDimPoint(*ipf, 190), evalOneDimPoint(*ipf, 10));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DBG_CH", mismatch));
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 5);

  behavior.clearMessages();
  info.setValue("NAV_HEADING", 187);
  std::unique_ptr<IvPFunction> complete_ipf(behavior.onRunState());
  EXPECT_EQ(complete_ipf, nullptr);
}
