#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_Loiter.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV loiter behavior: acquires convex loiter polygon and posts status.
TEST(BHVLoiterTest, AcquiresConvexLoiterPolygonAndPostsStatus)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", -100);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Loiter behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("polygon", "radial::x=0,y=0,radius=40,pts=4,snap=1"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("clockwise", "best"));
  ASSERT_TRUE(behavior.setParam("acquire_dist", "10"));
  ASSERT_TRUE(behavior.setParam("radius", "5"));
  ASSERT_TRUE(behavior.setParam("post_suffix", "survey"));
  EXPECT_FALSE(behavior.setParam("clockwise", "sideways"));
  EXPECT_FALSE(behavior.setParam("speed", "-1"));
  EXPECT_FALSE(behavior.setParam("patience", "100"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair report;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LOITER_REPORT_SURVEY", report));
  ASSERT_TRUE(report.is_string());
  EXPECT_NE(report.get_sdata().find("acquire_mode=true"), std::string::npos);

  VarDataPair acquire;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LOITER_ACQUIRE_SURVEY", acquire));
  EXPECT_DOUBLE_EQ(acquire.get_ddata(), 1);

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("pts={"), std::string::npos);
}

// Covers BHV loiter behavior: stable inside polygon uses alternate speed and custom hints.
TEST(BHVLoiterTest, StableInsidePolygonUsesAlternateSpeedAndCustomHints)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 1);

  BHV_Loiter behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("polygon", "radial::x=0,y=0,radius=40,pts=4,snap=1"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("speed_alt", "1"));
  ASSERT_TRUE(behavior.setParam("use_alt_speed", "true"));
  ASSERT_TRUE(behavior.setParam("post_suffix", "survey"));
  ASSERT_TRUE(behavior.setParam("visual_hints",
                                "edge_color=yellow,nextpt_color=green,label=loiter_box"));
  ASSERT_TRUE(behavior.setParam("ipf-type", "zaic_spd"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair mode;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LOITER_MODE_SURVEY", mode));
  EXPECT_EQ(mode.get_sdata(), "acquiring_internal");

  VarDataPair acquire;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LOITER_ACQUIRE_SURVEY", acquire));
  EXPECT_DOUBLE_EQ(acquire.get_ddata(), 0);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 1),
            evalCourseSpeedPoint(*ipf, 0, 2));

  VarDataPair polygon;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POLYGON", polygon));
  EXPECT_NE(polygon.get_sdata().find("label=loiter_box"), std::string::npos);
  EXPECT_NE(polygon.get_sdata().find("edge_color=yellow"), std::string::npos);
}

// Covers BHV loiter behavior: idle and complete erase viewer objects and post completion.
TEST(BHVLoiterTest, IdleAndCompleteEraseViewerObjectsAndPostCompletion)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", -100);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Loiter behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("polygon", "radial::x=0,y=0,radius=40,pts=4,snap=1"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));

  behavior.onIdleToRunState();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  behavior.clearMessages();
  behavior.onIdleState();

  bool erased_point = false;
  bool erased_polygon = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      erased_point |= (message.get_sdata().find("active=false") != std::string::npos);
    if(message.get_var() == "VIEW_POLYGON" && message.is_string())
      erased_polygon |= (message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(erased_point);
  EXPECT_TRUE(erased_polygon);

  behavior.clearMessages();
  behavior.onCompleteState();

  VarDataPair completed;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LOITER_COMPLETE", completed));
  EXPECT_EQ(completed.get_sdata(), "true");
}

// Covers BHV loiter behavior: center assign repositions polygon and config status.
TEST(BHVLoiterTest, CenterAssignRepositionsPolygonAndConfigStatus)
{
  InfoBuffer info;
  info.setValue("NAV_X", 100);
  info.setValue("NAV_Y", -20);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_Loiter behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("polygon", "radial::x=0,y=0,radius=30,pts=4,snap=1"));
  ASSERT_TRUE(behavior.setParam("center_assign", "x=100,y=-20"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));

  behavior.onIdleToRunState();

  VarDataPair settings;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_SETTINGS", settings));
  ASSERT_TRUE(settings.is_string());
  EXPECT_NE(settings.get_sdata().find("x=100.00"), std::string::npos);
  EXPECT_NE(settings.get_sdata().find("y=-20.00"), std::string::npos);

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair report;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LOITER_REPORT", report));
  ASSERT_TRUE(report.is_string());
  EXPECT_NE(report.get_sdata().find("acquire_mode=false"), std::string::npos);
}

// Covers BHV loiter behavior: empty loiter specification warns and suppresses objective.
TEST(BHVLoiterTest, EmptyLoiterSpecificationWarnsAndSuppressesObjective)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);

  BHV_Loiter behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  ASSERT_TRUE(warning.is_string());
  EXPECT_NE(warning.get_sdata().find("Empty/NULL Loiter Specification"), std::string::npos);

  bool erased_point = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      erased_point |= (message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(erased_point);
}
