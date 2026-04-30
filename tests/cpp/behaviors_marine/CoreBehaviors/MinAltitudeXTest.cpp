#include <gtest/gtest.h>

#include <memory>

#include "BHV_MinAltitudeX.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV min altitude x behavior: converts altitude clearance into maximum allowed depth.
TEST(BHVMinAltitudeXTest, ConvertsAltitudeClearanceIntoMaximumAllowedDepth)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 20);
  info.setValue("NAV_ALTITUDE", 15);

  BHV_MinAltitudeX behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("min_altitude", "5"));
  ASSERT_TRUE(behavior.setParam("missing_altitude_critical", "false"));
  EXPECT_FALSE(behavior.setParam("min_altitude", "-1"));
  EXPECT_FALSE(behavior.setParam("missing_altitude_critical", "maybe"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 25), evalOneDimPoint(*ipf, 40));
}

// Covers BHV min altitude x behavior: clips allowed depth at surface when bottom is too shallow.
TEST(BHVMinAltitudeXTest, ClipsAllowedDepthAtSurfaceWhenBottomIsTooShallow)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 2);
  info.setValue("NAV_ALTITUDE", 1);

  BHV_MinAltitudeX behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("min_altitude", "5"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalOneDimPoint(*ipf, 0), evalOneDimPoint(*ipf, 5));
  EXPECT_GT(evalOneDimPoint(*ipf, 0), evalOneDimPoint(*ipf, 50));
}

// Covers BHV min altitude x behavior: missing depth or altitude posts status and suppresses objective.
TEST(BHVMinAltitudeXTest, MissingDepthOrAltitudePostsStatusAndSuppressesObjective)
{
  InfoBuffer missing_depth;
  missing_depth.setValue("NAV_ALTITUDE", 20);

  BHV_MinAltitudeX no_depth(makeDepthDomain());
  no_depth.setInfoBuffer(&missing_depth);
  ASSERT_TRUE(no_depth.setParam("min_altitude", "5"));

  std::unique_ptr<IvPFunction> no_depth_ipf(no_depth.onRunState());
  EXPECT_EQ(no_depth_ipf, nullptr);
  EXPECT_FALSE(no_depth.stateOK());

  VarDataPair depth_status;
  ASSERT_TRUE(findBehaviorMessage(no_depth.getMessages(), "MIN_ALT_STATUS", depth_status));
  ASSERT_TRUE(depth_status.is_string());
  EXPECT_NE(depth_status.get_sdata().find("NAV_DEPTH"), std::string::npos);

  InfoBuffer missing_altitude;
  missing_altitude.setValue("NAV_DEPTH", 10);

  BHV_MinAltitudeX no_altitude(makeDepthDomain());
  no_altitude.setInfoBuffer(&missing_altitude);
  ASSERT_TRUE(no_altitude.setParam("min_altitude", "5"));
  ASSERT_TRUE(no_altitude.setParam("missing_altitude_critical", "false"));

  std::unique_ptr<IvPFunction> no_altitude_ipf(no_altitude.onRunState());
  EXPECT_EQ(no_altitude_ipf, nullptr);
  EXPECT_FALSE(no_altitude.stateOK());

  VarDataPair altitude_status;
  ASSERT_TRUE(findBehaviorMessage(no_altitude.getMessages(), "MIN_ALT_STATUS", altitude_status));
  ASSERT_TRUE(altitude_status.is_string());
  EXPECT_NE(altitude_status.get_sdata().find("NAV_ALTITUDE"), std::string::npos);
}

// Covers BHV min altitude x behavior: zero altitude produces no constraint objective.
TEST(BHVMinAltitudeXTest, ZeroAltitudeProducesNoConstraintObjective)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 10);
  info.setValue("NAV_ALTITUDE", 0);

  BHV_MinAltitudeX behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("min_altitude", "5"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());
}

// Covers BHV min altitude x behavior: zero minimum altitude allows depth to bottom estimate.
TEST(BHVMinAltitudeXTest, ZeroMinimumAltitudeAllowsDepthToBottomEstimate)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 30);
  info.setValue("NAV_ALTITUDE", 20);

  BHV_MinAltitudeX behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("min_altitude", "0"));
  ASSERT_TRUE(behavior.setParam("priority", "45"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_DOUBLE_EQ(ipf->getPWT(), 45);
  EXPECT_GT(evalOneDimPoint(*ipf, 50), evalOneDimPoint(*ipf, 80));
  EXPECT_GT(evalOneDimPoint(*ipf, 20), evalOneDimPoint(*ipf, 80));
}

// Covers BHV min altitude x behavior: missing altitude critical flag does not change current error path.
TEST(BHVMinAltitudeXTest, MissingAltitudeCriticalFlagDoesNotChangeCurrentErrorPath)
{
  InfoBuffer info;
  info.setValue("NAV_DEPTH", 10);

  BHV_MinAltitudeX critical(makeDepthDomain());
  critical.setInfoBuffer(&info);
  ASSERT_TRUE(critical.setParam("min_altitude", "5"));
  ASSERT_TRUE(critical.setParam("missing_altitude_critical", "true"));

  std::unique_ptr<IvPFunction> critical_ipf(critical.onRunState());
  EXPECT_EQ(critical_ipf, nullptr);
  EXPECT_FALSE(critical.stateOK());

  BHV_MinAltitudeX noncritical(makeDepthDomain());
  noncritical.setInfoBuffer(&info);
  ASSERT_TRUE(noncritical.setParam("min_altitude", "5"));
  ASSERT_TRUE(noncritical.setParam("missing_altitude_critical", "false"));

  std::unique_ptr<IvPFunction> noncritical_ipf(noncritical.onRunState());
  EXPECT_EQ(noncritical_ipf, nullptr);
  EXPECT_FALSE(noncritical.stateOK());

  VarDataPair status;
  ASSERT_TRUE(findBehaviorMessage(noncritical.getMessages(), "MIN_ALT_STATUS",
                                  status));
  EXPECT_EQ(status.get_sdata(), "Missing NAV_ALTITUDE info");
}
