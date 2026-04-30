#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "MBUtils.h"

namespace {

void expectVectorEq(const std::vector<std::string>& actual,
                    const std::vector<std::string>& expected)
{
  ASSERT_EQ(actual.size(), expected.size());
  for(std::size_t i = 0; i < expected.size(); ++i)
    EXPECT_EQ(actual[i], expected[i]) << "at index " << i;
}

}  // namespace

// Covers mb utils parse string behavior: splits plain delimited fields and preserves whitespace.
TEST(MBUtilsParseStringTest, SplitsPlainDelimitedFieldsAndPreservesWhitespace)
{
  expectVectorEq(parseString("apples,  pears,bananas", ','),
                 {"apples", "  pears", "bananas"});
  expectVectorEq(parseString(",alpha,,bravo,", ','),
                 {"", "alpha", "", "bravo"});
  expectVectorEq(parseString("alpha", ','), {"alpha"});
  EXPECT_TRUE(parseString("", ',').empty());
}

// Covers mb utils parse string behavior: preserves leading and interior empty fields but drops trailing empty.
TEST(MBUtilsParseStringTest, PreservesLeadingAndInteriorEmptyFieldsButDropsTrailingEmpty)
{
  expectVectorEq(parseString("alpha,", ','), {"alpha"});
  expectVectorEq(parseString(",,", ','), {"", ""});
  expectVectorEq(parseString("alpha,,", ','), {"alpha", ""});
  expectVectorEq(parseString("alpha,,,bravo", ','),
                 {"alpha", "", "", "bravo"});
  expectVectorEq(parseString(" alpha | | bravo | ", '|'),
                 {" alpha ", " ", " bravo ", " "});
}

// Covers mb utils parse string behavior: drops trailing empty for other MOOS separators.
TEST(MBUtilsParseStringTest, DropsTrailingEmptyForOtherMoosSeparators)
{
  expectVectorEq(parseString("alpha:", ':'), {"alpha"});
  expectVectorEq(parseString("key=", '='), {"key"});
  expectVectorEq(parseString("first#second#", '#'), {"first", "second"});
  expectVectorEq(parseString("line1\nline2\n", '\n'), {"line1", "line2"});
}

// Covers mb utils parse string behavior: splits multi character separators for mission lists.
TEST(MBUtilsParseStringTest, SplitsMultiCharacterSeparatorsForMissionLists)
{
  expectVectorEq(parseString("alpha $@$ bravo $@$ charlie", "$@$"),
                 {"alpha ", " bravo ", " charlie"});
  expectVectorEq(parseString("one<->two<->three", "<->"),
                 {"one", "two", "three"});
  expectVectorEq(parseString("alpha<-><->bravo<->", "<->"),
                 {"alpha", "", "bravo"});
}

// Covers mb utils parse string behavior: pins high ascii separator replacement collision.
TEST(MBUtilsParseStringTest, PinsHighAsciiSeparatorReplacementCollision)
{
  const std::string collision = std::string("alpha") + static_cast<char>(129) +
                                "bravo<->charlie";
  expectVectorEq(parseString(collision, "<->"),
                 {"alpha", "bravo", "charlie"});
}

// Covers mb utils parse string q behavior: preserves quoted and braced commas in MOOS specs.
TEST(MBUtilsParseStringQTest, PreservesQuotedAndBracedCommasInMoosSpecs)
{
  const std::string spec =
      "src_node=abe,dest_node=ben,var_name=MSG,string_val=\"hold,station\"";
  expectVectorEq(parseStringQ(spec, ','),
                 {"src_node=abe", "dest_node=ben", "var_name=MSG",
                  "string_val=\"hold,station\""});

  const std::string region =
      "polygon={0,0:10,0:10,10:0,10},label=op_region,active=true";
  expectVectorEq(parseStringQ(region, ','),
                 {"polygon={0,0:10,0:10,10:0,10}", "label=op_region",
                  "active=true"});
}

// Covers mb utils parse string q behavior: tracks nested braces and unbalanced protectors.
TEST(MBUtilsParseStringQTest, TracksNestedBracesAndUnbalancedProtectors)
{
  expectVectorEq(parseStringQ("outer={inner={a,b},c},tail=1", ','),
                 {"outer={inner={a,b},c}", "tail=1"});
  expectVectorEq(parseStringQ("note=\"alpha,beta,tail=1", ','),
                 {"note=\"alpha,beta,tail=1"});
  expectVectorEq(parseStringQ("poly={0,0:1,1,tail=1", ','),
                 {"poly={0,0:1,1,tail=1"});
}

// Covers mb utils parse string q behavior: bundles fields without splitting protected content.
TEST(MBUtilsParseStringQTest, BundlesFieldsWithoutSplittingProtectedContent)
{
  const std::string report =
      "community=shoreside,hostip=128.30.24.232,"
      "note=\"alpha,beta\",mode=survey";

  std::vector<std::string> bundles = parseStringQ(report, ',', 45);
  ASSERT_EQ(bundles.size(), 2u);
  EXPECT_EQ(bundles[0], "community=shoreside,hostip=128.30.24.232,");
  EXPECT_EQ(bundles[1], "note=\"alpha,beta\",mode=survey");
}

// Covers mb utils parse string q behavior: bundles oversized fields and empty inputs.
TEST(MBUtilsParseStringQTest, BundlesOversizedFieldsAndEmptyInputs)
{
  expectVectorEq(parseStringQ("", ',', 10), {});
  expectVectorEq(parseStringQ("alpha=1234567890,beta=2,gamma=3", ',', 10),
                 {"alpha=1234567890,", "beta=2,", "gamma=3"});
  expectVectorEq(parseStringQ("a=1,b=\"two,three\",c=4", ',', 11),
                 {"a=1,", "b=\"two,three\",", "c=4"});
}

// Covers mb utils parse string z behavior: honors selected protectors for NODE_REPORTs and lists.
TEST(MBUtilsParseStringZTest, HonorsSelectedProtectorsForNodeReportsAndLists)
{
  expectVectorEq(parseStringZ("name=abe,poly={0,0:5,0},speed=2", ',', "{"),
                 {"name=abe", "poly={0,0:5,0}", "speed=2"});
  expectVectorEq(parseStringZ("a=(1,2),b=[3,4],c=\"5,6\",d=7", ',', "([\""),
                 {"a=(1,2)", "b=[3,4]", "c=\"5,6\"", "d=7"});
  expectVectorEq(parseStringZ("a=(1,2),b=3", ',', "{"),
                 {"a=(1", "2)", "b=3"});
}

// Covers mb utils parse string z behavior: honors each protector only when requested.
TEST(MBUtilsParseStringZTest, HonorsEachProtectorOnlyWhenRequested)
{
  expectVectorEq(parseStringZ("a=(1,2),b=[3,4],c={5,6},d=\"7,8\"", ',', ""),
                 {"a=(1", "2)", "b=[3", "4]", "c={5", "6}", "d=\"7", "8\""});
  expectVectorEq(parseStringZ("a=(1,2),b=[3,4],c={5,6},d=\"7,8\"", ',', "([{"),
                 {"a=(1,2)", "b=[3,4]", "c={5,6}", "d=\"7", "8\""});
  expectVectorEq(parseStringZ("a=(outer,(inner,1)),b=2", ',', "("),
                 {"a=(outer,(inner,1))", "b=2"});
}

// Covers mb utils parse string to words behavior: keeps grouped mission config words together.
TEST(MBUtilsParseStringToWordsTest, KeepsGroupedMissionConfigWordsTogether)
{
  expectVectorEq(parseStringToWords("  alpha   bravo\tcharlie  "),
                 {"alpha", "bravo", "charlie"});
  expectVectorEq(parseStringToWords("points={0,0 10,0 10,10} label=survey", '{'),
                 {"points={0,0 10,0 10,10}", "label=survey"});
  expectVectorEq(parseStringToWords("say \"hold station\" now", '"'),
                 {"say", "\"hold station\" now"});
}

// Covers mb utils parse string to words behavior: handles other grouping delimiters and unbalanced groups.
TEST(MBUtilsParseStringToWordsTest, HandlesOtherGroupingDelimitersAndUnbalancedGroups)
{
  expectVectorEq(parseStringToWords("alpha (bravo charlie) delta", '('),
                 {"alpha", "(bravo charlie)", "delta"});
  expectVectorEq(parseStringToWords("alpha [bravo charlie] delta", '['),
                 {"alpha", "[bravo charlie]", "delta"});
  expectVectorEq(parseStringToWords("alpha {bravo charlie delta", '{'),
                 {"alpha", "{bravo charlie delta"});
  expectVectorEq(parseStringToWords("alpha {bravo {charlie} delta} echo", '{'),
                 {"alpha", "{bravo {charlie} delta}", "echo"});
}

// Covers mb utils parse quoted string behavior: splits only outside balanced quotes.
TEST(MBUtilsParseQuotedStringTest, SplitsOnlyOutsideBalancedQuotes)
{
  expectVectorEq(parseQuotedString("\"apples,pears\",bananas", ','),
                 {"\"apples,pears\"", "bananas"});
  expectVectorEq(parseQuotedString("one,\"two,too\",three", ','),
                 {"one", "\"two,too\"", "three"});
  expectVectorEq(parseQuotedString("\"one,two\",", ','),
                 {"\"one,two\""});
  EXPECT_TRUE(parseQuotedString("\"unterminated,field", ',').empty());
}

// Covers mb utils chomp string behavior: splits at first separator only.
TEST(MBUtilsChompStringTest, SplitsAtFirstSeparatorOnly)
{
  expectVectorEq(chompString("apples, pears, bananas", ','),
                 {"apples", " pears, bananas"});
  expectVectorEq(chompString("no-separator", ','),
                 {"no-separator", ""});
  expectVectorEq(chompString(",leading", ','),
                 {"", "leading"});
  expectVectorEq(chompString("trailing,", ','),
                 {"trailing", ""});
}

// Covers mb utils bite string behavior: destructively splits from left and right.
TEST(MBUtilsBiteStringTest, DestructivelySplitsFromLeftAndRight)
{
  std::string left = "alpha, beta, gamma";
  EXPECT_EQ(biteString(left, ','), "alpha");
  EXPECT_EQ(left, " beta, gamma");
  EXPECT_EQ(biteStringX(left, ','), "beta");
  EXPECT_EQ(left, "gamma");

  std::string right = "alpha, beta, gamma";
  EXPECT_EQ(rbiteString(right, ','), " gamma");
  EXPECT_EQ(right, "alpha, beta");
  EXPECT_EQ(rbiteStringX(right, ','), "beta");
  EXPECT_EQ(right, "alpha");
}

// Covers mb utils bite string behavior: nibbles string patterns used in config parsing.
TEST(MBUtilsBiteStringTest, NibblesStringPatternsUsedInConfigParsing)
{
  std::string str = "condition:=MODE==SURVEY";
  EXPECT_EQ(nibbleString(str, ":="), "condition");
  EXPECT_EQ(str, "MODE==SURVEY");

  std::string either = "left:right=unused";
  EXPECT_EQ(biteString(either, ':', '='), "left");
  EXPECT_EQ(either, "right=unused");
}

// Covers mb utils string cleanup behavior: strips and removes whitespace predictably.
TEST(MBUtilsStringCleanupTest, StripsAndRemovesWhitespacePredictably)
{
  EXPECT_EQ(removeWhite(" alpha,\t beta "), "alpha,beta");
  EXPECT_EQ(removeWhiteEnd(" alpha  \t"), " alpha");
  EXPECT_EQ(removeWhiteNL("a b\nc\rd\t"), "abcd");
  EXPECT_EQ(stripBlankEnds("\t alpha beta  "), "alpha beta");
  EXPECT_EQ(stripQuotes(" \"hello\" "), "hello");
  EXPECT_EQ(stripBraces(" {x=1,y=2} "), "x=1,y=2");
  EXPECT_EQ(stripChevrons(" <alpha> "), "alpha");
}
