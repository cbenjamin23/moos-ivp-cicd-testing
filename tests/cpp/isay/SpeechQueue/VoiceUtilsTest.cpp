#include <gtest/gtest.h>

#include "VoiceUtils.h"

// Source audit note: VoiceUtils is a deterministic allow-list for iSay voices.
// The tests pin case folding, aliases containing spaces/underscores, locale
// voices, and common invalid spellings without invoking any audio command.

TEST(VoiceUtilsTest, AcceptsNamedVoicesCaseInsensitively)
{
  EXPECT_TRUE(isVoice("agnes"));
  EXPECT_TRUE(isVoice("ALEX"));
  EXPECT_TRUE(isVoice("Victoria"));
  EXPECT_TRUE(isVoice("zarvox"));
}

TEST(VoiceUtilsTest, AcceptsSpaceAndUnderscoreAliases)
{
  EXPECT_TRUE(isVoice("bad news"));
  EXPECT_TRUE(isVoice("bad_news"));
  EXPECT_TRUE(isVoice("pipe organ"));
  EXPECT_TRUE(isVoice("pipe_organ"));
  EXPECT_TRUE(isVoice("good_news"));
}

TEST(VoiceUtilsTest, AcceptsLocaleVoices)
{
  EXPECT_TRUE(isVoice("en-us"));
  EXPECT_TRUE(isVoice("EN-SC"));
}

TEST(VoiceUtilsTest, RejectsUnknownBlankAndUntrimmedVoices)
{
  EXPECT_FALSE(isVoice(""));
  EXPECT_FALSE(isVoice("samantha"));
  EXPECT_FALSE(isVoice(" alex "));
  EXPECT_FALSE(isVoice("bad-news"));
  EXPECT_FALSE(isVoice("en_us"));
}
