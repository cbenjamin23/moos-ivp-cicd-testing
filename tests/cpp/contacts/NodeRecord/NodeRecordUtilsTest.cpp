#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "NodeRecord.h"
#include "NodeRecordUtils.h"

// Covers node record utils behavior: parses canonical NODE_REPORTer csp reports with extra properties.
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

// Covers node record utils behavior: parses csp aliases and last duplicate field wins.
TEST(NodeRecordUtilsTest, ParsesCspAliasesAndLastDuplicateFieldWins)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,TYPE=KAYAK,UTC_TIME=10,TIME=20,X=1,X=2,Y=3,"
      "SPEED=1.5,SPD=2.5,HEADING=45,HDG=90,DEP=4,DEPTH=5,"
      "LEN=6,LENGTH=7,ALT=8,ALTITUDE=9");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getType(), "KAYAK");
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), 20);
  EXPECT_DOUBLE_EQ(rec.getX(), 2);
  EXPECT_DOUBLE_EQ(rec.getY(), 3);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 2.5);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 90);
  EXPECT_DOUBLE_EQ(rec.getDepth(), 5);
  EXPECT_DOUBLE_EQ(rec.getLength(), 7);
  EXPECT_DOUBLE_EQ(rec.getAltitude(), 9);
}

// Covers node record utils behavior: parses plus signed numbers and treats scientific notation as property.
TEST(NodeRecordUtilsTest, ParsesPlusSignedNumbersAndTreatsScientificNotationAsProperty)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=+1.25,Y=-2.5,SPD=+0.5,HDG=+90,TIME=1e3");

  EXPECT_TRUE(rec.isSetX());
  EXPECT_TRUE(rec.isSetY());
  EXPECT_TRUE(rec.isSetSpeed());
  EXPECT_TRUE(rec.isSetHeading());
  EXPECT_FALSE(rec.isSetTimeStamp());
  EXPECT_DOUBLE_EQ(rec.getX(), 1.25);
  EXPECT_DOUBLE_EQ(rec.getY(), -2.5);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 0.5);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 90);
  EXPECT_EQ(rec.getProperty("TIME"), "1e3");
}

// Covers node record utils behavior: parses lowercase csp keys signed numbers and values containing equals.
TEST(NodeRecordUtilsTest, ParsesLowercaseCspKeysSignedNumbersAndValuesContainingEquals)
{
  NodeRecord rec = string2NodeRecordCSP(
      "name=alpha,type=uuv,time=-10.5,x=-1.25,y=2.5,lat=-42.35,lon=70.75,"
      "speed=0,heading=359.9,depth=-3.5,mode=survey,group=team_a,"
      "vsource=pNodeReporter,color=blue,payload=state=ready");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getType(), "uuv");
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), -10.5);
  EXPECT_DOUBLE_EQ(rec.getX(), -1.25);
  EXPECT_DOUBLE_EQ(rec.getY(), 2.5);
  EXPECT_DOUBLE_EQ(rec.getLat(), -42.35);
  EXPECT_DOUBLE_EQ(rec.getLon(), 70.75);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 0);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 359.9);
  EXPECT_DOUBLE_EQ(rec.getDepth(), -3.5);
  EXPECT_EQ(rec.getMode(), "survey");
  EXPECT_EQ(rec.getGroup(), "team_a");
  EXPECT_EQ(rec.getVSource(), "pNodeReporter");
  EXPECT_EQ(rec.getColor(), "blue");
  EXPECT_EQ(rec.getProperty("payload"), "state=ready");
}

// Covers node record utils behavior: csp parser trims blank ends around keys and values.
TEST(NodeRecordUtilsTest, CspParserTrimsBlankEndsAroundKeysAndValues)
{
  NodeRecord rec = string2NodeRecordCSP(
      " NAME = alpha , TYPE = UUV , X = 1 , Y = 2 , SPD = 3 , HDG = 4 , "
      " GROUP = alpha_team , PAYLOAD = ready ");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getType(), "UUV");
  EXPECT_DOUBLE_EQ(rec.getX(), 1);
  EXPECT_DOUBLE_EQ(rec.getY(), 2);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 3);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 4);
  EXPECT_EQ(rec.getGroup(), "alpha_team");
  EXPECT_EQ(rec.getProperty("PAYLOAD"), "ready");
}

// Covers node record utils behavior: parses csp trajectory with commas and mixed property types.
TEST(NodeRecordUtilsTest, ParsesCspTrajectoryWithCommasAndMixedPropertyTypes)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,TRAJECTORY={0,0:5,5:10,0},"
      "PAYLOAD=ready,NUMERIC_CUSTOM=42.5,BAD_TOKEN,EMPTY=");

  EXPECT_TRUE(rec.valid());
  EXPECT_TRUE(rec.isSetTrajectory());
  EXPECT_EQ(rec.getTrajectory(), "0,0:5,5:10,0");
  EXPECT_EQ(rec.getProperty("PAYLOAD"), "ready");
  EXPECT_EQ(rec.getProperty("NUMERIC_CUSTOM"), "42.5");
  EXPECT_TRUE(rec.hasProperty("BAD_TOKEN"));
  EXPECT_EQ(rec.getProperty("BAD_TOKEN"), "");
  EXPECT_TRUE(rec.hasProperty("EMPTY"));
  EXPECT_EQ(rec.getProperty("EMPTY"), "");
}

// Covers node record utils behavior: csp unknown braced properties preserve braces and commas.
TEST(NodeRecordUtilsTest, CspUnknownBracedPropertiesPreserveBracesAndCommas)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,NOTES={leg=1,status=ready},"
      "PLAIN={solo}");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getProperty("NOTES"), "{leg=1,status=ready}");
  EXPECT_EQ(rec.getProperty("PLAIN"), "{solo}");
}

// Covers node record utils behavior: csp parser leaves unsupported node record fields as properties.
TEST(NodeRecordUtilsTest, CspParserLeavesUnsupportedNodeRecordFieldsAsProperties)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,MODE_AUX=LEG1,BEAM=1.5,PITCH=0.25");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getModeAux(), "");
  EXPECT_FALSE(rec.isSetBeam());
  EXPECT_EQ(rec.getProperty("MODE_AUX"), "LEG1");
  EXPECT_EQ(rec.getProperty("BEAM"), "1.5");
  EXPECT_EQ(rec.getProperty("PITCH"), "0.25");
}

// Covers node record utils behavior: csp numeric looking string fields split between setters and properties.
TEST(NodeRecordUtilsTest, CspNumericLookingStringFieldsSplitBetweenSettersAndProperties)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=007,TYPE=1,MODE=2,ALLSTOP=0,COLOR=3,GROUP=4,VSOURCE=5,"
      "LOAD_WARNING=6,X=1,Y=2,SPD=3,HDG=4");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "007");
  EXPECT_EQ(rec.getType(), "1");
  EXPECT_EQ(rec.getMode(), "2");
  EXPECT_EQ(rec.getAllStop(), "0");

  EXPECT_EQ(rec.getColor(), "");
  EXPECT_EQ(rec.getGroup(), "");
  EXPECT_EQ(rec.getVSource(), "");
  EXPECT_EQ(rec.getLoadWarning(), "");
  EXPECT_EQ(rec.getProperty("COLOR"), "3");
  EXPECT_EQ(rec.getProperty("GROUP"), "4");
  EXPECT_EQ(rec.getProperty("VSOURCE"), "5");
  EXPECT_EQ(rec.getProperty("LOAD_WARNING"), "6");
}

// Covers node record utils behavior: csp properties are case sensitive and duplicate keys use last value.
TEST(NodeRecordUtilsTest, CspPropertiesAreCaseSensitiveAndDuplicateKeysUseLastValue)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,payload=ready,PAYLOAD=standby,"
      "payload=done");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getProperty("payload"), "done");
  EXPECT_EQ(rec.getProperty("PAYLOAD"), "standby");
  EXPECT_EQ(rec.getStringValue("PAYLOAD"), "done");
  EXPECT_EQ(rec.getStringValue("payload"), "done");
}

// Covers node record utils behavior: malformed known csp numerics become properties not set fields.
TEST(NodeRecordUtilsTest, MalformedKnownCspNumericsBecomePropertiesNotSetFields)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=west,Y=2,SPD=fast,HDG=north,TIME=soon,DEP=deep,"
      "LENGTH=long,ALT=high,TRANSPARENCY=clear,COG=east,SPD_OG=swift,"
      "HDG_OG=left");

  EXPECT_FALSE(rec.valid());
  EXPECT_FALSE(rec.isSetX());
  EXPECT_TRUE(rec.isSetY());
  EXPECT_FALSE(rec.isSetSpeed());
  EXPECT_FALSE(rec.isSetHeading());
  EXPECT_FALSE(rec.isSetTimeStamp());
  EXPECT_FALSE(rec.isSetDepth());
  EXPECT_FALSE(rec.isSetLength());
  EXPECT_FALSE(rec.isSetAltitude());
  EXPECT_FALSE(rec.isSetTransparency());
  EXPECT_FALSE(rec.isSetCourseOG());
  EXPECT_FALSE(rec.isSetSpeedOG());
  EXPECT_FALSE(rec.isSetHeadingOG());

  EXPECT_EQ(rec.getProperty("X"), "west");
  EXPECT_EQ(rec.getProperty("SPD"), "fast");
  EXPECT_EQ(rec.getProperty("TIME"), "soon");
  EXPECT_EQ(rec.getProperty("TRANSPARENCY"), "clear");
}

// Covers node record utils behavior: csp boolean and index parsing matches NODE_REPORTer conventions.
TEST(NodeRecordUtilsTest, CspBooleanAndIndexParsingMatchesNodeReporterConventions)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,THRUST_MODE_REVERSE=TRUE,INDEX=7.9");
  EXPECT_TRUE(rec.getThrustModeReverse());
  EXPECT_EQ(rec.getIndex(), 7);

  NodeRecord false_rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,THRUST_MODE_REVERSE=false");
  EXPECT_FALSE(false_rec.getThrustModeReverse());

  NodeRecord malformed_index = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,INDEX=not_numeric");
  EXPECT_EQ(malformed_index.getIndex(), 0);
  EXPECT_FALSE(malformed_index.hasProperty("INDEX"));
}

// Covers node record utils behavior: csp boolean and index duplicates do not behave like simple last wins.
TEST(NodeRecordUtilsTest, CspBooleanAndIndexDuplicatesDoNotBehaveLikeSimpleLastWins)
{
  NodeRecord true_then_false = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,THRUST_MODE_REVERSE=true,"
      "THRUST_MODE_REVERSE=false");
  EXPECT_TRUE(true_then_false.getThrustModeReverse());
  EXPECT_EQ(true_then_false.getProperty("THRUST_MODE_REVERSE"), "false");

  NodeRecord false_then_true = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,THRUST_MODE_REVERSE=false,"
      "THRUST_MODE_REVERSE=true");
  EXPECT_TRUE(false_then_true.getThrustModeReverse());
  EXPECT_EQ(false_then_true.getProperty("THRUST_MODE_REVERSE"), "false");

  NodeRecord index_reset = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,INDEX=7,INDEX=not_numeric");
  EXPECT_EQ(index_reset.getIndex(), 0);
  EXPECT_FALSE(index_reset.hasProperty("INDEX"));
}

// Covers node record utils behavior: numeric thrust mode reverse is stored as property not boolean.
TEST(NodeRecordUtilsTest, NumericThrustModeReverseIsStoredAsPropertyNotBoolean)
{
  NodeRecord rec = string2NodeRecordCSP(
      "NAME=alpha,X=1,Y=2,SPD=3,HDG=4,THRUST_MODE_REVERSE=1");

  EXPECT_TRUE(rec.valid());
  EXPECT_FALSE(rec.getThrustModeReverse());
  EXPECT_TRUE(rec.hasProperty("THRUST_MODE_REVERSE"));
  EXPECT_EQ(rec.getProperty("THRUST_MODE_REVERSE"), "1");
}

// Covers node record utils behavior: empty and punctuation only reports produce invalid empty records.
TEST(NodeRecordUtilsTest, EmptyAndPunctuationOnlyReportsProduceInvalidEmptyRecords)
{
  const std::vector<std::string> reports = {
      "",
      ",,,",
      "{}",
      "{\"\":\"\"}",
      "NAME=,X=,Y=,SPD=,HDG="};

  for(const std::string& report : reports) {
    NodeRecord rec = string2NodeRecord(report);
    EXPECT_FALSE(rec.valid()) << report;
    EXPECT_EQ(rec.getName(), "") << report;
    EXPECT_FALSE(rec.isSetX()) << report;
    EXPECT_FALSE(rec.isSetSpeed()) << report;
  }
}

// Covers node record utils behavior: parses JSON NODE_REPORTs used by JSON posters.
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

// Covers node record utils behavior: JSON thrust mode reverse is case insensitive true only.
TEST(NodeRecordUtilsTest, JsonThrustModeReverseIsCaseInsensitiveTrueOnly)
{
  NodeRecord upper = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"THRUST_MODE_REVERSE\":\"TRUE\"}");
  EXPECT_TRUE(upper.getThrustModeReverse());

  NodeRecord numeric = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"THRUST_MODE_REVERSE\":\"1\"}");
  EXPECT_FALSE(numeric.getThrustModeReverse());
}

// Covers node record utils behavior: JSON boolean and index duplicates do not behave like simple last wins.
TEST(NodeRecordUtilsTest, JsonBooleanAndIndexDuplicatesDoNotBehaveLikeSimpleLastWins)
{
  NodeRecord true_then_false = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"THRUST_MODE_REVERSE\":\"true\",\"THRUST_MODE_REVERSE\":\"false\"}");
  EXPECT_TRUE(true_then_false.getThrustModeReverse());
  EXPECT_FALSE(true_then_false.hasProperty("THRUST_MODE_REVERSE"));

  NodeRecord false_then_true = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"THRUST_MODE_REVERSE\":\"false\",\"THRUST_MODE_REVERSE\":\"true\"}");
  EXPECT_TRUE(false_then_true.getThrustModeReverse());

  NodeRecord index_reset = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"INDEX\":\"7\",\"INDEX\":\"not_numeric\"}");
  EXPECT_EQ(index_reset.getIndex(), 0);
  EXPECT_FALSE(index_reset.hasProperty("INDEX"));
}

// Covers node record utils behavior: JSON trajectory strips braces when value has no commas.
TEST(NodeRecordUtilsTest, JsonTrajectoryStripsBracesWhenValueHasNoCommas)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"TRAJECTORY\":\"{0:0:1:1}\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_TRUE(rec.isSetTrajectory());
  EXPECT_EQ(rec.getTrajectory(), "0:0:1:1");
}

// Covers node record utils behavior: parses JSON aliases and last duplicate field wins.
TEST(NodeRecordUtilsTest, ParsesJsonAliasesAndLastDuplicateFieldWins)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"TYPE\":\"UUV\",\"UTC_TIME\":\"10\",\"TIME\":\"20\","
      "\"X\":\"1\",\"X\":\"2\",\"Y\":\"3\",\"SPEED\":\"1.5\",\"SPD\":\"2.5\","
      "\"HEADING\":\"45\",\"HDG\":\"90\",\"DEPTH\":\"4\",\"DEP\":\"5\","
      "\"LENGTH\":\"6\",\"LEN\":\"7\",\"ALTITUDE\":\"8\",\"ALT\":\"9\","
      "\"YAW\":\"1.25\",\"COG\":\"91\",\"SPD_OG\":\"2.75\",\"HDG_OG\":\"92\","
      "\"TRANSPARENCY\":\"0.3\",\"INDEX\":\"4.9\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), 20);
  EXPECT_DOUBLE_EQ(rec.getX(), 2);
  EXPECT_DOUBLE_EQ(rec.getY(), 3);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 2.5);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 90);
  EXPECT_DOUBLE_EQ(rec.getDepth(), 5);
  EXPECT_DOUBLE_EQ(rec.getLength(), 7);
  EXPECT_DOUBLE_EQ(rec.getAltitude(), 9);
  EXPECT_DOUBLE_EQ(rec.getYaw(), 1.25);
  EXPECT_DOUBLE_EQ(rec.getCourseOG(), 91);
  EXPECT_DOUBLE_EQ(rec.getSpeedOG(), 2.75);
  EXPECT_DOUBLE_EQ(rec.getHeadingOG(), 92);
  EXPECT_DOUBLE_EQ(rec.getTransparency(), 0.3);
  EXPECT_EQ(rec.getIndex(), 4);
}

// Covers node record utils behavior: JSON string fields use last duplicate and preserve colons.
TEST(NodeRecordUtilsTest, JsonStringFieldsUseLastDuplicateAndPreserveColons)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"NAME\":\"bravo\",\"TYPE\":\"UUV\","
      "\"TYPE\":\"kayak\",\"MODE\":\"SURVEY\",\"MODE\":\"SURVEY:LEG1:RUN\","
      "\"COLOR\":\"red\",\"COLOR\":\"dark blue\",\"GROUP\":\"alpha\","
      "\"GROUP\":\"beta\",\"VSOURCE\":\"ais\",\"VSOURCE\":\"pNodeReporter\","
      "\"ALLSTOP\":\"false\",\"ALLSTOP\":\"manual=hold\","
      "\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "bravo");
  EXPECT_EQ(rec.getType(), "kayak");
  EXPECT_EQ(rec.getMode(), "SURVEY:LEG1:RUN");
  EXPECT_EQ(rec.getColor(), "dark blue");
  EXPECT_EQ(rec.getGroup(), "beta");
  EXPECT_EQ(rec.getVSource(), "pNodeReporter");
  EXPECT_EQ(rec.getAllStop(), "manual=hold");
}

// Covers node record utils behavior: JSON parser accepts unquoted numeric literals.
TEST(NodeRecordUtilsTest, JsonParserAcceptsUnquotedNumericLiterals)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":1,\"Y\":2,\"SPD\":3,\"HDG\":4,"
      "\"LAT\":42.35,\"LON\":-70.75}");

  EXPECT_TRUE(rec.valid());
  EXPECT_DOUBLE_EQ(rec.getX(), 1);
  EXPECT_DOUBLE_EQ(rec.getY(), 2);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 3);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 4);
  EXPECT_DOUBLE_EQ(rec.getLat(), 42.35);
  EXPECT_DOUBLE_EQ(rec.getLon(), -70.75);
}

// Covers node record utils behavior: JSON parser trims whitespace and preserves string punctuation.
TEST(NodeRecordUtilsTest, JsonParserTrimsWhitespaceAndPreservesStringPunctuation)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{ \"NAME\" : \"alpha\" , \"X\" : \"1\" , \"Y\" : \"2\" , "
      "\"SPD\" : \"3\" , \"HDG\" : \"4\" , \"MODE\" : \"SURVEY:LEG1\" , "
      "\"LOAD_WARNING\" : \"warn=low\" , \"COLOR\" : \"dark blue\" }");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getMode(), "SURVEY:LEG1");
  EXPECT_EQ(rec.getLoadWarning(), "warn=low");
  EXPECT_EQ(rec.getColor(), "dark blue");
}

// Covers node record utils behavior: JSON parser leaves unsupported fields unset and does not store properties.
TEST(NodeRecordUtilsTest, JsonParserLeavesUnsupportedFieldsUnsetAndDoesNotStoreProperties)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"MODE_AUX\":\"LEG1\",\"BEAM\":\"1.5\",\"PITCH\":\"0.25\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getModeAux(), "");
  EXPECT_FALSE(rec.isSetBeam());
  EXPECT_FALSE(rec.hasProperty("MODE_AUX"));
  EXPECT_FALSE(rec.hasProperty("BEAM"));
  EXPECT_FALSE(rec.hasProperty("PITCH"));
}

// Covers node record utils behavior: JSON numeric looking string fields are dropped when after numeric branch.
TEST(NodeRecordUtilsTest, JsonNumericLookingStringFieldsAreDroppedWhenAfterNumericBranch)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"007\",\"TYPE\":\"1\",\"MODE\":\"2\",\"ALLSTOP\":\"0\","
      "\"COLOR\":\"3\",\"GROUP\":\"4\",\"VSOURCE\":\"5\","
      "\"LOAD_WARNING\":\"6\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\","
      "\"HDG\":\"4\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "007");
  EXPECT_EQ(rec.getType(), "1");
  EXPECT_EQ(rec.getMode(), "2");
  EXPECT_EQ(rec.getAllStop(), "0");

  EXPECT_EQ(rec.getColor(), "");
  EXPECT_EQ(rec.getGroup(), "");
  EXPECT_EQ(rec.getVSource(), "");
  EXPECT_EQ(rec.getLoadWarning(), "");
  EXPECT_FALSE(rec.hasProperty("COLOR"));
  EXPECT_FALSE(rec.hasProperty("GROUP"));
  EXPECT_FALSE(rec.hasProperty("VSOURCE"));
  EXPECT_FALSE(rec.hasProperty("LOAD_WARNING"));
}

// Covers node record utils behavior: parses lowercase JSON keys and signed numbers.
TEST(NodeRecordUtilsTest, ParsesLowercaseJsonKeysAndSignedNumbers)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"name\":\"alpha\",\"type\":\"uuv\",\"time\":\"-10.5\","
      "\"x\":\"-1.25\",\"y\":\"2.5\",\"lat\":\"-42.35\",\"lon\":\"70.75\","
      "\"speed\":\"0\",\"heading\":\"359.9\",\"depth\":\"-3.5\","
      "\"mode\":\"survey\",\"group\":\"team_a\",\"vsource\":\"pNodeReporter\","
      "\"color\":\"blue\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_EQ(rec.getType(), "uuv");
  EXPECT_DOUBLE_EQ(rec.getTimeStamp(), -10.5);
  EXPECT_DOUBLE_EQ(rec.getX(), -1.25);
  EXPECT_DOUBLE_EQ(rec.getY(), 2.5);
  EXPECT_DOUBLE_EQ(rec.getLat(), -42.35);
  EXPECT_DOUBLE_EQ(rec.getLon(), 70.75);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 0);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 359.9);
  EXPECT_DOUBLE_EQ(rec.getDepth(), -3.5);
  EXPECT_EQ(rec.getMode(), "survey");
  EXPECT_EQ(rec.getGroup(), "team_a");
  EXPECT_EQ(rec.getVSource(), "pNodeReporter");
  EXPECT_EQ(rec.getColor(), "blue");
}

// Covers node record utils behavior: JSON parser drops unknown and malformed known fields.
TEST(NodeRecordUtilsTest, JsonParserDropsUnknownAndMalformedKnownFields)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"west\",\"Y\":\"2\",\"SPD\":\"fast\","
      "\"HDG\":\"north\",\"PAYLOAD\":\"ready\",\"NUMERIC_CUSTOM\":\"42\"}");

  EXPECT_FALSE(rec.valid());
  EXPECT_FALSE(rec.isSetX());
  EXPECT_TRUE(rec.isSetY());
  EXPECT_FALSE(rec.isSetSpeed());
  EXPECT_FALSE(rec.isSetHeading());
  EXPECT_FALSE(rec.hasProperty("PAYLOAD"));
  EXPECT_FALSE(rec.hasProperty("NUMERIC_CUSTOM"));
  EXPECT_EQ(rec.getProperty("PAYLOAD"), "");
}

// Covers node record utils behavior: dispatches only brace wrapped reports to JSON parser.
TEST(NodeRecordUtilsTest, DispatchesOnlyBraceWrappedReportsToJsonParser)
{
  NodeRecord json = string2NodeRecord("{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\","
                                      "\"SPD\":\"3\",\"HDG\":\"4\"}");
  EXPECT_TRUE(json.valid());
  EXPECT_EQ(json.getName(), "alpha");

  NodeRecord csp = string2NodeRecord("NAME=bravo,X=1,Y=2,SPD=3,HDG=4");
  EXPECT_TRUE(csp.valid());
  EXPECT_EQ(csp.getName(), "bravo");

  NodeRecord not_json = string2NodeRecord("\"NAME\":\"charlie\",\"X\":\"1\"");
  EXPECT_FALSE(not_json.valid());
  EXPECT_EQ(not_json.getName(), "");
}

// Covers node record utils behavior: JSON parser accepts unbraced input when called directly.
TEST(NodeRecordUtilsTest, JsonParserAcceptsUnbracedInputWhenCalledDirectly)
{
  NodeRecord rec = string2NodeRecordJSON(
      "\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\"");

  EXPECT_TRUE(rec.valid());
  EXPECT_EQ(rec.getName(), "alpha");
  EXPECT_DOUBLE_EQ(rec.getX(), 1);
  EXPECT_DOUBLE_EQ(rec.getY(), 2);
  EXPECT_DOUBLE_EQ(rec.getSpeed(), 3);
  EXPECT_DOUBLE_EQ(rec.getHeading(), 4);
}

// Covers node record utils behavior: JSON parser cannot preserve trajectory values containing commas.
TEST(NodeRecordUtilsTest, JsonParserCannotPreserveTrajectoryValuesContainingCommas)
{
  NodeRecord rec = string2NodeRecordJSON(
      "{\"NAME\":\"alpha\",\"X\":\"1\",\"Y\":\"2\",\"SPD\":\"3\",\"HDG\":\"4\","
      "\"TRAJECTORY\":\"0,0:1,1\"}");

  EXPECT_TRUE(rec.valid());
  EXPECT_FALSE(rec.isSetTrajectory());
  EXPECT_EQ(rec.getTrajectory(), "");
}

// Covers node record utils behavior: computes range COG and contact extrapolation.
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

// Covers node record utils behavior: range is symmetric for negative coordinates.
TEST(NodeRecordUtilsTest, RangeIsSymmetricForNegativeCoordinates)
{
  NodeRecord first("abe", "UUV");
  first.setX(-10);
  first.setY(5);
  NodeRecord second("ben", "UUV");
  second.setX(2);
  second.setY(-11);

  EXPECT_DOUBLE_EQ(rangeBetweenRecords(first, second), 20);
  EXPECT_DOUBLE_EQ(rangeBetweenRecords(second, first), 20);
}

// Covers node record utils behavior: extrapolates cardinal headings using marine convention.
TEST(NodeRecordUtilsTest, ExtrapolatesCardinalHeadingsUsingMarineConvention)
{
  struct Case {
    double heading;
    double expected_x;
    double expected_y;
  };
  const std::vector<Case> cases = {
      {0, 0, 10},
      {90, 10, 0},
      {180, 0, -10},
      {270, -10, 0},
  };

  for(const Case& test_case : cases) {
    NodeRecord moving("ben", "UUV");
    moving.setX(0);
    moving.setY(0);
    moving.setSpeed(2);
    moving.setHeading(test_case.heading);
    moving.setTimeStamp(10);

    NodeRecord projected = extrapolateRecord(moving, 15);
    EXPECT_NEAR(projected.getX(), test_case.expected_x, 1e-6)
        << "heading=" << test_case.heading;
    EXPECT_NEAR(projected.getY(), test_case.expected_y, 1e-6)
        << "heading=" << test_case.heading;
  }
}

// Covers node record utils behavior: extrapolation sanity checks return original record.
TEST(NodeRecordUtilsTest, ExtrapolationSanityChecksReturnOriginalRecord)
{
  NodeRecord moving("ben", "UUV");
  moving.setX(5);
  moving.setY(-5);
  moving.setSpeed(2);
  moving.setHeading(45);
  moving.setTimeStamp(10);

  EXPECT_DOUBLE_EQ(extrapolateRecord(moving, 0).getX(), 5);
  EXPECT_DOUBLE_EQ(extrapolateRecord(moving, -1).getY(), -5);
  EXPECT_DOUBLE_EQ(extrapolateRecord(moving, 10).getX(), 5);
  EXPECT_DOUBLE_EQ(extrapolateRecord(moving, 10.05).getY(), -5);

  NodeRecord no_timestamp("ben", "UUV");
  no_timestamp.setX(0);
  no_timestamp.setY(0);
  no_timestamp.setSpeed(2);
  no_timestamp.setHeading(90);
  NodeRecord projected = extrapolateRecord(no_timestamp, 5);
  EXPECT_NEAR(projected.getX(), 10, 1e-6);
  EXPECT_NEAR(projected.getY(), 0, 1e-6);
}

// Covers node record utils behavior: extrapolation honors zero and negative decay inputs.
TEST(NodeRecordUtilsTest, ExtrapolationHonorsZeroAndNegativeDecayInputs)
{
  NodeRecord moving("ben", "UUV");
  moving.setX(0);
  moving.setY(0);
  moving.setSpeed(2);
  moving.setHeading(90);
  moving.setTimeStamp(10);

  NodeRecord no_motion = extrapolateRecord(moving, 20, 0);
  EXPECT_NEAR(no_motion.getX(), 0, 1e-6);
  EXPECT_NEAR(no_motion.getY(), 0, 1e-6);

  NodeRecord negative_decay_is_ignored = extrapolateRecord(moving, 20, -1);
  EXPECT_NEAR(negative_decay_is_ignored.getX(), 20, 1e-6);
  EXPECT_NEAR(negative_decay_is_ignored.getY(), 0, 1e-6);
}

// Covers node record utils behavior: COG record covers cardinal and stationary cases.
TEST(NodeRecordUtilsTest, CogRecordCoversCardinalAndStationaryCases)
{
  NodeRecord prev("ben", "UUV");
  prev.setX(10);
  prev.setY(10);

  NodeRecord north("ben", "UUV");
  north.setX(10);
  north.setY(20);
  ASSERT_TRUE(cogRecord(prev, north));
  EXPECT_DOUBLE_EQ(north.getCourseOG(), 0);

  NodeRecord west("ben", "UUV");
  west.setX(0);
  west.setY(10);
  ASSERT_TRUE(cogRecord(prev, west));
  EXPECT_DOUBLE_EQ(west.getCourseOG(), 270);

  NodeRecord stationary("ben", "UUV");
  stationary.setX(10);
  stationary.setY(10);
  ASSERT_TRUE(cogRecord(prev, stationary));
  EXPECT_DOUBLE_EQ(stationary.getCourseOG(), 0);
}

// Covers node record utils behavior: COG record rejects partial coordinates on either record.
TEST(NodeRecordUtilsTest, CogRecordRejectsPartialCoordinatesOnEitherRecord)
{
  NodeRecord prev_missing_y("ben", "UUV");
  prev_missing_y.setX(0);
  NodeRecord curr("ben", "UUV");
  curr.setX(1);
  curr.setY(1);
  EXPECT_FALSE(cogRecord(prev_missing_y, curr));
  EXPECT_FALSE(curr.isSetCourseOG());

  NodeRecord prev("ben", "UUV");
  prev.setX(0);
  prev.setY(0);
  NodeRecord curr_missing_x("ben", "UUV");
  curr_missing_x.setY(1);
  EXPECT_FALSE(cogRecord(prev, curr_missing_x));
  EXPECT_FALSE(curr_missing_x.isSetCourseOG());
}

// Covers node record utils behavior: caps stale NODE_REPORT projection at max delta decay.
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
