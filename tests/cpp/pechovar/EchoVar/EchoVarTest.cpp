#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "EchoVar.h"

namespace {

class TestableEchoVar : public EchoVar {
 public:
  using EchoVar::addMapping;
  using EchoVar::expand;
  using EchoVar::handleFlipEntry;
  using EchoVar::holdMessage;
  using EchoVar::noCycles;

  void setConditionsMet(bool value) { m_conditions_met = value; }
  void setEchoLatestOnly(bool value) { m_echo_latest_only = value; }
  const std::vector<std::string>& sources() const { return m_var_source; }
  const std::vector<std::string>& targets() const { return m_var_target; }
  const std::vector<bool>& boolSwitches() const { return m_boolswitch; }
  const std::vector<std::string>& uniqueSources() const
  {
    return m_unique_sources;
  }
  std::vector<EFlipper>& flippers() { return m_eflippers; }
  const std::vector<unsigned int>& flipCounts() const { return m_eflip_count; }
  unsigned int hits(const std::string& key) const
  {
    auto it = m_map_hits.find(key);
    return it == m_map_hits.end() ? 0u : it->second;
  }
  std::vector<std::string> heldKeys() const
  {
    std::vector<std::string> keys;
    for(const CMOOSMsg& msg : m_held_messages)
      keys.push_back(msg.GetKey());
    return keys;
  }
};

CMOOSMsg smail(const std::string& key, const std::string& value)
{
  return CMOOSMsg(MOOS_NOTIFY, key, value, 0);
}

}  // namespace

// Source audit note: these tests cover EchoVar's deterministic mapping graph,
// cycle detection, expansion, held-message pruning, and FLIP entry parsing
// against pEchoVar. The MOOS mail release paths call Notify() and remain
// harness/runtime territory.

TEST(EchoVarMappingTest, RejectsEmptyMappingsWithoutMutation)
{
  TestableEchoVar app;

  EXPECT_FALSE(app.addMapping("", "DEST"));
  EXPECT_FALSE(app.addMapping("SRC", ""));

  EXPECT_TRUE(app.sources().empty());
  EXPECT_TRUE(app.targets().empty());
  EXPECT_TRUE(app.uniqueSources().empty());
}

TEST(EchoVarMappingTest, AddsUniqueSourcesAndIgnoresDuplicatePairs)
{
  TestableEchoVar app;

  EXPECT_TRUE(app.addMapping("SRC", "DEST", true));
  EXPECT_TRUE(app.addMapping("SRC", "DEST", false));
  EXPECT_TRUE(app.addMapping("SRC", "ALT", false));

  ASSERT_EQ(app.sources().size(), 2u);
  EXPECT_EQ(app.sources()[0], "SRC");
  EXPECT_EQ(app.targets()[0], "DEST");
  EXPECT_TRUE(app.boolSwitches()[0]);
  EXPECT_EQ(app.sources()[1], "SRC");
  EXPECT_EQ(app.targets()[1], "ALT");
  ASSERT_EQ(app.uniqueSources().size(), 1u);
  EXPECT_EQ(app.uniqueSources()[0], "SRC");
}

TEST(EchoVarMappingTest, DetectsAcyclicAndDirectCycleGraphs)
{
  TestableEchoVar acyclic;
  ASSERT_TRUE(acyclic.addMapping("A", "B"));
  ASSERT_TRUE(acyclic.addMapping("A", "C"));
  ASSERT_TRUE(acyclic.addMapping("B", "D"));
  EXPECT_TRUE(acyclic.noCycles());

  TestableEchoVar direct_cycle;
  ASSERT_TRUE(direct_cycle.addMapping("A", "A"));
  EXPECT_FALSE(direct_cycle.noCycles());
}

TEST(EchoVarMappingTest, DetectsIndirectCycles)
{
  TestableEchoVar app;
  ASSERT_TRUE(app.addMapping("A", "B"));
  ASSERT_TRUE(app.addMapping("B", "C"));
  ASSERT_TRUE(app.addMapping("C", "A"));

  EXPECT_FALSE(app.noCycles());
}

TEST(EchoVarMappingTest, ExpandsReachableTargetsRecursivelyWithoutDuplicates)
{
  TestableEchoVar app;
  ASSERT_TRUE(app.addMapping("A", "B"));
  ASSERT_TRUE(app.addMapping("A", "C"));
  ASSERT_TRUE(app.addMapping("B", "D"));
  ASSERT_TRUE(app.addMapping("C", "D"));
  ASSERT_TRUE(app.addMapping("D", "E"));

  const std::vector<std::string> expanded = app.expand({"B", "C"});

  ASSERT_EQ(expanded.size(), 4u);
  EXPECT_EQ(expanded[0], "B");
  EXPECT_EQ(expanded[1], "C");
  EXPECT_EQ(expanded[2], "D");
  EXPECT_EQ(expanded[3], "E");
}

TEST(EchoVarHoldMessageTest, HoldsDuplicateKeysWhenConditionsMet)
{
  TestableEchoVar app;
  app.setConditionsMet(true);

  app.holdMessage(smail("SRC", "one"));
  app.holdMessage(smail("SRC", "two"));
  app.holdMessage(smail("ALT", "three"));

  EXPECT_EQ(app.hits("SRC"), 2u);
  EXPECT_EQ(app.hits("ALT"), 1u);
  EXPECT_EQ(app.heldKeys(), (std::vector<std::string>{"SRC", "SRC", "ALT"}));
}

TEST(EchoVarHoldMessageTest, PrunesDuplicateKeysWhenPausedOrLatestOnly)
{
  TestableEchoVar paused;
  paused.setConditionsMet(false);
  paused.holdMessage(smail("SRC", "one"));
  paused.holdMessage(smail("ALT", "two"));
  paused.holdMessage(smail("SRC", "three"));
  EXPECT_EQ(paused.heldKeys(), (std::vector<std::string>{"ALT", "SRC"}));

  TestableEchoVar latest_only;
  latest_only.setEchoLatestOnly(true);
  latest_only.holdMessage(smail("SRC", "one"));
  latest_only.holdMessage(smail("ALT", "two"));
  latest_only.holdMessage(smail("SRC", "three"));
  EXPECT_EQ(latest_only.heldKeys(), (std::vector<std::string>{"ALT", "SRC"}));
}

TEST(EchoVarFlipEntryTest, RejectsMalformedFlipKeysWithoutCreatingFlippers)
{
  TestableEchoVar app;

  EXPECT_FALSE(app.handleFlipEntry("flip", "source_variable=SRC"));
  EXPECT_FALSE(app.handleFlipEntry("flop:one", "source_variable=SRC"));
  EXPECT_FALSE(app.handleFlipEntry("flip:one:extra", "source_variable=SRC"));

  EXPECT_TRUE(app.flippers().empty());
  EXPECT_TRUE(app.flipCounts().empty());
}

TEST(EchoVarFlipEntryTest, ParsesNamedFlipEntriesIntoOneReusableFlipper)
{
  TestableEchoVar app;

  EXPECT_TRUE(app.handleFlipEntry("flip:click", "source_variable = MVIEWER_LCLICK"));
  EXPECT_TRUE(app.handleFlipEntry("flip:click", "dest_variable = UP_LOITER"));
  EXPECT_TRUE(app.handleFlipEntry("flip:click", "component = x -> xcenter"));
  EXPECT_TRUE(app.handleFlipEntry("flip:click", "y -> ycenter"));
  EXPECT_TRUE(app.handleFlipEntry("flip:click", "filter = mode == survey"));
  EXPECT_TRUE(app.handleFlipEntry("flip:click", "vehicle == abe"));

  ASSERT_EQ(app.flippers().size(), 1u);
  EXPECT_EQ(app.flipCounts().size(), 1u);
  EXPECT_EQ(app.flippers()[0].getKey(), "click");
  EXPECT_EQ(app.flippers()[0].getSourceVar(), "MVIEWER_LCLICK");
  EXPECT_EQ(app.flippers()[0].getDestVar(), "UP_LOITER");
  EXPECT_EQ(app.flippers()[0].getComponents(), "x:xcenter,y:ycenter");
  EXPECT_EQ(app.flippers()[0].getFilters(), "mode:survey,vehicle:abe");
  EXPECT_TRUE(app.flippers()[0].valid());
}

TEST(EchoVarFlipEntryTest, CreatesSeparateFlippersForSeparateKeys)
{
  TestableEchoVar app;

  EXPECT_TRUE(app.handleFlipEntry("flip:one", "source_variable=SRC1"));
  EXPECT_TRUE(app.handleFlipEntry("flip:two", "source_variable=SRC2"));

  ASSERT_EQ(app.flippers().size(), 2u);
  EXPECT_EQ(app.flippers()[0].getKey(), "one");
  EXPECT_EQ(app.flippers()[1].getKey(), "two");
  EXPECT_EQ(app.flipCounts().size(), 2u);
}

TEST(EchoVarFlipEntryTest, ValidKeyWithUnhandledLineCreatesPlaceholderBeforeFailing)
{
  TestableEchoVar app;

  EXPECT_FALSE(app.handleFlipEntry("flip:pending", "not_a_flip_setting"));

  ASSERT_EQ(app.flippers().size(), 1u);
  EXPECT_EQ(app.flippers()[0].getKey(), "pending");
  EXPECT_FALSE(app.flippers()[0].valid());
}

TEST(EchoVarFlipEntryTest, PropagatesSourceDestinationAliasRejection)
{
  TestableEchoVar app;

  EXPECT_TRUE(app.handleFlipEntry("flip:bad", "source_variable=SRC"));
  EXPECT_FALSE(app.handleFlipEntry("flip:bad", "dest_variable=SRC"));

  ASSERT_EQ(app.flippers().size(), 1u);
  EXPECT_EQ(app.flippers()[0].getSourceVar(), "SRC");
  EXPECT_EQ(app.flippers()[0].getDestVar(), "");
}
