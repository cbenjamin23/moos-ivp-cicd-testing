#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_HSLine.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers bhvhs line behavior: projects desired heading speed line for one leg time.
TEST(BHVHSLineTest, ProjectsDesiredHeadingSpeedLineForOneLegTime)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("DESIRED_HEADING", 90);
  info.setValue("DESIRED_SPEED", 2);

  BHV_HSLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("time_on_leg", "10"));
  EXPECT_FALSE(behavior.setParam("time_on_leg", "0"));
  EXPECT_FALSE(behavior.setParam("time_on_leg", "fast"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  ASSERT_TRUE(seglist.is_string());
  const std::string spec = seglist.get_sdata();
  EXPECT_NE(spec.find("active=true"), std::string::npos);
  EXPECT_NE(spec.find("pts={0,0:20,0}"), std::string::npos);
}

// Covers bhvhs line behavior: scales projection by maximum desired speed observed.
TEST(BHVHSLineTest, ScalesProjectionByMaximumDesiredSpeedObserved)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("DESIRED_HEADING", 0);
  info.setValue("DESIRED_SPEED", 4);

  BHV_HSLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("time_on_leg", "10"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  EXPECT_EQ(first_ipf, nullptr);

  VarDataPair first_seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", first_seglist));
  ASSERT_TRUE(first_seglist.is_string());
  EXPECT_NE(first_seglist.get_sdata().find("pts={0,0:0,40}"), std::string::npos);

  info.setValue("DESIRED_SPEED", 2);

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  EXPECT_EQ(second_ipf, nullptr);

  bool saw_scaled_projection = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string())
      saw_scaled_projection |=
        (message.get_sdata().find("pts={0,0:0,10}") != std::string::npos);
  }
  EXPECT_TRUE(saw_scaled_projection);
}

// Covers bhvhs line behavior: idle after run posts inactive viewer segment.
TEST(BHVHSLineTest, IdleAfterRunPostsInactiveViewerSegment)
{
  InfoBuffer info;
  info.setValue("NAV_X", -5);
  info.setValue("NAV_Y", 7);
  info.setValue("DESIRED_HEADING", 180);
  info.setValue("DESIRED_SPEED", 1);

  BHV_HSLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> run_ipf(behavior.onRunState());
  EXPECT_EQ(run_ipf, nullptr);

  behavior.onIdleState();

  bool saw_inactive_segment = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string())
      saw_inactive_segment |=
        (message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(saw_inactive_segment);
}

// Covers bhvhs line behavior: missing helm outputs post warnings but still emit degenerate segment.
TEST(BHVHSLineTest, MissingHelmOutputsPostWarningsButStillEmitDegenerateSegment)
{
  InfoBuffer info;
  info.setValue("NAV_X", 3);
  info.setValue("NAV_Y", -4);

  BHV_HSLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  bool saw_heading_warning = false;
  bool saw_speed_warning = false;
  bool saw_segment = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "BHV_WARNING" && message.is_string()) {
      saw_heading_warning |=
        (message.get_sdata().find("DESIRED_HEADING") != std::string::npos);
      saw_speed_warning |=
        (message.get_sdata().find("DESIRED_SPEED") != std::string::npos);
    }
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string()) {
      saw_segment |=
        (message.get_sdata().find("pts={3,-4:3,-4}") != std::string::npos);
    }
  }
  EXPECT_TRUE(saw_heading_warning);
  EXPECT_TRUE(saw_speed_warning);
  EXPECT_TRUE(saw_segment);
}

// Covers bhvhs line behavior: negative desired speed extends projection behind desired heading.
TEST(BHVHSLineTest, NegativeDesiredSpeedExtendsProjectionBehindDesiredHeading)
{
  InfoBuffer info;
  info.setValue("NAV_X", 1);
  info.setValue("NAV_Y", 2);
  info.setValue("DESIRED_HEADING", 90);
  info.setValue("DESIRED_SPEED", -1);

  BHV_HSLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("time_on_leg", "5"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  ASSERT_TRUE(seglist.is_string());
  EXPECT_NE(seglist.get_sdata().find("pts={1,2:-4,2}"), std::string::npos);
}
