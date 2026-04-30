#include <gtest/gtest.h>

#include <memory>

#include "BHV_HeadingBias.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV heading bias behavior: requires heading and builds course objective.
TEST(BHVHeadingBiasTest, RequiresHeadingAndBuildsCourseObjective)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 350);

  BHV_HeadingBias behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("bias", "right"));
  EXPECT_TRUE(behavior.setParam("bias", "left"));
  EXPECT_FALSE(behavior.setParam("bias", "center"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  InfoBuffer missing_info;
  BHV_HeadingBias missing_heading(makeCourseSpeedDomain());
  missing_heading.setInfoBuffer(&missing_info);

  std::unique_ptr<IvPFunction> missing_ipf(missing_heading.onRunState());
  EXPECT_EQ(missing_ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(missing_heading.getMessages(), "BHV_WARNING", warning));
  EXPECT_TRUE(warning.is_string());
}

// Covers BHV heading bias behavior: left and right currently build same starboard bias objective.
TEST(BHVHeadingBiasTest, LeftAndRightCurrentlyBuildSameStarboardBiasObjective)
{
  // Pin current behavior: both accepted bias strings build the same objective
  // around the starboard-side heading preference.
  InfoBuffer info;
  info.setValue("NAV_HEADING", 350);

  BHV_HeadingBias right(makeCourseSpeedDomain());
  right.setInfoBuffer(&info);
  ASSERT_TRUE(right.setParam("bias", "right"));
  std::unique_ptr<IvPFunction> right_ipf(right.onRunState());
  ASSERT_NE(right_ipf, nullptr);

  BHV_HeadingBias left(makeCourseSpeedDomain());
  left.setInfoBuffer(&info);
  ASSERT_TRUE(left.setParam("bias", "left"));
  std::unique_ptr<IvPFunction> left_ipf(left.onRunState());
  ASSERT_NE(left_ipf, nullptr);

  EXPECT_DOUBLE_EQ(evalOneDimPoint(*right_ipf, 80),
                   evalOneDimPoint(*left_ipf, 80));
  EXPECT_DOUBLE_EQ(evalOneDimPoint(*right_ipf, 260),
                   evalOneDimPoint(*left_ipf, 260));
}

// Covers BHV heading bias behavior: fails without course dimension.
TEST(BHVHeadingBiasTest, FailsWithoutCourseDimension)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 90);

  BHV_HeadingBias behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("No 'heading/course' variable"),
            std::string::npos);
}

// Covers BHV heading bias behavior: starboard bias wraps across north with default priority.
TEST(BHVHeadingBiasTest, StarboardBiasWrapsAcrossNorthWithDefaultPriority)
{
  InfoBuffer info;
  info.setValue("NAV_HEADING", 300);

  BHV_HeadingBias behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("bias", "RIGHT"));
  EXPECT_FALSE(behavior.setParam("priority", "45"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 100);

  // The utility peak wraps across north, so symmetric headings around the bias
  // have equal utility.
  EXPECT_DOUBLE_EQ(evalOneDimPoint(*ipf, 30), evalOneDimPoint(*ipf, 210));
}

// Covers BHV heading bias behavior: missing heading warning names required navigation mail.
TEST(BHVHeadingBiasTest, MissingHeadingWarningNamesRequiredNavigationMail)
{
  BHV_HeadingBias behavior(makeCourseSpeedDomain());

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("HEADING"), std::string::npos);
}
