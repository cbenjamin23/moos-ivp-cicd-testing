#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "BHV_Convoy.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

namespace {

void configureConvoy(BHV_Convoy& behavior,
                     InfoBuffer& info,
                     LedgerSnap& ledger,
                     const std::string& contact)
{
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", contact));
  ASSERT_TRUE(behavior.setParam("inter_mark_range", "10"));
  ASSERT_TRUE(behavior.setParam("max_mark_range", "150"));
  ASSERT_TRUE(behavior.setParam("radius", "1"));
  ASSERT_TRUE(behavior.setParam("spd_slower", "0.5"));
  ASSERT_TRUE(behavior.setParam("spd_faster", "1.5"));
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

// Covers BHV convoy behavior: queues contact marks and builds waypoint objective.
TEST(BHVConvoyTest, QueuesContactMarksAndBuildsWaypointObjective)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 0, 100, 0, 2);

  BHV_Convoy behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("inter_mark_range", "10"));
  ASSERT_TRUE(behavior.setParam("max_mark_range", "150"));
  ASSERT_TRUE(behavior.setParam("cruise_speed", "2"));
  ASSERT_TRUE(behavior.setParam("rng_safety", "true"));
  EXPECT_FALSE(behavior.setParam("spd_slower", "2"));
  EXPECT_FALSE(behavior.setParam("max_mark_range", "0"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair qlen;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "QLEN", qlen));
  EXPECT_DOUBLE_EQ(qlen.get_ddata(), 0);

  VarDataPair wpty;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPTY", wpty));
  EXPECT_DOUBLE_EQ(wpty.get_ddata(), 100);

  VarDataPair point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", point));
  EXPECT_NE(point.get_sdata().find("abe_alpha_0"), std::string::npos);
}

// Covers BHV convoy behavior: range safety estop commands zero speed when tailgating contact.
TEST(BHVConvoyTest, RangeSafetyEstopCommandsZeroSpeedWhenTailgatingContact)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 90, 0, 1, 0, 100, 0, 2);

  BHV_Convoy behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("inter_mark_range", "10"));
  ASSERT_TRUE(behavior.setParam("max_mark_range", "150"));
  ASSERT_TRUE(behavior.setParam("radius", "2"));
  ASSERT_TRUE(behavior.setParam("rng_estop", "15"));
  ASSERT_TRUE(behavior.setParam("rng_tgating", "30"));
  ASSERT_TRUE(behavior.setParam("rng_lagging", "70"));
  ASSERT_TRUE(behavior.setParam("rng_safety", "true"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 0),
            evalCourseSpeedPoint(*ipf, 0, 2));
}

// Covers BHV convoy behavior: run to idle erases queued contact marks.
TEST(BHVConvoyTest, RunToIdleErasesQueuedContactMarks)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 0, 100, 0, 2);

  BHV_Convoy behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("inter_mark_range", "10"));
  ASSERT_TRUE(behavior.setParam("max_mark_range", "40"));
  ASSERT_TRUE(behavior.setParam("cruise_speed", "2"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  behavior.clearMessages();
  info.setCurrTime(101);
  ledger.setX("alpha", 0);
  ledger.setY("alpha", 120);
  ledger.setUTC("alpha", info.getCurrTime());
  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);

  behavior.clearMessages();
  behavior.onRunToIdleState();

  bool erased_mark = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      erased_mark |= (message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(erased_mark);
}

// Covers BHV convoy behavior: safety ranges and cruise speed are auto adjusted at config complete.
TEST(BHVConvoyTest, SafetyRangesAndCruiseSpeedAreAutoAdjustedAtConfigComplete)
{
  BHV_Convoy behavior(makeCourseSpeedDomain());

  ASSERT_TRUE(behavior.setParam("rng_safety", "true"));
  ASSERT_TRUE(behavior.setParam("rng_estop", "2"));
  ASSERT_TRUE(behavior.setParam("rng_tgating", "3"));
  ASSERT_TRUE(behavior.setParam("rng_lagging", "4"));
  ASSERT_TRUE(behavior.setParam("spd_max", "1"));
  ASSERT_TRUE(behavior.setParam("cruise_speed", "2"));

  behavior.onSetParamComplete();

  bool saw_range_warning = false;
  bool saw_speed_warning = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "BHV_WARNING" && message.is_string()) {
      saw_range_warning |=
        (message.get_sdata().find("Safety ranges were auto-adusted") != std::string::npos);
      saw_speed_warning |=
        (message.get_sdata().find("Cruise speed auto-capped") != std::string::npos);
    }
  }
  EXPECT_TRUE(saw_range_warning);
  EXPECT_TRUE(saw_speed_warning);
}

// Covers BHV convoy behavior: lagging range uses recent contact speed average and faster multiplier.
TEST(BHVConvoyTest, LaggingRangeUsesRecentContactSpeedAverageAndFasterMultiplier)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 0, 100, 0, 2);

  BHV_Convoy behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("inter_mark_range", "10"));
  ASSERT_TRUE(behavior.setParam("max_mark_range", "150"));
  ASSERT_TRUE(behavior.setParam("rng_safety", "true"));
  ASSERT_TRUE(behavior.setParam("rng_lagging", "70"));
  ASSERT_TRUE(behavior.setParam("spd_faster", "1.5"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair avg2;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "AVG2", avg2));
  EXPECT_DOUBLE_EQ(avg2.get_ddata(), 2);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 3),
            evalCourseSpeedPoint(*ipf, 0, 1));
}

// Covers BHV convoy behavior: mark queue drops oldest when total length exceeds max range.
TEST(BHVConvoyTest, MarkQueueDropsOldestWhenTotalLengthExceedsMaxRange)
{
  InfoBuffer info;
  info.setCurrTime(100);
  LedgerSnap ledger;
  seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1, 0, 100, 0, 2);

  BHV_Convoy behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  behavior.setLedgerSnap(&ledger);

  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("inter_mark_range", "5"));
  ASSERT_TRUE(behavior.setParam("max_mark_range", "15"));
  ASSERT_TRUE(behavior.setParam("cruise_speed", "2"));

  behavior.onSetParamComplete();
  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);

  info.setCurrTime(101);
  ledger.setY("alpha", 110);
  ledger.setUTC("alpha", info.getCurrTime());
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);

  info.setCurrTime(102);
  ledger.setY("alpha", 130);
  ledger.setUTC("alpha", info.getCurrTime());
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> third_ipf(behavior.onRunState());
  ASSERT_NE(third_ipf, nullptr);

  bool saw_oldest_erased = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if(message.get_var() == "VIEW_POINT" && message.is_string())
      saw_oldest_erased |=
        (message.get_sdata().find("abe_alpha_0") != std::string::npos &&
         message.get_sdata().find("active=false") != std::string::npos);
  }
  EXPECT_TRUE(saw_oldest_erased);

  VarDataPair wpty;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "WPTY", wpty));
  EXPECT_DOUBLE_EQ(wpty.get_ddata(), 110);
}

// Covers BHV convoy behavior: legacy range aliases select the same speed
// branches as their canonical parameter names.
TEST(BHVConvoyTest, LegacyRangeAliasesMatchCanonicalRanges)
{
  const std::vector<double> contact_ranges = {17, 35, 65};

  for(double contact_range : contact_ranges) {
    InfoBuffer canonical_info;
    canonical_info.setCurrTime(100);
    LedgerSnap canonical_ledger;
    seedOwnshipContactInfo(canonical_info, canonical_ledger, "alpha",
                           0, 0, 0, 1,
                           contact_range, 0, 0, 2);

    InfoBuffer alias_info;
    alias_info.setCurrTime(100);
    LedgerSnap alias_ledger;
    seedOwnshipContactInfo(alias_info, alias_ledger, "alpha",
                           0, 0, 0, 1,
                           contact_range, 0, 0, 2);

    BHV_Convoy canonical(makeCourseSpeedDomain());
    configureConvoy(canonical, canonical_info, canonical_ledger, "alpha");
    ASSERT_TRUE(canonical.setParam("rng_estop", "20"));
    ASSERT_TRUE(canonical.setParam("rng_tgating", "40"));
    ASSERT_TRUE(canonical.setParam("rng_lagging", "60"));
    canonical.onSetParamComplete();

    BHV_Convoy alias(makeCourseSpeedDomain());
    configureConvoy(alias, alias_info, alias_ledger, "alpha");
    ASSERT_TRUE(alias.setParam("range_estop", "20"));
    ASSERT_TRUE(alias.setParam("range_tailgating", "40"));
    ASSERT_TRUE(alias.setParam("range_lagging", "60"));
    alias.onSetParamComplete();

    std::unique_ptr<IvPFunction> canonical_ipf(canonical.onRunState());
    std::unique_ptr<IvPFunction> alias_ipf(alias.onRunState());
    ASSERT_NE(canonical_ipf, nullptr);
    ASSERT_NE(alias_ipf, nullptr);
    expectSameCourseSpeedObjective(*canonical_ipf, *alias_ipf);
  }
}

// Covers BHV convoy behavior: breadcrumb radius controls whether the oldest
// mark is consumed at a fixed ownship distance.
TEST(BHVConvoyTest, RadiusControlsBreadcrumbConsumption)
{
  InfoBuffer tight_info;
  tight_info.setCurrTime(100);
  LedgerSnap tight_ledger;
  seedOwnshipContactInfo(tight_info, tight_ledger, "alpha",
                         0, 0, 0, 1, 10, 0, 0, 2);

  BHV_Convoy tight(makeCourseSpeedDomain());
  configureConvoy(tight, tight_info, tight_ledger, "alpha");
  ASSERT_TRUE(tight.setParam("radius", "3"));

  std::unique_ptr<IvPFunction> tight_ipf(tight.onRunState());
  ASSERT_NE(tight_ipf, nullptr);

  bool tight_erased_mark = false;
  for(const VarDataPair& message : tight.getMessages()) {
    tight_erased_mark |= (message.get_var() == "VIEW_POINT") &&
                         message.is_string() &&
                         (message.get_sdata().find("active=false") !=
                          std::string::npos);
  }
  EXPECT_FALSE(tight_erased_mark);

  InfoBuffer wide_info;
  wide_info.setCurrTime(100);
  LedgerSnap wide_ledger;
  seedOwnshipContactInfo(wide_info, wide_ledger, "alpha",
                         0, 0, 0, 1, 10, 0, 0, 2);

  BHV_Convoy wide(makeCourseSpeedDomain());
  configureConvoy(wide, wide_info, wide_ledger, "alpha");
  ASSERT_TRUE(wide.setParam("radius", "16"));

  std::unique_ptr<IvPFunction> wide_ipf(wide.onRunState());
  EXPECT_EQ(wide_ipf, nullptr);

  bool wide_erased_mark = false;
  for(const VarDataPair& message : wide.getMessages()) {
    wide_erased_mark |= (message.get_var() == "VIEW_POINT") &&
                        message.is_string() &&
                        (message.get_sdata().find("active=false") !=
                         std::string::npos);
  }
  EXPECT_TRUE(wide_erased_mark);
}

// Covers BHV convoy behavior: disabling range safety preserves deliberately
// tight thresholds instead of normalizing them before speed selection.
TEST(BHVConvoyTest, SafetyDisabledPreservesConfiguredRanges)
{
  InfoBuffer disabled_info;
  disabled_info.setCurrTime(100);
  LedgerSnap disabled_ledger;
  seedOwnshipContactInfo(disabled_info, disabled_ledger, "alpha",
                         0, 0, 0, 1, 4, 0, 0, 2);

  BHV_Convoy disabled(makeCourseSpeedDomain());
  configureConvoy(disabled, disabled_info, disabled_ledger, "alpha");
  ASSERT_TRUE(disabled.setParam("rng_safety", "false"));
  ASSERT_TRUE(disabled.setParam("rng_estop", "2"));
  ASSERT_TRUE(disabled.setParam("rng_tgating", "5"));
  ASSERT_TRUE(disabled.setParam("rng_lagging", "6"));
  disabled.onSetParamComplete();

  std::unique_ptr<IvPFunction> disabled_ipf(disabled.onRunState());
  ASSERT_NE(disabled_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*disabled_ipf, 90, 1),
            evalCourseSpeedPoint(*disabled_ipf, 90, 0));

  InfoBuffer enabled_info;
  enabled_info.setCurrTime(100);
  LedgerSnap enabled_ledger;
  seedOwnshipContactInfo(enabled_info, enabled_ledger, "alpha",
                         0, 0, 0, 1, 4, 0, 0, 2);

  BHV_Convoy enabled(makeCourseSpeedDomain());
  configureConvoy(enabled, enabled_info, enabled_ledger, "alpha");
  ASSERT_TRUE(enabled.setParam("rng_safety", "true"));
  ASSERT_TRUE(enabled.setParam("rng_estop", "2"));
  ASSERT_TRUE(enabled.setParam("rng_tgating", "5"));
  ASSERT_TRUE(enabled.setParam("rng_lagging", "6"));
  enabled.onSetParamComplete();

  std::unique_ptr<IvPFunction> enabled_ipf(enabled.onRunState());
  ASSERT_NE(enabled_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*enabled_ipf, 90, 0),
            evalCourseSpeedPoint(*enabled_ipf, 90, 1));
}
