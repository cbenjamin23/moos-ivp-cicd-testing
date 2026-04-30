#include <gtest/gtest.h>

#include <string>

#include "CPAEvent.h"

// Covers CPA event behavior: defaults represent empty encounter.
TEST(CPAEventTest, DefaultsRepresentEmptyEncounter)
{
  CPAEvent event;
  EXPECT_EQ(event.getVName1(), "");
  EXPECT_EQ(event.getVName2(), "");
  EXPECT_DOUBLE_EQ(event.getCPA(), 0);
  EXPECT_DOUBLE_EQ(event.getEFF(), 0);
  EXPECT_DOUBLE_EQ(event.getX(), 0);
  EXPECT_DOUBLE_EQ(event.getY(), 0);
  EXPECT_EQ(event.getID(), -1);
  EXPECT_DOUBLE_EQ(event.getAlpha(), -1);
  EXPECT_DOUBLE_EQ(event.getBeta(), -1);
  EXPECT_EQ(event.getSpec(), "cpa=0");
}

// Covers CPA event behavior: serializes collision detector encounter fields.
TEST(CPAEventTest, SerializesCollisionDetectorEncounterFields)
{
  CPAEvent event("abe", "ben", 4.567);
  event.setEFF(0.812);
  event.setX(10.125);
  event.setY(-3.5);
  event.setID(17);
  event.setAlpha(23.4567);
  event.setBeta(201.25);
  event.setPType("port:port");
  event.setXType("aft:fore");

  EXPECT_EQ(event.getSpec(),
            "cpa=4.57,x=10.12,y=-3.5,v1=abe,v2=ben,eff=0.81,id=17,"
            "alpha=23.457,beta=201.25");
  EXPECT_EQ(event.getPType(), "port:port");
  EXPECT_EQ(event.getXType(), "aft:fore");
}

// Covers CPA event behavior: parses alog encounter payload used by encounter plots.
TEST(CPAEventTest, ParsesAlogEncounterPayloadUsedByEncounterPlots)
{
  CPAEvent event("v1=abe,v2=ben,cpa=6.25,eff=0.5,x=12,y=-4,id=9,"
                 "alpha=45,beta=225");

  EXPECT_EQ(event.getVName1(), "abe");
  EXPECT_EQ(event.getVName2(), "ben");
  EXPECT_DOUBLE_EQ(event.getCPA(), 6.25);
  EXPECT_DOUBLE_EQ(event.getEFF(), 0.5);
  EXPECT_DOUBLE_EQ(event.getX(), 12);
  EXPECT_DOUBLE_EQ(event.getY(), -4);
  EXPECT_EQ(event.getID(), 9);
  EXPECT_DOUBLE_EQ(event.getAlpha(), 45);
  EXPECT_DOUBLE_EQ(event.getBeta(), 225);
  EXPECT_EQ(event.getSpec(),
            "cpa=6.25,x=12,y=-4,v1=abe,v2=ben,eff=0.5,id=9,"
            "alpha=45,beta=225");
}

// Covers CPA event behavior: ignores unknown fields and unserialized passing types.
TEST(CPAEventTest, IgnoresUnknownFieldsAndUnserializedPassingTypes)
{
  CPAEvent event("v1=abe,v2=ben,cpa=3.2,eff=0,x=0,y=0,id=-1,"
                 "alpha=-1,beta=-1,ptype=star:port,xtype=fore:aft,"
                 "unknown=value");

  EXPECT_EQ(event.getVName1(), "abe");
  EXPECT_EQ(event.getVName2(), "ben");
  EXPECT_DOUBLE_EQ(event.getCPA(), 3.2);
  EXPECT_EQ(event.getPType(), "");
  EXPECT_EQ(event.getXType(), "");
  EXPECT_EQ(event.getSpec(), "cpa=3.2,v1=abe,v2=ben");
}

// Covers CPA event behavior: pins optional field emission thresholds.
TEST(CPAEventTest, PinsOptionalFieldEmissionThresholds)
{
  CPAEvent event("abe", "ben", -1.234);
  event.setEFF(0);
  event.setX(0);
  event.setY(0);
  event.setID(-1);
  event.setAlpha(0);
  event.setBeta(180);
  EXPECT_EQ(event.getSpec(), "cpa=-1.23,v1=abe,v2=ben");

  event.setAlpha(0.001);
  event.setBeta(-5.25);
  EXPECT_EQ(event.getSpec(), "cpa=-1.23,v1=abe,v2=ben,alpha=0.001,beta=-5.25");
}

// Covers CPA event behavior: parses malformed numbers with c style coercion.
TEST(CPAEventTest, ParsesMalformedNumbersWithCStyleCoercion)
{
  CPAEvent event("v1=abe,v2=ben,cpa=abc,eff=2meters,x=1.25m,y=-4.5,id=12x,"
                 "alpha=bad,beta=270deg");

  EXPECT_DOUBLE_EQ(event.getCPA(), 0);
  EXPECT_DOUBLE_EQ(event.getEFF(), 2);
  EXPECT_DOUBLE_EQ(event.getX(), 1.25);
  EXPECT_DOUBLE_EQ(event.getY(), -4.5);
  EXPECT_EQ(event.getID(), 12);
  EXPECT_DOUBLE_EQ(event.getAlpha(), 0);
  EXPECT_DOUBLE_EQ(event.getBeta(), 270);
  EXPECT_EQ(event.getSpec(), "cpa=0,x=1.25,y=-4.5,v1=abe,v2=ben,eff=2,id=12");
}

// Covers CPA event behavior: pins case sensitive parsing and beta emission dependency.
TEST(CPAEventTest, PinsCaseSensitiveParsingAndBetaEmissionDependency)
{
  CPAEvent parsed("V1=ABE,v2=ben,CPA=4.5,cpa=3.5,eff=0,x=0,y=0,id=-1,"
                  "alpha=0,beta=180");
  EXPECT_EQ(parsed.getVName1(), "");
  EXPECT_EQ(parsed.getVName2(), "ben");
  EXPECT_DOUBLE_EQ(parsed.getCPA(), 3.5);
  EXPECT_DOUBLE_EQ(parsed.getAlpha(), 0);
  EXPECT_DOUBLE_EQ(parsed.getBeta(), 180);
  EXPECT_EQ(parsed.getSpec(), "cpa=3.5,v2=ben");

  parsed.setAlpha(0.01);
  EXPECT_EQ(parsed.getSpec(), "cpa=3.5,v2=ben,alpha=0.01,beta=180");
}

// Covers CPA event behavior: allows setter only passing metadata without serialization.
TEST(CPAEventTest, AllowsSetterOnlyPassingMetadataWithoutSerialization)
{
  CPAEvent event("abe", "ben", 5.0);
  event.setPType("star:port");
  event.setXType("fore:aft");

  EXPECT_EQ(event.getPType(), "star:port");
  EXPECT_EQ(event.getXType(), "fore:aft");
  EXPECT_EQ(event.getSpec(), "cpa=5,v1=abe,v2=ben");
}
