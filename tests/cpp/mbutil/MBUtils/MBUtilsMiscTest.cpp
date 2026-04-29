#include <gtest/gtest.h>

#include <list>
#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "MBUtils.h"

namespace {

std::vector<char*> makeArgv(std::vector<std::string>& args)
{
  std::vector<char*> argv;
  for(std::string& arg : args)
    argv.push_back(&arg[0]);
  return argv;
}

}  // namespace

TEST(MBUtilsMiscTest, ShortensHelmModeStringsForUiDisplays)
{
  EXPECT_EQ(modeShorten("MODE_A@Alpha:Bravo$MODE_B@Charlie:Delta"),
            "Bravo, Delta");
  EXPECT_EQ(modeShorten("MODE_A@Alpha:Bravo$MODE_B@Charlie:Delta", false),
            "Alpha:Bravo, Charlie:Delta");
  EXPECT_EQ(modeShorten("SURVEYING:LEG1"), "LEG1");
  EXPECT_EQ(modeShorten("STATION_KEEP"), "STATION_KEEP");
}

TEST(MBUtilsMiscTest, ParsesApplicationNamesAndVehicleTypes)
{
  EXPECT_EQ(parseAppName("../pHelmIvP"), "pHelmIvP");
  EXPECT_EQ(parseAppName("/usr/local/bin/pMarineViewer"), "pMarineViewer");
  EXPECT_EQ(parseAppName("pNodeReporter"), "pNodeReporter");

  std::vector<std::string> path = tokenizePath("/usr/local/bin/pHelmIvP");
  ASSERT_EQ(path.size(), 4u);
  EXPECT_EQ(path[0], "usr");
  EXPECT_EQ(path[3], "pHelmIvP");

  EXPECT_TRUE(isKnownVehicleType("UUV"));
  EXPECT_TRUE(isKnownVehicleType("heron"));
  EXPECT_TRUE(isKnownVehicleType("skyw"));
  EXPECT_TRUE(isKnownVehicleType("skywalker"));
  EXPECT_TRUE(isKnownVehicleType("bcrayx"));
  EXPECT_FALSE(isKnownVehicleType("submarine"));
}

TEST(MBUtilsMiscTest, ComputesChecksumsDigitsAndMonths)
{
  EXPECT_EQ(checksumHexStr(""), "00");
  EXPECT_EQ(checksumHexStr("A"), "41");
  EXPECT_EQ(checksumHexStr("GPRMC"), "4B");

  EXPECT_EQ(digitsOnly("alpha-12.30 beta45"), "123045");
  EXPECT_EQ(charCount("VIEW_SEGLIST:pts={0,0:1,1}", ':'), 2u);
  EXPECT_EQ(intToMonth(1), "January");
  EXPECT_EQ(intToMonth(9, true), "Sep");
  EXPECT_EQ(intToMonth(13), "unknown");
  EXPECT_EQ(intToMonth(13, true), "unk");
}

TEST(MBUtilsMiscTest, SerializesCollectionsWithCallerSelectedSeparators)
{
  EXPECT_EQ(stringListToString(std::list<std::string>{"alpha", "bravo"}, '|'),
            "alpha|bravo");
  EXPECT_EQ(stringSetToString(std::set<std::string>{"delta", "alpha"}, ':'),
            "alpha:delta");
  EXPECT_EQ(stringVectorToString(std::vector<std::string>{"x", "y", "z"}, ';'),
            "x;y;z");
  EXPECT_EQ(uintVectorToString(std::vector<unsigned int>{3, 12, 99}, '/'),
            "3/12/99");
  EXPECT_EQ(stringSetToVector(std::set<std::string>{"zulu", "alpha"}),
            std::vector<std::string>({"alpha", "zulu"}));
}

TEST(MBUtilsMiscTest, ComputesMinMaxAndHexEncodings)
{
  EXPECT_DOUBLE_EQ(minElement({}), 0);
  EXPECT_DOUBLE_EQ(maxElement({}), 0);
  EXPECT_DOUBLE_EQ(minElement({4.5, -2.0, 7.0}), -2.0);
  EXPECT_DOUBLE_EQ(maxElement({4.5, -2.0, 7.0}), 7.0);

  EXPECT_EQ(doubleToHex(-0.1), "00");
  EXPECT_EQ(doubleToHex(0.5), "7F");
  EXPECT_EQ(doubleToHex(1.5), "FF");
}

TEST(MBUtilsArgTest, FindsAndValidatesCommandLineStyleArguments)
{
  std::vector<std::string> args = {
      "pExample", "--mission", "alpha.moos", "--speed", "2.5", "--verbose"};
  std::vector<char*> argv = makeArgv(args);
  int argc = static_cast<int>(argv.size());

  EXPECT_EQ(getArg(argc, argv.data(), 1, "--mission"), 2);
  EXPECT_EQ(getArg(argc, argv.data(), 1, "--speed", "-s"), 4);
  EXPECT_EQ(getArg(argc, argv.data(), 2, "--speed"), 0);
  EXPECT_TRUE(scanArgs(argc, argv.data(), "--verbose"));
  EXPECT_TRUE(scanArgs(argc, argv.data(), "-v", "--verbose"));
  EXPECT_FALSE(scanArgs(argc, argv.data(), "--quiet"));

  EXPECT_EQ(validateArgs(argc, argv.data(),
                         "--mission:1 --speed:1 --verbose:0"),
            0);
  EXPECT_EQ(validateArgs(argc, argv.data(), "--mission:1 --speed:0"), 3);
}

TEST(MBUtilsTextLayoutTest, JoinsJustifiesAndBreaksMissionHelpText)
{
  std::vector<std::string> joined =
      joinLines({"Behavior = BHV_Waypoint\\   ", "{ name = survey }",
                 "condition = MODE==SURVEY\\\\", "endflag = DONE=true"});
  ASSERT_EQ(joined.size(), 3u);
  EXPECT_EQ(joined[0], "Behavior = BHV_Waypoint{ name = survey }");
  EXPECT_EQ(joined[1], "condition = MODE==SURVEY\\\\");
  EXPECT_EQ(joined[2], "endflag = DONE=true");

  std::vector<std::string> preserved =
      joinLines({"line one\\", "line two", "line three"}, true);
  EXPECT_EQ(preserved,
            std::vector<std::string>({"line oneline two", "", "line three"}));

  EXPECT_EQ(breakLen("ABCDEFGHIJ", 4),
            std::vector<std::string>({"ABCD", "EFGH", "IJ"}));
  EXPECT_EQ(justifyLen("alpha beta gamma delta", 12),
            std::vector<std::string>({"alpha beta", "gamma", "delta"}));
}

TEST(MBUtilsFileAndRangeSetterTest, ChecksFileAccessAndCoupledRanges)
{
  const std::string path = "mbutils_file_access_fixture.txt";
  {
    std::ofstream out(path.c_str());
    out << "ServerHost = localhost\n";
  }

  EXPECT_TRUE(okFileToRead(path));
  EXPECT_FALSE(okFileToRead("missing_file.moos"));
  EXPECT_TRUE(okFileToWrite(path));
  EXPECT_TRUE(okFileToWrite("new_file_in_current_dir.txt"));
  EXPECT_FALSE(okFileToWrite("/definitely_missing_dir/new.moos"));

  double minv = 5;
  double maxv = 10;
  EXPECT_TRUE(setMinPartOfPairOnString(minv, maxv, "12"));
  EXPECT_DOUBLE_EQ(minv, 12);
  EXPECT_DOUBLE_EQ(maxv, 12);
  EXPECT_TRUE(setMaxPartOfPairOnString(minv, maxv, "8"));
  EXPECT_DOUBLE_EQ(minv, 8);
  EXPECT_DOUBLE_EQ(maxv, 8);
  EXPECT_FALSE(setMinPartOfPairOnString(minv, maxv, "-1"));
  EXPECT_TRUE(setMinPartOfPairOnString(minv, maxv, "-1", true));
  EXPECT_DOUBLE_EQ(minv, -1);

  std::remove(path.c_str());
}
