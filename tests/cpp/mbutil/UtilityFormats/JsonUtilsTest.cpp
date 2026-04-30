#include <gtest/gtest.h>

#include <string>

#include "JsonUtils.h"

// Covers JSON utils behavior: converts JSON report payloads to comma separated pairs.
TEST(JsonUtilsTest, ConvertsJsonReportPayloadsToCommaSeparatedPairs)
{
  EXPECT_EQ(jsonToCsp("{\"TargetID\":\"38886\", \"IsValid\":\"true\"}"),
            "TargetID=38886,IsValid=true");
  EXPECT_EQ(jsonToCsp("{\"x\":\"24\", \"y\":\"129\", \"name\":\"alpha\"}"),
            "x=24,y=129,name=alpha");

  const std::string detached_log_payload =
      "{\"temp\":\"98.5\", \"fuel\":\"14.5\", \"errs\":\"11\"}";
  EXPECT_EQ(jsonToCsp(detached_log_payload), "temp=98.5,fuel=14.5,errs=11");
}

// Covers JSON utils behavior: converts whitespace wrapped and bare JSON payloads.
TEST(JsonUtilsTest, ConvertsWhitespaceWrappedAndBareJsonPayloads)
{
  EXPECT_EQ(jsonToCsp(" { \"src\" : \"abe\" , \"dest\" : \"ben\" } "),
            "src=abe,dest=ben");
  EXPECT_EQ(jsonToCsp("\"TargetID\":\"38886\",\"IsValid\":\"true\""),
            "TargetID\"=\"38886\",\"IsValid\":\"true");
  EXPECT_EQ(jsonToCsp("\"{\\\"mode\\\":\\\"survey\\\"}\""),
            "\\\"mode\\\"=\\\"survey\\\"");
}

// Covers JSON utils behavior: preserves braced and quoted commas when converting from JSON.
TEST(JsonUtilsTest, PreservesBracedAndQuotedCommasWhenConvertingFromJson)
{
  EXPECT_EQ(jsonToCsp("{\"x\":\"24\", \"names\":\"{abe,ben,cal}\"}"),
            "x=24,names={abe,ben,cal}");
  EXPECT_EQ(jsonToCsp("{\"src\":\"abe\", \"note\":\"hold,station\"}"),
            "src=abe,note=hold,station");
  EXPECT_EQ(jsonToCsp("{\"region\":\"pts={0,0:10,0:10,10:0,10}\"}"),
            "region=pts={0,0:10,0:10,10:0,10}");
}

// Covers JSON utils behavior: converts NODE_REPORT csp to JSON for pNodeReporter.
TEST(JsonUtilsTest, ConvertsNodeReportCspToJsonForPNodeReporter)
{
  const std::string report =
      "NAME=abe,X=12.5,Y=-3.25,SPD=2.1,MODE=SURVEY,ALLSTOP=false";
  EXPECT_EQ(cspToJson(report),
            "{\"NAME\":\"abe\",\"X\":12.5,\"Y\":-3.25,\"SPD\":2.1,"
            "\"MODE\":\"SURVEY\",\"ALLSTOP\":\"false\"}");

  EXPECT_EQ(cspToJson("\"NAME=abe,X=1\""), "{\"NAME\":\"abe\",\"X\":1}");
  EXPECT_EQ(cspToJson("{NAME=abe,X=1}"), "");
}

// Covers JSON utils behavior: converts pNodeReporter-like numeric and string fields.
TEST(JsonUtilsTest, ConvertsPNodeReporterLikeNumericAndStringFields)
{
  const std::string node_report =
      "NAME=abe,TYPE=kayak,TIME=1737476534.76,X=-12.5,Y=24.75,LAT=42.358456,"
      "LON=-71.087589,SPD=1.4,HDG=270,DEPTH=0,MODE=ACTIVE:SURVEYING";

  EXPECT_EQ(cspToJson(node_report),
            "{\"NAME\":\"abe\",\"TYPE\":\"kayak\",\"TIME\":1737476534.76,"
            "\"X\":-12.5,\"Y\":24.75,\"LAT\":42.358456,\"LON\":-71.087589,"
            "\"SPD\":1.4,\"HDG\":270,\"DEPTH\":0,"
            "\"MODE\":\"ACTIVE:SURVEYING\"}");
}

// Covers JSON utils behavior: converts mission status csp with quoted keys and numeric strings.
TEST(JsonUtilsTest, ConvertsMissionStatusCspWithQuotedKeysAndNumericStrings)
{
  EXPECT_EQ(cspToJson("\"TargetID\"=38886,\"IsValid\"=true"),
            "{\"TargetID\":38886,\"IsValid\":\"true\"}");
  EXPECT_EQ(cspToJson("WARN=\"low_battery\",COUNT=\"007\""),
            "{\"WARN\":\"low_battery\",\"COUNT\":\"007\"}");
  EXPECT_EQ(cspToJson("MODE=loiter,DEPLOY=true,UTC=1737476534.76"),
            "{\"MODE\":\"loiter\",\"DEPLOY\":\"true\",\"UTC\":1737476534.76}");
}

// Covers JSON utils behavior: pins comma splitting quirk for csp to JSON.
TEST(JsonUtilsTest, PinsCommaSplittingQuirkForCspToJson)
{
  EXPECT_EQ(cspToJson("NAME=abe,NOTE=\"hold,station\""),
            "{\"NAME\":\"abe\",\"NOTE\":\"\"hold\",\"station\"\":\"\"}");
  EXPECT_EQ(cspToJson("NAMES={abe,ben,cal}"),
            "{\"NAMES\":\"{abe\",\"ben\":\"\",\"cal}\":\"\"}");
}

// Covers JSON utils behavior: pins empty and malformed payload behavior.
TEST(JsonUtilsTest, PinsEmptyAndMalformedPayloadBehavior)
{
  EXPECT_EQ(jsonToCsp("{}"), "");
  EXPECT_EQ(jsonToCsp(""), "");
  EXPECT_EQ(jsonToCsp("{\"MODE\"}"), "MODE=");
  EXPECT_EQ(jsonToCsp("{\"MODE\":\"ACTIVE:SURVEYING\"}"),
            "MODE=ACTIVE:SURVEYING");

  EXPECT_EQ(cspToJson("MODE"), "{\"MODE\":\"\"}");
  EXPECT_EQ(cspToJson(""), "{}");
  EXPECT_EQ(cspToJson("MODE=ACTIVE:LOITER"), "{\"MODE\":\"ACTIVE:LOITER\"}");
}
