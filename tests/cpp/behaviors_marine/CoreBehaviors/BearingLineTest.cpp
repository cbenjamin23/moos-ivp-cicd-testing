#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_BearingLine.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV bearing line behavior: posts line and bearing point from ownship to configured point.
TEST(BHVBearingLineTest, PostsLineAndBearingPointFromOwnshipToConfiguredPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);

  BHV_BearingLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("bearing_point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("line_pct", "50"));
  ASSERT_TRUE(behavior.setParam("show_pt", "false"));
  EXPECT_TRUE(behavior.setParam("line_pct", "500"));
  EXPECT_FALSE(behavior.setParam("bearing_point", "bad"));
  EXPECT_FALSE(behavior.setParam("show_pt", "maybe"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  ASSERT_TRUE(seglist.is_string());
  const std::string seg_spec = seglist.get_sdata();
  EXPECT_NE(seg_spec.find("pts={0,0:0,100}"), std::string::npos);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", point));
  ASSERT_TRUE(point.is_string());
  const std::string point_spec = point.get_sdata();
  EXPECT_NE(point_spec.find("x=0"), std::string::npos);
  EXPECT_NE(point_spec.find("y=100"), std::string::npos);
  EXPECT_NE(point_spec.find("active=false"), std::string::npos);
}

// Covers BHV bearing line behavior: clips line percent and toggles viewer point.
TEST(BHVBearingLineTest, ClipsLinePercentAndTogglesViewerPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 3);
  info.setValue("NAV_Y", 4);

  BHV_BearingLine full_line(makeCourseSpeedDomain());
  full_line.setInfoBuffer(&info);
  ASSERT_TRUE(full_line.setParam("bearing_point", "x=13,y=4"));
  ASSERT_TRUE(full_line.setParam("line_pct", "500"));

  std::unique_ptr<IvPFunction> full_ipf(full_line.onRunState());
  EXPECT_EQ(full_ipf, nullptr);

  VarDataPair full_seglist;
  ASSERT_TRUE(findBehaviorMessage(full_line.getMessages(), "VIEW_SEGLIST", full_seglist));
  ASSERT_TRUE(full_seglist.is_string());
  EXPECT_NE(full_seglist.get_sdata().find("pts={3,4:13,4}"), std::string::npos);

  VarDataPair full_point;
  ASSERT_TRUE(findBehaviorMessage(full_line.getMessages(), "VIEW_POINT", full_point));
  ASSERT_TRUE(full_point.is_string());
  EXPECT_NE(full_point.get_sdata().find("x=13"), std::string::npos);
  EXPECT_NE(full_point.get_sdata().find("y=4"), std::string::npos);
  EXPECT_EQ(full_point.get_sdata().find("active=false"), std::string::npos);

  BHV_BearingLine clipped_line(makeCourseSpeedDomain());
  clipped_line.setInfoBuffer(&info);
  ASSERT_TRUE(clipped_line.setParam("bearing_point", "13,4"));
  ASSERT_TRUE(clipped_line.setParam("line_pct", "-10"));

  std::unique_ptr<IvPFunction> clipped_ipf(clipped_line.onRunState());
  EXPECT_EQ(clipped_ipf, nullptr);

  VarDataPair clipped_seglist;
  ASSERT_TRUE(findBehaviorMessage(clipped_line.getMessages(), "VIEW_SEGLIST", clipped_seglist));
  ASSERT_TRUE(clipped_seglist.is_string());
  EXPECT_NE(clipped_seglist.get_sdata().find("pts={3,4:3,4}"), std::string::npos);
}

// Covers BHV bearing line behavior: idle and complete post erasable viewer objects.
TEST(BHVBearingLineTest, IdleAndCompletePostErasableViewerObjects)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);

  BHV_BearingLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("bearing_point", "x=0,y=25"));

  std::unique_ptr<IvPFunction> run_ipf(behavior.onRunState());
  EXPECT_EQ(run_ipf, nullptr);

  behavior.onIdleState();

  bool saw_inactive_seglist = false;
  bool saw_inactive_point = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string())
      saw_inactive_seglist |=
        (message.get_sdata().find("active=false") != std::string::npos);
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      saw_inactive_point |=
        (message.get_sdata().find("active=false") != std::string::npos);
  }

  EXPECT_TRUE(saw_inactive_seglist);
  EXPECT_TRUE(saw_inactive_point);
}

// Covers BHV bearing line behavior: missing navigation mail posts warnings but still posts viewer payloads.
TEST(BHVBearingLineTest, MissingNavigationMailPostsWarningsButStillPostsViewerPayloads)
{
  InfoBuffer info;

  BHV_BearingLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("bearing_point", "x=10,y=0"));
  ASSERT_TRUE(behavior.setParam("show_pt", "false"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_NE(warning.get_sdata().find("NAV_"), std::string::npos);

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  EXPECT_NE(seglist.get_sdata().find("pts={0,0:"), std::string::npos);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("active=false"), std::string::npos);
}

// Covers BHV bearing line behavior: zero range bearing point produces degenerate active seglist.
TEST(BHVBearingLineTest, ZeroRangeBearingPointProducesDegenerateActiveSeglist)
{
  InfoBuffer info;
  info.setValue("NAV_X", 7);
  info.setValue("NAV_Y", -3);

  BHV_BearingLine behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("bearing_point", "x=7,y=-3"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  EXPECT_NE(seglist.get_sdata().find("pts={7,-3:7,-3}"), std::string::npos);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", point));
  EXPECT_EQ(point.get_sdata().find("active=false"), std::string::npos);
}
