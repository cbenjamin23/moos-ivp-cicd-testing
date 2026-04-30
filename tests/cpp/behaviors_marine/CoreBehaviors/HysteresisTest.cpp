#include <gtest/gtest.h>

#include <memory>

#include "BHV_Hysteresis.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV hysteresis behavior: accepts heading window configuration.
TEST(BHVHysteresisTest, AcceptsHeadingWindowConfiguration)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1);

  BHV_Hysteresis behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("memory_time", "10"));
  ASSERT_TRUE(behavior.setParam("min_heading_window", "20"));
  ASSERT_TRUE(behavior.setParam("max_heading_window", "60"));
  ASSERT_TRUE(behavior.setParam("max_window_utility", "80"));
  EXPECT_FALSE(behavior.setParam("memory_time", "-1"));
  EXPECT_FALSE(behavior.setParam("max_window_utility", "101"));
  EXPECT_FALSE(behavior.setParam("turn_range", "30"));
}

// Covers BHV hysteresis behavior: missing memory is reported before navigation checks.
TEST(BHVHysteresisTest, MissingMemoryIsReportedBeforeNavigationChecks)
{
  InfoBuffer info;
  BHV_Hysteresis behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("memory_time"), std::string::npos);
}

// Covers BHV hysteresis behavior: heading window bounds are clamped to consistent ordering.
TEST(BHVHysteresisTest, HeadingWindowBoundsAreClampedToConsistentOrdering)
{
  BHV_Hysteresis behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("max_heading_window", "20"));
  ASSERT_TRUE(behavior.setParam("min_heading_window", "80"));
  ASSERT_TRUE(behavior.setParam("max_heading_window", "40"));

  EXPECT_FALSE(behavior.setParam("min_heading_window", "361"));
  EXPECT_FALSE(behavior.setParam("max_heading_window", "-1"));
}
