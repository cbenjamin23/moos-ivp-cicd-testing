#include <gtest/gtest.h>

#include <string>
#include <set>
#include <vector>

#include "VarDataPair.h"
#include "VarDataPairUtils.h"

// Covers var data pair behavior: constructs numeric and string postings.
TEST(VarDataPairTest, ConstructsNumericAndStringPostings)
{
  VarDataPair numeric("RETURN", 1.0);
  EXPECT_TRUE(numeric.valid());
  EXPECT_EQ(numeric.get_var(), "RETURN");
  EXPECT_FALSE(numeric.is_string());
  EXPECT_DOUBLE_EQ(numeric.get_ddata(), 1.0);
  EXPECT_EQ(numeric.getPrintable(), "RETURN=1");

  VarDataPair text("MESSAGE", "hold station");
  EXPECT_TRUE(text.valid());
  EXPECT_TRUE(text.is_string());
  EXPECT_EQ(text.get_sdata(), "hold station");
  EXPECT_EQ(text.getPrintable(), "MESSAGE=hold station");

  VarDataPair quoted_number("MODE", "\"27\"", "auto");
  EXPECT_TRUE(quoted_number.valid());
  EXPECT_TRUE(quoted_number.is_string());
  EXPECT_TRUE(quoted_number.is_quoted());
  EXPECT_EQ(quoted_number.get_sdata(), "27");
  EXPECT_EQ(quoted_number.getPrintable(), "MODE=\"27\"");
}

// Covers var data pair behavior: pins setter overwrite and zero value behavior.
TEST(VarDataPairTest, PinsSetterOverwriteAndZeroValueBehavior)
{
  // Current setters do not symmetrically replace string/numeric state, and a
  // numeric zero posting is treated as invalid unless supplied in a valid spec.
  VarDataPair pair;
  EXPECT_EQ(pair.why_invalid(), 1);
  EXPECT_TRUE(pair.set_var("DEPLOY"));
  EXPECT_FALSE(pair.set_var("BAD VAR"));
  EXPECT_TRUE(pair.set_sdata("true"));
  EXPECT_TRUE(pair.valid());
  EXPECT_FALSE(pair.set_ddata(1.0));
  EXPECT_TRUE(pair.is_string());
  EXPECT_FALSE(pair.set_ddata(2.5, true));
  EXPECT_FALSE(pair.is_string());
  EXPECT_DOUBLE_EQ(pair.get_ddata(), 2.5);
  EXPECT_EQ(pair.getPrintable(), "DEPLOY=2.5");

  VarDataPair zero;
  EXPECT_TRUE(zero.set_var("ZERO"));
  EXPECT_TRUE(zero.set_ddata(0.0));
  EXPECT_FALSE(zero.valid());
  EXPECT_DOUBLE_EQ(zero.get_ddata(), 0.0);
}

// Covers var data pair behavior: extracts macros from behavior flag strings.
TEST(VarDataPairTest, ExtractsMacrosFromBehaviorFlagStrings)
{
  VarDataPair pair("CONTACT_SUMMARY", "vname=$[VNAME],rng=$[RANGE],rng=$[RANGE]");
  EXPECT_TRUE(pair.is_solo_macro() == false);

  std::vector<std::string> macros = pair.getMacroVector();
  ASSERT_EQ(macros.size(), 3u);
  EXPECT_EQ(macros[0], "VNAME");
  EXPECT_EQ(macros[1], "RANGE");
  EXPECT_EQ(macros[2], "RANGE");

  std::set<std::string> macro_set = pair.getMacroSet();
  EXPECT_EQ(macro_set, std::set<std::string>({"RANGE", "VNAME"}));

  VarDataPair solo("NODE_REPORT_LOCAL", "$[NODE_REPORT]");
  EXPECT_TRUE(solo.is_solo_macro());

  VarDataPair not_solo("NODE_REPORT_LOCAL", "$[NODE_REPORT],$[UTC]");
  EXPECT_FALSE(not_solo.is_solo_macro());
}

// Covers var data pair utils behavior: parses range and destination tagged postings.
TEST(VarDataPairUtilsTest, ParsesRangeAndDestinationTaggedPostings)
{
  // Behavior flag specs can carry range tags, destination tags, and conditions
  // before the variable assignment.
  VarDataPair pair;
  ASSERT_TRUE(setVarDataPairOnString(
      pair, "@cpa #group RETURN_HOME=true [if] MTYPE=survey"));

  EXPECT_TRUE(pair.valid());
  EXPECT_EQ(pair.get_post_tag(), "@cpa");
  EXPECT_EQ(pair.get_dest_tag(), "group");
  EXPECT_EQ(pair.get_condition(), "MTYPE=survey");
  EXPECT_EQ(pair.get_var(), "RETURN_HOME");
  EXPECT_TRUE(pair.is_string());
  EXPECT_EQ(pair.get_sdata(), "true");
  EXPECT_EQ(pair.getPrintable(), "@cpa group RETURN_HOME=true");
}

// Covers var data pair utils behavior: parses contact behavior range and broadcast tags.
TEST(VarDataPairUtilsTest, ParsesContactBehaviorRangeAndBroadcastTags)
{
  VarDataPair pair;
  ASSERT_TRUE(setVarDataPairOnString(
      pair, "@>55 #all+ CONTACT_WARNING=abe [if] MODE==ACTIVE"));

  EXPECT_TRUE(pair.valid());
  EXPECT_EQ(pair.get_post_tag(), "@>55");
  EXPECT_EQ(pair.get_dest_tag(), "all+");
  EXPECT_EQ(pair.get_condition(), "MODE==ACTIVE");
  EXPECT_EQ(pair.get_var(), "CONTACT_WARNING");
  EXPECT_EQ(pair.get_sdata(), "abe");
  EXPECT_EQ(pair.getPrintable(), "@>55 all+ CONTACT_WARNING=abe");

  ASSERT_TRUE(setVarDataPairOnString(
      pair, "<25 RETURN_FLAG=station_keep [if] CONTACT=ben"));
  EXPECT_EQ(pair.get_post_tag(), "<25");
  EXPECT_EQ(pair.get_dest_tag(), "");
  EXPECT_EQ(pair.get_condition(), "CONTACT=ben");
  EXPECT_EQ(pair.get_var(), "RETURN_FLAG");
}

// Covers var data pair utils behavior: parses quoted string values and rejects malformed inputs.
TEST(VarDataPairUtilsTest, ParsesQuotedStringValuesAndRejectsMalformedInputs)
{
  VarDataPair pair;
  ASSERT_TRUE(setVarDataPairOnString(pair, "SAY=\"hold,station\""));
  EXPECT_TRUE(pair.valid());
  EXPECT_EQ(pair.get_var(), "SAY");
  EXPECT_TRUE(pair.is_string());
  EXPECT_EQ(pair.get_sdata(), "hold,station");
  EXPECT_EQ(pair.getPrintable(), "SAY=hold,station");

  EXPECT_FALSE(setVarDataPairOnString(pair, ""));
  EXPECT_FALSE(setVarDataPairOnString(pair, "NO_VALUE="));
  EXPECT_FALSE(setVarDataPairOnString(pair, "=NO_VAR"));
}

// Covers var data pair utils behavior: auto parses numbers booleans and stringified numbers.
TEST(VarDataPairUtilsTest, AutoParsesNumbersBooleansAndStringifiedNumbers)
{
  VarDataPair pair;
  ASSERT_TRUE(setVarDataPairOnString(pair, "DESIRED_SPEED=2.4"));
  EXPECT_FALSE(pair.is_string());
  EXPECT_DOUBLE_EQ(pair.get_ddata(), 2.4);
  EXPECT_EQ(pair.getPrintable(), "DESIRED_SPEED=2.4");

  ASSERT_TRUE(setVarDataPairOnString(pair, "DEPLOY=true"));
  EXPECT_TRUE(pair.is_string());
  EXPECT_EQ(pair.get_sdata(), "true");
  EXPECT_EQ(pair.getPrintable(), "DEPLOY=true");

  ASSERT_TRUE(setVarDataPairOnString(pair, "MODE=\"007\""));
  EXPECT_TRUE(pair.is_string());
  EXPECT_TRUE(pair.is_quoted());
  EXPECT_EQ(pair.get_sdata(), "007");
  EXPECT_EQ(pair.getPrintable(), "MODE=\"007\"");
}

// Covers var data pair utils behavior: adds pairs and supports clear all.
TEST(VarDataPairUtilsTest, AddsPairsAndSupportsClearAll)
{
  std::vector<VarDataPair> pairs;
  EXPECT_TRUE(addVarDataPairOnString(pairs, "DEPLOY=true"));
  EXPECT_TRUE(addVarDataPairOnString(pairs, "MESSAGE=hello"));
  ASSERT_EQ(pairs.size(), 2u);
  EXPECT_EQ(longestField(pairs), 5u);

  EXPECT_TRUE(addVarDataPairOnString(pairs, "clearall"));
  EXPECT_TRUE(pairs.empty());
}

// Covers var data pair utils behavior: longest field ignores numeric data fields.
TEST(VarDataPairUtilsTest, LongestFieldIgnoresNumericDataFields)
{
  std::vector<VarDataPair> pairs;
  ASSERT_TRUE(addVarDataPairOnString(pairs, "DESIRED_SPEED=2.5"));
  ASSERT_TRUE(addVarDataPairOnString(pairs, "STATION_MODE=keep_station"));
  ASSERT_TRUE(addVarDataPairOnString(pairs, "DEPLOY=true"));

  EXPECT_EQ(longestField(pairs), std::string("keep_station").size());
}

// Covers var data pair behavior: round trips structured var data pair specs.
TEST(VarDataPairTest, RoundTripsStructuredVarDataPairSpecs)
{
  VarDataPair parsed = stringToVarDataPair("var=MSG,sval={hello,abe},key=k1,ptype=post");
  EXPECT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.get_var(), "MSG");
  EXPECT_EQ(parsed.get_sdata(), "hello,abe");
  EXPECT_EQ(parsed.get_key(), "k1");
  EXPECT_EQ(parsed.get_ptype(), "post");

  VarDataPair bad = stringToVarDataPair("var=BAD NAME,sval=hello");
  EXPECT_FALSE(bad.valid());
}

// Covers var data pair behavior: pins structured spec edge cases.
TEST(VarDataPairTest, PinsStructuredSpecEdgeCases)
{
  // stringToVarDataPair accepts the structured string form only for supported
  // fields; numeric dval and duplicate sval currently invalidate the result.
  VarDataPair numeric = stringToVarDataPair("var=SPD,dval=2.5,key=helm,ptype=post");
  EXPECT_FALSE(numeric.valid());
  EXPECT_EQ(numeric.get_var(), "");
  EXPECT_DOUBLE_EQ(numeric.get_ddata(), 0);
  EXPECT_EQ(numeric.get_key(), "");
  EXPECT_EQ(numeric.get_ptype(), "");

  VarDataPair zero = stringToVarDataPair("var=ZERO,dval=0");
  EXPECT_FALSE(zero.valid());

  VarDataPair unknown = stringToVarDataPair("var=MSG,sval=hello,unknown=ignored");
  EXPECT_TRUE(unknown.valid());
  EXPECT_EQ(unknown.get_sdata(), "hello");

  VarDataPair duplicate = stringToVarDataPair("var=MSG,sval=hello,sval=again");
  EXPECT_FALSE(duplicate.valid());
}
