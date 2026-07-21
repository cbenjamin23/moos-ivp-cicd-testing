#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <memory>

#include "BHV_CutRange.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

namespace {

void configureCutRange(BHV_CutRange& behavior,
                       InfoBuffer& info,
                       LedgerSnap& ledger,
                       const std::string& patience,
                       const std::string& max_patience)
{
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "200"));
  ASSERT_TRUE(behavior.setParam("patience", patience));
  ASSERT_TRUE(behavior.setParam("max_patience", max_patience));
  behavior.onSetParamComplete();
}

double maxCourseSpeedDifference(IvPFunction& lhs, IvPFunction& rhs)
{
  double max_difference = 0;
  for(unsigned int course = 0; course < 360; ++course) {
    for(unsigned int speed = 0; speed < 6; ++speed) {
      const double difference =
        std::fabs(evalCourseSpeedPoint(lhs, course, speed) -
                  evalCourseSpeedPoint(rhs, course, speed));
      max_difference = std::max(max_difference, difference);
    }
  }
  return(max_difference);
}

void expectSameCourseSpeedObjective(IvPFunction& lhs, IvPFunction& rhs)
{
  for(unsigned int course = 0; course < 360; ++course) {
    for(unsigned int speed = 0; speed < 6; ++speed) {
      EXPECT_DOUBLE_EQ(evalCourseSpeedPoint(lhs, course, speed),
                       evalCourseSpeedPoint(rhs, course, speed));
    }
  }
}

}  // namespace

// Covers BHV cut range behavior: pursues contact inside giveup range and builds CPA objective.
TEST(BHVCutRangeTest, PursuesContactInsideGiveupRangeAndBuildsCPAObjective)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 180, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "150"));
  ASSERT_TRUE(behavior.setParam("pursueflag", "CUT_PURSUIT=true"));
  ASSERT_TRUE(behavior.setParam("patience", "50"));
  EXPECT_FALSE(behavior.setParam("patience", "101"));
  EXPECT_FALSE(behavior.setParam("giveup_range", "-1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair pursue;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "CUT_PURSUIT", pursue));
  EXPECT_EQ(pursue.get_sdata(), "true");
}

// Covers BHV cut range behavior: giveup range drops objective after pursuit was active.
TEST(BHVCutRangeTest, GiveupRangeDropsObjectiveAfterPursuitWasActive)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 180, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "150"));
  ASSERT_TRUE(behavior.setParam("giveupflag", "CUT_GIVEUP=true"));
  ASSERT_TRUE(behavior.setParam("max_patience", "40"));
  ASSERT_TRUE(behavior.setParam("patience", "80"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> pursuit_ipf(behavior.onRunState());
  ASSERT_NE(pursuit_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(101);
  ledger.setX("alpha", 0);
  ledger.setY("alpha", 180);
  ledger.setHdg("alpha", 180);
  ledger.setSpd("alpha", 1);
  ledger.setUTC("alpha", info.getCurrTime());

  std::unique_ptr<IvPFunction> giveup_ipf(behavior.onRunState());
  EXPECT_EQ(giveup_ipf, nullptr);

  VarDataPair giveup;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "CUT_GIVEUP", giveup));
  EXPECT_EQ(giveup.get_sdata(), "true");
}

// Covers BHV cut range behavior: relevance scales priority weight between inner and outer ranges.
TEST(BHVCutRangeTest, RelevanceScalesPriorityWeightBetweenInnerAndOuterRanges)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 2, 0, 100, 180, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "150"));
  ASSERT_TRUE(behavior.setParam("giveup_range", "200"));
  ASSERT_TRUE(behavior.setParam("priority", "80"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_DOUBLE_EQ(ipf->getPWT(), 40);
}

// Covers BHV cut range behavior: spawnable template posts contact manager alert request.
TEST(BHVCutRangeTest, SpawnableTemplatePostsContactManagerAlertRequest)
{
  BHV_CutRange behavior(makeCourseSpeedDomain());

  behavior.setDynamicallySpawnable(true);
  ASSERT_TRUE(behavior.setParam("updates", "CUT_UPDATES"));
  ASSERT_TRUE(behavior.setParam("pwt_outer_dist", "175"));

  behavior.onHelmStart();

  VarDataPair request;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BCM_ALERT_REQUEST", request));
  EXPECT_NE(request.get_sdata().find("id="), std::string::npos);
  EXPECT_NE(request.get_sdata().find("on_flag=CUT_UPDATES=name=$[VNAME] # contact=$[VNAME]"),
            std::string::npos);
  EXPECT_NE(request.get_sdata().find("alert_range=175"), std::string::npos);
  EXPECT_NE(request.get_sdata().find("cpa_range=176"), std::string::npos);
}

// Covers BHV cut range behavior: max_patience clamps the configured patience
// before constructing the objective function.
TEST(BHVCutRangeTest, MaxPatienceClampsConfiguredPatience)
{
  InfoBuffer clamped_info;
  clamped_info.setCurrTime(100);
  LedgerSnap clamped_ledger;
  seedOwnshipContactInfo(clamped_info, clamped_ledger, "alpha",
                         0, 0, 0, 2, 100, 0, 90, 1);
  BHV_CutRange clamped(makeCourseSpeedDomain());
  configureCutRange(clamped, clamped_info, clamped_ledger, "90", "45");

  InfoBuffer explicit_info;
  explicit_info.setCurrTime(100);
  LedgerSnap explicit_ledger;
  seedOwnshipContactInfo(explicit_info, explicit_ledger, "alpha",
                         0, 0, 0, 2, 100, 0, 90, 1);
  BHV_CutRange explicit_45(makeCourseSpeedDomain());
  configureCutRange(explicit_45, explicit_info, explicit_ledger, "45", "100");

  InfoBuffer high_info;
  high_info.setCurrTime(100);
  LedgerSnap high_ledger;
  seedOwnshipContactInfo(high_info, high_ledger, "alpha",
                         0, 0, 0, 2, 100, 0, 90, 1);
  BHV_CutRange explicit_90(makeCourseSpeedDomain());
  configureCutRange(explicit_90, high_info, high_ledger, "90", "100");

  std::unique_ptr<IvPFunction> clamped_ipf(clamped.onRunState());
  std::unique_ptr<IvPFunction> explicit_45_ipf(explicit_45.onRunState());
  std::unique_ptr<IvPFunction> explicit_90_ipf(explicit_90.onRunState());
  ASSERT_NE(clamped_ipf, nullptr);
  ASSERT_NE(explicit_45_ipf, nullptr);
  ASSERT_NE(explicit_90_ipf, nullptr);

  expectSameCourseSpeedObjective(*clamped_ipf, *explicit_45_ipf);
  EXPECT_GT(maxCourseSpeedDifference(*clamped_ipf, *explicit_90_ipf), 1.0);
}

// Covers BHV cut range behavior: a runtime patience update changes the
// generated objective without changing contact geometry.
TEST(BHVCutRangeTest, RuntimePatienceUpdateChangesObjective)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha",
                         0, 0, 0, 2, 100, 0, 90, 1);

  BHV_CutRange behavior(makeCourseSpeedDomain());
  configureCutRange(behavior, info, ledger, "5", "100");

  std::unique_ptr<IvPFunction> low_patience_ipf(behavior.onRunState());
  ASSERT_NE(low_patience_ipf, nullptr);

  ASSERT_TRUE(behavior.setParam("patience", "80"));
  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> high_patience_ipf(behavior.onRunState());
  ASSERT_NE(high_patience_ipf, nullptr);

  EXPECT_GT(maxCourseSpeedDifference(*low_patience_ipf,
                                     *high_patience_ipf), 1.0);
}

// Covers BHV cut range behavior: giveup_dist is behaviorally identical to
// giveup_range through pursuit and give-up transitions.
TEST(BHVCutRangeTest, GiveupDistAliasMatchesGiveupRange)
{
  InfoBuffer canonical_info;
  canonical_info.setCurrTime(100);
  LedgerSnap canonical_ledger;
  seedOwnshipContactInfo(canonical_info, canonical_ledger, "alpha",
                         0, 0, 0, 2, 100, 0, 180, 1);
  BHV_CutRange canonical(makeCourseSpeedDomain());
  canonical.setInfoBuffer(&canonical_info);
  canonical.setLedgerSnap(&canonical_ledger);
  ASSERT_TRUE(canonical.setParam("contact", "alpha"));
  ASSERT_TRUE(canonical.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(canonical.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(canonical.setParam("giveup_range", "150"));
  ASSERT_TRUE(canonical.setParam("giveupflag", "CUT_GIVEUP=true"));

  InfoBuffer alias_info;
  alias_info.setCurrTime(100);
  LedgerSnap alias_ledger;
  seedOwnshipContactInfo(alias_info, alias_ledger, "alpha",
                         0, 0, 0, 2, 100, 0, 180, 1);
  BHV_CutRange alias(makeCourseSpeedDomain());
  alias.setInfoBuffer(&alias_info);
  alias.setLedgerSnap(&alias_ledger);
  ASSERT_TRUE(alias.setParam("contact", "alpha"));
  ASSERT_TRUE(alias.setParam("pwt_inner_dist", "50"));
  ASSERT_TRUE(alias.setParam("pwt_outer_dist", "200"));
  ASSERT_TRUE(alias.setParam("giveup_dist", "150"));
  ASSERT_TRUE(alias.setParam("giveupflag", "CUT_GIVEUP=true"));

  std::unique_ptr<IvPFunction> canonical_pursuit(canonical.onRunState());
  std::unique_ptr<IvPFunction> alias_pursuit(alias.onRunState());
  ASSERT_NE(canonical_pursuit, nullptr);
  ASSERT_NE(alias_pursuit, nullptr);

  canonical.clearMessages();
  alias.clearMessages();
  canonical_info.setCurrTime(101);
  alias_info.setCurrTime(101);
  canonical_ledger.setX("alpha", 180);
  alias_ledger.setX("alpha", 180);
  canonical_ledger.setUTC("alpha", 101);
  alias_ledger.setUTC("alpha", 101);

  std::unique_ptr<IvPFunction> canonical_giveup(canonical.onRunState());
  std::unique_ptr<IvPFunction> alias_giveup(alias.onRunState());
  EXPECT_EQ(canonical_giveup, nullptr);
  EXPECT_EQ(alias_giveup, nullptr);

  VarDataPair canonical_flag;
  VarDataPair alias_flag;
  ASSERT_TRUE(findBehaviorMessage(canonical.getMessages(),
                                  "CUT_GIVEUP", canonical_flag));
  ASSERT_TRUE(findBehaviorMessage(alias.getMessages(),
                                  "CUT_GIVEUP", alias_flag));
  EXPECT_EQ(canonical_flag.get_sdata(), "true");
  EXPECT_EQ(alias_flag.get_sdata(), "true");
}
