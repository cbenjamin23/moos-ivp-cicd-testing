#include <gtest/gtest.h>

#include "AOF_CPA.h"
#include "AOF_R13.h"
#include "AOF_R14.h"
#include "AOF_R16.h"
#include "AOF_R17.h"
#include "ColregsTestUtils.h"

namespace {

template <typename AOF>
void configureContactAOF(AOF& aof,
                         double cnx = 80,
                         double cny = 0,
                         double cnh = 270,
                         double cnv = 0)
{
  aof.setParam("osx", 0.0);
  aof.setParam("osy", 0.0);
  aof.setParam("cnx", cnx);
  aof.setParam("cny", cny);
  aof.setParam("cnh", cnh);
  aof.setParam("cnv", cnv);
  aof.setParam("collision_distance", 10.0);
  aof.setParam("all_clear_distance", 60.0);
  aof.setParam("tol", 120.0);
}

}  // namespace

// Covers colregs AOF behavior: CPA utility increases with safer initial separation.
TEST(ColregsAOFTest, CPAUtilityIncreasesWithSaferInitialSeparation)
{
  AOF_CPA near_aof(makeColregsDomain());
  configureContactAOF(near_aof, 12, 0, 270, 0);
  ASSERT_TRUE(near_aof.initialize());

  AOF_CPA far_aof(makeColregsDomain());
  configureContactAOF(far_aof, 90, 0, 270, 0);
  ASSERT_TRUE(far_aof.initialize());

  IvPBox stopped = makeCourseSpeedBox(90, 0);
  EXPECT_LT(near_aof.evalBox(&stopped), far_aof.evalBox(&stopped));
  EXPECT_GE(far_aof.evalBox(&stopped), 100);
}

// Covers colregs AOF behavior: rule ao fs require rule specific parameters.
TEST(ColregsAOFTest, RuleAOFsRequireRuleSpecificParameters)
{
  AOF_R13 overtaking(makeColregsDomain());
  configureContactAOF(overtaking);
  EXPECT_FALSE(overtaking.initialize());
  EXPECT_TRUE(overtaking.setParam("passing_side", "port"));
  EXPECT_FALSE(overtaking.setParam("passing_side", "middle"));
  EXPECT_TRUE(overtaking.initialize());

  AOF_R16 giveway(makeColregsDomain());
  configureContactAOF(giveway);
  giveway.setParam("osh", 0.0);
  EXPECT_FALSE(giveway.initialize());
  EXPECT_TRUE(giveway.setParam("passing_side", "stern"));
  EXPECT_TRUE(giveway.setParam("pts_port_turns_ok", "false"));
  EXPECT_TRUE(giveway.initialize());

  AOF_R17 standon(makeColregsDomain());
  configureContactAOF(standon);
  standon.setParam("osh", 0.0);
  standon.setParam("original_course", 90.0);
  EXPECT_FALSE(standon.initialize());
  standon.setParam("original_speed", 2.0);
  EXPECT_TRUE(standon.setParam("passing_side", "neither"));
  EXPECT_TRUE(standon.initialize());
}

// Covers colregs AOF behavior: rule17 hold mode rewards original course and speed.
TEST(ColregsAOFTest, Rule17HoldModeRewardsOriginalCourseAndSpeed)
{
  AOF_R17 standon(makeColregsDomain());
  configureContactAOF(standon);
  standon.setParam("osh", 90.0);
  standon.setParam("original_course", 90.0);
  standon.setParam("original_speed", 2.0);
  ASSERT_TRUE(standon.setParam("passing_side", "neither"));
  ASSERT_TRUE(standon.initialize());

  IvPBox hold = makeCourseSpeedBox(90, 2);
  IvPBox large_change = makeCourseSpeedBox(140, 5);

  EXPECT_DOUBLE_EQ(standon.evalBox(&hold), 100);
  EXPECT_DOUBLE_EQ(standon.evalBox(&large_change), 0);
}

// Covers colregs AOF behavior: rule14 and rule16 build evaluable utilities.
TEST(ColregsAOFTest, Rule14AndRule16BuildEvaluableUtilities)
{
  AOF_R14 headon(makeColregsDomain());
  configureContactAOF(headon, 0, 80, 180, 2);
  ASSERT_TRUE(headon.initialize());

  AOF_R16 giveway(makeColregsDomain());
  configureContactAOF(giveway, 80, 0, 270, 2);
  giveway.setParam("osh", 0.0);
  ASSERT_TRUE(giveway.setParam("passing_side", "stern"));
  ASSERT_TRUE(giveway.initialize());

  IvPBox starboard_turn = makeCourseSpeedBox(45, 2);
  EXPECT_GE(headon.evalBox(&starboard_turn), 0);
  EXPECT_GE(giveway.evalBox(&starboard_turn), 0);
}

// Covers colregs AOF behavior: rule13 overtaking passing side is accepted and stopped course is neutral.
TEST(ColregsAOFTest, Rule13OvertakingPassingSideIsAcceptedAndStoppedCourseIsNeutral)
{
  AOF_R13 pass_port(makeColregsDomain());
  configureContactAOF(pass_port, 0, 80, 0, 1);
  ASSERT_TRUE(pass_port.setParam("passing_side", "port"));
  ASSERT_TRUE(pass_port.initialize());

  AOF_R13 pass_starboard(makeColregsDomain());
  configureContactAOF(pass_starboard, 0, 80, 0, 1);
  ASSERT_TRUE(pass_starboard.setParam("passing_side", "starboard"));
  ASSERT_TRUE(pass_starboard.initialize());

  IvPBox starboard_of_contact = makeCourseSpeedBox(30, 3);
  IvPBox port_of_contact = makeCourseSpeedBox(330, 3);
  IvPBox stopped = makeCourseSpeedBox(0, 0);

  EXPECT_GE(pass_port.evalBox(&port_of_contact), 0);
  EXPECT_GE(pass_port.evalBox(&starboard_of_contact), 0);
  EXPECT_GE(pass_starboard.evalBox(&starboard_of_contact), 0);
  EXPECT_GE(pass_starboard.evalBox(&port_of_contact), 0);
  EXPECT_GE(pass_port.evalBox(&stopped), pass_port.evalBox(&starboard_of_contact));
  EXPECT_GE(pass_starboard.evalBox(&stopped),
            pass_starboard.evalBox(&port_of_contact));
}

// Covers colregs AOF behavior: rule16 stern passing conservative mode disallows port turns.
TEST(ColregsAOFTest, Rule16SternPassingConservativeModeDisallowsPortTurns)
{
  AOF_R16 default_policy(makeColregsDomain());
  configureContactAOF(default_policy, 80, 0, 270, 2);
  default_policy.setParam("osh", 0.0);
  ASSERT_TRUE(default_policy.setParam("passing_side", "stern"));
  ASSERT_TRUE(default_policy.initialize());

  AOF_R16 conservative_policy(makeColregsDomain());
  configureContactAOF(conservative_policy, 80, 0, 270, 2);
  conservative_policy.setParam("osh", 0.0);
  ASSERT_TRUE(conservative_policy.setParam("passing_side", "stern"));
  ASSERT_TRUE(conservative_policy.setParam("pts_port_turns_ok", "false"));
  ASSERT_TRUE(conservative_policy.initialize());

  IvPBox starboard_turn = makeCourseSpeedBox(45, 2);
  IvPBox port_turn = makeCourseSpeedBox(315, 2);

  EXPECT_GE(default_policy.evalBox(&starboard_turn), 0);
  EXPECT_DOUBLE_EQ(conservative_policy.evalBox(&port_turn), 0);
  EXPECT_GE(conservative_policy.evalBox(&starboard_turn),
            conservative_policy.evalBox(&port_turn));
}

// Covers colregs AOF behavior: rule16 bow passing can require minimum bow cross distance.
TEST(ColregsAOFTest, Rule16BowPassingCanRequireMinimumBowCrossDistance)
{
  AOF_R16 bow_policy(makeColregsDomain());
  configureContactAOF(bow_policy, -80, 0, 90, 1);
  bow_policy.setParam("osh", 0.0);
  ASSERT_TRUE(bow_policy.setParam("passing_side", "bow"));
  ASSERT_TRUE(bow_policy.setParam("ok_cn_bow_cross_dist", "20"));
  ASSERT_FALSE(bow_policy.setParam("ok_cn_bow_cross_dist", "-1"));
  ASSERT_TRUE(bow_policy.initialize());

  IvPBox close_cross = makeCourseSpeedBox(45, 5);
  IvPBox no_right_turn = makeCourseSpeedBox(315, 2);

  EXPECT_DOUBLE_EQ(bow_policy.evalBox(&close_cross), 0);
  EXPECT_GE(bow_policy.evalBox(&no_right_turn), 0);
}

// Covers colregs AOF behavior: rule17 in extremis rejects port turn for contact on port side.
TEST(ColregsAOFTest, Rule17InExtremisRejectsPortTurnForContactOnPortSide)
{
  AOF_R17 standon(makeColregsDomain());
  configureContactAOF(standon, -40, 80, 180, 2);
  standon.setParam("osh", 0.0);
  standon.setParam("osv", 2.0);
  standon.setParam("original_course", 0.0);
  standon.setParam("original_speed", 2.0);
  ASSERT_TRUE(standon.setParam("passing_side", "inextremis"));
  ASSERT_TRUE(standon.initialize());

  IvPBox port_turn = makeCourseSpeedBox(330, 2);
  IvPBox starboard_turn = makeCourseSpeedBox(30, 2);

  EXPECT_DOUBLE_EQ(standon.evalBox(&port_turn), 0);
  EXPECT_GT(standon.evalBox(&starboard_turn),
            standon.evalBox(&port_turn));
}
