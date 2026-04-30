#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_AvoidCollision.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV avoid collision behavior: builds avoidance objective for close ledger contact.
TEST(BHVAvoidCollisionTest, BuildsAvoidanceObjectiveForCloseLedgerContact)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 80, 180, 2);

  BHV_AvoidCollision behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "500"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "10"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "80"));
  ASSERT_TRUE(behavior.setParam("post_per_contact_info", "true"));
  EXPECT_FALSE(behavior.setParam("pwt_grade", "steep"));
  EXPECT_FALSE(behavior.setParam("completed_dist", "-1"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair range;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RANGE_AVD_alpha", range));
  EXPECT_DOUBLE_EQ(range.get_ddata(), 80);
}

// Covers BHV avoid collision behavior: far contact completes without objective and posts range.
TEST(BHVAvoidCollisionTest, FarContactCompletesWithoutObjectiveAndPostsRange)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 700, 180, 2);

  BHV_AvoidCollision behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "500"));
  ASSERT_TRUE(behavior.setParam("post_per_contact_info", "true"));
  ASSERT_TRUE(behavior.setParam("pwt_grade", "quadratic"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair range;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "RANGE_AVD_alpha", range));
  EXPECT_DOUBLE_EQ(range.get_ddata(), 700);
}

// Covers BHV avoid collision behavior: refinery and validity checks still build objective.
TEST(BHVAvoidCollisionTest, RefineryAndValidityChecksStillBuildObjective)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 80, 180, 2);

  BHV_AvoidCollision behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "500"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "10"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "80"));
  ASSERT_TRUE(behavior.setParam("use_refinery", "true"));
  ASSERT_TRUE(behavior.setParam("check_validity", "true"));
  ASSERT_TRUE(behavior.setParam("verbose", "false"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair valid;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VALID_CHECK_OK", valid));
  ASSERT_TRUE(valid.is_string());
  EXPECT_EQ(valid.get_sdata(), "true");
}

// Covers BHV avoid collision behavior: relevance scales priority weight and bearing line uses range color.
TEST(BHVAvoidCollisionTest, RelevanceScalesPriorityWeightAndBearingLineUsesRangeColor)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 90, 180, 2);

  BHV_AvoidCollision behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "500"));
  ASSERT_TRUE(behavior.setParam("min_util_cpa_dist", "10"));
  ASSERT_TRUE(behavior.setParam("max_util_cpa_dist", "90"));
  ASSERT_TRUE(behavior.setParam("pwt_grade", "linear"));
  ASSERT_TRUE(behavior.setParam("priority", "80"));
  ASSERT_TRUE(behavior.setParam("bearing_lines", "info:range,green:100,red:150"));
  ASSERT_TRUE(behavior.setParam("bearing_line_label_show", "true"));

  behavior.onEveryState("running");
  behavior.clearMessages();

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 40);

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  ASSERT_TRUE(seglist.is_string());
  EXPECT_NE(seglist.get_sdata().find("pts={0,0:0,90}"), std::string::npos);
  EXPECT_NE(seglist.get_sdata().find("edge_color=green"), std::string::npos);
  EXPECT_NE(seglist.get_sdata().find("label_color=white"), std::string::npos);
}

// Covers BHV avoid collision behavior: spawnable template posts contact manager alert request.
TEST(BHVAvoidCollisionTest, SpawnableTemplatePostsContactManagerAlertRequest)
{
  BHV_AvoidCollision behavior(makeCourseSpeedDomain());

  behavior.setDynamicallySpawnable(true);
  ASSERT_TRUE(behavior.setParam("updates", "AVD_UPDATES"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "180"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "240"));
  ASSERT_TRUE(behavior.setParam("match_type", "kayak"));

  behavior.onHelmStart();

  VarDataPair request;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BCM_ALERT_REQUEST", request));
  ASSERT_TRUE(request.is_string());
  EXPECT_NE(request.get_sdata().find("on_flag=AVD_UPDATES=name=$[VNAME] # contact=$[VNAME]"),
            std::string::npos);
  EXPECT_NE(request.get_sdata().find("alert_range=180"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("cpa_range=240"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("match_type=kayak"), std::string::npos);
}

// Covers BHV avoid collision behavior: contact flags expand range and contact name during every state.
TEST(BHVAvoidCollisionTest, ContactFlagsExpandRangeAndContactNameDuringEveryState)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 80, 180, 2);

  BHV_AvoidCollision behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("cnflag", "<100 AVD_CONTACT = name=$[CN_NAME],rng=$[RANGE]"));
  ASSERT_FALSE(behavior.setParam("cnflag", "AVD_CONTACT = missing_tag"));

  behavior.onEveryState("running");

  VarDataPair flag;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "AVD_CONTACT", flag));
  ASSERT_TRUE(flag.is_string());
  EXPECT_NE(flag.get_sdata().find("name=alpha"), std::string::npos);
  EXPECT_NE(flag.get_sdata().find("rng=80"), std::string::npos);
}

// Covers BHV avoid collision behavior: inactive and complete erase bearing line.
TEST(BHVAvoidCollisionTest, InactiveAndCompleteEraseBearingLine)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 80, 180, 2);

  BHV_AvoidCollision behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "30"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("completed_dist", "500"));
  ASSERT_TRUE(behavior.setParam("bearing_lines", "info:relevance,white:0.25,red:1"));

  behavior.onEveryState("running");
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  behavior.clearMessages();
  behavior.onInactiveState();

  VarDataPair seglist;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_SEGLIST", seglist));
  ASSERT_TRUE(seglist.is_string());
  EXPECT_NE(seglist.get_sdata().find("active=false"), std::string::npos);
}
