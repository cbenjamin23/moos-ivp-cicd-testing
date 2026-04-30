#include <gtest/gtest.h>

#include <memory>

#include "BHV_Shadow.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV shadow behavior: mirrors contact course and speed from ledger contact.
TEST(BHVShadowTest, MirrorsContactCourseAndSpeedFromLedgerContact)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 100, 0, 90, 2);

  BHV_Shadow behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("heading_peakwidth", "10"));
  ASSERT_TRUE(behavior.setParam("speed_peakwidth", "0.1"));
  EXPECT_FALSE(behavior.setParam("pwt_outer_dist", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 90, 2),
            evalCourseSpeedPoint(*ipf, 270, 0));

  VarDataPair relevance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SHADOW_RELEVANCE", relevance));
  EXPECT_DOUBLE_EQ(relevance.get_ddata(), 1);
}

// Covers BHV shadow behavior: suppresses objective outside relevance range but posts contact telemetry.
TEST(BHVShadowTest, SuppressesObjectiveOutsideRelevanceRangeButPostsContactTelemetry)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 250, 0, 90, 2);

  BHV_Shadow behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("hdg_basewidth", "120"));
  ASSERT_TRUE(behavior.setParam("spd_basewidth", "1.5"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);

  VarDataPair relevance;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SHADOW_RELEVANCE", relevance));
  EXPECT_DOUBLE_EQ(relevance.get_ddata(), 0);

  VarDataPair contact_x;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SHADOW_CONTACT_X", contact_x));
  EXPECT_DOUBLE_EQ(contact_x.get_ddata(), 250);

  VarDataPair contact_heading;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "SHADOW_CONTACT_HEADING",
                                  contact_heading));
  EXPECT_DOUBLE_EQ(contact_heading.get_ddata(), 90);
}

// Covers BHV shadow behavior: priority weight is applied when shadowing is relevant.
TEST(BHVShadowTest, PriorityWeightIsAppliedWhenShadowingIsRelevant)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 60, 0, 90, 2);

  BHV_Shadow behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("priority", "65"));
  ASSERT_TRUE(behavior.setParam("heading_basewidth", "120"));
  ASSERT_TRUE(behavior.setParam("speed_basewidth", "1.5"));
  EXPECT_FALSE(behavior.setParam("heading_basewidth", "-1"));
  EXPECT_FALSE(behavior.setParam("speed_basewidth", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 65);
}

// Covers BHV shadow behavior: missing ownship mail suppresses objective and posts warning.
TEST(BHVShadowTest, MissingOwnshipMailSuppressesObjectiveAndPostsWarning)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  ledger.setX("alpha", 60);
  ledger.setY("alpha", 0);
  ledger.setHdg("alpha", 90);
  ledger.setSpd("alpha", 2);
  ledger.setUTC("alpha", info.getCurrTime());

  BHV_Shadow behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_WARNING", warning));
  EXPECT_FALSE(warning.get_sdata().empty());
}
