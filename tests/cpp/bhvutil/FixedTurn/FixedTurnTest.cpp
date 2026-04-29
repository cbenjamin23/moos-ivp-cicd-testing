#include <gtest/gtest.h>

#include <vector>

#include "FixedTurn.h"
#include "FixedTurnSet.h"
#include "VarDataPair.h"

TEST(FixedTurnTest, DefaultsRepresentCallerProvidedTurnSettings)
{
  FixedTurn turn;

  EXPECT_DOUBLE_EQ(turn.getTurnSpd(), -1);
  EXPECT_DOUBLE_EQ(turn.getTurnModHdg(), -1);
  EXPECT_DOUBLE_EQ(turn.getTurnFixHdg(), -1);
  EXPECT_DOUBLE_EQ(turn.getTurnTimeOut(), -1);
  EXPECT_EQ(turn.getTurnDir(), "auto");
  EXPECT_EQ(turn.getTurnKey(), "");
  EXPECT_TRUE(turn.getFlags().empty());
}

TEST(FixedTurnTest, ParsesFixedTurnBehaviorParametersAndFlags)
{
  FixedTurn turn;

  ASSERT_TRUE(turn.setTurnParams(
      "spd=2.5,mhdg=45,fix=180,turn=port,key=avoid_left,timeout=12,"
      "flag=TURN_COMPLETE=true"));

  EXPECT_DOUBLE_EQ(turn.getTurnSpd(), 2.5);
  EXPECT_DOUBLE_EQ(turn.getTurnModHdg(), 45);
  EXPECT_DOUBLE_EQ(turn.getTurnFixHdg(), 180);
  EXPECT_DOUBLE_EQ(turn.getTurnTimeOut(), 12);
  EXPECT_EQ(turn.getTurnDir(), "port");
  EXPECT_EQ(turn.getTurnKey(), "avoid_left");

  std::vector<VarDataPair> flags = turn.getFlags();
  ASSERT_EQ(flags.size(), 1u);
  EXPECT_EQ(flags[0].get_var(), "TURN_COMPLETE");
  EXPECT_TRUE(flags[0].is_string());
  EXPECT_EQ(flags[0].get_sdata(), "true");
}

TEST(FixedTurnTest, RejectsUnknownParametersInvalidNumbersAndKeyChangesAtomically)
{
  FixedTurn turn;
  ASSERT_TRUE(turn.setTurnParams("key=turn_a,spd=1.5,turn=starboard"));

  EXPECT_FALSE(turn.setTurnParams("bogus=1"));
  EXPECT_FALSE(turn.setTurnParams("spd=fast"));
  EXPECT_FALSE(turn.setTurnParams("turn=up"));
  EXPECT_FALSE(turn.setTurnParams("key=turn_b,spd=2"));

  EXPECT_EQ(turn.getTurnKey(), "turn_a");
  EXPECT_DOUBLE_EQ(turn.getTurnSpd(), 1.5);
  EXPECT_EQ(turn.getTurnDir(), "star");
}

TEST(FixedTurnSetTest, AddsUpdatesAndAdvancesKeyedTurnSchedule)
{
  FixedTurnSet turns;
  EXPECT_TRUE(turns.completed());

  ASSERT_TRUE(turns.setTurnParams("key=first,spd=1,turn=port"));
  ASSERT_TRUE(turns.setTurnParams("key=second,spd=2,turn=starboard"));
  EXPECT_EQ(turns.size(), 2u);
  EXPECT_FALSE(turns.completed());
  EXPECT_EQ(turns.getFixedTurn().getTurnKey(), "first");

  ASSERT_TRUE(turns.setTurnParams("key=first,timeout=8"));
  EXPECT_EQ(turns.size(), 2u);
  EXPECT_DOUBLE_EQ(turns.getFixedTurn().getTurnTimeOut(), 8);

  turns.increment();
  EXPECT_EQ(turns.getFixedTurn().getTurnKey(), "second");
  turns.increment();
  EXPECT_TRUE(turns.completed());
  EXPECT_EQ(turns.getFixedTurn().getTurnDir(), "auto");
}

TEST(FixedTurnSetTest, RepeatsScheduleWhenConfiguredAndCanClear)
{
  FixedTurnSet turns;
  ASSERT_TRUE(turns.setTurnParams("key=first,spd=1"));
  ASSERT_TRUE(turns.setTurnParams("key=second,spd=2"));
  ASSERT_TRUE(turns.setRepeats("true"));

  turns.increment();
  EXPECT_EQ(turns.getFixedTurn().getTurnKey(), "second");
  turns.increment();
  EXPECT_FALSE(turns.completed());
  EXPECT_EQ(turns.getFixedTurn().getTurnKey(), "first");

  ASSERT_TRUE(turns.setTurnParams("clear"));
  EXPECT_EQ(turns.size(), 0u);
  EXPECT_FALSE(turns.completed());
}
