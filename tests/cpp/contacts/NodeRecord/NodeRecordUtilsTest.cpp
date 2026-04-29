#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "NodeRecord.h"
#include "NodeRecordUtils.h"

TEST(NodeRecordUtilsTest, ParsesCanonicalNodeReporterCspReportsWithExtraProperties)
{
  NodeRecord rec = string2NodeRecord(
      "NAME=alpha,TYPE=UUV,TIME=1252348077.59,X=51.71,Y=-35.50,"
      "LAT=43.825089,LON=-70.330030,SPD=2.00,HDG=119.06,YAW=2.0779,"
      "DEPTH=0.00,LENGTH=4.0,MODE=DRIVE,GROUP=A,VSOURCE=ais,"
      "COLOR=yellow,ALLSTOP=false,LOAD_WARNING=none,HDG_OG=121.5,"
      "COG=120.25,SPD_OG=2.10,ALT=9.5,TRANSPARENCY=0.65,INDEX=7,"
      "THRUST_MODE_REVERSE=true,TRAJECTORY={0,0:1,1},PAYLOAD=ready");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getType(), "UUV");
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), 1252348077.59);
  EXPECT_DOUBLE_EQ(rec.getX(), 51.71);
  EXPECT_DOUBLE_EQ(rec.getY(), -35.50);
  EXPECT_DOUBLE_EQ(rec.getLat(), 43.825089);
  EXPECT_DOUBLE_EQ(rec.getLon(), -70.330030);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 2.00);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 119.06);
  EXPECT_DOUBLE_EQ(rec.getYaw(), 2.0779);
  EXPECT_DOUBLE_EQ(rec.getDepth(), 0.00);
  EXPECT_DOUBLE_EQ(rec.getLength(), 4.0);
  EXPECT_DOUBLE_EQ(rec.getHeadingOG(), 121.5);
  EXPECT_DOUBLE_EQ(rec.getCourseOG(), 120.25);
  EXPECT_DOUBLE_EQ(rec.getSpeedOG(), 2.10);
  EXPECT_DOUBLE_EQ(rec.getAltitude(), 9.5);
  EXPECT_DOUBLE_EQ(rec.getTransparency(), 0.65);
  EXPECT_EQ(rec.getIndex(), 7);
  EXPECT_TRUE(rec.getThrustModeReverse());
  EXPECT_EQ(rec.getTrajectory(), "0,0:1,1");
  EXPECT_EQ(rec.getProperty("PAYLOAD"), "ready");
}

TEST(NodeRecordUtilsTest, ParsesJsonNodeReportsUsedByJsonPosters)
{
  NodeRecord rec = string2NodeRecord(
      "{\"NAME\":\"alpha\",\"TYPE\":\"UUV\",\"UTC_TIME\":\"125.5\","
      "\"X\":\"1\",\"Y\":\"2\",\"LAT\":\"42.35\",\"LON\":\"-70.75\","
      "\"SPD\":\"3\",\"HDG\":\"90\",\"DEP\":\"4\",\"MODE\":\"DRIVE\","
      "\"GROUP\":\"A\",\"VSOURCE\":\"ais\",\"COLOR\":\"green\","
      "\"ALLSTOP\":\"false\",\"LOAD_WARNING\":\"none\","
      "\"THRUST_MODE_REVERSE\":\"true\",\"TRAJECTORY\":\"0:0:1:1\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getType(), "UUV");
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), 125.5);
  EXPECT_DOUBLE_EQ(rec.getX(), 1);
  EXPECT_DOUBLE_EQ(rec.getY(), 2);
  EXPECT_DOUBLE_EQ(rec.getLat(), 42.35);
  EXPECT_DOUBLE_EQ(rec.getLon(), -70.75);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 3);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 90);
  EXPECT_DOUBLE_EQ(rec.getDepth(), 4);
  EXPECT_EQ(rec.getMode(), "DRIVE");
  EXPECT_EQ(rec.getGroup(), "A");
  EXPECT_EQ(rec.getVSource(), "ais");
  EXPECT_EQ(rec.getColor(), "green");
  EXPECT_EQ(rec.getAllStop(), "false");
  EXPECT_EQ(rec.getLoadWarning(), "none");
  EXPECT_TRUE(rec.getThrustModeReverse());
  EXPECT_EQ(rec.getTrajectory(), "0:0:1:1");
}

TEST(NodeRecordUtilsTest, ComputesRangeCogAndContactExtrapolation)
{
  NodeRecord ownship("abe", "UUV");
  ownship.setX(0);
  ownship.setY(0);
  NodeRecord contact("ben", "UUV");
  contact.setX(3);
  contact.setY(4);
  EXPECT_DOUBLE_EQ(rangeBetweenRecords(ownship, contact), 5);

  NodeRecord prev("ben", "UUV");
  prev.setX(0);
  prev.setY(0);
  NodeRecord curr("ben", "UUV");
  curr.setX(10);
  curr.setY(0);
  ASSERT_TRUE(cogRecord(prev, curr));
  EXPECT_DOUBLE_EQ(curr.getCourseOG(), 90);

  NodeRecord missing_xy("ben", "UUV");
  EXPECT_FALSE(cogRecord(prev, missing_xy));

  NodeRecord moving("ben", "UUV");
  moving.setX(0);
  moving.setY(0);
  moving.setSpeed(2);
  moving.setHeading(90);
  moving.setTimeStamp(10);
  NodeRecord projected = extrapolateRecord(moving, 15);
  EXPECT_NEAR(projected.getX(), 10, 1e-6);
  EXPECT_NEAR(projected.getY(), 0, 1e-6);

  NodeRecord unchanged = extrapolateRecord(moving, 10.04);
  EXPECT_DOUBLE_EQ(unchanged.getX(), 0);
  EXPECT_DOUBLE_EQ(unchanged.getY(), 0);
}

TEST(NodeRecordUtilsTest, CapsStaleNodeReportProjectionAtMaxDeltaDecay)
{
  NodeRecord moving("ben", "UUV");
  moving.setX(0);
  moving.setY(0);
  moving.setSpeed(2);
  moving.setHeading(90);
  moving.setTimeStamp(10);

  NodeRecord projected = extrapolateRecord(moving, 20, 5);
  EXPECT_NEAR(projected.getX(), 10, 1e-6);
  EXPECT_NEAR(projected.getY(), 0, 1e-6);
}
