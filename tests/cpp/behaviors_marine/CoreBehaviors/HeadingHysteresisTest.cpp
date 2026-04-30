#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "BHV_HeadingHysteresis.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV heading hysteresis behavior: filters configured heading variable into telemetry.
TEST(BHVHeadingHysteresisTest, FiltersConfiguredHeadingVariableIntoTelemetry)
{
  InfoBuffer info;
  info.setCurrTime(5);
  info.setValue("HDG_UNFILTERED", 90);

  BHV_HeadingHysteresis behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("memory_time", "10"));
  ASSERT_TRUE(behavior.setParam("filter_var", "HDG_UNFILTERED"));
  EXPECT_FALSE(behavior.setParam("memory_time", "-1"));
  EXPECT_FALSE(behavior.setParam("filter_var", "BAD VAR"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair average;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HIST_HEADING_AVERAGE", average));
  EXPECT_DOUBLE_EQ(average.get_ddata(), 90);

  VarDataPair variance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HIST_HEADING_VARIANCE", variance));
  EXPECT_DOUBLE_EQ(variance.get_ddata(), 0);
}

// Covers BHV heading hysteresis behavior: averages wrapped headings and prunes stale history.
TEST(BHVHeadingHysteresisTest, AveragesWrappedHeadingsAndPrunesStaleHistory)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("DESIRED_HEADING_UNFILTERED", 350);

  BHV_HeadingHysteresis behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("memory_time", "5"));

  EXPECT_EQ(std::unique_ptr<IvPFunction>(behavior.onRunState()), nullptr);
  behavior.clearMessages();

  info.setCurrTime(1);
  info.setValue("DESIRED_HEADING_UNFILTERED", 10);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(behavior.onRunState()), nullptr);

  VarDataPair average;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HIST_HEADING_AVERAGE", average));
  EXPECT_LT(std::min(std::abs(average.get_ddata()), std::abs(average.get_ddata() - 360)), 1e-6);

  VarDataPair variance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HIST_HEADING_VARIANCE", variance));
  EXPECT_NEAR(variance.get_ddata(), std::sqrt(50.0), 1e-6);

  behavior.clearMessages();
  info.setCurrTime(10);
  info.setValue("DESIRED_HEADING_UNFILTERED", 90);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(behavior.onRunState()), nullptr);

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HIST_HEADING_AVERAGE", average));
  EXPECT_DOUBLE_EQ(average.get_ddata(), 90);
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "HIST_HEADING_VARIANCE", variance));
  EXPECT_DOUBLE_EQ(variance.get_ddata(), 0);
}

// Covers BHV heading hysteresis behavior: warns when required memory or filtered mail is missing.
TEST(BHVHeadingHysteresisTest, WarnsWhenRequiredMemoryOrFilteredMailIsMissing)
{
  InfoBuffer info;
  info.setCurrTime(1);

  BHV_HeadingHysteresis missing_memory(makeCourseSpeedDomain());
  missing_memory.setInfoBuffer(&info);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(missing_memory.onRunState()), nullptr);
  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(missing_memory.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("memory_time"), std::string::npos);

  BHV_HeadingHysteresis missing_heading(makeCourseSpeedDomain());
  missing_heading.setInfoBuffer(&info);
  ASSERT_TRUE(missing_heading.setParam("memory_time", "5"));
  EXPECT_EQ(std::unique_ptr<IvPFunction>(missing_heading.onRunState()), nullptr);
  ASSERT_TRUE(findBehaviorMessage(missing_heading.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("not found"), std::string::npos);
}

// Covers BHV heading hysteresis behavior: idle state samples contribute to next run average.
TEST(BHVHeadingHysteresisTest, IdleStateSamplesContributeToNextRunAverage)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("DESIRED_HEADING_UNFILTERED", 30);

  BHV_HeadingHysteresis behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("memory_time", "10"));

  behavior.onIdleState();
  EXPECT_TRUE(behavior.getMessages().empty());

  info.setCurrTime(1);
  info.setValue("DESIRED_HEADING_UNFILTERED", 60);
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair average;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(),
                                  "HIST_HEADING_AVERAGE", average));
  EXPECT_NEAR(average.get_ddata(), 45, 1e-6);

  VarDataPair variance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(),
                                  "HIST_HEADING_VARIANCE", variance));
  EXPECT_NEAR(variance.get_ddata(), std::sqrt(450.0) / 2.0, 1e-6);
}

// Covers BHV heading hysteresis behavior: zero memory keeps only current timestamp sample.
TEST(BHVHeadingHysteresisTest, ZeroMemoryKeepsOnlyCurrentTimestampSample)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("DESIRED_HEADING_UNFILTERED", 180);

  BHV_HeadingHysteresis behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("memory_time", "0"));
  EXPECT_EQ(std::unique_ptr<IvPFunction>(behavior.onRunState()), nullptr);

  behavior.clearMessages();
  info.setCurrTime(0);
  info.setValue("DESIRED_HEADING_UNFILTERED", 220);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(behavior.onRunState()), nullptr);

  VarDataPair average;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(),
                                  "HIST_HEADING_AVERAGE", average));
  EXPECT_NEAR(average.get_ddata(), 200, 1e-6);

  behavior.clearMessages();
  info.setCurrTime(0.1);
  info.setValue("DESIRED_HEADING_UNFILTERED", 90);
  EXPECT_EQ(std::unique_ptr<IvPFunction>(behavior.onRunState()), nullptr);

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(),
                                  "HIST_HEADING_AVERAGE", average));
  EXPECT_DOUBLE_EQ(average.get_ddata(), 90);
}
