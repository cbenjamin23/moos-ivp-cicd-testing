#include <gtest/gtest.h>

#include <set>
#include <string>

#include "MBUtils.h"

// Covers mb utils token parse behavior: extracts string double and bool values from specs.
TEST(MBUtilsTokenParseTest, ExtractsStringDoubleAndBoolValuesFromSpecs)
{
  const std::string spec =
      "name=abe, speed=2.5, active=true, note=\"alpha,beta\"";

  std::string name;
  EXPECT_TRUE(tokParse(spec, "name", ',', '=', name));
  EXPECT_EQ(name, "abe");

  double speed = 0;
  EXPECT_TRUE(tokParse(spec, "speed", ',', '=', speed));
  EXPECT_DOUBLE_EQ(speed, 2.5);

  bool active = false;
  EXPECT_TRUE(tokParse(spec, "active", ',', '=', active));
  EXPECT_TRUE(active);

  std::string note;
  EXPECT_TRUE(tokParse(spec, "note", ',', '=', note));
  EXPECT_EQ(note, "\"alpha,beta\"");
}

// Covers mb utils token parse behavior: preserves raw string values but trims keys.
TEST(MBUtilsTokenParseTest, PreservesRawStringValuesButTrimsKeys)
{
  const std::string spec = " name = abe , mode = SURVEY , note = spaced value ";

  std::string name;
  EXPECT_TRUE(tokParse(spec, "name", ',', '=', name));
  EXPECT_EQ(name, " abe ");

  EXPECT_EQ(tokStringParse(spec, "mode"), "SURVEY");
  EXPECT_EQ(tokStringParse(spec, "note"), "spaced value");
}

// Covers mb utils token parse behavior: rejects malformed pairs and leaves typed values alone.
TEST(MBUtilsTokenParseTest, RejectsMalformedPairsAndLeavesTypedValuesAlone)
{
  std::string value = "unchanged";
  EXPECT_FALSE(tokParse("name=abe,badpair,speed=2", "speed", ',', '=', value));
  EXPECT_EQ(value, "error");

  double number = 42;
  EXPECT_FALSE(tokParse("speed=fast", "speed", ',', '=', number));
  EXPECT_DOUBLE_EQ(number, 42);

  bool flag = true;
  EXPECT_FALSE(tokParse("active=maybe", "active", ',', '=', flag));
  EXPECT_TRUE(flag);
}

// Covers mb utils token parse behavior: returns before later malformed pairs after target.
TEST(MBUtilsTokenParseTest, ReturnsBeforeLaterMalformedPairsAfterTarget)
{
  std::string value;
  EXPECT_TRUE(tokParse("speed=2,badpair", "speed", ',', '=', value));
  EXPECT_EQ(value, "2");
  EXPECT_FALSE(tokParse("badpair,speed=2", "speed", ',', '=', value));
  EXPECT_EQ(value, "error");
}

// Covers mb utils token string parse behavior: returns trimmed values and all keys.
TEST(MBUtilsTokenStringParseTest, ReturnsTrimmedValuesAndAllKeys)
{
  const std::string spec = "x=12, y=-4, label=alpha, label=beta";

  EXPECT_EQ(tokStringParse(spec, "x"), "12");
  EXPECT_EQ(tokStringParse(spec, "y"), "-4");
  EXPECT_EQ(tokStringParse(spec, "label"), "alpha");
  EXPECT_EQ(tokStringParse(spec, "missing"), "");
  EXPECT_DOUBLE_EQ(tokDoubleParse(spec, "x"), 12);
  EXPECT_DOUBLE_EQ(tokDoubleParse(spec, "missing"), 0);

  std::set<std::string> keys = tokStringAll(spec);
  EXPECT_EQ(keys.count("x"), 1u);
  EXPECT_EQ(keys.count("y"), 1u);
  EXPECT_EQ(keys.count("label"), 1u);
  EXPECT_EQ(keys.size(), 3u);
}

// Covers mb utils token string parse behavior: protects quoted and braced group separators.
TEST(MBUtilsTokenStringParseTest, ProtectsQuotedAndBracedGroupSeparators)
{
  const std::string spec =
      "msg=\"hold,station\",poly={0,0:10,0:10,10},speed=2.5";

  EXPECT_EQ(tokStringParse(spec, "msg"), "\"hold,station\"");
  EXPECT_EQ(tokStringParse(spec, "poly"), "{0,0:10,0:10,10}");
  EXPECT_EQ(tokStringParse(spec, "speed"), "2.5");

  std::set<std::string> keys = tokStringAll(spec);
  EXPECT_EQ(keys, (std::set<std::string>{"msg", "poly", "speed"}));
}

// Covers mb utils token string parse behavior: pins value separator limitations.
TEST(MBUtilsTokenStringParseTest, PinsValueSeparatorLimitations)
{
  EXPECT_EQ(tokStringParse("expr=a=b,mode=survey", "expr"), "");
  EXPECT_EQ(tokStringParse("mode=survey,expr=a=b", "mode"), "survey");
  EXPECT_DOUBLE_EQ(tokDoubleParse("poly={0,0:1,1},speed=2", "speed"), 0);
}

// Covers mb utils token string parse behavior: supports custom separators used by mode specs.
TEST(MBUtilsTokenStringParseTest, SupportsCustomSeparatorsUsedByModeSpecs)
{
  const std::string spec = "MODE:SURVEY#SPEED:2.0#ACTIVE:true";
  EXPECT_EQ(tokStringParse(spec, "MODE", '#', ':'), "SURVEY");
  EXPECT_DOUBLE_EQ(tokDoubleParse(spec, "SPEED", '#', ':'), 2.0);

  bool active = false;
  EXPECT_TRUE(tokParse(spec, "ACTIVE", '#', ':', active));
  EXPECT_TRUE(active);
}

// Covers mb utils augment spec behavior: appends fields with caller selected separators.
TEST(MBUtilsAugmentSpecTest, AppendsFieldsWithCallerSelectedSeparators)
{
  EXPECT_EQ(augmentSpec("", "x=1"), "x=1");
  EXPECT_EQ(augmentSpec("x=1", "y=2"), "x=1,y=2");
  EXPECT_EQ(augmentSpec("x=1", "y=2", '#'), "x=1#y=2");
  EXPECT_EQ(augmentSpec("x=1", ""), "x=1");
}
