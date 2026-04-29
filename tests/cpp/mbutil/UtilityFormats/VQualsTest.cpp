#include <gtest/gtest.h>

#include <vector>

#include "VQuals.h"

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
