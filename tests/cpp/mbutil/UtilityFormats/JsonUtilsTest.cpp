#include <gtest/gtest.h>

#include <string>

#include "JsonUtils.h"

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

TEST(JsonUtilsTest, ConvertsWhitespaceWrappedAndBareJsonPayloads)
{
  EXPECT_EQ(jsonToCsp(" { \"src\" : \"abe\" , \"dest\" : \"ben\" } "),
            "src=abe,dest=ben");
  EXPECT_EQ(jsonToCsp("\"TargetID\":\"38886\",\"IsValid\":\"true\""),
            "TargetID\"=\"38886\",\"IsValid\":\"true");
  EXPECT_EQ(jsonToCsp("\"{\\\"mode\\\":\\\"survey\\\"}\""),
            "\\\"mode\\\"=\\\"survey\\\"");
}

TEST(JsonUtilsTest, PreservesBracedAndQuotedCommasWhenConvertingFromJson)
{
  EXPECT_EQ(jsonToCsp("{\"x\":\"24\", \"names\":\"{abe,ben,cal}\"}"),
            "x=24,names={abe,ben,cal}");
  EXPECT_EQ(jsonToCsp("{\"src\":\"abe\", \"note\":\"hold,station\"}"),
            "src=abe,note=hold,station");
  EXPECT_EQ(jsonToCsp("{\"region\":\"pts={0,0:10,0:10,10:0,10}\"}"),
            "region=pts={0,0:10,0:10,10:0,10}");
}

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

TEST(JsonUtilsTest, ConvertsMissionStatusCspWithQuotedKeysAndNumericStrings)
{
  EXPECT_EQ(cspToJson("\"TargetID\"=38886,\"IsValid\"=true"),
            "{\"TargetID\":38886,\"IsValid\":\"true\"}");
  EXPECT_EQ(cspToJson("WARN=\"low_battery\",COUNT=\"007\""),
            "{\"WARN\":\"low_battery\",\"COUNT\":\"007\"}");
  EXPECT_EQ(cspToJson("MODE=loiter,DEPLOY=true,UTC=1737476534.76"),
            "{\"MODE\":\"loiter\",\"DEPLOY\":\"true\",\"UTC\":1737476534.76}");
}

TEST(JsonUtilsTest, PinsCommaSplittingQuirkForCspToJson)
{
  EXPECT_EQ(cspToJson("NAME=abe,NOTE=\"hold,station\""),
            "{\"NAME\":\"abe\",\"NOTE\":\"\"hold\",\"station\"\":\"\"}");
  EXPECT_EQ(cspToJson("NAMES={abe,ben,cal}"),
            "{\"NAMES\":\"{abe\",\"ben\":\"\",\"cal}\":\"\"}");
}
