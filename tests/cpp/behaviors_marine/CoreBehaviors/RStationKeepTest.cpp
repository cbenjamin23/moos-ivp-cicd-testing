#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_RStationKeep.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers bhvr station keep behavior: seeks station point relative to contact.
TEST(BHVRStationKeepTest, SeeksStationPointRelativeToContact)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);
  info.setValue("ALPHA_NAV_X", 0);
  info.setValue("ALPHA_NAV_Y", 100);
  info.setValue("ALPHA_NAV_HEADING", 0);
  info.setValue("ALPHA_NAV_SPEED", 2);

  BHV_RStationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("rstation_position", "20,180"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "5"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "20"));
  ASSERT_TRUE(behavior.setParam("extra_speed", "2"));
  EXPECT_FALSE(behavior.setParam("inner_radius", "0"));
  EXPECT_FALSE(behavior.setParam("extra_speed", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DIST_TO_RSTATION", dist));
  EXPECT_DOUBLE_EQ(dist.get_ddata(), 80);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RVIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("abe_rstation"), std::string::npos);
}

// Covers bhvr station keep behavior: returns no objective inside inner radius but keeps viewer point.
TEST(BHVRStationKeepTest, ReturnsNoObjectiveInsideInnerRadiusButKeepsViewerPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 95);
  info.setValue("NAV_HEADING", 180);
  info.setValue("NAV_SPEED", 0.5);
  info.setValue("ALPHA_NAV_X", 0);
  info.setValue("ALPHA_NAV_Y", 100);
  info.setValue("ALPHA_NAV_HEADING", 0);
  info.setValue("ALPHA_NAV_SPEED", 2);

  BHV_RStationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("rstation_position", "5,180"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "6"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DIST_TO_RSTATION", dist));
  EXPECT_DOUBLE_EQ(dist.get_ddata(), 0);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RVIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("x=0"), std::string::npos);
  EXPECT_NE(point.get_sdata().find("y=95"), std::string::npos);
}

// Covers bhvr station keep behavior: missing contact mail suppresses objective and warns.
TEST(BHVRStationKeepTest, MissingContactMailSuppressesObjectiveAndWarns)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);
  info.setValue("ALPHA_NAV_X", 0);
  info.setValue("ALPHA_NAV_Y", 100);

  BHV_RStationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("rstation_position", "20,180"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  ASSERT_TRUE(warning.is_string());
  EXPECT_FALSE(warning.get_sdata().empty());
}

// Covers bhvr station keep behavior: contact heading rotates relative station offset across north.
TEST(BHVRStationKeepTest, ContactHeadingRotatesRelativeStationOffsetAcrossNorth)
{
  InfoBuffer info;
  info.setValue("NAV_X", 20);
  info.setValue("NAV_Y", 80);
  info.setValue("NAV_HEADING", 270);
  info.setValue("NAV_SPEED", 1);
  info.setValue("ALPHA_NAV_X", 10);
  info.setValue("ALPHA_NAV_Y", 100);
  info.setValue("ALPHA_NAV_HEADING", 350);
  info.setValue("ALPHA_NAV_SPEED", 2);

  BHV_RStationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("rstation_position", "20,20"));
  ASSERT_TRUE(behavior.setParam("inner_radius", "2"));
  ASSERT_TRUE(behavior.setParam("outer_radius", "25"));
  ASSERT_TRUE(behavior.setParam("extra_speed", "3"));
  ASSERT_TRUE(behavior.setParam("priority", "45"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_DOUBLE_EQ(ipf->getPWT(), 45);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RVIEW_POINT", point));
  ASSERT_TRUE(point.is_string());
  const std::string spec = point.get_sdata();
  EXPECT_NE(spec.find("x=13"), std::string::npos);
  EXPECT_NE(spec.find("y=120"), std::string::npos);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 350, 3),
            evalCourseSpeedPoint(*ipf, 90, 0));
}

// Covers bhvr station keep behavior: center activate captures current ownship offset when idle.
TEST(BHVRStationKeepTest, CenterActivateCapturesCurrentOwnshipOffsetWhenIdle)
{
  InfoBuffer info;
  info.setValue("NAV_X", 10);
  info.setValue("NAV_Y", 80);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);
  info.setValue("ALPHA_NAV_X", 10);
  info.setValue("ALPHA_NAV_Y", 100);
  info.setValue("ALPHA_NAV_HEADING", 0);
  info.setValue("ALPHA_NAV_SPEED", 2);

  BHV_RStationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("rstation_position", "30,180"));
  ASSERT_TRUE(behavior.setParam("center_activate", "true"));
  ASSERT_FALSE(behavior.setParam("center_activate", "sometimes"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  info.setValue("NAV_X", 20);
  info.setValue("NAV_Y", 75);
  behavior.onIdleState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  EXPECT_EQ(second_ipf, nullptr);

  VarDataPair dist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "DIST_TO_RSTATION", dist));
  EXPECT_DOUBLE_EQ(dist.get_ddata(), 0);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RVIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("x=20"), std::string::npos);
  EXPECT_NE(point.get_sdata().find("y=75"), std::string::npos);
}

// Covers bhvr station keep behavior: idle erases station circle and viewer point.
TEST(BHVRStationKeepTest, IdleErasesStationCircleAndViewerPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);
  info.setValue("ALPHA_NAV_X", 0);
  info.setValue("ALPHA_NAV_Y", 100);
  info.setValue("ALPHA_NAV_HEADING", 0);
  info.setValue("ALPHA_NAV_SPEED", 2);

  BHV_RStationKeep behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("rstation_position", "20,180"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  behavior.clearMessages();
  behavior.onIdleState();

  VarDataPair circle;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RSTATION_CIRCLE", circle));
  ASSERT_TRUE(circle.is_string());
  EXPECT_NE(circle.get_sdata().find(",0,abe_rstation"), std::string::npos);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RVIEW_POINT", point));
  ASSERT_TRUE(point.is_string());
  EXPECT_NE(point.get_sdata().find("active=false"), std::string::npos);
}
