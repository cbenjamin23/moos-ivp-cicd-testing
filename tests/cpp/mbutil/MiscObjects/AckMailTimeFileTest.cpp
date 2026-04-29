#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <list>
#include <string>
#include <vector>

#include "AckMessage.h"
#include "FileBuffer.h"
#include "MailFlagSet.h"
#include "NumericAssertions.h"
#include "Odometer.h"
#include "TStamp.h"
#include "VarDataPair.h"

TEST(AckMessageTest, BuildsAndParsesAckMessageSpecs)
{
  AckMessage ack("abe", "ben", "ack_42");
  EXPECT_TRUE(ack.valid());
  EXPECT_EQ(ack.getSpec(), "id=ack_42, src=abe, dest=ben");

  AckMessage parsed = string2AckMessage("src=abe,dest=ben,id=ack_42");
  EXPECT_TRUE(parsed.valid());
  EXPECT_EQ(parsed.getSourceNode(), "abe");
  EXPECT_EQ(parsed.getDestNode(), "ben");
  EXPECT_EQ(parsed.getMessageID(), "ack_42");

  EXPECT_FALSE(string2AckMessage("src=abe,dest=ben").valid());
  EXPECT_FALSE(string2AckMessage("src=abe,id=ack_42").valid());
}

TEST(AckMessageTest, RejectsWhitespaceAndSelfAddressingThroughSetters)
{
  AckMessage ack;
  ack.setSourceNode("abe");
  ack.setDestNode("abe");
  EXPECT_EQ(ack.getDestNode(), "");

  ack.setDestNode("ben");
  ack.setSourceNode("bad source");
  ack.setDestNode("bad dest");
  ack.setMessageID("ack_1");
  EXPECT_TRUE(ack.valid());
  EXPECT_EQ(ack.getSourceNode(), "abe");
  EXPECT_EQ(ack.getDestNode(), "ben");
}

TEST(MailFlagSetTest, ExpandsMailCountAndUtcMacrosForMatchingKeys)
{
  MailFlagSet flags;
  EXPECT_TRUE(flags.addFlag("@REPORT#MAIL_UPDATE=$[CNT]"));
  EXPECT_TRUE(flags.addFlag("@REPORT#MAIL_TOTAL=$[MCNT]"));
  EXPECT_TRUE(flags.addFlag("@ANY#LAST_MAIL=$[KEY]"));
  EXPECT_FALSE(flags.addFlag("REPORT#BAD=true"));

  EXPECT_EQ(flags.size(), 2u);
  ASSERT_TRUE(flags.handleMail("REPORT", 12.345));
  std::vector<VarDataPair> generated = flags.getNewFlags();
  ASSERT_EQ(generated.size(), 2u);
  EXPECT_EQ(generated[0].get_var(), "MAIL_UPDATE");
  EXPECT_EQ(generated[0].get_sdata(), "1");
  EXPECT_EQ(generated[1].get_var(), "MAIL_TOTAL");
  EXPECT_EQ(generated[1].get_sdata(), "1");
  EXPECT_TRUE(flags.getNewFlags().empty());

  ASSERT_TRUE(flags.handleMail("OTHER", 15.0));
  generated = flags.getNewFlags();
  ASSERT_EQ(generated.size(), 1u);
  EXPECT_EQ(generated[0].get_var(), "LAST_MAIL");
  EXPECT_EQ(generated[0].get_sdata(), "$[KEY]");
}

TEST(MailFlagSetTest, TracksPerKeyCountsAnyFallbackAndKeyOrdering)
{
  MailFlagSet flags;
  EXPECT_TRUE(flags.addFlag("@ANY#ANY_COUNT=$[CNT]"));
  EXPECT_TRUE(flags.addFlag("@DEPLOY#DEPLOY_COUNT=$[CNT]"));
  EXPECT_TRUE(flags.addFlag("@DEPLOY#TOTAL_COUNT=$[MCNT]"));
  EXPECT_TRUE(flags.addFlag("@DEPLOY#WHEN=$[UTC]"));
  EXPECT_TRUE(flags.addFlag("@MISSION_HASH#HASH_SEEN=true"));

  EXPECT_EQ(flags.getMailFlagKeys(),
            std::vector<std::string>({"ANY", "DEPLOY", "MISSION_HASH"}));

  ASSERT_TRUE(flags.handleMail("DEPLOY", 12.345));
  std::vector<VarDataPair> generated = flags.getNewFlags();
  ASSERT_EQ(generated.size(), 3u);
  EXPECT_EQ(generated[0].get_var(), "DEPLOY_COUNT");
  EXPECT_EQ(generated[0].get_sdata(), "1");
  EXPECT_EQ(generated[1].get_var(), "TOTAL_COUNT");
  EXPECT_EQ(generated[1].get_sdata(), "1");
  EXPECT_EQ(generated[2].get_var(), "WHEN");
  EXPECT_EQ(generated[2].get_sdata(), "12.35");

  ASSERT_TRUE(flags.handleMail("MISSION_HASH", 20));
  generated = flags.getNewFlags();
  ASSERT_EQ(generated.size(), 1u);
  EXPECT_EQ(generated[0].get_var(), "HASH_SEEN");
  EXPECT_TRUE(generated[0].is_string());
  EXPECT_EQ(generated[0].get_sdata(), "true");

  ASSERT_TRUE(flags.handleMail("NAV_X", 30));
  generated = flags.getNewFlags();
  ASSERT_EQ(generated.size(), 1u);
  EXPECT_EQ(generated[0].get_var(), "ANY_COUNT");
  EXPECT_EQ(generated[0].get_sdata(), "1");
}

TEST(MailFlagSetTest, LeavesUnsupportedMacrosAndAutoParsesNumericFlags)
{
  MailFlagSet flags;
  EXPECT_TRUE(flags.addFlag("@APPCAST#APPCAST_SEEN=$[TIME]"));
  EXPECT_TRUE(flags.addFlag("@APPCAST#APPCAST_LEVEL=2.5"));

  ASSERT_TRUE(flags.handleMail("APPCAST", 99.9));
  std::vector<VarDataPair> generated = flags.getNewFlags();
  ASSERT_EQ(generated.size(), 2u);
  EXPECT_EQ(generated[0].get_var(), "APPCAST_SEEN");
  EXPECT_EQ(generated[0].get_sdata(), "$[TIME]");
  EXPECT_TRUE(generated[1].valid());
  EXPECT_FALSE(generated[1].is_string());
  EXPECT_DOUBLE_EQ(generated[1].get_ddata(), 2.5);

  EXPECT_FALSE(flags.handleMail("UNWATCHED", 101.0));
  EXPECT_TRUE(flags.getNewFlags().empty());
}

TEST(TStampTest, ParsesAndFormatsCompactTimeStrings)
{
  TStamp stamp;
  EXPECT_FALSE(stamp.valid());
  EXPECT_TRUE(stamp.setTime(1, 2, 3.456));
  EXPECT_TRUE(stamp.valid());
  EXPECT_EQ(stamp.getTimeStr(), "010203.456");

  EXPECT_TRUE(stamp.setTimeStr("235959.25"));
  EXPECT_EQ(stamp.getTimeStr(), "235959.25");
  EXPECT_FALSE(stamp.setTimeStr("240000"));
  EXPECT_FALSE(stamp.setTimeStr("1260AA"));
  EXPECT_FALSE(stamp.setTimeStr("12345"));
}

TEST(TStampTest, PinsSetTimeValidationBoundary)
{
  TStamp stamp;
  EXPECT_FALSE(stamp.setTime(24, 0, 0));
  EXPECT_FALSE(stamp.setTime(23, 60, 0));
  EXPECT_FALSE(stamp.setTime(23, 59, 60));
  EXPECT_TRUE(stamp.setTime(0, 0, -1));
  EXPECT_TRUE(stamp.valid());
  EXPECT_EQ(stamp.getTimeStr(), "00000-1");
}

TEST(TStampTest, ParsesPartialSecondsAndPreservesPriorValueOnFailure)
{
  TStamp stamp;
  EXPECT_TRUE(stamp.setTimeStr("000001"));
  EXPECT_TRUE(stamp.valid());
  EXPECT_EQ(stamp.getTimeStr(), "000001");

  EXPECT_TRUE(stamp.setTimeStr("123456."));
  EXPECT_EQ(stamp.getTimeStr(), "123456");

  EXPECT_TRUE(stamp.setTimeStr("123456.007"));
  EXPECT_EQ(stamp.getTimeStr(), "123456.007");

  EXPECT_FALSE(stamp.setTimeStr("123456x007"));
  EXPECT_EQ(stamp.getTimeStr(), "123456.007");
  EXPECT_FALSE(stamp.setTimeStr("123456.bad"));
  EXPECT_EQ(stamp.getTimeStr(), "123456.007");
  EXPECT_FALSE(stamp.setTimeStr("236060"));
  EXPECT_EQ(stamp.getTimeStr(), "123456.007");
}

TEST(OdometerTest, AccumulatesDistanceExtentElapsedTimeAndPauseState)
{
  Odometer odom;
  odom.updateTime(10);
  EXPECT_DOUBLE_EQ(odom.getTotalElapsed(), 0);
  odom.updateTime(15);
  EXPECT_DOUBLE_EQ(odom.getTotalElapsed(), 5);

  odom.updateDistance(0, 0);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 0);
  odom.updateDistance(3, 4);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 5);
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 5);

  odom.pause();
  EXPECT_TRUE(odom.isPaused());
  odom.updateDistance(6, 8);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 5);
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 5);

  odom.unpause();
  odom.updateDistance(9, 12);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 10);
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 15);

  odom.resetExtent();
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 0);
  odom.updateDistance(9, 17);
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 5);
}

TEST(OdometerTest, ResetsOriginDistanceAndElapsedTimeForMissionLegs)
{
  Odometer odom;
  odom.updateDistance(100, 200);
  odom.updateDistance(103, 204);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 5);
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 5);

  odom.reset(50);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 0);
  EXPECT_DOUBLE_EQ(odom.getMaxExtent(), 0);
  EXPECT_DOUBLE_EQ(odom.getTotalElapsed(), 0);
  EXPECT_DOUBLE_EQ(odom.getTotalElapsed(55), 5);

  odom.setX(10);
  odom.updateDistance();
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 0);
  odom.setY(10);
  odom.updateDistance();
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 0);

  odom.updateDistance(13, 14);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 5);

  odom.pause();
  odom.updateDistance(23, 14);
  odom.unpause();
  odom.updateDistance(23, 24);
  EXPECT_DOUBLE_EQ(odom.getTotalDist(), 15);
}

TEST(FileBufferTest, ReadsLinesListsLimitsAndSlashContinuations)
{
  const std::string path = "mbutil_filebuffer_fixture.txt";
  {
    std::ofstream out(path.c_str());
    out << "alpha\n";
    out << "bravo\n";
    out << "charlie";
  }

  std::vector<std::string> lines = fileBuffer(path);
  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(lines[0], "alpha");
  EXPECT_EQ(lines[1], "bravo");
  EXPECT_EQ(lines[2], "charlie");

  std::list<std::string> list_lines = fileBufferList(path);
  EXPECT_EQ(list_lines.size(), 3u);

  std::vector<std::string> limited = fileBuffer(path, 2);
  ASSERT_EQ(limited.size(), 1u);
  EXPECT_EQ(limited[0], "alpha");

  const std::string slash_path = "mbutil_filebuffer_slash_fixture.txt";
  {
    std::ofstream out(slash_path.c_str());
    out << "alpha\\\n";
    out << "bravo\n";
    out << "charlie";
  }

  std::vector<std::string> slash_lines = fileBufferSlash(slash_path);
  ASSERT_EQ(slash_lines.size(), 2u);
  EXPECT_EQ(slash_lines[0], "alphabravo");
  EXPECT_EQ(slash_lines[1], "charlie");

  std::remove(path.c_str());
  std::remove(slash_path.c_str());
}

TEST(FileBufferTest, PinsMissingEmptyAndLimitBoundaryBehavior)
{
  EXPECT_TRUE(fileBuffer("does_not_exist.moos").empty());
  EXPECT_TRUE(fileBufferList("does_not_exist.moos").empty());
  EXPECT_TRUE(fileBufferSlash("does_not_exist.moos").empty());

  const std::string empty_path = "mbutil_filebuffer_empty_fixture.txt";
  {
    std::ofstream out(empty_path.c_str());
  }
  std::vector<std::string> empty_lines = fileBuffer(empty_path);
  ASSERT_EQ(empty_lines.size(), 1u);
  EXPECT_EQ(empty_lines[0], "");

  const std::string path = "mbutil_filebuffer_limit_fixture.txt";
  {
    std::ofstream out(path.c_str());
    out << "one\n";
    out << "two\n";
    out << "three\n";
  }

  std::vector<std::string> one_limit = fileBuffer(path, 1);
  ASSERT_EQ(one_limit.size(), 1u);
  EXPECT_EQ(one_limit[0], "one");
  std::vector<std::string> two_limit = fileBuffer(path, 3);
  ASSERT_EQ(two_limit.size(), 2u);
  EXPECT_EQ(two_limit[0], "one");
  EXPECT_EQ(two_limit[1], "two");

  std::remove(empty_path.c_str());
  std::remove(path.c_str());
}

TEST(FileBufferTest, JoinsMissionStyleBackslashContinuations)
{
  const std::string path = "mbutil_filebuffer_mission_continuation.txt";
  {
    std::ofstream out(path.c_str());
    out << "Behavior = BHV_Waypoint\\   \n";
    out << "{ name = survey }\\\n";
    out << "condition = MODE==SURVEY\n";
    out << "endflag = RETURN=true\\\n";
  }

  std::vector<std::string> lines = fileBufferSlash(path);
  ASSERT_EQ(lines.size(), 2u);
  EXPECT_EQ(lines[0],
            "Behavior = BHV_Waypoint{ name = survey }condition = MODE==SURVEY");
  EXPECT_EQ(lines[1], "endflag = RETURN=true");

  std::vector<std::string> limited = fileBufferSlash(path, 1);
  ASSERT_EQ(limited.size(), 1u);
  EXPECT_EQ(limited[0],
            "Behavior = BHV_Waypoint{ name = survey }condition = MODE==SURVEY");

  std::remove(path.c_str());
}
