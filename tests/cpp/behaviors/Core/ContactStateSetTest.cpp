#include <gtest/gtest.h>

#include <string>

#include "ContactStateSet.h"

// Covers contact state set behavior: archives current contact state and clamps history lookup.
TEST(ContactStateSetTest, ArchivesCurrentContactStateAndClampsHistoryLookup)
{
  ContactStateSet states;

  states.archiveCurrent();
  EXPECT_EQ(states.size(), 0u);

  states.set_cnutc(10);
  states.set_range(100);
  states.set_os_aft_of_cn(true);
  states.set_cn_port_of_os(true);
  states.archiveCurrent();

  states.set_cnutc(11);
  states.set_range(80);
  states.set_os_aft_of_cn(false);
  states.set_cn_port_of_os(false);
  states.set_cn_fore_of_os(true);

  EXPECT_EQ(states.size(), 1u);
  EXPECT_DOUBLE_EQ(states.cnutc(0), 11);
  EXPECT_DOUBLE_EQ(states.cnutc(1), 10);
  EXPECT_DOUBLE_EQ(states.cnutc(99), 10);
  EXPECT_DOUBLE_EQ(states.range_gamma(), 0);
  EXPECT_TRUE(states.os_passed_cn());
  EXPECT_TRUE(states.cn_crossed_os());
  EXPECT_TRUE(states.cn_crossed_os_bow());
}

// Covers contact state set behavior: retains five historical contact snapshots newest first.
TEST(ContactStateSetTest, RetainsFiveHistoricalContactSnapshotsNewestFirst)
{
  ContactStateSet states;

  for(unsigned int i = 1; i <= 7; ++i) {
    states.set_cnutc(100 + i);
    states.set_helm_iter(10 * i);
    states.set_range(200 - i);
    states.archiveCurrent();
  }

  states.set_cnutc(108);
  states.set_helm_iter(80);
  states.set_range(192);

  ASSERT_EQ(states.size(), 5u);
  EXPECT_DOUBLE_EQ(states.cnutc(0), 108);
  EXPECT_DOUBLE_EQ(states.cnutc(1), 107);
  EXPECT_DOUBLE_EQ(states.cnutc(2), 106);
  EXPECT_DOUBLE_EQ(states.cnutc(5), 103);
  EXPECT_DOUBLE_EQ(states.cnutc(99), 103);
  EXPECT_EQ(states.helm_iter(0), 80u);
  EXPECT_EQ(states.helm_iter(1), 70u);
  EXPECT_EQ(states.helm_iter(99), 30u);
}

// Covers contact state set behavior: exposes contact behavior kinematics and macro expansion.
TEST(ContactStateSetTest, ExposesContactBehaviorKinematicsAndMacroExpansion)
{
  ContactStateSet states;
  states.set_cnutc(50);
  states.set_update_ok(true);
  states.set_range(120);
  states.set_cn_spd_in_os_pos(1.75);
  states.set_os_cn_rel_bng(45.25);
  states.set_cn_os_rel_bng(225.25);
  states.set_os_cn_abs_bng(310.5);
  states.set_rate_of_closure(-0.55);
  states.set_bearing_rate(2.25);
  states.set_contact_rate(0.85);
  states.set_range_gamma(0.33);
  states.set_range_epsilon(-0.12);
  states.set_os_aft_of_cn(true);
  states.set_cn_star_of_os(true);

  EXPECT_TRUE(states.update_ok());
  EXPECT_DOUBLE_EQ(states.cn_spd_in_os_pos(), 1.75);
  EXPECT_DOUBLE_EQ(states.os_cn_rel_bng(), 45.25);
  EXPECT_DOUBLE_EQ(states.cn_os_rel_bng(), 225.25);
  EXPECT_DOUBLE_EQ(states.os_cn_abs_bng(), 310.5);
  EXPECT_DOUBLE_EQ(states.rate_of_closure(), -0.55);
  EXPECT_DOUBLE_EQ(states.bearing_rate(), 2.25);
  EXPECT_DOUBLE_EQ(states.contact_rate(), 0.85);
  EXPECT_DOUBLE_EQ(states.range_gamma(), 0.33);
  EXPECT_DOUBLE_EQ(states.range_epsilon(), -0.12);

  std::string text = "$[ROC]|$[BNG_RATE]|$[OS_AFT_OF_CN]|$[CN_STAR_OF_OS]";
  text = states.cnMacroExpand(text, "ROC");
  text = states.cnMacroExpand(text, "BNG_RATE");
  text = states.cnMacroExpand(text, "OS_AFT_OF_CN");
  text = states.cnMacroExpand(text, "CN_STAR_OF_OS");
  EXPECT_EQ(text, "-0.6|2.2|true|true");
}

// Covers contact state set behavior: detects range transitions and closest point of approach.
TEST(ContactStateSetTest, DetectsRangeTransitionsAndClosestPointOfApproach)
{
  ContactStateSet cpa_states;

  cpa_states.set_cnutc(1);
  cpa_states.set_range(100);
  cpa_states.archiveCurrent();
  cpa_states.set_cnutc(2);
  cpa_states.set_range(80);
  cpa_states.archiveCurrent();
  cpa_states.set_cnutc(3);
  cpa_states.set_range(60);
  cpa_states.archiveCurrent();
  cpa_states.set_cnutc(4);
  cpa_states.set_range(61);

  EXPECT_TRUE(cpa_states.cpa_reached(0.5));

  ContactStateSet range_states;
  range_states.set_cnutc(10);
  range_states.set_range(80);
  range_states.archiveCurrent();
  range_states.set_cnutc(11);
  range_states.set_range(60);

  EXPECT_TRUE(range_states.rng_entered(65));
  EXPECT_FALSE(range_states.rng_exited(65));

  range_states.archiveCurrent();
  range_states.set_cnutc(12);
  range_states.set_range(90);

  EXPECT_FALSE(range_states.rng_entered(65));
  EXPECT_TRUE(range_states.rng_exited(65));
}

// Covers contact state set behavior: reports passing and crossing side specific events.
TEST(ContactStateSetTest, ReportsPassingAndCrossingSideSpecificEvents)
{
  ContactStateSet states;

  states.set_cnutc(1);
  states.set_os_aft_of_cn(false);
  states.set_os_port_of_cn(true);
  states.set_cn_aft_of_os(false);
  states.set_cn_star_of_os(true);
  states.archiveCurrent();

  states.set_cnutc(2);
  states.set_os_aft_of_cn(true);
  states.set_os_port_of_cn(false);
  states.set_os_star_of_cn(true);
  states.set_cn_aft_of_os(true);
  states.set_cn_port_of_os(true);

  EXPECT_TRUE(states.os_passed_cn());
  EXPECT_TRUE(states.os_passed_cn_star());
  EXPECT_FALSE(states.os_passed_cn_port());
  EXPECT_TRUE(states.cn_passed_os());
  EXPECT_TRUE(states.cn_passed_os_port());
  EXPECT_TRUE(states.os_crossed_cn());
  EXPECT_TRUE(states.os_crossed_cn_stern());
  EXPECT_TRUE(states.cn_crossed_os());
  EXPECT_TRUE(states.cn_crossed_os_stern());
}

// Covers contact state set behavior: reports bow specific crossing events and aliases.
TEST(ContactStateSetTest, ReportsBowSpecificCrossingEventsAndAliases)
{
  ContactStateSet states;

  states.set_cnutc(1);
  states.set_os_port_of_cn(true);
  states.set_cn_port_of_os(true);
  states.archiveCurrent();

  states.set_cnutc(2);
  states.set_os_port_of_cn(false);
  states.set_os_fore_of_cn(true);
  states.set_os_crosses_cn(true);
  states.set_cn_port_of_os(false);
  states.set_cn_fore_of_os(true);
  states.set_cn_crosses_os(true);

  EXPECT_TRUE(states.os_crossed_cn());
  EXPECT_TRUE(states.os_crossed_cn_bow());
  EXPECT_FALSE(states.os_crossed_cn_stern());
  EXPECT_TRUE(states.os_crosses_cn_bow_or_stern());
  EXPECT_TRUE(states.cn_crossed_os());
  EXPECT_TRUE(states.cn_crossed_os_bow());
  EXPECT_FALSE(states.cn_crossed_os_stern());
  EXPECT_TRUE(states.cn_crosses_os_bow_or_stern());
}

// Covers contact state set behavior: does not report passing or crossing without side transition.
TEST(ContactStateSetTest, DoesNotReportPassingOrCrossingWithoutSideTransition)
{
  ContactStateSet states;

  states.set_cnutc(1);
  states.set_os_aft_of_cn(false);
  states.set_os_port_of_cn(true);
  states.set_cn_aft_of_os(false);
  states.set_cn_port_of_os(false);
  states.archiveCurrent();

  states.set_cnutc(2);
  states.set_os_aft_of_cn(false);
  states.set_os_port_of_cn(true);
  states.set_cn_aft_of_os(false);
  states.set_cn_port_of_os(false);

  EXPECT_FALSE(states.os_passed_cn());
  EXPECT_FALSE(states.os_passed_cn_port());
  EXPECT_FALSE(states.os_passed_cn_star());
  EXPECT_FALSE(states.cn_passed_os());
  EXPECT_FALSE(states.cn_passed_os_port());
  EXPECT_FALSE(states.cn_passed_os_star());
  EXPECT_FALSE(states.os_crossed_cn());
  EXPECT_FALSE(states.os_crossed_cn_bow());
  EXPECT_FALSE(states.os_crossed_cn_stern());
  EXPECT_FALSE(states.cn_crossed_os());
  EXPECT_FALSE(states.cn_crossed_os_bow());
  EXPECT_FALSE(states.cn_crossed_os_stern());
}
