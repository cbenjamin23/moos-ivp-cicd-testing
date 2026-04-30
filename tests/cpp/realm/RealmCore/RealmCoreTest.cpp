#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "InfoCastSettings.h"
#include "RealmCast.h"
#include "RealmCastSet.h"
#include "RealmCastTree.h"
#include "RealmRepo.h"
#include "RealmSummary.h"
#include "WatchCast.h"
#include "WatchCluster.h"

namespace {

bool vectorContainsSubstring(const std::vector<std::string>& lines,
                             const std::string& needle)
{
  for(const std::string& line : lines) {
    if(line.find(needle) != std::string::npos)
      return true;
  }
  return false;
}

RealmCast makeRealmCast(const std::string& node,
                        const std::string& proc,
                        const std::string& msg)
{
  RealmCast cast;
  cast.setNodeName(node);
  cast.setProcName(proc);
  cast.msg(msg);
  return cast;
}

WatchCast makeWatchCast(const std::string& node,
                        const std::string& var,
                        const std::string& sval)
{
  WatchCast cast;
  cast.setNode(node);
  cast.setVarName(var);
  cast.setSource("pMarineViewer");
  cast.setCommunity("shoreside");
  cast.setLocTime(12.34);
  cast.setUtcTime(100.5);
  cast.setSVal(sval);
  return cast;
}

}  // namespace

// Covers realm cast behavior: serializes and parses pMarineViewer realm cast payload.
TEST(RealmCastTest, SerializesAndParsesPMarineViewerRealmCastPayload)
{
  RealmCast cast = makeRealmCast("abe", "pHelmIvP",
                                 "mode=survey\nwarnings=0");
  cast.setCount(7);

  std::string payload = cast.getRealmCastString();
  EXPECT_NE(payload.find("node=abe!Q!proc=pHelmIvP!Q!count=7!Q!messages="),
            std::string::npos);
  EXPECT_NE(payload.find("mode=survey!Z!warnings=0!Z!"), std::string::npos);

  RealmCast parsed = string2RealmCast(payload);
  EXPECT_EQ(parsed.getNodeName(), "abe");
  EXPECT_EQ(parsed.getProcName(), "pHelmIvP");
  EXPECT_EQ(parsed.getCount(), 7u);
  EXPECT_NE(parsed.getFormattedString().find("pHelmIvP"), std::string::npos);
  EXPECT_NE(parsed.getFormattedString().find("abe (7)"), std::string::npos);
  EXPECT_NE(parsed.getFormattedString().find("mode=survey"), std::string::npos);
}

// Covers realm cast behavior: empty or missing fields parse as unknown but remain counted.
TEST(RealmCastTest, EmptyOrMissingFieldsParseAsUnknownButRemainCounted)
{
  RealmCast parsed = string2RealmCast("messages=status ok!Z!");
  EXPECT_EQ(parsed.getNodeName(), "");
  EXPECT_EQ(parsed.getProcName(), "");
  EXPECT_EQ(parsed.getCount(), 0u);
  EXPECT_GT(parsed.size(), 0u);

  RealmCastSet set;
  EXPECT_TRUE(set.addRealmCast(parsed));
  EXPECT_TRUE(set.hasProc("unknown_proc"));
  EXPECT_EQ(set.getRealmCastCount("unknown_proc"), 1u);
}

// Covers realm summary behavior: deduplicates channels and formats realmcast channels.
TEST(RealmSummaryTest, DeduplicatesChannelsAndFormatsRealmcastChannels)
{
  RealmSummary summary;
  summary.setNode("abe");
  summary.addProc("pHelmIvP");
  summary.addProc("pNodeReporter");
  summary.addProc("pHelmIvP");
  summary.addHistVar("NAV_X");
  summary.addHistVar("NAV_X");

  EXPECT_TRUE(summary.valid());
  EXPECT_EQ(summary.size(), 2u);
  EXPECT_EQ(summary.getProcs(),
            (std::vector<std::string>{"pHelmIvP", "pNodeReporter"}));
  EXPECT_EQ(summary.getHistVars(), (std::vector<std::string>{"NAV_X"}));
  EXPECT_EQ(summary.get_spec(),
            "node=abe#channels=pHelmIvP,pNodeReporter,NAV_X");
}

// Covers realm summary behavior: parses realmcast channels and rejects unknown sections.
TEST(RealmSummaryTest, ParsesRealmcastChannelsAndRejectsUnknownSections)
{
  RealmSummary parsed =
      string2RealmSummary("node=abe#channels=pHelmIvP,pNodeReporter");
  EXPECT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.getNode(), "abe");
  EXPECT_EQ(parsed.getProcs(),
            (std::vector<std::string>{"pHelmIvP", "pNodeReporter"}));

  EXPECT_FALSE(string2RealmSummary("node=abe#bad=pHelmIvP").valid());
  EXPECT_FALSE(string2RealmSummary("channels=pHelmIvP").valid());
  EXPECT_EQ(RealmSummary().get_spec(), "realm_summary_err");
}

// Covers watch cast behavior: serializes string and double watchcast payloads.
TEST(WatchCastTest, SerializesStringAndDoubleWatchcastPayloads)
{
  WatchCast string_cast = makeWatchCast("abe", "DEPLOY", "true");
  EXPECT_TRUE(string_cast.valid());
  EXPECT_FALSE(string_cast.isDouble());
  EXPECT_EQ(string_cast.get_spec(),
            "node=abe!X!var=DEPLOY!X!src=pMarineViewer!X!comm=shoreside!X!"
            "loc_time=12.34!X!utc_time=100.5!X!sval=true");

  WatchCast parsed = string2WatchCast(string_cast.get_spec());
  EXPECT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.getNode(), "abe");
  EXPECT_EQ(parsed.getVarName(), "DEPLOY");
  EXPECT_EQ(parsed.getSVal(), "true");
  EXPECT_DOUBLE_EQ(parsed.getLocTime(), 12.34);

  WatchCast double_cast = makeWatchCast("abe", "NAV_X", "");
  double_cast.setDVal(42.1254);
  EXPECT_FALSE(double_cast.valid());
  double_cast = WatchCast();
  double_cast.setNode("abe");
  double_cast.setVarName("NAV_X");
  double_cast.setSource("pNodeReporter");
  double_cast.setCommunity("alpha");
  double_cast.setDVal(42.1254);
  EXPECT_TRUE(double_cast.valid());
  EXPECT_NE(double_cast.get_spec().find("dval=42.125"), std::string::npos);
}

// Covers watch cast behavior: rejects incomplete or ambiguous payloads.
TEST(WatchCastTest, RejectsIncompleteOrAmbiguousPayloads)
{
  EXPECT_FALSE(string2WatchCast("node=abe!X!var=DEPLOY!X!sval=true").valid());
  EXPECT_FALSE(string2WatchCast(
                   "node=abe!X!var=DEPLOY!X!src=pHelmIvP!X!comm=alpha!X!"
                   "sval=true!X!dval=1")
                   .valid());
  EXPECT_FALSE(string2WatchCast(
                   "node=abe!X!var=DEPLOY!X!src=pHelmIvP!X!comm=alpha!X!"
                   "sval=true!X!unexpected=x")
                   .valid());
}

// Covers realm cast set behavior: uses known proc ids and onstart proc selection.
TEST(RealmCastSetTest, UsesKnownProcIdsAndOnstartProcSelection)
{
  RealmSummary summary =
      string2RealmSummary("node=abe#channels=pHelmIvP,pHostInfo,pCustom");
  RealmCastSet set;
  EXPECT_TRUE(set.addRealmSummary(summary, "pHostInfo"));

  EXPECT_EQ(set.getTotalProcCount(), 3u);
  EXPECT_EQ(set.getCurrentProc(), "pHostInfo");
  EXPECT_EQ(set.getProcNameFromID("0"), "pHelmIvP");
  EXPECT_EQ(set.getProcNameFromID("3"), "pHostInfo");
  EXPECT_TRUE(set.hasProc("pCustom"));
  EXPECT_TRUE(set.setCurrentProc("pCustom"));
  EXPECT_FALSE(set.setCurrentProc("missing"));
}

// Covers realm cast set behavior: counts latest realmcasts per process.
TEST(RealmCastSetTest, CountsLatestRealmcastsPerProcess)
{
  RealmCastSet set;
  EXPECT_TRUE(set.addRealmCast(makeRealmCast("abe", "pHelmIvP", "first")));
  EXPECT_FALSE(set.addRealmCast(makeRealmCast("abe", "pHelmIvP", "second")));
  EXPECT_TRUE(set.addRealmCast(makeRealmCast("abe", "pNodeReporter", "nav")));

  EXPECT_EQ(set.getTotalRealmCastCount(), 3u);
  EXPECT_EQ(set.getRealmCastCount("pHelmIvP"), 2u);
  EXPECT_EQ(set.getRealmCast("pHelmIvP").getCount(), 2u);
  EXPECT_NE(set.report().find("proc=pHelmIvP[2]"), std::string::npos);
}

// Covers realm cast tree behavior: tracks nodes procs ids and selection.
TEST(RealmCastTreeTest, TracksNodesProcsIdsAndSelection)
{
  RealmCastTree tree;
  RealmSummary abe =
      string2RealmSummary("node=abe#channels=pHelmIvP,pNodeReporter");
  RealmSummary shore = string2RealmSummary("node=shoreside#channels=pHostInfo");
  EXPECT_TRUE(tree.addRealmSummary(abe, "abe", "pNodeReporter"));
  EXPECT_TRUE(tree.addRealmSummary(shore, "", ""));

  EXPECT_EQ(tree.getTreeNodeCount(), 2u);
  EXPECT_EQ(tree.getTreeProcCount(), 3u);
  EXPECT_EQ(tree.getCurrentNode(), "abe");
  EXPECT_EQ(tree.getCurrentProc(), "pNodeReporter");
  EXPECT_EQ(tree.getNodeNameFromID("0"), "shoreside");
  EXPECT_TRUE(tree.hasNodeProc("abe", "pHelmIvP"));

  EXPECT_TRUE(tree.addRealmCast(makeRealmCast("abe", "pHelmIvP", "helm")));
  EXPECT_EQ(tree.getTreeRealmCastCount(), 1u);
  EXPECT_EQ(tree.getProcRealmCastCount("abe", "pHelmIvP"), 1u);
  EXPECT_EQ(tree.getRealmCast("abe", "pHelmIvP").getProcName(), "pHelmIvP");

  EXPECT_TRUE(tree.removeNode("abe"));
  EXPECT_FALSE(tree.hasNode("abe"));
  EXPECT_EQ(tree.getCurrentNode(), "shoreside");
  EXPECT_FALSE(tree.removeNode("abe"));
}

// Covers realm repo behavior: adds realm summaries and reports node proc changes.
TEST(RealmRepoTest, AddsRealmSummariesAndReportsNodeProcChanges)
{
  RealmRepo repo;
  EXPECT_EQ(repo.getRefreshMode(), "events");
  EXPECT_TRUE(repo.setRefreshMode("streaming"));
  EXPECT_TRUE(repo.setRefreshMode("PAUSED"));
  EXPECT_FALSE(repo.setRefreshMode("fast"));
  EXPECT_EQ(repo.getRefreshMode(), "paused");

  bool changed = false;
  repo.setOnStartPreferences("abe:pNodeReporter");
  EXPECT_TRUE(repo.addRealmSummary(
      string2RealmSummary("node=abe#channels=pHelmIvP,pNodeReporter"),
      changed));
  EXPECT_TRUE(changed);
  EXPECT_EQ(repo.getCurrentNode(), "abe");
  EXPECT_EQ(repo.getCurrentProc(), "pNodeReporter");
  EXPECT_FALSE(repo.getNodeProcChanged());
  EXPECT_FALSE(repo.getNodeProcChanged());

  EXPECT_TRUE(repo.addRealmCast(makeRealmCast("abe", "pHelmIvP", "helm")));
  EXPECT_EQ(repo.getRealmCastCount(), 1u);
  EXPECT_EQ(repo.getRealmCastCount("abe"), 1u);
  EXPECT_EQ(repo.getRealmCastCount("abe", "pHelmIvP"), 1u);
}

// Covers realm repo behavior: routes watchcasts into configured clusters.
TEST(RealmRepoTest, RoutesWatchcastsIntoConfiguredClusters)
{
  RealmRepo repo;
  EXPECT_TRUE(repo.addWatchCluster("key=mission,vars=DEPLOY:RETURN"));
  EXPECT_FALSE(repo.addWatchCluster("key=mission,vars=DEPLOY"));
  EXPECT_EQ(repo.getClusterKeys(), (std::vector<std::string>{"mission"}));
  EXPECT_EQ(repo.getClusterVars("mission"),
            (std::vector<std::string>{"DEPLOY", "RETURN"}));

  EXPECT_TRUE(repo.addWatchCast(makeWatchCast("abe", "DEPLOY", "true").get_spec()));
  EXPECT_FALSE(repo.addWatchCast(makeWatchCast("abe", "NAV_X", "12").get_spec()));
  EXPECT_EQ(repo.getWatchCastCount(), 2u);
  EXPECT_EQ(repo.getWatchCastCount("mission"), 1u);
  EXPECT_EQ(repo.getWatchCount("mission", "DEPLOY"), 1u);

  EXPECT_TRUE(repo.setOnStartPreferences("mission:DEPLOY"));
  EXPECT_TRUE(repo.checkStartCluster());
  EXPECT_EQ(repo.getCurrClusterKey(), "mission");
  EXPECT_EQ(repo.getCurrClusterVar(), "DEPLOY");
  EXPECT_EQ(repo.getCurrClusterVars(), "DEPLOY:RETURN");

  InfoCastSettings settings;
  std::vector<std::string> report = repo.getClusterReport(settings);
  EXPECT_TRUE(vectorContainsSubstring(report, "mission"));
  EXPECT_TRUE(vectorContainsSubstring(report, "DEPLOY"));
  EXPECT_TRUE(vectorContainsSubstring(report, "true"));
}
