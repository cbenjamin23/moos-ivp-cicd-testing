#include <gtest/gtest.h>

#include <string>

#include "NodeRecord.h"

namespace {

bool contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

}  // namespace

// Covers node record behavior: stores core NODE_REPORT fields and string lookup aliases.
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
  EXPECT_EQ(rec.getStringValue("y"), "-5.5");
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

// Covers node record behavior: defaults return fallbacks and unset numeric values are formatted as zero.
TEST(NodeRecordTest, DefaultsReturnFallbacksAndUnsetNumericValuesAreFormattedAsZero)
{
  NodeRecord rec;

  EXPECT_EQ(rec.getName("fallback_name"), "fallback_name");
  EXPECT_EQ(rec.getType("fallback_type"), "fallback_type");
  EXPECT_EQ(rec.getGroup("fallback_group"), "fallback_group");
  EXPECT_EQ(rec.getVSource("fallback_source"), "fallback_source");
  EXPECT_EQ(rec.getColor("fallback_color"), "fallback_color");
  EXPECT_EQ(rec.getMode("fallback_mode"), "fallback_mode");
  EXPECT_EQ(rec.getModeAux("fallback_aux"), "fallback_aux");
  EXPECT_EQ(rec.getAllStop("fallback_stop"), "fallback_stop");
  EXPECT_EQ(rec.getLoadWarning("fallback_warning"), "fallback_warning");

  EXPECT_FALSE(rec.isSetXY());
  EXPECT_FALSE(rec.isSetSpeed());
  EXPECT_FALSE(rec.isSetHeading());
  EXPECT_FALSE(rec.isSetTimeStamp());
  EXPECT_EQ(rec.getElapsedTime(100), -1);

  EXPECT_EQ(rec.getStringValue("x"), "0");
  EXPECT_EQ(rec.getStringValue("lat"), "0");
  EXPECT_EQ(rec.getStringValue("speed"), "0");
  EXPECT_EQ(rec.getStringValue("transparency"), "1");
  EXPECT_EQ(rec.getStringValue("trajectory"), "");
}

// Covers node record behavior: string lookup does not expose core string fields unless they are properties.
TEST(NodeRecordTest, StringLookupDoesNotExposeCoreStringFieldsUnlessTheyAreProperties)
{
  NodeRecord rec("abe", "UUV");
  rec.setGroup("alpha");
  rec.setVSource("pNodeReporter");
  rec.setColor("red");
  rec.setMode("SURVEY");
  rec.setAllStop("false");

  EXPECT_EQ(rec.getStringValue("name"), "");
  EXPECT_EQ(rec.getStringValue("type"), "");
  EXPECT_EQ(rec.getStringValue("group"), "");
  EXPECT_EQ(rec.getStringValue("vsource"), "");
  EXPECT_EQ(rec.getStringValue("color"), "");
  EXPECT_EQ(rec.getStringValue("mode"), "");
  EXPECT_EQ(rec.getStringValue("allstop"), "");

  rec.setProperty("name", "property_name");
  rec.setProperty("type", "property_type");
  rec.setProperty("mode", "property_mode");
  EXPECT_EQ(rec.getStringValue("name"), "property_name");
  EXPECT_EQ(rec.getStringValue("type"), "property_type");
  EXPECT_EQ(rec.getStringValue("mode"), "property_mode");
}

// Covers node record behavior: stores string fields without normalizing case or whitespace.
TEST(NodeRecordTest, StoresStringFieldsWithoutNormalizingCaseOrWhitespace)
{
  NodeRecord rec;
  rec.setName("  abe  ");
  rec.setType(" kayak ");
  rec.setGroup(" Alpha Team ");
  rec.setVSource("pNodeReporter");
  rec.setColor("Dark Blue");
  rec.setMode("SURVEY:LEG1");
  rec.setModeAux("Loiter");
  rec.setAllStop("FALSE");
  rec.setLoadWarning(" nominal ");
  rec.setProperty("Payload_Status", "READY");

  EXPECT_EQ(rec.getName(), "  abe  ");
  EXPECT_EQ(rec.getType(), " kayak ");
  EXPECT_EQ(rec.getGroup(), " Alpha Team ");
  EXPECT_EQ(rec.getColor(), "Dark Blue");
  EXPECT_EQ(rec.getMode(), "SURVEY:LEG1");
  EXPECT_EQ(rec.getModeAux(), "Loiter");
  EXPECT_EQ(rec.getAllStop(), "FALSE");
  EXPECT_EQ(rec.getLoadWarning(), " nominal ");
  EXPECT_TRUE(rec.hasProperty("Payload_Status"));
  EXPECT_FALSE(rec.hasProperty("payload_status"));
  EXPECT_EQ(rec.getProperty("Payload_Status"), "READY");
}

// Covers node record behavior: properties can be overwritten and empty values still exist.
TEST(NodeRecordTest, PropertiesCanBeOverwrittenAndEmptyValuesStillExist)
{
  NodeRecord rec("abe", "UUV");
  rec.setProperty("payload", "ready");
  rec.setProperty("payload", "standby");
  rec.setProperty("empty", "");

  EXPECT_TRUE(rec.hasProperty("payload"));
  EXPECT_EQ(rec.getProperty("payload"), "standby");
  EXPECT_TRUE(rec.hasProperty("empty"));
  EXPECT_EQ(rec.getProperty("empty"), "");
  EXPECT_TRUE(contains(rec.getSpec(), "empty="));
  EXPECT_TRUE(contains(rec.getSpec(), "payload=standby"));
}

// Covers node record behavior: properties can shadow serialized fields and string lookup lowercases keys.
TEST(NodeRecordTest, PropertiesCanShadowSerializedFieldsAndStringLookupLowercasesKeys)
{
  NodeRecord rec("abe", "UUV");
  rec.setX(10);
  rec.setY(20);
  rec.setProperty("NAME", "upper_property");
  rec.setProperty("name", "lower_property");
  rec.setProperty("X", "upper_x_property");
  rec.setProperty("x", "lower_x_property");

  const std::string spec = rec.getSpec();
  EXPECT_TRUE(contains(spec, "NAME=abe"));
  EXPECT_TRUE(contains(spec, "X=10"));
  EXPECT_TRUE(contains(spec, "NAME=upper_property"));
  EXPECT_TRUE(contains(spec, "name=lower_property"));
  EXPECT_TRUE(contains(spec, "X=upper_x_property"));
  EXPECT_TRUE(contains(spec, "x=lower_x_property"));

  EXPECT_EQ(rec.getStringValue("NAME"), "lower_property");
  EXPECT_EQ(rec.getStringValue("name"), "lower_property");
  EXPECT_EQ(rec.getStringValue("X"), "10");
  EXPECT_EQ(rec.getStringValue("x"), "10");
}

// Covers node record behavior: string lookup covers numeric aliases and property fallbacks.
TEST(NodeRecordTest, StringLookupCoversNumericAliasesAndPropertyFallbacks)
{
  NodeRecord rec("abe", "UUV");
  rec.setLat(42.35845678);
  rec.setLon(-70.78456789);
  rec.setCourseOG(271.25);
  rec.setYaw(1.23456789);
  rec.setDepth(9.876);
  rec.setAltitude(12.345);
  rec.setBeam(1.234);
  rec.setTransparency(0.333);
  rec.setProperty("battery", "87%");

  EXPECT_EQ(rec.getStringValue("LAT"), "42.35845678");
  EXPECT_EQ(rec.getStringValue("lon"), "-70.78456789");
  EXPECT_EQ(rec.getStringValue("cog"), "271.25");
  EXPECT_EQ(rec.getStringValue("yaw"), "1.2346");
  EXPECT_EQ(rec.getStringValue("depth"), "9.88");
  EXPECT_EQ(rec.getStringValue("altitude"), "12.35");
  EXPECT_EQ(rec.getStringValue("beam"), "1.23");
  EXPECT_EQ(rec.getStringValue("transparency"), "0.33");
  EXPECT_EQ(rec.getStringValue("battery"), "87%");
}

// Covers node record behavior: numeric setters overwrite values and expose set flags.
TEST(NodeRecordTest, NumericSettersOverwriteValuesAndExposeSetFlags)
{
  NodeRecord rec("abe", "UUV");
  rec.setX(1);
  rec.setX(2);
  rec.setY(3);
  rec.setY(4);
  rec.setPitch(-0.25);
  rec.setPitch(0.5);
  rec.setSpeed(1);
  rec.setSpeed(2);
  rec.setTimeStamp(100);
  rec.setTimeStamp(90);

  EXPECT_TRUE(rec.isSetX());
  EXPECT_TRUE(rec.isSetY());
  EXPECT_TRUE(rec.isSetPitch());
  EXPECT_TRUE(rec.isSetSpeed());
  EXPECT_TRUE(rec.isSetTimeStamp());
  EXPECT_DOUBLE_EQ(rec.getX(), 2);
  EXPECT_DOUBLE_EQ(rec.getY(), 4);
  EXPECT_DOUBLE_EQ(rec.getPitch(), 0.5);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 2);
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), 90);
  EXPECT_DOUBLE_EQ(rec.getElapsedTime(100), 10);
  EXPECT_DOUBLE_EQ(rec.getElapsedTime(80), -10);
}

// Covers node record behavior: set flags cover optional numeric fields after explicit setters.
TEST(NodeRecordTest, SetFlagsCoverOptionalNumericFieldsAfterExplicitSetters)
{
  NodeRecord rec("abe", "UUV");
  rec.setSpeedOG(1.1);
  rec.setHeadingOG(2.2);
  rec.setCourseOG(3.3);
  rec.setYaw(4.4);
  rec.setDepth(5.5);
  rec.setAltitude(6.6);
  rec.setLength(7.7);
  rec.setBeam(8.8);
  rec.setTransparency(0.9);

  EXPECT_TRUE(rec.isSetSpeedOG());
  EXPECT_TRUE(rec.isSetHeadingOG());
  EXPECT_TRUE(rec.isSetCourseOG());
  EXPECT_TRUE(rec.isSetYaw());
  EXPECT_TRUE(rec.isSetDepth());
  EXPECT_TRUE(rec.isSetAltitude());
  EXPECT_TRUE(rec.isSetLength());
  EXPECT_TRUE(rec.isSetBeam());
  EXPECT_TRUE(rec.isSetTransparency());
  EXPECT_DOUBLE_EQ(rec.getSpeedOG(), 1.1);
  EXPECT_DOUBLE_EQ(rec.getHeadingOG(), 2.2);
  EXPECT_DOUBLE_EQ(rec.getCourseOG(), 3.3);
  EXPECT_DOUBLE_EQ(rec.getYaw(), 4.4);
  EXPECT_DOUBLE_EQ(rec.getDepth(), 5.5);
  EXPECT_DOUBLE_EQ(rec.getAltitude(), 6.6);
  EXPECT_DOUBLE_EQ(rec.getLength(), 7.7);
  EXPECT_DOUBLE_EQ(rec.getBeam(), 8.8);
  EXPECT_DOUBLE_EQ(rec.getTransparency(), 0.9);
}

// Covers node record behavior: copy and assignment preserve state but detach properties.
TEST(NodeRecordTest, CopyAndAssignmentPreserveStateButDetachProperties)
{
  NodeRecord original("abe", "UUV");
  original.setX(1);
  original.setY(2);
  original.setSpeed(3);
  original.setHeading(4);
  original.setProperty("payload", "ready");

  NodeRecord copy(original);
  NodeRecord assigned;
  assigned = original;

  original.setX(10);
  original.setProperty("payload", "changed");

  EXPECT_TRUE(copy.valid());
  EXPECT_DOUBLE_EQ(copy.getX(), 1);
  EXPECT_EQ(copy.getProperty("payload"), "ready");
  EXPECT_TRUE(assigned.valid());
  EXPECT_DOUBLE_EQ(assigned.getX(), 1);
  EXPECT_EQ(assigned.getProperty("payload"), "ready");
  EXPECT_DOUBLE_EQ(original.getX(), 10);
  EXPECT_EQ(original.getProperty("payload"), "changed");
}

// Covers node record behavior: serializes full NODE_REPORTs in stable MOOS field order.
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

// Covers node record behavior: terse serialization protects local and global coordinate policies.
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

  global.setCoordPolicyGlobal(false);
  const std::string local_again = global.getSpec(true);
  EXPECT_TRUE(contains(local_again, "X=10"));
  EXPECT_TRUE(contains(local_again, "Y=20"));
  EXPECT_FALSE(contains(local_again, "LAT="));
  EXPECT_FALSE(contains(local_again, "LON="));
}

// Covers node record behavior: global coordinate policy suppresses local coordinates in full reports.
TEST(NodeRecordTest, GlobalCoordinatePolicySuppressesLocalCoordinatesInFullReports)
{
  NodeRecord rec("abe", "UUV");
  rec.setX(10);
  rec.setY(20);
  rec.setLat(42.35);
  rec.setLon(-70.75);
  rec.setSpeed(2);
  rec.setHeading(180);
  rec.setCoordPolicyGlobal(true);

  const std::string spec = rec.getSpec(false);
  EXPECT_FALSE(contains(spec, "X="));
  EXPECT_FALSE(contains(spec, "Y="));
  EXPECT_TRUE(contains(spec, "LAT=42.35"));
  EXPECT_TRUE(contains(spec, "LON=-70.75"));
}

// Covers node record behavior: global coordinate policy without lat lon suppresses all coordinates.
TEST(NodeRecordTest, GlobalCoordinatePolicyWithoutLatLonSuppressesAllCoordinates)
{
  NodeRecord rec("abe", "UUV");
  rec.setX(10);
  rec.setY(20);
  rec.setSpeed(2);
  rec.setHeading(180);
  rec.setCoordPolicyGlobal(true);

  const std::string spec = rec.getSpec(false);
  EXPECT_FALSE(contains(spec, "X="));
  EXPECT_FALSE(contains(spec, "Y="));
  EXPECT_FALSE(contains(spec, "LAT="));
  EXPECT_FALSE(contains(spec, "LON="));
  EXPECT_TRUE(contains(spec, "SPD=2"));
  EXPECT_TRUE(contains(spec, "HDG=180"));
}

// Covers node record behavior: terse serialization suppresses depth and yaw but retains operational fields.
TEST(NodeRecordTest, TerseSerializationSuppressesDepthAndYawButRetainsOperationalFields)
{
  NodeRecord rec("abe", "UUV");
  rec.setX(10);
  rec.setY(20);
  rec.setSpeed(2);
  rec.setHeading(180);
  rec.setDepth(9);
  rec.setYaw(3.14159);
  rec.setTimeStamp(100.5);
  rec.setTransparency(0.4);
  rec.setLength(4.2);
  rec.setBeam(1.7);
  rec.setTrajectory("0,0:1,1");
  rec.setProperty("PAYLOAD", "ready");

  const std::string terse = rec.getSpec(true);
  EXPECT_FALSE(contains(terse, "DEP="));
  EXPECT_FALSE(contains(terse, "YAW="));
  EXPECT_TRUE(contains(terse, "TIME=100.5"));
  EXPECT_TRUE(contains(terse, "TRANSPARENCY=0.4"));
  EXPECT_TRUE(contains(terse, "LENGTH=4.2"));
  EXPECT_TRUE(contains(terse, "BEAM=1.7"));
  EXPECT_TRUE(contains(terse, "TRAJECTORY={0,0:1,1}"));
  EXPECT_TRUE(contains(terse, "PAYLOAD=ready"));
}

// Covers node record behavior: yaw serialization currently derives from heading not stored yaw.
TEST(NodeRecordTest, YawSerializationCurrentlyDerivesFromHeadingNotStoredYaw)
{
  NodeRecord rec("abe", "UUV");
  rec.setHeading(90);
  rec.setYaw(123.456);

  EXPECT_EQ(rec.getYaw(), 123.456);
  EXPECT_TRUE(contains(rec.getSpec(), "YAW=0"));

  rec.setHeading(0);
  EXPECT_TRUE(contains(rec.getSpec(), "YAW=1.5707963"));
}

// Covers node record behavior: properties serialize in map order after known fields.
TEST(NodeRecordTest, PropertiesSerializeInMapOrderAfterKnownFields)
{
  NodeRecord rec("abe", "UUV");
  rec.setHeading(0);
  rec.setYaw(1);
  rec.setProperty("zeta", "last");
  rec.setProperty("alpha", "first");
  rec.setProperty("payload", "ready");

  EXPECT_EQ(rec.getSpec(),
            "NAME=abe,HDG=0,TYPE=UUV,YAW=1.5707963,alpha=first,payload=ready,"
            "zeta=last");
}

// Covers node record behavior: trajectory serialization always wraps stored value in braces.
TEST(NodeRecordTest, TrajectorySerializationAlwaysWrapsStoredValueInBraces)
{
  NodeRecord empty_traj("abe", "UUV");
  empty_traj.setTrajectory("");
  EXPECT_TRUE(empty_traj.isSetTrajectory());
  EXPECT_TRUE(contains(empty_traj.getSpec(), "TRAJECTORY={}"));

  NodeRecord already_braced("abe", "UUV");
  already_braced.setTrajectory("{0,0:1,1}");
  EXPECT_TRUE(contains(already_braced.getSpec(), "TRAJECTORY={{0,0:1,1}}"));
}

// Covers node record behavior: index and thrust reverse serialize only when non default.
TEST(NodeRecordTest, IndexAndThrustReverseSerializeOnlyWhenNonDefault)
{
  NodeRecord rec("abe", "UUV");
  EXPECT_FALSE(contains(rec.getSpec(), "INDEX="));
  EXPECT_FALSE(contains(rec.getSpec(), "THRUST_MODE_REVERSE="));

  rec.setIndex(-3);
  rec.setThrustModeReverse(true);
  EXPECT_TRUE(contains(rec.getSpec(), "INDEX=-3"));
  EXPECT_TRUE(contains(rec.getSpec(), "THRUST_MODE_REVERSE=true"));

  rec.setIndex(0);
  EXPECT_FALSE(contains(rec.getSpec(), "INDEX="));
  EXPECT_TRUE(contains(rec.getSpec(), "THRUST_MODE_REVERSE=true"));

  rec.setThrustModeReverse(false);
  EXPECT_FALSE(contains(rec.getSpec(), "THRUST_MODE_REVERSE="));
}

// Covers node record behavior: validates required local or global NODE_REPORT coordinates.
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

// Covers node record behavior: default validity requires complete local or global coordinate set.
TEST(NodeRecordTest, DefaultValidityRequiresCompleteLocalOrGlobalCoordinateSet)
{
  NodeRecord x_only("abe", "UUV");
  x_only.setX(1);
  x_only.setSpeed(2);
  x_only.setHeading(3);
  EXPECT_FALSE(x_only.valid());

  NodeRecord y_only("abe", "UUV");
  y_only.setY(1);
  y_only.setSpeed(2);
  y_only.setHeading(3);
  EXPECT_FALSE(y_only.valid());

  NodeRecord lat_only("abe", "UUV");
  lat_only.setLat(42.35);
  lat_only.setSpeed(2);
  lat_only.setHeading(3);
  EXPECT_FALSE(lat_only.valid());

  NodeRecord lon_only("abe", "UUV");
  lon_only.setLon(-70.75);
  lon_only.setSpeed(2);
  lon_only.setHeading(3);
  EXPECT_FALSE(lon_only.valid());
}

// Covers node record behavior: validates all supported requirement fields and ignores unknown checks.
TEST(NodeRecordTest, ValidatesAllSupportedRequirementFieldsAndIgnoresUnknownChecks)
{
  NodeRecord rec("abe", "UUV");
  std::string why;

  EXPECT_FALSE(rec.valid(" name, type, mode, mode_aux, allstop, load_warning, "
                         "x, y, speed, heading, depth, time, transparency, "
                         "length, beam, lat, lon, trajectory, unknown ",
                         why));
  EXPECT_EQ(why,
            "missing:mode,mode_aux,allstop,load_warning,x,y,speed,heading,"
            "depth,time,transparency,length,beam,lat,lon,trajectory");

  rec.setMode("SURVEY");
  rec.setModeAux("LEG1");
  rec.setAllStop("false");
  rec.setLoadWarning("none");
  rec.setX(1);
  rec.setY(2);
  rec.setSpeed(3);
  rec.setHeading(4);
  rec.setDepth(5);
  rec.setTimeStamp(6);
  rec.setTransparency(0.7);
  rec.setLength(8);
  rec.setBeam(9);
  rec.setYaw(10);
  rec.setLat(42.35);
  rec.setLon(-70.75);
  rec.setTrajectory("0,0:1,1");

  EXPECT_TRUE(rec.valid("name,type,mode,mode_aux,allstop,load_warning,x,y,"
                        "speed,heading,depth,time,transparency,length,beam,"
                        "yaw,lat,lon,trajectory,unknown",
                        why));

  NodeRecord no_type("abe", "");
  EXPECT_FALSE(no_type.valid("type", why));
  EXPECT_EQ(why, "missing:type");
}

// Covers node record behavior: validates optional numeric fields individually.
TEST(NodeRecordTest, ValidatesOptionalNumericFieldsIndividually)
{
  NodeRecord rec("abe", "UUV");
  std::string why;
  EXPECT_FALSE(rec.valid("type,depth,time,transparency,length,beam", why));
  EXPECT_EQ(why, "missing:depth,time,transparency,length,beam");

  rec.setDepth(0);
  rec.setTimeStamp(0);
  rec.setTransparency(0);
  rec.setLength(0);
  rec.setBeam(0);
  rec.setYaw(0);
  EXPECT_TRUE(rec.valid("depth,time,transparency,length,beam,yaw", why));
}

// Covers node record behavior: validation reports duplicate missing fields and leaves why on success.
TEST(NodeRecordTest, ValidationReportsDuplicateMissingFieldsAndLeavesWhyOnSuccess)
{
  NodeRecord rec("abe", "UUV");
  std::string why;

  EXPECT_FALSE(rec.valid("x, x, y, made_up, y", why));
  EXPECT_EQ(why, "missing:x,x,y,y");

  rec.setX(1);
  rec.setY(2);
  why = "previous failure";
  EXPECT_TRUE(rec.valid("x,y,made_up", why));
  EXPECT_EQ(why, "previous failure");
}

// Covers node record behavior: is set lat lon currently only checks longitude flag.
TEST(NodeRecordTest, IsSetLatLonCurrentlyOnlyChecksLongitudeFlag)
{
  NodeRecord only_lon;
  only_lon.setLon(-70.75);
  EXPECT_FALSE(only_lon.isSetLatitude());
  EXPECT_TRUE(only_lon.isSetLongitude());
  EXPECT_TRUE(only_lon.isSetLatLon());

  NodeRecord only_lat;
  only_lat.setLat(42.35);
  EXPECT_TRUE(only_lat.isSetLatitude());
  EXPECT_FALSE(only_lat.isSetLongitude());
  EXPECT_FALSE(only_lat.isSetLatLon());
}
