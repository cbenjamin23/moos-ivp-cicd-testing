#include <gtest/gtest.h>

#include <memory>

#include "BHV_MaintainHeading.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV maintain heading behavior: captures heading on active and resets after idle.
TEST(BHVMaintainHeadingTest, CapturesHeadingOnActiveAndResetsAfterIdle)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 45.0);

  BHV_MaintainHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading", "on_active"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "5"));
  ASSERT_TRUE(behavior.setParam("basewidth", "80"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "10"));
  EXPECT_FALSE(behavior.setParam("heading", "east"));
  EXPECT_FALSE(behavior.setParam("peakwidth", "-1"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*first_ipf, 45), evalOneDimPoint(*first_ipf, 90));

  info.setValue("NAV_HEADING", 90.0);
  std::unique_ptr<IvPFunction> still_locked_ipf(behavior.onRunState());
  ASSERT_NE(still_locked_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*still_locked_ipf, 45), evalOneDimPoint(*still_locked_ipf, 90));

  behavior.onIdleState();
  std::unique_ptr<IvPFunction> reset_ipf(behavior.onRunState());
  ASSERT_NE(reset_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*reset_ipf, 90), evalOneDimPoint(*reset_ipf, 45));
}

// Covers BHV maintain heading behavior: explicit heading normalizes and ignores changing nav heading.
TEST(BHVMaintainHeadingTest, ExplicitHeadingNormalizesAndIgnoresChangingNavHeading)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 10.0);

  BHV_MaintainHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading", "-1"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "0"));
  ASSERT_TRUE(behavior.setParam("basewidth", "40"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "20"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*first_ipf, 359), evalOneDimPoint(*first_ipf, 180));

  info.setValue("NAV_HEADING", 180.0);
  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*second_ipf, 359), evalOneDimPoint(*second_ipf, 180));
}

// Covers BHV maintain heading behavior: warns on dubious explicit heading and missing nav heading.
TEST(BHVMaintainHeadingTest, WarnsOnDubiousExplicitHeadingAndMissingNavHeading)
{
  BHV_MaintainHeading dubious(makeCourseSpeedDomain());
  ASSERT_TRUE(dubious.setParam("heading", "-181"));
  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(dubious.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("Dubious initial heading"),
            std::string::npos);

  BHV_MaintainHeading missing_heading(makeCourseSpeedDomain());
  std::unique_ptr<IvPFunction> ipf(missing_heading.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair missing_warning;
  ASSERT_TRUE(findBehaviorMessage(missing_heading.getMessages(), "BHV_WARNING",
                                  missing_warning));
  EXPECT_NE(missing_warning.get_sdata().find("No Ownship NAV_HEADING"),
            std::string::npos);
}

// Covers BHV maintain heading behavior: priority weight and wide ZAIC params propagate to objective.
TEST(BHVMaintainHeadingTest, PriorityWeightAndWideZaicParamsPropagateToObjective)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 180);

  BHV_MaintainHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading", "180"));
  ASSERT_TRUE(behavior.setParam("priority", "35"));
  ASSERT_TRUE(behavior.setParam("peakwidth", "180"));
  ASSERT_TRUE(behavior.setParam("basewidth", "0"));
  ASSERT_TRUE(behavior.setParam("summitdelta", "150"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 35);

  EXPECT_GT(evalOneDimPoint(*ipf, 180), evalOneDimPoint(*ipf, 0));
}

// Covers BHV maintain heading behavior: on active mode recaptures heading after idle transition.
TEST(BHVMaintainHeadingTest, OnActiveModeRecapturesHeadingAfterIdleTransition)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 20);

  BHV_MaintainHeading behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("heading", "on_active"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*first_ipf, 20), evalOneDimPoint(*first_ipf, 120));

  behavior.onIdleState();
  info.setValue("NAV_HEADING", 120);

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*second_ipf, 120), evalOneDimPoint(*second_ipf, 20));
}
