#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "IterBlockHelm.h"

namespace {

class TestableIterBlockHelm : public IterBlockHelm {
 public:
  const std::vector<BehaviorRecord>& active() const { return m_active_bhv; }
  const std::vector<BehaviorRecord>& running() const { return m_running_bhv; }
  const std::vector<BehaviorRecord>& idle() const { return m_idle_bhv; }
  const std::vector<BehaviorRecord>& completed() const
  {
    return m_completed_bhv;
  }
};

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers IterBlockHelm's deterministic parsed
// helm-summary state, decision-variable change tracking, mode-string parsing,
// and initialize-copy behavior. It does not run pHelmIvP or uHelmScope.

TEST(IterBlockHelmTest, DefaultsAreZeroAndNotHalted)
{
  IterBlockHelm block;

  EXPECT_EQ(block.getIteration(), 0u);
  EXPECT_EQ(block.getCountIPF(), 0u);
  EXPECT_EQ(block.getWarnings(), 0u);
  EXPECT_DOUBLE_EQ(block.getUTCTime(), 0);
  EXPECT_DOUBLE_EQ(block.getSolveTime(), 0);
  EXPECT_DOUBLE_EQ(block.getCreateTime(), 0);
  EXPECT_DOUBLE_EQ(block.getLoopTime(), 0);
  EXPECT_FALSE(block.getHalted());
  EXPECT_EQ(block.getDecVarCnt(), 0u);
}

TEST(IterBlockHelmTest, ScalarSettersRoundTrip)
{
  IterBlockHelm block;
  block.setIteration(12);
  block.setCountIPF(3);
  block.setWarnings(2);
  block.setUTCTime(44.5);
  block.setSolveTime(0.2);
  block.setCreateTime(0.3);
  block.setLoopTime(0.4);
  block.setHalted(true);

  EXPECT_EQ(block.getIteration(), 12u);
  EXPECT_EQ(block.getCountIPF(), 3u);
  EXPECT_EQ(block.getWarnings(), 2u);
  EXPECT_DOUBLE_EQ(block.getUTCTime(), 44.5);
  EXPECT_DOUBLE_EQ(block.getSolveTime(), 0.2);
  EXPECT_DOUBLE_EQ(block.getCreateTime(), 0.3);
  EXPECT_DOUBLE_EQ(block.getLoopTime(), 0.4);
  EXPECT_TRUE(block.getHalted());
}

TEST(IterBlockHelmTest, ParsesActiveBehaviorsWithMetrics)
{
  TestableIterBlockHelm block;

  block.setActiveBHVs(
      " waypoint $ 10 $ 100 $ 4 $ 0.03 $ 2/3 $ 7 $ BHV_Waypoint : loiter$0$n/a");

  ASSERT_EQ(block.active().size(), 2u);
  EXPECT_EQ(block.active()[0].getName(), "waypoint");
  EXPECT_EQ(block.active()[0].getPriority(), "100");
  EXPECT_EQ(block.active()[0].getPieces(), "4");
  EXPECT_EQ(block.active()[0].getTimeCPU(), "0.03");
  EXPECT_EQ(block.active()[0].getUpdates(), "2/3");
  EXPECT_EQ(block.active()[0].getIPFCount(), "7");
  EXPECT_EQ(block.active()[0].getOriginal(), "BHV_Waypoint");
  EXPECT_EQ(block.active()[1].getName(), "loiter");
  EXPECT_EQ(block.active()[1].getUpdates(), "");

  const std::vector<std::string> summaries = block.getActiveBHV(12);
  ASSERT_EQ(summaries.size(), 2u);
  EXPECT_TRUE(Contains(summaries[0], "waypoint [2.00]"));
  EXPECT_TRUE(Contains(summaries[0], "(pwt=100)"));
  EXPECT_TRUE(Contains(summaries[0], "(pcs=4)"));
  EXPECT_TRUE(Contains(summaries[0], "(cpu=0.03)"));
  EXPECT_TRUE(Contains(summaries[0], "(upd=2/3)"));
  EXPECT_TRUE(Contains(summaries[1], "loiter [always]"));
}

TEST(IterBlockHelmTest, SetBehaviorListsClearPreviousValues)
{
  TestableIterBlockHelm block;
  block.setRunningBHVs("one$1");
  ASSERT_EQ(block.running().size(), 1u);

  block.setRunningBHVs("two$2");

  ASSERT_EQ(block.running().size(), 1u);
  EXPECT_EQ(block.running()[0].getName(), "two");
}

TEST(IterBlockHelmTest, ParsesRunningIdleAndCompletedBehaviorLists)
{
  TestableIterBlockHelm block;
  block.setRunningBHVs("run$5$1/1:run_no_updates$6$n/a");
  block.setIdleBHVs("idle$7$2/3:waiting$0$n/a");
  block.setCompletedBHVs("done$8$3/4");

  ASSERT_EQ(block.running().size(), 2u);
  EXPECT_EQ(block.running()[0].getUpdates(), "1/1");
  EXPECT_EQ(block.running()[1].getUpdates(), "");
  EXPECT_EQ(block.getCountIdle(), 2u);

  const std::vector<std::string> idle_concise = block.getIdleBHV(10, true);
  ASSERT_EQ(idle_concise.size(), 2u);
  EXPECT_EQ(idle_concise[0], "idle");
  EXPECT_EQ(idle_concise[1], "waiting");

  const std::vector<std::string> completed_concise =
      block.getCompletedBHV(10, true);
  ASSERT_EQ(completed_concise.size(), 1u);
  EXPECT_EQ(completed_concise[0], "done");
}

TEST(IterBlockHelmTest, EmptyBehaviorStringClearsList)
{
  TestableIterBlockHelm block;
  block.setIdleBHVs("idle$1");
  ASSERT_EQ(block.getCountIdle(), 1u);

  block.setIdleBHVs("");

  EXPECT_EQ(block.getCountIdle(), 0u);
}

TEST(IterBlockHelmTest, DecisionVariablesTrackNewAndChangedValues)
{
  IterBlockHelm block;

  block.addDecVarVal("course", "90");
  block.addDecVarVal("speed", "2");
  block.addDecVarVal("course", "90");

  ASSERT_EQ(block.getDecVarCnt(), 2u);
  EXPECT_EQ(block.getDecVar(0), "course");
  EXPECT_EQ(block.getDecVal(0), "90");
  EXPECT_TRUE(block.getDecChg(0));

  block.addDecVarVal("course", "100");
  EXPECT_EQ(block.getDecVal(0), "100");
  EXPECT_TRUE(block.getDecChg(0));
  EXPECT_EQ(block.getDecVar(99), "");
  EXPECT_FALSE(block.getDecChg(99));
}

TEST(IterBlockHelmTest, InitializeCopiesStateButClearsDecisionChangeFlags)
{
  IterBlockHelm source;
  source.addDecVarVal("course", "90");
  source.setModeString("MODE@ACTIVE#SUBMODE@LOITER");
  source.setSolveTime(1.2);
  source.setCreateTime(2.3);
  source.setLoopTime(3.4);
  source.setHalted(true);
  source.setCountIPF(5);
  source.setWarnings(6);

  IterBlockHelm copy;
  copy.initialize(source);

  ASSERT_EQ(copy.getDecVarCnt(), 1u);
  EXPECT_EQ(copy.getDecVar(0), "course");
  EXPECT_EQ(copy.getDecVal(0), "90");
  EXPECT_FALSE(copy.getDecChg(0));
  EXPECT_EQ(copy.getModeStr(), "MODE=ACTIVE, SUBMODE=LOITER");
  EXPECT_DOUBLE_EQ(copy.getSolveTime(), 1.2);
  EXPECT_DOUBLE_EQ(copy.getCreateTime(), 2.3);
  EXPECT_DOUBLE_EQ(copy.getLoopTime(), 3.4);
  EXPECT_TRUE(copy.getHalted());
  EXPECT_EQ(copy.getCountIPF(), 5u);
  EXPECT_EQ(copy.getWarnings(), 6u);
}

TEST(IterBlockHelmTest, InitializeDoesNotCopyIterationOrUtcTime)
{
  IterBlockHelm source;
  source.setIteration(9);
  source.setUTCTime(123.4);

  IterBlockHelm copy;
  copy.initialize(source);

  EXPECT_EQ(copy.getIteration(), 0u);
  EXPECT_DOUBLE_EQ(copy.getUTCTime(), 0);
}

TEST(IterBlockHelmTest, ModeStringFormatsSingleAndMultipleModes)
{
  IterBlockHelm block;

  block.setModeString("MODE@ACTIVE");
  EXPECT_EQ(block.getModeStr(), "ACTIVE");

  block.setModeString("MODE@ACTIVE#SUB@LOITER");
  EXPECT_EQ(block.getModeStr(), "MODE=ACTIVE, SUB=LOITER");

  block.setModeString("");
  EXPECT_EQ(block.getModeStr(), "");
}
