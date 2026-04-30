#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "HashUtils.h"
#include "MacroUtils.h"
#include "MBUtils.h"

// Covers macro utils behavior: expands scalar macros in both supported forms.
TEST(MacroUtilsTest, ExpandsScalarMacrosInBothSupportedForms)
{
  EXPECT_EQ(macroExpand("name=$(VNAME),alt=$[VNAME]", "VNAME", "abe"),
            "name=abe,alt=abe");
  EXPECT_EQ(macroExpand("speed=$(SPD)", "SPD", 2.125, 2), "speed=2.12");
  EXPECT_EQ(macroExpand("index=$[IX]", "IX", 7), "index=7");
  EXPECT_EQ(macroExpand("count=$(COUNT)", "COUNT", static_cast<unsigned int>(12)),
            "count=12");
  EXPECT_EQ(macroExpandBool("active=$(ACTIVE)", "ACTIVE", true),
            "active=true");
}

// Covers macro utils behavior: pins mission macro examples and numeric digit clamps.
TEST(MacroUtilsTest, PinsMissionMacroExamplesAndNumericDigitClamps)
{
  // Behavior flags often mix $() and $[] macro styles; numeric expansion clamps
  // requested precision into the supported formatter range.
  std::string contact_flag =
      "post=CONTACT_INFO,range=$(RANGE),contact=$[CN_NAME],own=$(OWNSHIP)";
  contact_flag = macroExpand(contact_flag, "RANGE", 12.3456, 1);
  contact_flag = macroExpand(contact_flag, "CN_NAME", "ben");
  contact_flag = macroExpand(contact_flag, "OWNSHIP", "abe");
  EXPECT_EQ(contact_flag, "post=CONTACT_INFO,range=12.3,contact=ben,own=abe");

  EXPECT_EQ(macroExpand("x=$(OSX)", "OSX", 12.75, -4), "x=13");
  EXPECT_EQ(macroExpand("x=$(OSX)", "OSX", 12.123456789, 20),
            "x=12.12345679");
  EXPECT_EQ(macroExpand("a=$(CN),b=$(CNX)", "CN", "abe"),
            "a=abe,b=$(CNX)");
  EXPECT_EQ(macroExpandBool("active=$[ACTIVE],done=$(DONE)",
                            "ACTIVE", false),
            "active=false,done=$(DONE)");
}

// Covers macro utils behavior: finds and reduces macros with defaults.
TEST(MacroUtilsTest, FindsAndReducesMacrosWithDefaults)
{
  EXPECT_TRUE(hasMacro("color=$(COLOR)", "COLOR"));
  EXPECT_TRUE(hasMacro("color=$[COLOR]", "COLOR"));
  EXPECT_FALSE(hasMacro("color=%(COLOR)", "COLOR"));

  std::vector<std::string> macros =
      getMacrosFromString("name=$[VNAME],speed=$(SPD),raw=%[ALT]");
  ASSERT_EQ(macros.size(), 1u);
  EXPECT_EQ(macros[0], "VNAME");

  std::vector<std::string> paren_macros =
      getMacrosFromString("name=$[VNAME],speed=$(SPD)", '$', '(');
  ASSERT_EQ(paren_macros.size(), 1u);
  EXPECT_EQ(paren_macros[0], "SPD");

  EXPECT_EQ(macroDefault("COLOR:=green"), "green");
  EXPECT_EQ(macroDefault("COLOR=red"), "red");
  EXPECT_EQ(macroDefault("COLOR"), "");
  EXPECT_EQ(macroBase("COLOR:=green", ":="), "COLOR");
  EXPECT_EQ(expandMacrosWithDefault("color=$(COLOR:=green),mode=%(MODE:=survey)"),
            "color=green,mode=SURVEY");
  EXPECT_EQ(reduceMacrosToBase("color=$(COLOR:=green)", ":=", "COLOR"),
            "color=$(COLOR)");
}

// Covers macro utils behavior: finds custom macro forms and pins reduction no ops.
TEST(MacroUtilsTest, FindsCustomMacroFormsAndPinsReductionNoOps)
{
  std::vector<std::string> brace_macros =
      getMacrosFromString("x=${X},bad=$[],y=${Y:=42}", '$', '{');
  ASSERT_EQ(brace_macros.size(), 2u);
  EXPECT_EQ(brace_macros[0], "X");
  EXPECT_EQ(brace_macros[1], "Y:=42");

  std::vector<std::string> percent_macros =
      getMacrosFromString("mode=%(MODE:=survey),plain=%[RAW]", '%', '(');
  ASSERT_EQ(percent_macros.size(), 1u);
  EXPECT_EQ(percent_macros[0], "MODE:=survey");

  EXPECT_EQ(expandMacrosWithDefault(
                "name=$(VNAME),color=$(COLOR:=red),mode=%(MODE:=survey)"),
            "name=$(VNAME),color=red,mode=SURVEY");
  EXPECT_EQ(reduceMacrosToBase("x=$(X*=12),y=%(Y*=abc),z=$(Z:=9)",
                               "*=", "X"),
            "x=$(X),y=%(Y*=abc),z=$(Z:=9)");
  EXPECT_EQ(reduceMacrosToBase("x=$(X*=12)", "", "X"), "x=$(X*=12)");
  EXPECT_EQ(reduceMacrosToBase("x=$(X*=12)", "*=", ""), "x=$(X*=12)");
}

// Covers macro utils behavior: detects counter macros and generates requested hash lengths.
TEST(MacroUtilsTest, DetectsCounterMacrosAndGeneratesRequestedHashLengths)
{
  EXPECT_EQ(getCounterMacro("leg=$[CTR_LEG]"), "CTR_LEG");
  EXPECT_EQ(getCounterMacro("leg=$(CTR_LEG)"), "");

  std::string expanded = macroHashExpand("id=$(HASH4),alt=$[HASH2]", "HASH4");
  EXPECT_FALSE(hasMacro(expanded, "HASH4"));
  EXPECT_TRUE(hasMacro(expanded, "HASH2"));
  EXPECT_EQ(tokStringParse(expanded, "id").size(), 4u);

  std::string unchanged = macroHashExpand("id=$(UNKNOWN)", "UNKNOWN");
  EXPECT_EQ(unchanged, "id=$(UNKNOWN)");
}

// Covers macro utils behavior: pins counter and hash macro edge cases.
TEST(MacroUtilsTest, PinsCounterAndHashMacroEdgeCases)
{
  // Counter macros are recognized only in the $[CTR_*] form, and hash macros
  // currently support single-digit lengths below ten.
  EXPECT_EQ(getCounterMacro("first=$[CTR_ONE],second=$[CTR_TWO]"), "CTR_ONE");
  EXPECT_EQ(getCounterMacro("leg=$[ctr_leg]"), "");
  EXPECT_EQ(getCounterMacro("leg=$[CTR_LEG"), "");

  std::string h6 = macroHashExpand("id=$(HASH),alt=$[HASH6]", "HASH");
  EXPECT_EQ(tokStringParse(h6, "id").size(), 6u);
  EXPECT_TRUE(hasMacro(h6, "HASH6"));

  std::string h9 = macroHashExpand("id=$(HASH9)", "HASH9");
  EXPECT_EQ(tokStringParse(h9, "id").size(), 9u);
  EXPECT_TRUE(isAlphaNum(tokStringParse(h9, "id")));

  EXPECT_EQ(macroHashExpand("id=$(HASH10)", "HASH10"), "id=$(HASH10)");
}

// Covers hash utils behavior: generates random hashes with expected character classes.
TEST(HashUtilsTest, GeneratesRandomHashesWithExpectedCharacterClasses)
{
  std::string alnum = hashAlphaNum(12);
  EXPECT_EQ(alnum.size(), 12u);
  EXPECT_TRUE(isAlphaNum(alnum));

  std::string upper = hashAlphaUpper(8);
  EXPECT_EQ(upper.size(), 8u);
  EXPECT_EQ(upper, toupper(upper));
  EXPECT_TRUE(isAlphaNum(upper));

  EXPECT_EQ(hashAlphaNum(0), "");
  EXPECT_EQ(randomWord({}), "NULL");
  EXPECT_EQ(randomWord({"alpha"}), "alpha");
}

// Covers hash utils behavior: extracts mission and z hash short forms.
TEST(HashUtilsTest, ExtractsMissionAndZHashShortForms)
{
  EXPECT_EQ(missionHashShort("250121-1122S-GLAD-ARLO"), "GLAD-ARLO");
  EXPECT_EQ(missionHashShort("mhash=250121-1122S-GLAD-ARLO,utc=1737476534.76"),
            "GLAD-ARLO");
  EXPECT_EQ(missionHashLShort("250121-1122S-GLAD-ARLO"), "S-GLAD-ARLO");
  EXPECT_DOUBLE_EQ(missionHashUTC("mhash=250121-1122S-GLAD-ARLO,utc=1737476534.76"),
                   1737476534.76);
  EXPECT_EQ(missionHashShort("short"), "");
  EXPECT_EQ(missionHashLShort("short"), "-");
  EXPECT_DOUBLE_EQ(missionHashUTC("mhash=250121-1122S-GLAD-ARLO"), 0);
}

// Covers hash utils behavior: pins mission hash formats used by mission hash apps.
TEST(HashUtilsTest, PinsMissionHashFormatsUsedByMissionHashApps)
{
  // Mission hashes carry date/time plus two word tokens; the short forms are
  // consumed by mission-hash app summaries and appcasts.
  std::string hash = missionHash();
  EXPECT_EQ(hash.size(), 22u);
  EXPECT_EQ(hash[6], '-');
  EXPECT_EQ(hash[12], '-');
  EXPECT_EQ(hash.find('-', 13), 17u);
  EXPECT_EQ(missionHashShort(hash).size(), 9u);
  EXPECT_EQ(missionHashLShort(hash).size(), 11u);
  EXPECT_EQ(missionHashShort("mhash=" + hash + ",utc=1737476534.76"),
            missionHashShort(hash));
  EXPECT_GT(missionHashUTC("mhash=" + hash + ",utc=1737476534.76"), 0);

  EXPECT_EQ(missionHashShort("mhash=bad,utc=12.5"), "");
  EXPECT_DOUBLE_EQ(missionHashUTC("utc=12.5,mhash=bad"), 12.5);
}

// Covers hash utils behavior: provides stable word pools for mission hashes.
TEST(HashUtilsTest, ProvidesStableWordPoolsForMissionHashes)
{
  EXPECT_TRUE(vectorContains(adjectives4(), "Foxy"));
  EXPECT_TRUE(vectorContains(nouns4(), "Weed"));
  EXPECT_TRUE(vectorContains(names4(), "Arlo"));
  EXPECT_TRUE(vectorContains(us_cities7(), "SANIBEL", false));
  EXPECT_TRUE(vectorContains(intcities7(), "Beijing"));

  std::string adj = randomWord({"ABLE", "FOXY", "GLAD"});
  EXPECT_TRUE(vectorContains({"ABLE", "FOXY", "GLAD"}, adj));

  EXPECT_EQ(getCurrYear().size(), 2u);
  EXPECT_EQ(getCurrYear(true).size(), 4u);
  EXPECT_EQ(getCurrMonth().size(), 2u);
  EXPECT_EQ(getCurrDay().size(), 2u);
  EXPECT_EQ(getCurrHour().size(), 2u);
  EXPECT_EQ(getCurrMinute().size(), 2u);
  EXPECT_EQ(getCurrSeconds().size(), 2u);
  EXPECT_EQ(getCurrDate().size(), 10u);
  EXPECT_EQ(getCurrTime().size(), 8u);
  EXPECT_GT(getCurrTimeUTC(), 0);
}
