#include <gtest/gtest.h>

#include <string>

#include "Utterance.h"

namespace {

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Source audit note: this suite covers Utterance parsing, invalid-field
// behavior, voice handling, priority semantics, spec generation, type
// classification, timestamps, and comparison operators. Sayer's system audio
// execution is intentionally excluded from CTest coverage.

TEST(UtteranceTest, DefaultsAreEmptyAndZeroValued)
{
  Utterance utter;

  EXPECT_TRUE(utter.isEmpty());
  EXPECT_EQ(utter.getType(), "empty");
  EXPECT_EQ(utter.getSpec(), "");
  EXPECT_DOUBLE_EQ(utter.getRate(), 0);
  EXPECT_DOUBLE_EQ(utter.getPriority(), 0);
  EXPECT_DOUBLE_EQ(utter.getTimeReceived(), 0);
  EXPECT_DOUBLE_EQ(utter.getTimePosted(), 0);
  EXPECT_FALSE(utter.isTopPriority());
}

TEST(UtteranceTest, PlainStringBecomesTextWhenNoStructuredMarkersAppear)
{
  Utterance utter("hello, world");

  EXPECT_EQ(utter.getText(), "hello, world");
  EXPECT_EQ(utter.getType(), "text");
  EXPECT_EQ(utter.getSpec(), "say={hello, world}");
}

TEST(UtteranceTest, PlainStringPathAlsoAppliesToUnbracedSayLikeText)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString("say=hello"));

  EXPECT_EQ(utter.getText(), "say=hello");
  EXPECT_EQ(utter.getType(), "text");
}

TEST(UtteranceTest, ParsesBracedSayWithCommaAndMetadata)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString(
      "say={hello, shoreside}, rate=180.5, priority=3, source=pHelmIvP"));

  EXPECT_EQ(utter.getText(), "hello, shoreside");
  EXPECT_DOUBLE_EQ(utter.getRate(), 180.5);
  EXPECT_DOUBLE_EQ(utter.getPriority(), 3);
  EXPECT_EQ(utter.getSource(), "pHelmIvP");
  EXPECT_EQ(utter.getType(), "text");
}

TEST(UtteranceTest, ParsesQuotedSayWithoutComma)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString("say=\"quoted text\", voice=ALEX"));

  EXPECT_EQ(utter.getText(), "quoted text");
  EXPECT_EQ(utter.getVoice(), "alex");
}

TEST(UtteranceTest, QuotedSayWithCommaIsRejectedAfterPartialTextMutation)
{
  Utterance utter;

  EXPECT_FALSE(utter.initFromString("say=\"hello, world\""));

  EXPECT_EQ(utter.getText(), "\"hello");
}

TEST(UtteranceTest, ParsesFileOnlyAndMixedUtterances)
{
  Utterance file_utter("file=alarm.wav");
  EXPECT_EQ(file_utter.getFile(), "alarm.wav");
  EXPECT_EQ(file_utter.getType(), "file");
  EXPECT_EQ(file_utter.getSpec(), "file=alarm.wav");

  Utterance mixed("say={fallback}, file=alarm.wav");
  EXPECT_EQ(mixed.getText(), "fallback");
  EXPECT_EQ(mixed.getFile(), "alarm.wav");
  EXPECT_EQ(mixed.getType(), "mixed");
}

TEST(UtteranceTest, RejectsUnknownParamAfterKeepingEarlierFields)
{
  Utterance utter;

  EXPECT_FALSE(utter.initFromString("say={hello}, unknown=value"));

  EXPECT_EQ(utter.getText(), "hello");
  EXPECT_EQ(utter.getType(), "text");
}

TEST(UtteranceTest, RejectsNonNumericRateAndPriority)
{
  Utterance rate;
  EXPECT_FALSE(rate.initFromString("say={hello}, rate=fast"));
  EXPECT_EQ(rate.getText(), "hello");
  EXPECT_DOUBLE_EQ(rate.getRate(), 0);

  Utterance priority;
  EXPECT_FALSE(priority.initFromString("say={hello}, priority=high"));
  EXPECT_EQ(priority.getText(), "hello");
  EXPECT_DOUBLE_EQ(priority.getPriority(), 0);
  EXPECT_FALSE(priority.isTopPriority());
}

TEST(UtteranceTest, NumericRateAndPriorityAcceptNegativeValues)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString("say={hello}, rate=-20, priority=-3"));

  EXPECT_DOUBLE_EQ(utter.getRate(), -20);
  EXPECT_DOUBLE_EQ(utter.getPriority(), -3);
}

TEST(UtteranceTest, TopPriorityFlagTakesSpecPrecedenceOverNumericPriority)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString("say={urgent}, priority=5, priority=top"));

  EXPECT_DOUBLE_EQ(utter.getPriority(), 5);
  EXPECT_TRUE(utter.isTopPriority());
  EXPECT_TRUE(Contains(utter.getSpec(), "priority=top"));
  EXPECT_FALSE(Contains(utter.getSpec(), "priority=5"));
}

TEST(UtteranceTest, InvalidVoiceParamIsAcceptedButIgnored)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString("say={hello}, voice=samantha"));

  EXPECT_EQ(utter.getVoice(), "");
  EXPECT_EQ(utter.getText(), "hello");
}

TEST(UtteranceTest, VoiceParamLowercasesValidVoices)
{
  Utterance utter;

  EXPECT_TRUE(utter.initFromString("say={hello}, voice=BAD NEWS"));

  EXPECT_EQ(utter.getVoice(), "bad news");
}

TEST(UtteranceTest, SetVoiceRejectsInvalidVoiceWithoutMutation)
{
  Utterance utter;

  EXPECT_FALSE(utter.setVoice("samantha"));

  EXPECT_EQ(utter.getVoice(), "");
}

TEST(UtteranceTest, SetVoiceAcceptsValidVoiceButStoresBooleanConvertedChar)
{
  Utterance utter;

  EXPECT_TRUE(utter.setVoice("alex"));

  ASSERT_EQ(utter.getVoice().size(), 1u);
  EXPECT_EQ(static_cast<unsigned char>(utter.getVoice()[0]), 1u);
}

TEST(UtteranceTest, ManualSettersAppearInSpecAndType)
{
  Utterance utter;
  utter.setText("manual");
  utter.setFile("clip.wav");
  utter.setSource("src");
  utter.setNode("alpha");
  utter.setRate(150);
  utter.setPriority(2.5);
  utter.setTimeReceived(10);
  utter.setTimePosted(12);

  const std::string spec = utter.getSpec();

  EXPECT_EQ(utter.getType(), "mixed");
  EXPECT_DOUBLE_EQ(utter.getTimeReceived(), 10);
  EXPECT_DOUBLE_EQ(utter.getTimePosted(), 12);
  EXPECT_TRUE(Contains(spec, "say={manual}"));
  EXPECT_TRUE(Contains(spec, "file=clip.wav"));
  EXPECT_TRUE(Contains(spec, "source=src"));
  EXPECT_TRUE(Contains(spec, "node=alpha"));
  EXPECT_TRUE(Contains(spec, "rate=150"));
  EXPECT_TRUE(Contains(spec, "priority=2.5"));
}

TEST(UtteranceTest, ComparisonOperatorsRankHigherPriorityFirst)
{
  Utterance high;
  high.setPriority(10);
  Utterance low;
  low.setPriority(1);

  EXPECT_TRUE(high < low);
  EXPECT_TRUE(low > high);
  EXPECT_FALSE(low < high);
  EXPECT_FALSE(high > low);
}
