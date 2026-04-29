#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "HashUtils.h"
#include "MacroUtils.h"
#include "MBUtils.h"

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
