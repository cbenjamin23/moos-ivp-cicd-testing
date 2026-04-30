#include <gtest/gtest.h>

#include <memory>

#include "BHV_ZigZag.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV zig zag behavior: starts configured starboard zig and builds heading speed objective.
TEST(BHVZigZagTest, StartsConfiguredStarboardZigAndBuildsHeadingSpeedObjective)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_ZigZag behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("stem_hdg", "0"));
  ASSERT_TRUE(behavior.setParam("zig_angle", "20"));
  ASSERT_TRUE(behavior.setParam("zig_first", "star"));
  ASSERT_TRUE(behavior.setParam("hdg_thresh", "5"));
  ASSERT_TRUE(behavior.setParam("starflag", "ZIG_STAR=true"));
  EXPECT_FALSE(behavior.setParam("zig_angle", "90"));
  EXPECT_FALSE(behavior.setParam("zig_first", "middle"));
  EXPECT_FALSE(behavior.setParam("speed", "-1"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 20, 2),
            evalCourseSpeedPoint(*ipf, 200, 2));

  VarDataPair star;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "ZIG_STAR", star));
  EXPECT_EQ(star.get_sdata(), "true");

  VarDataPair view_line;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", view_line));
  EXPECT_NE(view_line.get_sdata().find("zig_set_"), std::string::npos);
}

// Covers BHV zig zag behavior: active derived stem and speed use current navigation.
TEST(BHVZigZagTest, ActiveDerivedStemAndSpeedUseCurrentNavigation)
{
  InfoBuffer info;
  info.setCurrTime(20);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 350);
  info.setValue("NAV_SPEED", 3);

  BHV_ZigZag behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "1"));
  ASSERT_TRUE(behavior.setParam("speed_on_active", "true"));
  ASSERT_TRUE(behavior.setParam("stem_on_active", "true"));
  ASSERT_TRUE(behavior.setParam("zig_angle", "20"));
  ASSERT_TRUE(behavior.setParam("zig_first", "left"));
  ASSERT_TRUE(behavior.setParam("mod_speed", "10"));
  ASSERT_TRUE(behavior.setParam("mod_speed", "reset"));
  ASSERT_TRUE(behavior.setParam("visual_hints",
                                "set_hdg_color=yellow,req_hdg_color=green"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 330, 3),
            evalCourseSpeedPoint(*ipf, 10, 3));
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 330, 3),
            evalCourseSpeedPoint(*ipf, 330, 1));

  bool saw_set_line = false;
  bool saw_req_line = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string()) {
      saw_set_line |= (message.get_sdata().find("zig_set_") != std::string::npos) &&
                      (message.get_sdata().find("edge_color=yellow") != std::string::npos);
      saw_req_line |= (message.get_sdata().find("zig_req_") != std::string::npos) &&
                      (message.get_sdata().find("edge_color=green") != std::string::npos);
    }
  }
  EXPECT_TRUE(saw_set_line);
  EXPECT_TRUE(saw_req_line);
}

// Covers BHV zig zag behavior: completes after configured stem odometry and erases on idle.
TEST(BHVZigZagTest, CompletesAfterConfiguredStemOdometryAndErasesOnIdle)
{
  InfoBuffer info;
  info.setCurrTime(30);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 2);

  BHV_ZigZag behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("stem_hdg", "0"));
  ASSERT_TRUE(behavior.setParam("zig_angle", "20"));
  ASSERT_TRUE(behavior.setParam("max_stem_odo", "5"));
  ASSERT_TRUE(behavior.setParam("max_zig_zags", "1"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  info.setCurrTime(31);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 10);
  std::unique_ptr<IvPFunction> completed_ipf(behavior.onRunState());
  EXPECT_EQ(completed_ipf, nullptr);

  behavior.clearMessages();
  behavior.onRunToIdleState();

  bool erased_set = false;
  bool erased_req = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_SEGLIST" && message.is_string()) {
      erased_set |= (message.get_sdata().find("zig_set_") != std::string::npos) &&
                    (message.get_sdata().find("active=false") != std::string::npos);
      erased_req |= (message.get_sdata().find("zig_req_") != std::string::npos) &&
                    (message.get_sdata().find("active=false") != std::string::npos);
    }
  }
  EXPECT_TRUE(erased_set);
  EXPECT_TRUE(erased_req);
}

// Covers BHV zig zag behavior: mission style fierce zigging uses current heading and leg flags.
TEST(BHVZigZagTest, MissionStyleFierceZiggingUsesCurrentHeadingAndLegFlags)
{
  InfoBuffer info;
  info.setCurrTime(40);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", 3);

  BHV_ZigZag behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  // Mirrors the zig block in ivp/missions/x1_legs/alpha.bhv.
  ASSERT_TRUE(behavior.setParam("speed", "3.0"));
  ASSERT_TRUE(behavior.setParam("stem_on_active", "true"));
  ASSERT_TRUE(behavior.setParam("zig_first", "star"));
  ASSERT_TRUE(behavior.setParam("max_zig_zags", "1"));
  ASSERT_TRUE(behavior.setParam("zig_angle", "55"));
  ASSERT_TRUE(behavior.setParam("hdg_thresh", "2"));
  ASSERT_TRUE(behavior.setParam("fierce_zigging", "true"));
  ASSERT_TRUE(behavior.setParam("starflag", "ZZ_SIDE=star,z=$[ZIGS]"));
  ASSERT_TRUE(behavior.setParam("portflag", "ZZ_SIDE=port,z=$[ZIGS]"));
  ASSERT_TRUE(behavior.setParam("zigflag", "ZZ_COUNT=$[ZIGS]"));
  ASSERT_TRUE(behavior.setParam("starflagx", "ZZ_STAR_DONE=$[ZIGS]"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*first_ipf, 145, 3),
            evalCourseSpeedPoint(*first_ipf, 35, 3));

  VarDataPair side;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "ZZ_SIDE", side));
  EXPECT_EQ(side.get_sdata(), "star,z=0");

  behavior.clearMessages();
  info.setCurrTime(41);
  info.setValue("NAV_HEADING", 145);
  info.setValue("NAV_X", 10);
  info.setValue("NAV_Y", 10);

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);

  // Fierce zigging commands a relative heading from current heading after
  // changing legs, rather than immediately asking for the full set heading.
  EXPECT_GT(evalCourseSpeedPoint(*second_ipf, 90, 3),
            evalCourseSpeedPoint(*second_ipf, 35, 3));

  VarDataPair zig_count;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "ZZ_COUNT", zig_count));
  EXPECT_DOUBLE_EQ(zig_count.get_ddata(), 1);

  VarDataPair star_done;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "ZZ_STAR_DONE", star_done));
  EXPECT_DOUBLE_EQ(star_done.get_ddata(), 1);

  bool saw_port_entry = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    saw_port_entry |= (message.get_var() == "ZZ_SIDE") &&
                      message.is_string() &&
                      (message.get_sdata() == "port,z=1");
  }
  EXPECT_TRUE(saw_port_entry);
}

// Covers BHV zig zag behavior: visual hints can disable both viewer lines.
TEST(BHVZigZagTest, VisualHintsCanDisableBothViewerLines)
{
  InfoBuffer info;
  info.setCurrTime(50);
  info.setValue("NAV_X", 1);
  info.setValue("NAV_Y", 2);
  info.setValue("NAV_HEADING", 10);
  info.setValue("NAV_SPEED", 2);

  BHV_ZigZag behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("stem_hdg", "10"));
  ASSERT_TRUE(behavior.setParam("zig_angle", "30"));
  ASSERT_TRUE(behavior.setParam("visual_hints", "draw_set_hdg=false,draw_req_hdg=false"));
  EXPECT_FALSE(behavior.setParam("visual_hints", "req_hdg_color=not_a_color"));

  behavior.onIdleToRunState();
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  for(const VarDataPair& message : behavior.getMessages())
    EXPECT_NE(message.get_var(), "VIEW_SEGLIST");
}

// Covers BHV zig zag behavior: missing or stale navigation suppresses objective with errors.
TEST(BHVZigZagTest, MissingOrStaleNavigationSuppressesObjectiveWithErrors)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);

  BHV_ZigZag behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("stem_hdg", "0"));
  ASSERT_TRUE(behavior.setParam("zig_angle", "20"));
  behavior.onIdleToRunState();

  std::unique_ptr<IvPFunction> missing_speed_ipf(behavior.onRunState());
  EXPECT_EQ(missing_speed_ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair missing_speed_error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR",
                                  missing_speed_error));
  EXPECT_NE(missing_speed_error.get_sdata().find("NAV_SPEED"),
            std::string::npos);

  InfoBuffer stale_info;
  stale_info.setCurrTime(0);
  stale_info.setValue("NAV_X", 0);
  stale_info.setValue("NAV_Y", 0);
  stale_info.setValue("NAV_HEADING", 0);
  stale_info.setValue("NAV_SPEED", 2);
  stale_info.setCurrTime(6);

  BHV_ZigZag stale_behavior(makeCourseSpeedDomain());
  stale_behavior.setInfoBuffer(&stale_info);
  ASSERT_TRUE(stale_behavior.setParam("speed", "2"));
  ASSERT_TRUE(stale_behavior.setParam("stem_hdg", "0"));
  ASSERT_TRUE(stale_behavior.setParam("zig_angle", "20"));

  std::unique_ptr<IvPFunction> stale_ipf(stale_behavior.onRunState());
  EXPECT_EQ(stale_ipf, nullptr);
  EXPECT_FALSE(stale_behavior.stateOK());
}
