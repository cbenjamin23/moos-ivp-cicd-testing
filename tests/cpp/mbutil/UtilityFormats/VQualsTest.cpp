#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "VQuals.h"

// Covers v quals behavior: provides deterministic names used by pick pos.
TEST(VQualsTest, ProvidesDeterministicNamesUsedByPickPos)
{
  EXPECT_EQ(getIndexVName1(0), "abe");
  EXPECT_EQ(getIndexVName1(25), "zan");
  EXPECT_EQ(getIndexVName1(51), "zahl");
  EXPECT_EQ(getIndexVName1(52), "vname1");

  EXPECT_EQ(getIndexVName2(0), "avi");
  EXPECT_EQ(getIndexVName2(51), "zack");
  EXPECT_EQ(getIndexVName2(99), "vname2");

  EXPECT_EQ(getIndexVName3(0), "ada");
  EXPECT_EQ(getIndexVName3(51), "zula");
  EXPECT_EQ(getIndexVName3(99), "vname3");

  EXPECT_EQ(getIndexVName4(0), "art");
  EXPECT_EQ(getIndexVName4(51), "zeke");
  EXPECT_EQ(getIndexVName4(99), "vname4");
}

// Covers v quals behavior: provides stable color cycle used for generated contacts.
TEST(VQualsTest, ProvidesStableColorCycleUsedForGeneratedContacts)
{
  const std::vector<std::string> expected = {
      "yellow", "red", "dodger_blue", "green", "purple", "orange",
      "white", "dark_green", "dark_red", "cyan", "coral", "brown",
      "bisque", "white", "pink", "darkslateblue", "brown", "burlywood",
      "goldenrod", "ivory", "khaki", "lime", "peru", "powderblue",
      "plum", "sienna", "sandybrown", "navy", "olive", "magenta"};

  for(unsigned int i = 0; i < expected.size(); ++i)
    EXPECT_EQ(getIndexVColor(i), expected[i]) << "color index " << i;

  EXPECT_EQ(getIndexVColor(30), "yellow");
  EXPECT_EQ(getIndexVColor(99), "yellow");
}

// Covers v quals behavior: pins name list boundaries for generated vehicle fixtures.
TEST(VQualsTest, PinsNameListBoundariesForGeneratedVehicleFixtures)
{
  EXPECT_EQ(getIndexVName1(26), "apia");
  EXPECT_EQ(getIndexVName1(40), "oslo");
  EXPECT_EQ(getIndexVName2(26), "abby");
  EXPECT_EQ(getIndexVName2(40), "olga");
  EXPECT_EQ(getIndexVName3(26), "adel");
  EXPECT_EQ(getIndexVName3(40), "oral");
  EXPECT_EQ(getIndexVName4(26), "arlo");
  EXPECT_EQ(getIndexVName4(40), "owen");
}

// Covers v quals behavior: pins complete pick pos vehicle name caches.
TEST(VQualsTest, PinsCompletePickPosVehicleNameCaches)
{
  const std::vector<std::string> name1 = {
      "abe", "ben", "cal", "deb", "eve", "fin", "gil", "hix", "ike",
      "jim", "kim", "lou", "max", "ned", "oak", "pal", "que", "ray",
      "sam", "tim", "ula", "val", "wes", "xiu", "yen", "zan", "apia",
      "baka", "cary", "doha", "evie", "fahy", "galt", "hays", "iola",
      "jing", "kiev", "lima", "mesa", "nuuk", "oslo", "pace", "quay",
      "rome", "sako", "troy", "ubly", "vimy", "waco", "xane", "york",
      "zahl"};
  const std::vector<std::string> name2 = {
      "avi", "bee", "cap", "dan", "ebb", "fay", "geo", "ham", "ivy",
      "jan", "kay", "lee", "mel", "nat", "ott", "pat", "qua", "ron",
      "sid", "tad", "umm", "vik", "wik", "xik", "yee", "zed", "abby",
      "bill", "clem", "dana", "eddy", "fran", "gabe", "hans", "ivan",
      "jade", "kent", "lacy", "mary", "noel", "olga", "paul", "quin",
      "rick", "sage", "theo", "uber", "vick", "ward", "xavi", "yoel",
      "zack"};
  const std::vector<std::string> name3 = {
      "ada", "bob", "cam", "dee", "ema", "flo", "gus", "hal", "ira",
      "jax", "kia", "leo", "mia", "noa", "ora", "pam", "qix", "roy",
      "sky", "tom", "una", "van", "wim", "xyz", "yip", "zip", "adel",
      "bama", "chad", "dash", "emma", "fern", "gary", "hank", "isla",
      "joan", "kyle", "lisa", "mona", "nick", "oral", "page", "quip",
      "rice", "seth", "tony", "ugly", "vice", "webb", "xray", "yara",
      "zula"};
  const std::vector<std::string> name4 = {
      "art", "bud", "coy", "doc", "ell", "fed", "guy", "hip", "icy",
      "jed", "ken", "lex", "may", "nim", "oma", "pig", "qal", "rob",
      "sue", "ted", "unk", "von", "wam", "xoo", "yap", "zap", "arlo",
      "brad", "chip", "doug", "evan", "ford", "greg", "harm", "ivor",
      "jack", "kurt", "lane", "mack", "nina", "owen", "pete", "quem",
      "rosa", "stan", "troy", "ulan", "vipp", "wood", "xelp", "yoga",
      "zeke"};

  const std::vector<std::function<std::string(unsigned int)>> getters = {
      getIndexVName1, getIndexVName2, getIndexVName3, getIndexVName4};
  const std::vector<std::vector<std::string>> expected = {name1, name2, name3,
                                                          name4};

  for(unsigned int family = 0; family < getters.size(); ++family) {
    ASSERT_EQ(expected[family].size(), 52u);
    for(unsigned int i = 0; i < 52; ++i)
      EXPECT_EQ(getters[family](i), expected[family][i])
          << "family " << family + 1 << " index " << i;
  }
}

// Covers v quals behavior: falls back after pick pos cache range.
TEST(VQualsTest, FallsBackAfterPickPosCacheRange)
{
  EXPECT_EQ(getIndexVName1(52), "vname1");
  EXPECT_EQ(getIndexVName2(52), "vname2");
  EXPECT_EQ(getIndexVName3(52), "vname3");
  EXPECT_EQ(getIndexVName4(52), "vname4");

  EXPECT_EQ(getIndexVName1(1000), "vname1");
  EXPECT_EQ(getIndexVName2(1000), "vname2");
  EXPECT_EQ(getIndexVName3(1000), "vname3");
  EXPECT_EQ(getIndexVName4(1000), "vname4");
}
