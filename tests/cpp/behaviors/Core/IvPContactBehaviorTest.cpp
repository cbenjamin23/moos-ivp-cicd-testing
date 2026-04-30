#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "BehaviorCoreTestUtils.h"
#include "InfoBuffer.h"
#include "IvPContactBehavior.h"

namespace {

class ExposedContactBehavior : public IvPContactBehavior {
public:
  explicit ExposedContactBehavior(IvPDomain domain) : IvPContactBehavior(domain) {}

  using IvPBehavior::getPostMortem;
  using IvPBehavior::setBehaviorName;
  using IvPBehavior::setBehaviorType;
  using IvPContactBehavior::applyAbleFilter;
  using IvPContactBehavior::expandMacros;
  using IvPContactBehavior::filterCheckHolds;
  using IvPContactBehavior::onEveryState;
  using IvPContactBehavior::platformUpdateOK;
  using IvPContactBehavior::postingPerContactInfo;
  using IvPContactBehavior::setComplete;
  using IvPContactBehavior::updatePlatformInfo;

  void allowDisable() { m_can_disable = true; }
  double contactRange() const { return m_contact_range; }
};

void seedOwnship(InfoBuffer& buffer)
{
  buffer.setCurrTime(100);
  buffer.setValue("NAV_X", 0);
  buffer.setValue("NAV_Y", 0);
  buffer.setValue("NAV_HEADING", 90);
  buffer.setValue("NAV_SPEED", 2);
}

}  // namespace

// Covers IvP contact behavior behavior: updates contact geometry from ledger and expands macros.
TEST(IvPContactBehaviorTest, UpdatesContactGeometryFromLedgerAndExpandsMacros)
{
  InfoBuffer buffer;
  seedOwnship(buffer);

  LedgerSnap ledger;
  seedLedgerContact(ledger, "alpha", 30, 40, 180, 3, 98);
  ledger.setGroup("alpha", "blue");
  ledger.setType("alpha", "kayak");
  ledger.setVSource("alpha", "ais");

  ExposedContactBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setBehaviorName("trail_alpha"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("post_per_contact_info", "true"));

  EXPECT_TRUE(behavior.updatePlatformInfo());
  EXPECT_TRUE(behavior.platformUpdateOK());
  EXPECT_TRUE(behavior.postingPerContactInfo());
  EXPECT_DOUBLE_EQ(behavior.contactRange(), 50);

  std::string expanded = behavior.expandMacros(
      "cn=$[CN_NAME],type=$[CN_VTYPE],grp=$[CN_GROUP],rng=$[RANGE],"
      "osrel=$[OS_CN_REL_BNG]");
  EXPECT_TRUE(containsText(expanded, "cn=alpha"));
  EXPECT_TRUE(containsText(expanded, "type=kayak"));
  EXPECT_TRUE(containsText(expanded, "rng=50"));
  EXPECT_FALSE(containsText(expanded, "$["));
}

// Covers IvP contact behavior behavior: posts contact flags for range conditions with macros.
TEST(IvPContactBehaviorTest, PostsContactFlagsForRangeConditionsWithMacros)
{
  InfoBuffer buffer;
  seedOwnship(buffer);

  LedgerSnap ledger;
  seedLedgerContact(ledger, "alpha", 30, 40, 180, 3, 98);

  ExposedContactBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setBehaviorName("contact_flags"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("cnflag", "<60 CLOSE_RANGE=$[RANGE]"));
  ASSERT_TRUE(behavior.setParam("cnflag", ">60 FAR_RANGE=$[RANGE]"));

  behavior.onEveryState("running");

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* close = findMessage(messages, "CLOSE_RANGE");
  ASSERT_NE(close, nullptr);
  EXPECT_FALSE(close->is_string());
  EXPECT_DOUBLE_EQ(close->get_ddata(), 50);
  EXPECT_EQ(findMessage(messages, "FAR_RANGE"), nullptr);
}

// Covers IvP contact behavior behavior: posts contact flags on range enter and exit transitions.
TEST(IvPContactBehaviorTest, PostsContactFlagsOnRangeEnterAndExitTransitions)
{
  InfoBuffer buffer;
  seedOwnship(buffer);

  LedgerSnap ledger;
  seedLedgerContact(ledger, "alpha", 100, 0, 180, 3, 100);

  ExposedContactBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setBehaviorName("contact_events"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("cnflag", "@<60 ENTERED_RANGE=1"));
  ASSERT_TRUE(behavior.setParam("cnflag", "@>60 EXITED_RANGE=1"));

  behavior.onEveryState("running");
  EXPECT_EQ(findMessage(behavior.getMessages(), "ENTERED_RANGE"), nullptr);
  EXPECT_EQ(findMessage(behavior.getMessages(), "EXITED_RANGE"), nullptr);

  buffer.setCurrTime(101);
  seedLedgerContact(ledger, "alpha", 50, 0, 180, 3, 101);
  behavior.onEveryState("running");
  const VarDataPair* entered = findMessage(behavior.getMessages(),
                                           "ENTERED_RANGE");
  ASSERT_NE(entered, nullptr);
  EXPECT_FALSE(entered->is_string());
  EXPECT_EQ(findMessage(behavior.getMessages(), "EXITED_RANGE"), nullptr);

  behavior.clearMessages();
  buffer.setCurrTime(102);
  seedLedgerContact(ledger, "alpha", 75, 0, 180, 3, 102);
  behavior.onEveryState("running");
  const VarDataPair* exited = findMessage(behavior.getMessages(),
                                          "EXITED_RANGE");
  ASSERT_NE(exited, nullptr);
  EXPECT_FALSE(exited->is_string());
  EXPECT_EQ(findMessage(behavior.getMessages(), "ENTERED_RANGE"), nullptr);
}

// Covers IvP contact behavior behavior: contact conditions expand configured contact macro.
TEST(IvPContactBehaviorTest, ContactConditionsExpandConfiguredContactMacro)
{
  ExposedContactBehavior behavior(makeBehaviorDomain());
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("condition", "$[CONTACT]_MODE=tracking"));

  behavior.onSetParamComplete();

  std::vector<std::string> info_vars = behavior.getInfoVars();
  EXPECT_NE(std::find(info_vars.begin(), info_vars.end(), "ALPHA_MODE"),
            info_vars.end());
}

// Covers IvP contact behavior behavior: able filter disables enables and expunges matching contact.
TEST(IvPContactBehaviorTest, AbleFilterDisablesEnablesAndExpungesMatchingContact)
{
  InfoBuffer buffer;
  seedOwnship(buffer);

  LedgerSnap ledger;
  seedLedgerContact(ledger, "alpha", 30, 40, 180, 3, 98);
  ledger.setVSource("alpha", "ais");

  ExposedContactBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setBehaviorName("filterable"));
  behavior.setBehaviorType("BHV_Trail");
  behavior.allowDisable();
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.updatePlatformInfo());

  EXPECT_FALSE(behavior.applyAbleFilter("action=disable"));
  EXPECT_TRUE(behavior.applyAbleFilter("action=disable,contact=bravo"));
  EXPECT_EQ(behavior.isRunnable(), "running");

  EXPECT_TRUE(behavior.applyAbleFilter("action=disable,contact=alpha"));
  EXPECT_EQ(behavior.isRunnable(), "disabled");

  EXPECT_TRUE(behavior.applyAbleFilter("action=enable,bhv_type=BHV_Trail"));
  EXPECT_EQ(behavior.isRunnable(), "running");

  EXPECT_TRUE(behavior.applyAbleFilter("action=expunge,vsource=ais"));
  EXPECT_EQ(behavior.isRunnable(), "completed");
  EXPECT_TRUE(containsText(behavior.getPostMortem(), "rng=50"));
  EXPECT_TRUE(containsText(behavior.getPostMortem(), "bng="));
}

// Covers IvP contact behavior behavior: missing contact warning retracts after ledger update.
TEST(IvPContactBehaviorTest, MissingContactWarningRetractsAfterLedgerUpdate)
{
  InfoBuffer buffer;
  seedOwnship(buffer);
  LedgerSnap ledger;

  ExposedContactBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setBehaviorName("watch_alpha"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));

  EXPECT_FALSE(behavior.updatePlatformInfo());
  bool found_missing_warning = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if((message.get_var() == "BHV_WARNING") &&
       containsText(message.get_sdata(),
                    "alpha x/y/heading/speed info not found"))
      found_missing_warning = true;
  }
  EXPECT_TRUE(found_missing_warning);
  EXPECT_TRUE(behavior.stateOK());

  behavior.clearMessages();
  seedLedgerContact(ledger, "alpha", 30, 40, 180, 3, 101);
  EXPECT_TRUE(behavior.updatePlatformInfo());
  bool found_retract_warning = false;
  for(const VarDataPair& message : behavior.getMessages()) {
    if((message.get_var() == "BHV_WARNING") &&
       containsText(message.get_sdata(),
                    "alpha x/y/heading/speed info not found") &&
       containsText(message.get_key(), "retract"))
      found_retract_warning = true;
  }
  EXPECT_TRUE(found_retract_warning);
}

// Covers IvP contact behavior behavior: exit filters evaluate ledger type.
TEST(IvPContactBehaviorTest, ExitFiltersEvaluateLedgerType)
{
  InfoBuffer buffer;
  seedOwnship(buffer);

  LedgerSnap ledger;
  seedLedgerContact(ledger, "alpha", 30, 40, 180, 3, 98);
  ledger.setGroup("alpha", "blue");
  ledger.setType("alpha", "kayak");

  ExposedContactBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.setLedgerSnap(&ledger);
  ASSERT_TRUE(behavior.setBehaviorName("filter_alpha"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("contact", "alpha"));
  ASSERT_TRUE(behavior.setParam("match_type", "kayak"));
  ASSERT_TRUE(behavior.setParam("exit_on_filter_vtype", "true"));

  ASSERT_TRUE(behavior.updatePlatformInfo());
  EXPECT_TRUE(behavior.filterCheckHolds());

  ledger.setType("alpha", "ship");
  ASSERT_TRUE(behavior.updatePlatformInfo());
  EXPECT_FALSE(behavior.filterCheckHolds());
}
