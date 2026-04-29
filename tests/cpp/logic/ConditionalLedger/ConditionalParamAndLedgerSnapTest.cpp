#include <gtest/gtest.h>

#include <string>

#include "ConditionalParam.h"
#include "LedgerSnap.h"

TEST(ConditionalParamTest, ParsesCombinedParamValueAndConditionSyntax)
{
  ConditionalParam param;
  EXPECT_TRUE(param.setFromString("speed = 2.0 [MODE=ACTIVE] // comment"));
  EXPECT_TRUE(param.ok());
  EXPECT_EQ(param.getParam(), "speed");
  EXPECT_EQ(param.getParamVal(), "2.0");

  // Current implementation parses the pieces but does not install the
  // condition on the one-string overload.
  EXPECT_FALSE(param.getCondition().eval());
}

TEST(ConditionalParamTest, ParsesSplitParamAndValueConditionSyntax)
{
  ConditionalParam param;
  EXPECT_TRUE(param.setFromString("speed", "2.0 [MODE=ACTIVE] // comment"));
  EXPECT_TRUE(param.ok());
  EXPECT_EQ(param.getParam(), "speed");
  EXPECT_EQ(param.getParamVal(), "2.0");

  // The split overload currently parses "MODE=ACTIVE]" as the condition,
  // preserving the trailing bracket as part of the literal.
  LogicCondition condition = param.getCondition();
  EXPECT_EQ(condition.getRawCondition(), "MODE=ACTIVE]");
  condition.setVarVal("MODE", "ACTIVE");
  EXPECT_FALSE(condition.eval());
  condition.setVarVal("MODE", "ACTIVE]");
  EXPECT_TRUE(condition.eval());
}

TEST(ConditionalParamTest, RejectsMalformedConditionalParamStrings)
{
  ConditionalParam param;
  EXPECT_FALSE(param.setFromString("speed = 2.0 MODE=ACTIVE"));
  EXPECT_FALSE(param.setFromString("speed", "2.0 MODE=ACTIVE"));
  EXPECT_FALSE(param.ok());
}

TEST(LedgerSnapTest, StoresContactLedgerFieldsUsedByBehaviorPopulation)
{
  LedgerSnap ledger;
  ledger.setX("abe", 10.25);
  ledger.setY("abe", -5.5);
  ledger.setHdg("abe", 270);
  ledger.setSpd("abe", 2.25);
  ledger.setDep("abe", 4.5);
  ledger.setLat("abe", 42.35);
  ledger.setLon("abe", -70.75);
  ledger.setUTC("abe", 12345.25);
  ledger.setGroup("abe", "alpha");
  ledger.setType("abe", "uuv");
  ledger.setVSource("abe", "pNodeReporter");

  bool ok = false;
  EXPECT_TRUE(ledger.hasVName("abe"));
  EXPECT_EQ(ledger.size(), 1u);
  EXPECT_DOUBLE_EQ(ledger.getInfoDouble("abe", "x", ok), 10.25);
  EXPECT_TRUE(ok);
  EXPECT_DOUBLE_EQ(ledger.getInfoDouble("abe", "y", ok), -5.5);
  EXPECT_TRUE(ok);
  EXPECT_EQ(ledger.getInfoString("abe", "group", ok), "alpha");
  EXPECT_TRUE(ok);
  EXPECT_EQ(ledger.getInfoString("abe", "type", ok), "uuv");
  EXPECT_TRUE(ok);
  EXPECT_EQ(ledger.getInfoString("abe", "vsource", ok), "pNodeReporter");
  EXPECT_TRUE(ok);
}

TEST(LedgerSnapTest, SerializesNodeSpecAndReportsMissingFields)
{
  LedgerSnap ledger;
  ledger.setX("abe", 10.25);
  ledger.setY("abe", -5.5);
  ledger.setHdg("abe", 270);
  ledger.setSpd("abe", 2.25);
  ledger.setDep("abe", 4.5);
  ledger.setLat("abe", 42.35);
  ledger.setLon("abe", -70.75);
  ledger.setUTC("abe", 12345.25);
  ledger.setGroup("abe", "alpha");
  ledger.setType("abe", "uuv");
  ledger.setVSource("abe", "pNodeReporter");

  EXPECT_EQ(ledger.getSpec("abe"),
            "x=10.25,y=-5.5,h=270,v=2.25,d=4.5,lat=42.35,lon=-70.75,"
            "utc=12345.25,grp=alpha,type=uuv,vsrc=pNodeReporter");

  bool ok = true;
  EXPECT_DOUBLE_EQ(ledger.getInfoDouble("abe", "missing", ok), 0);
  EXPECT_FALSE(ok);
  EXPECT_EQ(ledger.getInfoString("abe", "missing", ok), "");
  EXPECT_FALSE(ok);
}

TEST(LedgerSnapTest, PinsClearLeavingVSourceMapInCurrentImplementation)
{
  LedgerSnap ledger;
  ledger.setX("abe", 10.25);
  ledger.setVSource("abe", "pNodeReporter");
  ledger.clear();

  EXPECT_FALSE(ledger.hasVName("abe"));
  EXPECT_EQ(ledger.size(), 1u);
  bool ok = false;
  EXPECT_EQ(ledger.getInfoString("abe", "vsource", ok), "pNodeReporter");
  EXPECT_TRUE(ok);
}
