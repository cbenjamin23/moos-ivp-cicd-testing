#include <gtest/gtest.h>

#include <string>

#include "NodeRecord.h"

namespace {

bool contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

TEST(NodeRecordTest, StoresCoreNodeReportFieldsAndStringLookupAliases)
{
  NodeRecord rec("abe", "UUV");
  EXPECT_EQ(rec.getName("fallback"), "abe");
  EXPECT_EQ(rec.getType("fallback"), "UUV");
  EXPECT_EQ(rec.getGroup("fallback"), "fallback");
  EXPECT_EQ(rec.getElapsedTime(100), -1);

  rec.setX(10.25);
  rec.setY(-5.5);
  rec.setLat(42.35);
  rec.setLon(-70.75);
  rec.setSpeed(2.25);
  rec.setSpeedOG(2.5);
  rec.setHeading(90);
  rec.setHeadingOG(91);
  rec.setCourseOG(92);
  rec.setDepth(4.5);
  rec.setAltitude(8);
  rec.setLength(4);
  rec.setBeam(1.5);
  rec.setTimeStamp(123.45);
  rec.setTransparency(0.75);
  rec.setTrajectory("0,0:10,0");
  rec.setGroup("alpha");
  rec.setVSource("pNodeReporter");
  rec.setColor("red");
  rec.setMode("SURVEY");
  rec.setModeAux("LEG1");
  rec.setAllStop("false");
  rec.setLoadWarning("none");
  rec.setIndex(3);
  rec.setThrustModeReverse(true);
  rec.setProperty("payload", "ready");

  EXPECT_TRUE(rec.isSetXY());
  EXPECT_TRUE(rec.isSetLatitude());
  EXPECT_TRUE(rec.isSetLongitude());
  EXPECT_TRUE(rec.isSetSpeedOG());
  EXPECT_TRUE(rec.isSetCourseOG());
  EXPECT_TRUE(rec.isSetTrajectory());
  EXPECT_TRUE(rec.getThrustModeReverse());
  EXPECT_NEAR(rec.getElapsedTime(130.45), 7.0, 1e-9);

  EXPECT_EQ(rec.getStringValue("x"), "10.25");
  EXPECT_EQ(rec.getStringValue("spd"), "2.25");
  EXPECT_EQ(rec.getStringValue("speed_og"), "2.5");
  EXPECT_EQ(rec.getStringValue("hdg"), "90");
  EXPECT_EQ(rec.getStringValue("heading_og"), "91");
  EXPECT_EQ(rec.getStringValue("dep"), "4.5");
  EXPECT_EQ(rec.getStringValue("alt"), "8");
  EXPECT_EQ(rec.getStringValue("len"), "4");
  EXPECT_EQ(rec.getStringValue("utime"), "123.45");
  EXPECT_EQ(rec.getStringValue("traj"), "0,0:10,0");
  EXPECT_EQ(rec.getStringValue("payload"), "ready");
  EXPECT_EQ(rec.getStringValue("unknown"), "");
}

TEST(NodeRecordTest, SerializesFullNodeReportsInStableMoosFieldOrder)
{
  NodeRecord rec("abe", "UUV");
  rec.setX(10.25);
  rec.setY(-5.5);
  rec.setLat(42.35);
  rec.setLon(-70.75);
  rec.setSpeed(2.25);
  rec.setHeading(90);
  rec.setCourseOG(92);
  rec.setDepth(4.5);
  rec.setColor("red");
  rec.setGroup("alpha");
  rec.setVSource("pNodeReporter");
  rec.setMode("SURVEY");
  rec.setModeAux("LEG1");
  rec.setAllStop("false");
  rec.setLoadWarning("none");
  rec.setAltitude(8);
  rec.setSpeedOG(2.5);
  rec.setHeadingOG(91);
  rec.setIndex(3);
  rec.setThrustModeReverse(true);
  rec.setYaw(1.5708);
  rec.setTimeStamp(123.45);
  rec.setTransparency(0.75);
  rec.setLength(4);
  rec.setBeam(1.5);
  rec.setTrajectory("0,0:10,0");
  rec.setProperty("payload", "ready");
  rec.setProperty("task", "survey");

  EXPECT_EQ(rec.getSpec(),
            "NAME=abe,X=10.25,Y=-5.5,SPD=2.25,HDG=90,COG=92,DEP=4.5,"
            "LAT=42.35,LON=-70.75,TYPE=UUV,COLOR=red,GROUP=alpha,"
            "VSOURCE=pNodeReporter,MODE=SURVEY,MODE_AUX=LEG1,ALLSTOP=false,"
            "LOAD_WARNING=none,ALTITUDE=8,SPD_OG=2.5,HDG_OG=91,INDEX=3,"
            "THRUST_MODE_REVERSE=true,YAW=0,TIME=123.45,TRANSPARENCY=0.75,"
            "LENGTH=4,BEAM=1.5,TRAJECTORY={0,0:10,0},payload=ready,"
            "task=survey");
}

TEST(NodeRecordTest, TerseSerializationProtectsLocalAndGlobalCoordinatePolicies)
{
  NodeRecord local("abe", "UUV");
  local.setX(10);
  local.setY(20);
  local.setLat(42.35);
  local.setLon(-70.75);
  local.setSpeed(2);
  local.setHeading(180);
  local.setDepth(9);

  const std::string local_terse = local.getSpec(true);
  EXPECT_TRUE(contains(local_terse, "X=10"));
  EXPECT_TRUE(contains(local_terse, "Y=20"));
  EXPECT_FALSE(contains(local_terse, "LAT="));
  EXPECT_FALSE(contains(local_terse, "LON="));
  EXPECT_FALSE(contains(local_terse, "DEP="));

  NodeRecord global = local;
  global.setCoordPolicyGlobal(true);
  const std::string global_terse = global.getSpec(true);
  EXPECT_FALSE(contains(global_terse, "X="));
  EXPECT_FALSE(contains(global_terse, "Y="));
  EXPECT_TRUE(contains(global_terse, "LAT=42.35"));
  EXPECT_TRUE(contains(global_terse, "LON=-70.75"));
  EXPECT_FALSE(contains(global_terse, "DEP="));
}

TEST(NodeRecordTest, ValidatesRequiredLocalOrGlobalNodeReportCoordinates)
{
  NodeRecord empty;
  std::string why;
  EXPECT_FALSE(empty.valid("name,x,y,speed,heading", why));
  EXPECT_EQ(why, "missing:name,x,y,speed,heading");

  NodeRecord local("abe", "UUV");
  local.setX(1);
  local.setY(2);
  local.setSpeed(3);
  local.setHeading(4);
  EXPECT_TRUE(local.valid());

  NodeRecord global("abe", "UUV");
  global.setLat(42.35);
  global.setLon(-70.75);
  global.setSpeed(3);
  global.setHeading(4);
  EXPECT_TRUE(global.valid());

  NodeRecord missing_lon("abe", "UUV");
  missing_lon.setLat(42.35);
  missing_lon.setSpeed(3);
  missing_lon.setHeading(4);
  EXPECT_FALSE(missing_lon.valid("name,lat,lon,speed,heading", why));
  EXPECT_EQ(why, "missing:lon");
}
