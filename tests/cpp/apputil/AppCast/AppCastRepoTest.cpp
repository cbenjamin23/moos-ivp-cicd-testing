#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "AppCastRepo.h"
#include "AppCastSet.h"
#include "AppCastTree.h"
#include "MOOS/libMOOS/Thirdparty/AppCasting/AppCast.h"

namespace {

AppCast makeAppCast(const std::string& node,
                    const std::string& proc,
                    unsigned int iter,
                    const std::string& body = "status=ok")
{
  AppCast appcast;
  appcast.setNodeName(node);
  appcast.setProcName(proc);
  appcast.setIteration(iter);
  appcast.msg(body);
  return appcast;
}

AppCast makeWarnAppCast(const std::string& node,
                        const std::string& proc,
                        unsigned int cfg_warnings,
                        unsigned int run_warnings)
{
  AppCast appcast = makeAppCast(node, proc, 1, "warnings=present");
  for(unsigned int i = 0; i < cfg_warnings; ++i)
    appcast.cfgWarning("config warning " + std::to_string(i));
  for(unsigned int i = 0; i < run_warnings; ++i)
    appcast.runWarning("run warning " + std::to_string(i));
  return appcast;
}

}  // namespace

// Covers app cast set behavior: indexes MOOS processes with stable special ids.
TEST(AppCastSetTest, IndexesMoosProcessesWithStableSpecialIds)
{
  AppCastSet set;
  EXPECT_TRUE(set.addAppCast(makeAppCast("abe", "pHelmIvP", 1)));
  EXPECT_TRUE(set.addAppCast(makeAppCast("abe", "uFldNodeBroker", 1)));
  EXPECT_TRUE(set.addAppCast(makeAppCast("abe", "pHostInfo", 1)));
  EXPECT_TRUE(set.addAppCast(makeAppCast("abe", "pBasicContactMgr", 1)));
  EXPECT_TRUE(set.addAppCast(makeAppCast("abe", "pMarinePIDV22", 1)));
  EXPECT_FALSE(set.addAppCast(makeAppCast("abe", "pHelmIvP", 2)));

  EXPECT_TRUE(set.hasProc("pHelmIvP"));
  EXPECT_TRUE(set.hasProc("pMarinePIDV22"));
  EXPECT_EQ(set.getProcNameFromID("0"), "pHelmIvP");
  EXPECT_EQ(set.getProcNameFromID("1"), "uFldNodeBroker");
  EXPECT_EQ(set.getProcNameFromID("2"), "pBasicContactMgr");
  EXPECT_EQ(set.getProcNameFromID("3"), "pHostInfo");
  EXPECT_EQ(set.getProcNameFromID("a"), "pMarinePIDV22");
  EXPECT_EQ(set.getProcNameFromID("missing"), "");
  EXPECT_EQ(set.getTotalProcCount(), 5u);
  EXPECT_EQ(set.getTotalAppCastCount(), 6u);
  EXPECT_EQ(set.getAppCastCount("pHelmIvP"), 2u);
  EXPECT_EQ(set.getAppCast("pHelmIvP").getIteration(), 2u);
}

// Covers app cast set behavior: ignores empty app casts as latest proc content.
TEST(AppCastSetTest, IgnoresEmptyAppCastsAsLatestProcContent)
{
  AppCastSet set;
  AppCast empty;
  empty.setNodeName("abe");
  empty.setProcName("pHelmIvP");
  empty.setIteration(1);

  EXPECT_FALSE(set.addAppCast(empty));
  EXPECT_FALSE(set.hasProc("pHelmIvP"));
  EXPECT_EQ(set.getTotalAppCastCount(), 1u);
  EXPECT_EQ(set.getTotalProcCount(), 0u);
}

// Covers app cast set behavior: sums warnings from latest app cast per process.
TEST(AppCastSetTest, SumsWarningsFromLatestAppCastPerProcess)
{
  AppCastSet set;
  EXPECT_TRUE(set.addAppCast(makeWarnAppCast("abe", "pHelmIvP", 1, 2)));
  EXPECT_TRUE(set.addAppCast(makeWarnAppCast("abe", "pMarinePIDV22", 2, 1)));
  EXPECT_FALSE(set.addAppCast(makeWarnAppCast("abe", "pHelmIvP", 3, 0)));

  EXPECT_EQ(set.getCfgWarningCount("pHelmIvP"), 3u);
  EXPECT_EQ(set.getRunWarningCount("pHelmIvP"), 0u);
  EXPECT_EQ(set.getCfgWarningCount("pMarinePIDV22"), 2u);
  EXPECT_EQ(set.getRunWarningCount("pMarinePIDV22"), 1u);
  EXPECT_EQ(set.getTotalCfgWarningCount(), 5u);
  EXPECT_EQ(set.getTotalRunWarningCount(), 1u);
  EXPECT_EQ(set.getTotalWarningCount(), 6u);
}

// Covers app cast set behavior: assigns generated ids while skipping reserved hotkeys.
TEST(AppCastSetTest, AssignsGeneratedIdsWhileSkippingReservedHotkeys)
{
  AppCastSet set;
  for(unsigned int i = 0; i < 8; ++i) {
    ASSERT_TRUE(set.addAppCast(
        makeAppCast("abe", "custom_" + std::to_string(i), i + 1)));
  }

  EXPECT_EQ(set.getProcNameFromID("a"), "custom_0");
  EXPECT_EQ(set.getProcNameFromID("b"), "custom_1");
  EXPECT_EQ(set.getProcNameFromID("c"), "custom_2");
  EXPECT_EQ(set.getProcNameFromID("d"), "custom_3");
  EXPECT_EQ(set.getProcNameFromID("e"), "");
  EXPECT_EQ(set.getProcNameFromID("f"), "custom_4");
  EXPECT_EQ(set.getProcNameFromID("g"), "custom_5");
  EXPECT_EQ(set.getProcNameFromID("h"), "");
  EXPECT_EQ(set.getProcNameFromID("i"), "custom_6");
  EXPECT_EQ(set.getProcNameFromID("j"), "custom_7");

  EXPECT_EQ(set.getProcs(),
            (std::vector<std::string>{"custom_0", "custom_1", "custom_2",
                                      "custom_3", "custom_4", "custom_5",
                                      "custom_6", "custom_7"}));
}

// Covers app cast tree behavior: tracks nodes processes counts and warnings.
TEST(AppCastTreeTest, TracksNodesProcessesCountsAndWarnings)
{
  AppCastTree tree;
  EXPECT_TRUE(tree.addAppCast(makeWarnAppCast("shoreside", "uProcessWatch", 1, 1)));
  EXPECT_TRUE(tree.addAppCast(makeWarnAppCast("abe", "pHelmIvP", 0, 2)));
  EXPECT_TRUE(tree.addAppCast(makeWarnAppCast("abe", "pMarinePIDV22", 1, 0)));
  EXPECT_TRUE(tree.addAppCast(makeWarnAppCast("ben", "pHelmIvP", 0, 0)));

  EXPECT_EQ(tree.getTreeAppCastCount(), 4u);
  EXPECT_EQ(tree.getTreeNodeCount(), 3u);
  EXPECT_EQ(tree.getTreeProcCount(), 4u);
  EXPECT_TRUE(tree.hasNode("shoreside"));
  EXPECT_TRUE(tree.hasNodeProc("abe", "pHelmIvP"));
  EXPECT_FALSE(tree.hasNodeProc("abe", "uProcessWatch"));
  EXPECT_EQ(tree.getNodeNameFromID("0"), "shoreside");
  EXPECT_EQ(tree.getNodeNameFromID("a"), "abe");
  EXPECT_EQ(tree.getNodeNameFromID("b"), "ben");
  EXPECT_EQ(tree.getProcNameFromID("abe", "0"), "pHelmIvP");
  EXPECT_EQ(tree.getProcNameFromID("abe", "a"), "pMarinePIDV22");

  EXPECT_EQ(tree.getNodeAppCastCount("abe"), 2u);
  EXPECT_EQ(tree.getNodeTotalProcCount("abe"), 2u);
  EXPECT_EQ(tree.getNodeCfgWarningCount("abe"), 1u);
  EXPECT_EQ(tree.getNodeRunWarningCount("abe"), 2u);
  EXPECT_EQ(tree.getProcAppCastCount("abe", "pHelmIvP"), 1u);
  EXPECT_EQ(tree.getProcRunWarningCount("abe", "pHelmIvP"), 2u);
  EXPECT_EQ(tree.getAppCast("abe", "pMarinePIDV22").getProcName(), "pMarinePIDV22");
}

// Covers app cast tree behavior: removes nodes and their ids without disturbing others.
TEST(AppCastTreeTest, RemovesNodesAndTheirIdsWithoutDisturbingOthers)
{
  AppCastTree tree;
  ASSERT_TRUE(tree.addAppCast(makeAppCast("shoreside", "uProcessWatch", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("abe", "pHelmIvP", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("ben", "pHelmIvP", 1)));

  EXPECT_FALSE(tree.removeNode("cal"));
  EXPECT_TRUE(tree.removeNode("abe"));
  EXPECT_FALSE(tree.hasNode("abe"));
  EXPECT_TRUE(tree.hasNode("ben"));
  EXPECT_EQ(tree.getNodeNameFromID("a"), "");
  EXPECT_EQ(tree.getNodeNameFromID("b"), "ben");
  EXPECT_EQ(tree.getTreeNodeCount(), 2u);
  EXPECT_EQ(tree.getTreeProcCount(), 2u);
}

// Covers app cast tree behavior: assigns node ids with shoreside and reserved hotkey skips.
TEST(AppCastTreeTest, AssignsNodeIdsWithShoresideAndReservedHotkeySkips)
{
  AppCastTree tree;
  ASSERT_TRUE(tree.addAppCast(makeAppCast("shoreside", "uProcessWatch", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("abe", "pHelmIvP", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("ben", "pHelmIvP", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("cal", "pHelmIvP", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("deb", "pHelmIvP", 1)));
  ASSERT_TRUE(tree.addAppCast(makeAppCast("eve", "pHelmIvP", 1)));

  EXPECT_EQ(tree.getNodeNameFromID("0"), "shoreside");
  EXPECT_EQ(tree.getNodeNameFromID("a"), "abe");
  EXPECT_EQ(tree.getNodeNameFromID("b"), "ben");
  EXPECT_EQ(tree.getNodeNameFromID("c"), "cal");
  EXPECT_EQ(tree.getNodeNameFromID("d"), "deb");
  EXPECT_EQ(tree.getNodeNameFromID("e"), "");
  EXPECT_EQ(tree.getNodeNameFromID("f"), "eve");
}

// Covers app cast repo behavior: maintains current node proc and sorted MOOS ui lists.
TEST(AppCastRepoTest, MaintainsCurrentNodeProcAndSortedMoosUiLists)
{
  AppCastRepo repo;
  EXPECT_TRUE(repo.addAppCast(makeAppCast("ben", "uXMS", 1)));
  EXPECT_TRUE(repo.addAppCast(makeAppCast("abe", "zCustom", 1)));
  EXPECT_TRUE(repo.addAppCast(makeAppCast("abe", "pHelmIvP", 1)));
  EXPECT_TRUE(repo.addAppCast(makeAppCast("shoreside", "uProcessWatch", 1)));

  EXPECT_EQ(repo.getCurrentNode(), "ben");
  EXPECT_EQ(repo.getCurrentProc(), "uXMS");
  EXPECT_EQ(repo.getNodeCount(), 3u);
  EXPECT_EQ(repo.getProcCount(), 4u);
  EXPECT_EQ(repo.getCurrentNodes(),
            (std::vector<std::string>{"shoreside", "abe", "ben"}));

  EXPECT_TRUE(repo.setCurrentNode("abe"));
  EXPECT_EQ(repo.getCurrentNode(), "abe");
  EXPECT_EQ(repo.getCurrentProcs(), (std::vector<std::string>{"pHelmIvP", "zCustom"}));
  EXPECT_TRUE(repo.setCurrentProc("0"));
  EXPECT_EQ(repo.getCurrentProc(), "pHelmIvP");
  EXPECT_TRUE(repo.setCurrentProc("zCustom"));
  EXPECT_EQ(repo.getCurrentProc(), "zCustom");
  EXPECT_FALSE(repo.setCurrentProc("missing"));
  EXPECT_EQ(repo.getCurrentProc(), "zCustom");

  EXPECT_TRUE(repo.setCurrentNode("0"));
  EXPECT_EQ(repo.getCurrentNode(), "shoreside");
  EXPECT_EQ(repo.getCurrentProcs(), (std::vector<std::string>{"uProcessWatch"}));

  EXPECT_TRUE(repo.setCurrentNode("abe"));
  EXPECT_EQ(repo.getCurrentProc(), "zCustom");
}

// Covers app cast repo behavior: updates refresh mode and removes nodes.
TEST(AppCastRepoTest, UpdatesRefreshModeAndRemovesNodes)
{
  AppCastRepo repo;
  ASSERT_TRUE(repo.addAppCast(makeAppCast("abe", "pHelmIvP", 1)));
  ASSERT_TRUE(repo.addAppCast(makeAppCast("ben", "pMarinePIDV22", 1)));

  EXPECT_EQ(repo.getRefreshMode(), "events");
  EXPECT_TRUE(repo.setRefreshMode("STREAMING"));
  EXPECT_EQ(repo.getRefreshMode(), "streaming");
  EXPECT_TRUE(repo.setRefreshMode("paused"));
  EXPECT_EQ(repo.getRefreshMode(), "paused");
  EXPECT_FALSE(repo.setRefreshMode("fast"));
  EXPECT_EQ(repo.getRefreshMode(), "paused");

  EXPECT_FALSE(repo.removeNode("cal"));
  EXPECT_TRUE(repo.removeNode("abe"));
  EXPECT_EQ(repo.getNodeCount(), 1u);
  EXPECT_EQ(repo.getProcCount("abe"), 0u);
  EXPECT_EQ(repo.getAppCastCount("ben"), 1u);
  EXPECT_EQ(repo.getAppCastCount("ben", "pMarinePIDV22"), 1u);
}

// Covers app cast repo behavior: parses MOOS app cast strings and strips terminal colors.
TEST(AppCastRepoTest, ParsesMoosAppCastStringsAndStripsTerminalColors)
{
  AppCast appcast = makeWarnAppCast("abe", "pHelmIvP", 1, 1);
  const std::string colored = "\33[7;32m" + appcast.getAppCastString() +
                              "\33[0m";

  AppCastRepo repo(true);
  EXPECT_TRUE(repo.addAppCast(colored));

  EXPECT_EQ(repo.getCurrentNode(), "abe");
  EXPECT_EQ(repo.getCurrentProc(), "pHelmIvP");
  EXPECT_EQ(repo.getNodeCount(), 1u);
  EXPECT_EQ(repo.getProcCount(), 1u);
  EXPECT_EQ(repo.actree().getNodeCfgWarningCount("abe"), 1u);
  EXPECT_EQ(repo.actree().getNodeRunWarningCount("abe"), 1u);
  EXPECT_EQ(repo.actree().getAppCast("abe", "pHelmIvP").getIteration(), 1u);
}
