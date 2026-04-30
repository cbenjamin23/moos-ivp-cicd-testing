#include <gtest/gtest.h>

#include <memory>

#include "BHV_ConstantSpeed.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV constant speed behavior: builds speed objective and posts mismatch telemetry.
TEST(BHVConstantSpeedTest, BuildsSpeedObjectiveAndPostsMismatchTelemetry)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 1.25);

  BHV_ConstantSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "3"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "0"));
  ASSERT_TRUE(behavior.setParam("basewidth", "1"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "20"));
  ASSERT_TRUE(behavior.setParam("speed_mismatch_var", "SPD_ERR"));
  EXPECT_FALSE(behavior.setParam("speed_mismatch_var", "BAD VAR"));
  EXPECT_FALSE(behavior.setParam("speed", "fast"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 3), evalOneDimPoint(*ipf, 0));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SPD_ERR", mismatch));
  EXPECT_FALSE(mismatch.is_string());
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 1.75);
}

// Covers BHV constant speed behavior: clips negative mission speed and still builds objective.
TEST(BHVConstantSpeedTest, ClipsNegativeMissionSpeedAndStillBuildsObjective)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 0.4);

  BHV_ConstantSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "-2"));
  ASSERT_TRUE(behavior.setParam("basewidth", "-10"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "-1"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "150"));
  ASSERT_TRUE(behavior.setParam("speed_mismatch_var", "SPD_ERR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 0), evalOneDimPoint(*ipf, 4));

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SPD_ERR", mismatch));
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 0.4);
}

// Covers BHV constant speed behavior: warns on missing nav speed but keeps objective available.
TEST(BHVConstantSpeedTest, WarnsOnMissingNavSpeedButKeepsObjectiveAvailable)
{
  BHV_ConstantSpeed behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("speed_mismatch_var", "SPD_ERR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("No ownship SPEED"), std::string::npos);

  VarDataPair mismatch;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "SPD_ERR", mismatch));
}

// Covers BHV constant speed behavior: fails without speed dimension in helm domain.
TEST(BHVConstantSpeedTest, FailsWithoutSpeedDimensionInHelmDomain)
{
  BHV_ConstantSpeed behavior(makeDepthDomain());

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("No 'speed' variable"), std::string::npos);
}

// Covers BHV constant speed behavior: mission style alpha constant speed config applies priority.
TEST(BHVConstantSpeedTest, MissionStyleAlphaConstantSpeedConfigAppliesPriority)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 0.2);

  BHV_ConstantSpeed behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  // Mirrors ivp/missions/s1_alpha/alpha.bhv constant speed settings.
  ASSERT_TRUE(behavior.setParam("pwt", "200"));
  ASSERT_TRUE(behavior.setParam("perpetual", "true"));
  ASSERT_TRUE(behavior.setParam("updates", "SPEED_UPDATE"));
  ASSERT_TRUE(behavior.setParam("speed", "0.5"));
  ASSERT_TRUE(behavior.setParam("duration", "10"));
  ASSERT_TRUE(behavior.setParam("duration_reset", "CONST_SPD_RESET=true"));
  ASSERT_TRUE(behavior.setParam("speed_mismatch_var", "SPD_ERR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 200);

  VarDataPair mismatch;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SPD_ERR", mismatch));
  EXPECT_DOUBLE_EQ(mismatch.get_ddata(), 0.3);
}
