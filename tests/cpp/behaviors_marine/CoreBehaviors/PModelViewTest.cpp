#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_PModelView.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV_P model view behavior: accepts viewer styling and returns no objective.
TEST(BHVPModelViewTest, AcceptsViewerStylingAndReturnsNoObjective)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_HEADING", 90);
  info.setValue("DESIRED_SPEED", 0);

  BHV_PModelView behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("edge_color", "white"));
  ASSERT_TRUE(behavior.setParam("vertex_color", "green"));
  ASSERT_TRUE(behavior.setParam("vertex_size", "3"));
  EXPECT_FALSE(behavior.setParam("edge_color", "not_a_color"));
  EXPECT_FALSE(behavior.setParam("vertex_size", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());
}

// Covers BHV_P model view behavior: draws and erases turn path when desired speed is non zero.
TEST(BHVPModelViewTest, DrawsAndErasesTurnPathWhenDesiredSpeedIsNonZero)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_HEADING", 135);
  info.setValue("DESIRED_SPEED", 1.5);

  BHV_PModelView behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair seglr;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLR", seglr));
  ASSERT_TRUE(seglr.is_string());
  EXPECT_NE(seglr.get_sdata().find("label="), std::string::npos);
  EXPECT_NE(seglr.get_sdata().find("_seglr"), std::string::npos);
  EXPECT_NE(seglr.get_sdata().find("ray_len=20"), std::string::npos);
  EXPECT_NE(seglr.get_sdata().find("vertex_color=lime_green"), std::string::npos);

  behavior.onRunToIdleState();

  bool saw_inactive_path = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLR" && message.is_string())
      saw_inactive_path |=
        (message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(saw_inactive_path);
}

// Covers BHV_P model view behavior: missing platform mail posts behavior errors.
TEST(BHVPModelViewTest, MissingPlatformMailPostsBehaviorErrors)
{
  InfoBuffer info;
  info.setValue("DESIRED_HEADING", 90);
  info.setValue("DESIRED_SPEED", 1);

  BHV_PModelView behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  ASSERT_TRUE(error.is_string());
  EXPECT_FALSE(error.get_sdata().empty());
}

// Covers BHV_P model view behavior: missing desired speed suppresses turn path without platform error.
TEST(BHVPModelViewTest, MissingDesiredSpeedSuppressesTurnPathWithoutPlatformError)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 45);
  info.setValue("NAV_SPEED", 2);

  BHV_PModelView behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair seglr;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLR", seglr));
}

// Covers BHV_P model view behavior: accepted style params do not override fixed turn path styling.
TEST(BHVPModelViewTest, AcceptedStyleParamsDoNotOverrideFixedTurnPathStyling)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);
  info.setValue("DESIRED_HEADING", 90);
  info.setValue("DESIRED_SPEED", 1);

  BHV_PModelView behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("edge_color", "white"));
  ASSERT_TRUE(behavior.setParam("vertex_color", "red"));
  ASSERT_TRUE(behavior.setParam("vertex_size", "6"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair seglr;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLR", seglr));
  EXPECT_NE(seglr.get_sdata().find("vertex_color=lime_green"), std::string::npos);
  EXPECT_NE(seglr.get_sdata().find("vertex_size=2"), std::string::npos);
}
