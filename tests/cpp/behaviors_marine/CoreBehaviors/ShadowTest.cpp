#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "BHV_Shadow.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

class TestableBHVShadow : public BHV_Shadow {
public:
  explicit TestableBHVShadow(IvPDomain domain) : BHV_Shadow(domain) {}
  using IvPBehavior::setBehaviorName;
};

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

// Covers BHV shadow behavior: pwt_outer_dist uses a strict range boundary,
// remaining relevant just inside the configured distance and inactive at it.
TEST(BHVShadowTest, OuterDistanceIsRelevantOnlyInsideStrictBoundary)
{
  auto run_at_range = [](double range) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                           range, 0, 90, 2);

    BHV_Shadow behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    EXPECT_TRUE(behavior.setParam("pwt_outer_dist", "120"));

    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    VarDataPair relevance;
    EXPECT_TRUE(findBehaviorMessage(behavior.getMessages(),
                                    "SHADOW_RELEVANCE", relevance));
    return std::make_pair(std::move(ipf), relevance.get_ddata());
  };

  auto inside = run_at_range(119);
  ASSERT_NE(inside.first, nullptr);
  EXPECT_DOUBLE_EQ(inside.second, 1);

  auto boundary = run_at_range(120);
  EXPECT_EQ(boundary.first, nullptr);
  EXPECT_DOUBLE_EQ(boundary.second, 0);
}

// Covers the nondefault heading objective shape and proves that the short
// width aliases build the same complete objective as their canonical names.
TEST(BHVShadowTest, WidthParametersShapeObjectiveAndAliasesMatchCanonical)
{
  auto build_ipf = [](const std::vector<std::pair<std::string, std::string>>& params) {
    InfoBuffer info;
    info.setCurrTime(100);
    LedgerSnap ledger;
    seedOwnshipContactInfo(info, ledger, "alpha", 0, 0, 0, 1,
                           60, 0, 90, 2);

    BHV_Shadow behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setParam("contact", "alpha"));
    for(const auto& param : params)
      EXPECT_TRUE(behavior.setParam(param.first, param.second));
    return std::unique_ptr<IvPFunction>(behavior.onRunState());
  };

  std::unique_ptr<IvPFunction> default_ipf = build_ipf({});
  std::unique_ptr<IvPFunction> narrow_heading = build_ipf({
    {"heading_peakwidth", "8"},
    {"heading_basewidth", "80"},
  });
  ASSERT_NE(default_ipf, nullptr);
  ASSERT_NE(narrow_heading, nullptr);
  EXPECT_NE(evalCourseSpeedPoint(*narrow_heading, 130, 2),
            evalCourseSpeedPoint(*default_ipf, 130, 2));
  EXPECT_NE(evalCourseSpeedPoint(*narrow_heading, 170, 2),
            evalCourseSpeedPoint(*default_ipf, 170, 2));

  std::unique_ptr<IvPFunction> canonical = build_ipf({
    {"heading_peakwidth", "25"},
    {"heading_basewidth", "170"},
    {"speed_peakwidth", "0.3"},
    {"speed_basewidth", "1.5"},
  });
  std::unique_ptr<IvPFunction> aliases = build_ipf({
    {"hdg_peakwidth", "25"},
    {"hdg_basewidth", "170"},
    {"spd_peakwidth", "0.3"},
    {"spd_basewidth", "1.5"},
  });
  ASSERT_NE(canonical, nullptr);
  ASSERT_NE(aliases, nullptr);
  for(unsigned int course = 0; course <= 360; ++course) {
    for(unsigned int speed = 0; speed <= 5; ++speed) {
      EXPECT_DOUBLE_EQ(evalCourseSpeedPoint(*canonical, course, speed),
                       evalCourseSpeedPoint(*aliases, course, speed));
    }
  }
}

// Covers the inherited missing-contact policy used by Shadow: the same absent
// ledger contact is a warning when allowed and a behavior error when required.
TEST(BHVShadowTest, MissingContactPolicyPostsExactWarningOrError)
{
  auto run_missing = [](bool missing_ok) {
    InfoBuffer info;
    info.setCurrTime(100);
    info.setValue("NAV_X", 0);
    info.setValue("NAV_Y", 0);
    info.setValue("NAV_HEADING", 90);
    info.setValue("NAV_SPEED", 1);
    LedgerSnap ledger;

    TestableBHVShadow behavior(makeCourseSpeedDomain());
    behavior.setInfoBuffer(&info);
    behavior.setLedgerSnap(&ledger);
    EXPECT_TRUE(behavior.setBehaviorName("shadow_contact"));
    EXPECT_TRUE(behavior.setParam("contact", "ghost"));
    EXPECT_TRUE(behavior.setParam("on_no_contact_ok",
                                  missing_ok ? "true" : "false"));

    std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
    EXPECT_EQ(ipf, nullptr);
    return behavior.getMessages();
  };

  const std::vector<VarDataPair> warning_messages = run_missing(true);
  VarDataPair warning;
  ASSERT_TRUE(findBehaviorMessage(warning_messages, "BHV_WARNING", warning));
  ASSERT_TRUE(warning.is_string());
  EXPECT_EQ(warning.get_sdata(),
            "shadow_contact: ghost x/y/heading/speed info not found");
  VarDataPair unexpected_error;
  EXPECT_FALSE(findBehaviorMessage(warning_messages, "BHV_ERROR",
                                   unexpected_error));

  const std::vector<VarDataPair> error_messages = run_missing(false);
  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(error_messages, "BHV_ERROR", error));
  ASSERT_TRUE(error.is_string());
  EXPECT_EQ(error.get_sdata(),
            "shadow_contact: ghost x/y/heading/speed info not found");
}

// Covers every malformed startup value used by the Shadow harness, including
// parameters inherited from IvPContactBehavior.
TEST(BHVShadowTest, RejectsEveryMalformedHarnessParameter)
{
  BHV_Shadow behavior(makeCourseSpeedDomain());

  EXPECT_FALSE(behavior.setParam("pwt_outer_dist", "-1"));
  EXPECT_FALSE(behavior.setParam("heading_peakwidth", "-5"));
  EXPECT_FALSE(behavior.setParam("heading_basewidth", "-5"));
  EXPECT_FALSE(behavior.setParam("speed_peakwidth", "-0.1"));
  EXPECT_FALSE(behavior.setParam("speed_basewidth", "-1"));
  EXPECT_FALSE(behavior.setParam("decay", "bad"));
  EXPECT_FALSE(behavior.setParam("decay", "60,30"));
  EXPECT_FALSE(behavior.setParam("extrapolate", "maybe"));
  EXPECT_FALSE(behavior.setParam("on_no_contact_ok", "maybe"));
  EXPECT_FALSE(behavior.setParam("time_on_leg", "-1"));
  EXPECT_FALSE(behavior.setParam("cnflag",
                                 "soon SHADOW_RANGE_NEAR=$[RANGE]"));
  EXPECT_FALSE(behavior.setParam("post_per_contact_info", "maybe"));
}
