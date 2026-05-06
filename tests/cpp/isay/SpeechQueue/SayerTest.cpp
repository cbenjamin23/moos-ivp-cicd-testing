#include <gtest/gtest.h>

#define private protected
#include "Sayer.h"
#undef private

namespace {

class TestableSayer : public Sayer {
 public:
  using Sayer::addUtterance;
  using Sayer::handleSetVolume;

  double volume() const { return m_volume; }
  unsigned int queueSize() const { return m_utter_pqueue.size(); }
  Utterance popUtterance() { return m_utter_pqueue.pop(); }
};

}  // namespace

// Source audit note: this suite covers Sayer helper behavior that is safe for
// CTest: volume parsing/clipping and utterance queue admission. It intentionally
// does not call sayUtterance(), which builds and executes platform audio
// commands with system().

TEST(SayerTest, StartsWithDocumentedDefaults)
{
  TestableSayer sayer;

  EXPECT_DOUBLE_EQ(sayer.volume(), 1);
  EXPECT_EQ(sayer.queueSize(), 0u);
}

TEST(SayerTest, HandlesNamedVolumeLevels)
{
  TestableSayer sayer;

  EXPECT_TRUE(sayer.handleSetVolume("mute"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 0);
  EXPECT_TRUE(sayer.handleSetVolume("vsoft"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 0.1);
  EXPECT_TRUE(sayer.handleSetVolume("soft"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 0.4);
  EXPECT_TRUE(sayer.handleSetVolume("normal"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 1);
  EXPECT_TRUE(sayer.handleSetVolume("loud"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 1.5);
  EXPECT_TRUE(sayer.handleSetVolume("vloud"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 2);
}

TEST(SayerTest, HandlesRelativeVolumeChangesWithClipping)
{
  TestableSayer sayer;

  EXPECT_TRUE(sayer.handleSetVolume("mute"));
  EXPECT_TRUE(sayer.handleSetVolume("softer"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 0);

  EXPECT_TRUE(sayer.handleSetVolume("vloud"));
  EXPECT_TRUE(sayer.handleSetVolume("louder"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 2);

  EXPECT_TRUE(sayer.handleSetVolume("normal"));
  EXPECT_TRUE(sayer.handleSetVolume("louder"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 1.1);
  EXPECT_TRUE(sayer.handleSetVolume("softer"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 1.0);
}

TEST(SayerTest, HandlesNumericVolumeWithClipping)
{
  TestableSayer sayer;

  EXPECT_TRUE(sayer.handleSetVolume("1.25"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 1.25);
  EXPECT_TRUE(sayer.handleSetVolume("-3"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 0);
  EXPECT_TRUE(sayer.handleSetVolume("99"));
  EXPECT_DOUBLE_EQ(sayer.volume(), 2);
}

TEST(SayerTest, RejectsUnknownAndCaseChangedVolumeWithoutMutation)
{
  TestableSayer sayer;

  EXPECT_TRUE(sayer.handleSetVolume("normal"));
  EXPECT_FALSE(sayer.handleSetVolume("LOUD"));
  EXPECT_FALSE(sayer.handleSetVolume(""));
  EXPECT_FALSE(sayer.handleSetVolume("quiet"));

  EXPECT_DOUBLE_EQ(sayer.volume(), 1);
}

TEST(SayerTest, AddsValidUtteranceWithProvidedSource)
{
  TestableSayer sayer;

  EXPECT_TRUE(sayer.addUtterance("say={hello}", "alpha:pHelmIvP"));

  ASSERT_EQ(sayer.queueSize(), 1u);
  const Utterance utter = sayer.popUtterance();
  EXPECT_EQ(utter.getText(), "hello");
  EXPECT_EQ(utter.getSource(), "alpha:pHelmIvP");
}

TEST(SayerTest, UtteranceSourceFieldOverridesProvidedSource)
{
  TestableSayer sayer;

  EXPECT_TRUE(
      sayer.addUtterance("say={hello}, source=explicit", "alpha:pHelmIvP"));

  ASSERT_EQ(sayer.queueSize(), 1u);
  EXPECT_EQ(sayer.popUtterance().getSource(), "explicit");
}

TEST(SayerTest, InvalidStructuredUtteranceIsRejectedAndNotQueued)
{
  TestableSayer sayer;

  EXPECT_FALSE(sayer.addUtterance("say={hello}, priority=high", "alpha:pHelmIvP"));

  EXPECT_EQ(sayer.queueSize(), 0u);
}

TEST(SayerTest, TopPriorityUtteranceIsQueuedAheadOfExistingEntries)
{
  TestableSayer sayer;

  ASSERT_TRUE(sayer.addUtterance("say={normal}, priority=5", "src"));
  ASSERT_TRUE(sayer.addUtterance("say={top}, priority=top", "src"));

  ASSERT_EQ(sayer.queueSize(), 2u);
  EXPECT_EQ(sayer.popUtterance().getText(), "top");
  EXPECT_EQ(sayer.popUtterance().getText(), "normal");
}

TEST(SayerTest, QueueLimitDropsAdditionalValidUtterancesAsHandled)
{
  TestableSayer sayer;

  for(int i = 0; i < 10; ++i)
    EXPECT_TRUE(sayer.addUtterance("say={entry}", "src"));

  EXPECT_EQ(sayer.queueSize(), 10u);
  EXPECT_TRUE(sayer.addUtterance("say={overflow}", "src"));
  EXPECT_EQ(sayer.queueSize(), 10u);
}

TEST(SayerTest, QueueLimitShortCircuitsParsingAndTreatsInvalidOverflowAsHandled)
{
  TestableSayer sayer;

  for(int i = 0; i < 10; ++i)
    ASSERT_TRUE(sayer.addUtterance("say={entry}", "src"));

  EXPECT_TRUE(sayer.addUtterance("say={bad}, priority=high", "src"));
  EXPECT_EQ(sayer.queueSize(), 10u);
}
