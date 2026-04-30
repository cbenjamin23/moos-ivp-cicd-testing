#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "BehaviorCoreTestUtils.h"
#include "InfoBuffer.h"
#include "IvPBehavior.h"

namespace {

class ExposedBehavior : public IvPBehavior {
public:
  explicit ExposedBehavior(IvPDomain domain) : IvPBehavior(domain) {}

  using IvPBehavior::addInfoVars;
  using IvPBehavior::checkConditions;
  using IvPBehavior::checkForDurationReset;
  using IvPBehavior::checkForUpdatedCommsPolicy;
  using IvPBehavior::durationExceeded;
  using IvPBehavior::expandMacros;
  using IvPBehavior::getBehaviorAge;
  using IvPBehavior::getBufferDoubleValX;
  using IvPBehavior::getBufferStringValX;
  using IvPBehavior::getMaxOSV;
  using IvPBehavior::getOwnGroup;
  using IvPBehavior::getPriorityWt;
  using IvPBehavior::getStateSpaceVars;
  using IvPBehavior::getUpdateResults;
  using IvPBehavior::postDurationStatus;
  using IvPBehavior::postFlags;
  using IvPBehavior::postGMessage;
  using IvPBehavior::postMessage;
  using IvPBehavior::postWMessage;
  using IvPBehavior::postXMessage;
  using IvPBehavior::resetRegulatedMessage;
  using IvPBehavior::regulateOffboardMessage;
  using IvPBehavior::setBehaviorName;
  using IvPBehavior::setBehaviorType;
  using IvPBehavior::setComplete;
  using IvPBehavior::setHelmIteration;
  using IvPBehavior::updatePlatformInfo;
  using IvPBehavior::updateStateDurations;

  std::string commsPolicyForTest() const { return commsPolicy(); }
};

}  // namespace

// Covers IvP behavior behavior: common params gate runnable on conditions duration and starvation.
TEST(IvPBehaviorTest, CommonParamsGateRunnableOnConditionsDurationAndStarvation)
{
  InfoBuffer buffer;
  buffer.setCurrTime(10);
  buffer.setValue("DEPLOY", "false");
  buffer.setValue("NAV_X", 12.0);

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("survey"));
  EXPECT_TRUE(behavior.setParam("condition", "DEPLOY=true"));
  EXPECT_TRUE(behavior.setParam("duration", "4"));
  EXPECT_TRUE(behavior.setParam("duration_status", "BHV_REMAINING"));
  EXPECT_TRUE(behavior.setParam("no_starve", "NAV_X,2"));

  EXPECT_EQ(behavior.isRunnable(), "idle");

  buffer.setValue("DEPLOY", "true");
  EXPECT_EQ(behavior.isRunnable(), "running");

  buffer.setCurrTime(13);
  EXPECT_EQ(behavior.isRunnable(), "idle");

  buffer.setValue("NAV_X", 12.5);
  EXPECT_EQ(behavior.isRunnable(), "running");

  behavior.clearMessages();
  buffer.setCurrTime(18);
  EXPECT_EQ(behavior.isRunnable(), "completed");
  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* remaining = findMessage(messages, "BHV_REMAINING");
  ASSERT_NE(remaining, nullptr);
  EXPECT_DOUBLE_EQ(remaining->get_ddata(), 0);
}

// Covers IvP behavior behavior: reads platform info and expands core macros in flags.
TEST(IvPBehaviorTest, ReadsPlatformInfoAndExpandsCoreMacrosInFlags)
{
  InfoBuffer buffer;
  buffer.setCurrTime(50);
  buffer.setValue("NAV_X", 10);
  buffer.setValue("NAV_Y", 20);
  buffer.setValue("NAV_HEADING", 725);
  buffer.setValue("NAV_SPEED", 2.5);
  buffer.setValue("ABE_NAV_GROUP", "red");
  buffer.setValue("MODE", "survey");

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("core_bhv"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.updatePlatformInfo());
  EXPECT_EQ(behavior.getOwnGroup(), "red");
  EXPECT_DOUBLE_EQ(behavior.getMaxOSV(), 5);

  ASSERT_TRUE(behavior.setParam("runflag",
                                "SUMMARY=\"$[OWNSHIP]:$[BHVNAME]:$[OSX]:$[OSH]\""
                                " [if] MODE=survey"));
  behavior.postFlags("runflags");
  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* summary = findMessage(messages, "SUMMARY");
  ASSERT_NE(summary, nullptr);
  EXPECT_EQ(summary->get_sdata(), "abe:core_bhv:10:5");
}

// Covers IvP behavior behavior: updates apply fresh matching strings and report failures.
TEST(IvPBehaviorTest, UpdatesApplyFreshMatchingStringsAndReportFailures)
{
  // Behavior updates are consumed from InfoBuffer mail once; repeating the same
  // update string is reported as a duplicate instead of being applied again.
  InfoBuffer buffer;
  buffer.setCurrTime(20);

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("speed_keep"));
  ASSERT_TRUE(behavior.setParam("updates", "BHV_UPDATE"));

  buffer.setValue("BHV_UPDATE", "name=speed_keep#pwt=37#build_info=precision=low");
  EXPECT_TRUE(behavior.checkUpdates());
  EXPECT_DOUBLE_EQ(behavior.getPriorityWt(), 37);
  EXPECT_EQ(behavior.getUpdateSummary(), "1/1");
  ASSERT_EQ(behavior.getUpdateResults().size(), 1u);
  EXPECT_TRUE(containsText(behavior.getUpdateResults()[0], "result=success"));

  EXPECT_FALSE(behavior.checkUpdates());
  ASSERT_EQ(behavior.getUpdateResults().size(), 1u);
  EXPECT_TRUE(containsText(behavior.getUpdateResults()[0], "result=duplicate"));

  buffer.setValue("BHV_UPDATE", "name=speed_keep#bogus=10");
  EXPECT_FALSE(behavior.checkUpdates());
  EXPECT_EQ(behavior.getUpdateSummary(), "1/2");
  const std::vector<VarDataPair> messages = behavior.getMessages();
  EXPECT_NE(findMessage(messages, "BHV_WARNING"), nullptr);
  EXPECT_NE(findMessage(messages, "BHV_CONFIG_WARNING"), nullptr);
}

// Covers IvP behavior behavior: remaps local posts and builds offboard group messages.
TEST(IvPBehaviorTest, RemapsLocalPostsAndBuildsOffboardGroupMessages)
{
  InfoBuffer buffer;
  buffer.setCurrTime(30);
  buffer.setValue("ABE_NAV_GROUP", "red");

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("messenger"));
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.setParam("post_mapping", "LOCAL,REMAPPED"));
  ASSERT_TRUE(behavior.setParam("post_mapping", "QUIET,silent"));

  behavior.postMessage("LOCAL", std::string("hello"));
  behavior.postMessage("QUIET", std::string("hidden"));
  behavior.postGMessage("CONTACT_NOTE", std::string("hold"));

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* remapped = findMessage(messages, "REMAPPED");
  ASSERT_NE(remapped, nullptr);
  EXPECT_EQ(remapped->get_sdata(), "hello");
  EXPECT_EQ(findMessage(messages, "QUIET"), nullptr);

  const VarDataPair* node_message = findMessage(messages, "NODE_MESSAGE_LOCAL");
  ASSERT_NE(node_message, nullptr);
  EXPECT_TRUE(containsText(node_message->get_sdata(), "src_node=abe"));
  EXPECT_TRUE(containsText(node_message->get_sdata(), "dest_group=red"));
  EXPECT_TRUE(containsText(node_message->get_sdata(), "var_name=CONTACT_NOTE"));
  EXPECT_TRUE(containsText(node_message->get_sdata(), "hold"));
}

// Covers IvP behavior behavior: destination tagged flags post locally and offboard.
TEST(IvPBehaviorTest, DestinationTaggedFlagsPostLocallyAndOffboard)
{
  InfoBuffer buffer;
  buffer.setCurrTime(40);
  buffer.setValue("NAV_X", 0);
  buffer.setValue("NAV_Y", 0);
  buffer.setValue("NAV_HEADING", 90);
  buffer.setValue("NAV_SPEED", 2.75);
  buffer.setValue("ABE_NAV_GROUP", "red");
  buffer.setValue("MODE", "survey");

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("flagger"));
  behavior.setBehaviorType("BHV_Test");
  ASSERT_TRUE(behavior.setParam("us", "abe"));
  ASSERT_TRUE(behavior.updatePlatformInfo());
  ASSERT_TRUE(behavior.setParam("runflag",
                                "#all+ SPEED_REPORT=$[OSV] [if] MODE=survey"));

  behavior.postFlags("runflags");

  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* local = findMessage(messages, "SPEED_REPORT");
  ASSERT_NE(local, nullptr);
  EXPECT_FALSE(local->is_string());
  EXPECT_DOUBLE_EQ(local->get_ddata(), 2.75);

  const VarDataPair* node_message = findMessage(messages, "NODE_MESSAGE_LOCAL");
  ASSERT_NE(node_message, nullptr);
  EXPECT_TRUE(containsText(node_message->get_sdata(), "src_node=abe"));
  EXPECT_TRUE(containsText(node_message->get_sdata(), "dest_node=all"));
  EXPECT_TRUE(containsText(node_message->get_sdata(), "var_name=SPEED_REPORT"));
  EXPECT_TRUE(containsText(node_message->get_sdata(), "double_val=2.75"));
}

// Covers IvP behavior behavior: duration reset variable restarts timer only on fresh post.
TEST(IvPBehaviorTest, DurationResetVariableRestartsTimerOnlyOnFreshPost)
{
  InfoBuffer buffer;
  buffer.setCurrTime(100);

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("timed"));
  ASSERT_TRUE(behavior.setParam("duration", "5"));
  ASSERT_TRUE(behavior.setParam("duration_status", "BHV_REMAINING"));
  ASSERT_TRUE(behavior.setParam("duration_reset", "RESET_TIMER=true"));

  EXPECT_EQ(behavior.isRunnable(), "running");

  buffer.setCurrTime(103);
  EXPECT_EQ(behavior.isRunnable(), "running");

  buffer.setValue("RESET_TIMER", "true");
  EXPECT_TRUE(behavior.checkForDurationReset());
  EXPECT_FALSE(behavior.checkForDurationReset());

  const std::vector<VarDataPair> reset_messages = behavior.getMessages();
  const VarDataPair* remaining = findMessage(reset_messages, "BHV_REMAINING");
  ASSERT_NE(remaining, nullptr);
  EXPECT_DOUBLE_EQ(remaining->get_ddata(), 5);

  buffer.setCurrTime(107);
  EXPECT_EQ(behavior.isRunnable(), "running");

  buffer.setCurrTime(113);
  EXPECT_EQ(behavior.isRunnable(), "completed");
}

// Covers IvP behavior behavior: comms policy updates cannot relax configured ceiling.
TEST(IvPBehaviorTest, CommsPolicyUpdatesCannotRelaxConfiguredCeiling)
{
  InfoBuffer buffer;
  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setParam("comms_policy", "lean"));

  buffer.setValue("COMMS_POLICY", "open");
  behavior.checkForUpdatedCommsPolicy();
  EXPECT_EQ(behavior.commsPolicyForTest(), "lean");

  buffer.setValue("COMMS_POLICY", "dire");
  behavior.checkForUpdatedCommsPolicy();
  EXPECT_EQ(behavior.commsPolicyForTest(), "dire");

  ASSERT_TRUE(behavior.setParam("comms_policy", "dire"));
  buffer.setValue("COMMS_POLICY", "lean");
  behavior.checkForUpdatedCommsPolicy();
  EXPECT_EQ(behavior.commsPolicyForTest(), "dire");
}

// Covers IvP behavior behavior: buffer access converts types and no warning vars stay quiet.
TEST(IvPBehaviorTest, BufferAccessConvertsTypesAndNoWarningVarsStayQuiet)
{
  InfoBuffer buffer;
  buffer.setCurrTime(1);
  buffer.setValue("SPEED_STR", "2.75");
  buffer.setValue("NUMERIC_MODE", 7.5);

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  behavior.addInfoVars("MISSING_OK", "no_warning");

  double speed = 0;
  EXPECT_TRUE(behavior.getBufferDoubleValX("SPEED_STR", speed));
  EXPECT_DOUBLE_EQ(speed, 2.75);

  std::string numeric_mode;
  EXPECT_TRUE(behavior.getBufferStringValX("NUMERIC_MODE", numeric_mode));
  EXPECT_EQ(numeric_mode, "7.500000");

  std::string missing;
  EXPECT_FALSE(behavior.getBufferStringValX("MISSING_OK", missing));
  EXPECT_EQ(findMessage(behavior.getMessages(), "BHV_WARNING"), nullptr);
}

// Covers IvP behavior behavior: reports missing platform inputs as behavior errors.
TEST(IvPBehaviorTest, ReportsMissingPlatformInputsAsBehaviorErrors)
{
  InfoBuffer buffer;
  buffer.setCurrTime(1);
  buffer.setValue("NAV_X", 10);
  buffer.setValue("NAV_Y", 20);

  ExposedBehavior behavior(makeBehaviorDomain());
  behavior.setInfoBuffer(&buffer);
  ASSERT_TRUE(behavior.setBehaviorName("needs_heading"));

  EXPECT_FALSE(behavior.updatePlatformInfo());
  EXPECT_FALSE(behavior.stateOK());
  const std::vector<VarDataPair> messages = behavior.getMessages();
  const VarDataPair* error = findMessage(messages, "BHV_ERROR");
  ASSERT_NE(error, nullptr);
  EXPECT_TRUE(containsText(error->get_sdata(), "No ownship heading"));

  behavior.resetStateOK();
  EXPECT_TRUE(behavior.stateOK());
}
