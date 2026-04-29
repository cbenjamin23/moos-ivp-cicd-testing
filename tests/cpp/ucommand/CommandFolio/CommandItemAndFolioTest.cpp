#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "CommandFolio.h"
#include "CommandItem.h"
#include "CommandPost.h"

namespace {

CommandItem makeCommand(const std::string& label,
                        const std::string& var,
                        const std::string& value,
                        const std::vector<std::string>& receivers)
{
  CommandItem item;
  item.setCmdLabel(label);
  EXPECT_TRUE(item.setCmdPostStr(var, value));
  for(const std::string& receiver : receivers)
    EXPECT_TRUE(item.addCmdPostReceiver(receiver));
  return item;
}

}  // namespace

TEST(CommandItemTest, StoresStringAndDoubleCommandPosts)
{
  CommandItem item;
  item.setCmdLabel("deploy");
  item.setCmdColor("green");
  EXPECT_TRUE(item.setCmdPostStr("DEPLOY", "true"));
  EXPECT_EQ(item.getCmdLabel(), "deploy");
  EXPECT_EQ(item.getCmdColor(), "green");
  EXPECT_EQ(item.getCmdPostVar(), "DEPLOY");
  EXPECT_EQ(item.getCmdPostStr(), "true");
  EXPECT_DOUBLE_EQ(item.getCmdPostDbl(), 0);
  EXPECT_EQ(item.getCmdPostType(), "string");

  EXPECT_TRUE(item.setCmdPostDbl("RETURN_SPEED", 1.5));
  EXPECT_EQ(item.getCmdPostVar(), "RETURN_SPEED");
  EXPECT_EQ(item.getCmdPostStr(), "");
  EXPECT_DOUBLE_EQ(item.getCmdPostDbl(), 1.5);
  EXPECT_EQ(item.getCmdPostType(), "double");
}

TEST(CommandItemTest, RejectsWhitespaceInMoosVarsAndReceivers)
{
  CommandItem item;
  EXPECT_FALSE(item.setCmdPostStr("BAD VAR", "true"));
  EXPECT_EQ(item.getCmdPostVar(), "");
  EXPECT_FALSE(item.setCmdPostDbl("BAD VAR", 1));
  EXPECT_FALSE(item.addCmdPostReceiver("abe ben"));
  EXPECT_EQ(item.totalReceivers(), 0u);

  EXPECT_TRUE(item.addCmdPostReceiver("abe"));
  EXPECT_TRUE(item.addCmdPostReceiver("all"));
  EXPECT_TRUE(item.addCmdPostReceiver("each"));
  EXPECT_EQ(item.totalReceivers(), 3u);
  EXPECT_TRUE(item.hasReceiver("abe"));
  EXPECT_FALSE(item.hasReceiver("ben"));
  EXPECT_EQ(item.getCmdReceiver(3), "");
}

TEST(CommandItemTest, LimitsReceiversToKnownVehiclesAndBroadcastTokens)
{
  CommandItem item = makeCommand("return", "RETURN", "true",
                                 {"abe", "ben", "bad", "all", "each"});
  item.limitedVNames({"abe", "ben"});

  std::vector<std::string> receivers = item.getAllReceivers();
  ASSERT_EQ(receivers.size(), 4u);
  EXPECT_EQ(receivers[0], "abe");
  EXPECT_EQ(receivers[1], "ben");
  EXPECT_EQ(receivers[2], "all");
  EXPECT_EQ(receivers[3], "each");
}

TEST(CommandFolioTest, StoresCommandsAndIndexesReceiversLabelsAndColors)
{
  CommandFolio folio;
  CommandItem invalid;
  invalid.setCmdLabel("invalid");
  ASSERT_TRUE(invalid.setCmdPostStr("DEPLOY", "true"));
  EXPECT_FALSE(folio.addCmdItem(invalid));

  CommandItem deploy = makeCommand("deploy", "DEPLOY", "true", {"abe", "ben"});
  deploy.setCmdColor("green");
  CommandItem station = makeCommand("station", "STATION_KEEP", "true",
                                    {"ben", "cal"});
  station.setCmdColor("blue");
  EXPECT_TRUE(folio.addCmdItem(deploy));
  EXPECT_TRUE(folio.addCmdItem(station));
  EXPECT_EQ(folio.size(), 2u);

  EXPECT_EQ(folio.getCmdItem(0).getCmdLabel(), "deploy");
  EXPECT_EQ(folio.getCmdItem(99).getCmdLabel(), "");
  EXPECT_EQ(folio.getCmdColor("deploy"), "green");
  EXPECT_EQ(folio.getCmdColor("missing"), "");
  EXPECT_EQ(folio.getAllReceivers(), (std::set<std::string>{"abe", "ben", "cal"}));
  EXPECT_EQ(folio.getAllLabels("ben"), (std::set<std::string>{"deploy", "station"}));
  EXPECT_EQ(folio.getAllLabels("abe"), (std::set<std::string>{"deploy"}));
}

TEST(CommandFolioTest, AppliesVehicleLimitsAcrossExistingCommands)
{
  CommandFolio folio;
  EXPECT_TRUE(folio.addCmdItem(
      makeCommand("deploy", "DEPLOY", "true", {"abe", "ben", "all"})));
  EXPECT_TRUE(folio.addCmdItem(
      makeCommand("return", "RETURN", "true", {"cal", "each"})));

  folio.limitedVNames({"abe"});
  EXPECT_TRUE(folio.hasLimitedVNames());
  EXPECT_EQ(folio.getLimitedVNames(), (std::set<std::string>{"abe"}));
  EXPECT_EQ(folio.getAllReceivers(), (std::set<std::string>{"abe", "all", "each"}));
  EXPECT_EQ(folio.getAllLabels("ben"), std::set<std::string>{});
  EXPECT_EQ(folio.getAllLabels("abe"), (std::set<std::string>{"deploy"}));
  EXPECT_EQ(folio.getAllLabels("each"), (std::set<std::string>{"return"}));
}

TEST(CommandPostTest, WrapsPendingCommandMetadataFromGuiSelection)
{
  CommandItem deploy = makeCommand("deploy", "DEPLOY", "true", {"abe"});
  CommandPost post;
  post.setCommandItem(deploy);
  post.setCommandTarg("abe");
  post.setCommandPID("uCommand_42");
  post.setCommandTest(true);

  EXPECT_EQ(post.getCommandItem().getCmdLabel(), "deploy");
  EXPECT_EQ(post.getCommandTarg(), "abe");
  EXPECT_EQ(post.getCommandPID(), "uCommand_42");
  EXPECT_TRUE(post.getCommandTest());
}
