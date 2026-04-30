#include <gtest/gtest.h>

#include <vector>

#include "FixedTurn.h"
#include "FixedTurnSet.h"
#include "VarDataPair.h"

// Covers fixed turn behavior: defaults represent caller provided turn settings.
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

// Covers fixed turn behavior: parses fixed turn behavior parameters and flags.
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

// Covers fixed turn behavior: supports auto syntax and pins accepted random direction behavior.
TEST(FixedTurnTest, SupportsAutoSyntaxAndPinsAcceptedRandomDirectionBehavior)
{
  FixedTurn turn;
  ASSERT_TRUE(turn.setTurnParams("key=turn_a,spd=3,turn=rand,timeout=5"));
  EXPECT_DOUBLE_EQ(turn.getTurnSpd(), 3);
  EXPECT_EQ(turn.getTurnDir(), "auto");
  EXPECT_DOUBLE_EQ(turn.getTurnTimeOut(), 5);

  EXPECT_TRUE(turn.setTurnParams("spd=auto,turn=auto"));
  EXPECT_DOUBLE_EQ(turn.getTurnSpd(), -1);
  EXPECT_EQ(turn.getTurnDir(), "auto");

  EXPECT_FALSE(turn.setTurnParams("flag=NOT_A_PAIR"));
  EXPECT_TRUE(turn.getFlags().empty());
  EXPECT_EQ(turn.getTurnKey(), "turn_a");
}

// Covers fixed turn behavior: rejects unknown parameters invalid numbers and key changes atomically.
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

// Covers fixed turn behavior: adds only valid var data pairs programmatically.
TEST(FixedTurnTest, AddsOnlyValidVarDataPairsProgrammatically)
{
  FixedTurn turn;
  VarDataPair invalid;
  EXPECT_FALSE(turn.addVarDataPair(invalid));

  VarDataPair flag("TURN_DONE", "true", "auto");
  EXPECT_TRUE(turn.addVarDataPair(flag));
  ASSERT_EQ(turn.getFlags().size(), 1u);
  EXPECT_EQ(turn.getFlags()[0].get_var(), "TURN_DONE");
}

// Covers fixed turn set behavior: adds updates and advances keyed turn schedule.
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

// Covers fixed turn set behavior: clear prefix resets existing schedule before adding new turn.
TEST(FixedTurnSetTest, ClearPrefixResetsExistingScheduleBeforeAddingNewTurn)
{
  FixedTurnSet turns;
  ASSERT_TRUE(turns.setTurnParams("key=first,spd=1"));
  ASSERT_TRUE(turns.setTurnParams("key=second,spd=2"));

  ASSERT_TRUE(turns.setTurnParams("clear,key=third,spd=3,turn=port"));
  EXPECT_EQ(turns.size(), 1u);
  EXPECT_FALSE(turns.completed());
  EXPECT_EQ(turns.getFixedTurn().getTurnKey(), "third");
  EXPECT_DOUBLE_EQ(turns.getFixedTurn().getTurnSpd(), 3);
  EXPECT_EQ(turns.getFixedTurn().getTurnDir(), "port");
}

// Covers fixed turn set behavior: repeats schedule when configured and can clear.
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
