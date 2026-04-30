#include <gtest/gtest.h>

#include <string>

#include "HostRecord.h"
#include "HostRecordUtils.h"

// Covers host record behavior: builds phi host info style specs.
TEST(HostRecordTest, BuildsPhiHostInfoStyleSpecs)
{
  HostRecord record;
  record.set("abe", "192.168.1.112", "9001", "9201", "2");
  record.setKeyword("alpha");
  record.setKey("broker-key");
  record.setStatus("ready");
  record.setTimeStamp("123.456");

  EXPECT_TRUE(record.valid());
  EXPECT_TRUE(record.valid("port_udp"));
  EXPECT_TRUE(record.valid("time_warp"));
  EXPECT_EQ(record.getSpecTerse(),
            "community=abe,hostip=192.168.1.112,port_db=9001,port_udp=9201,"
            "keyword=alpha");
  EXPECT_EQ(record.getSpec(),
            "community=abe,hostip=192.168.1.112,port_db=9001,port_udp=9201,"
            "keyword=alpha,timewarp=2,status=2,time=123.456,key=broker-key");
}

// Covers host record behavior: removes primary host from alternate host list.
TEST(HostRecordTest, RemovesPrimaryHostFromAlternateHostList)
{
  HostRecord record;
  record.set("ben", "10.0.0.5", "9002");
  record.setHostIPAlts("10.0.0.5,10.0.0.6,10.0.0.7");

  EXPECT_EQ(record.getSpecTerse(),
            "community=ben,hostip=10.0.0.5,port_db=9002,"
            "hostip_alts=10.0.0.6,10.0.0.7");

  record.setHostIPAlts("10.0.0.6,10.0.0.5");
  EXPECT_EQ(record.getSpecTerse(),
            "community=ben,hostip=10.0.0.5,port_db=9002,"
            "hostip_alts=10.0.0.6,");
}

// Covers host record behavior: extracts udp port from p share i route when unset.
TEST(HostRecordTest, ExtractsUdpPortFromPShareIRouteWhenUnset)
{
  HostRecord record;
  record.set("alpha", "192.168.7.6", "9000");
  record.setPShareIRoutes("192.168.7.1:9200");

  EXPECT_EQ(record.getPortUDP(), "9200");
  EXPECT_TRUE(record.valid("pshare_iroutes"));
  EXPECT_TRUE(record.valid("port_udp"));
  EXPECT_EQ(record.getSpecTerse(),
            "community=alpha,hostip=192.168.7.6,port_db=9000,port_udp=9200,"
            "pshare_iroutes=192.168.7.1:9200");

  record.setPShareIRoutes("multicast_8");
  EXPECT_EQ(record.getPortUDP(), "9200");
  EXPECT_EQ(record.getPShareIRoutes(), "multicast_8");
}

// Covers host record behavior: validates required fields and optional checks.
TEST(HostRecordTest, ValidatesRequiredFieldsAndOptionalChecks)
{
  HostRecord record;
  EXPECT_FALSE(record.valid());

  record.setCommunity("abe");
  record.setHostIP("not-an-ip");
  record.setPortDB("9001");
  EXPECT_FALSE(record.valid());

  record.setHostIP("127.0.0.1");
  EXPECT_TRUE(record.valid());
  EXPECT_FALSE(record.valid("port_udp"));
  record.setPortUDP("9201");
  EXPECT_TRUE(record.valid("port_udp"));
  record.setPortUDP("udp");
  EXPECT_FALSE(record.valid("port_udp"));

  record.setPortUDP("9201");
  EXPECT_FALSE(record.valid("time_warp"));
  record.setTimeWarp("fast");
  EXPECT_FALSE(record.valid("time_warp"));
  record.setTimeWarp("5");
  EXPECT_TRUE(record.valid("time_warp"));
}

// Covers host record behavior: pins status serialization and i route port extraction rules.
TEST(HostRecordTest, PinsStatusSerializationAndIRoutePortExtractionRules)
{
  // Status text is preserved for accessors, but serialization currently emits
  // the timewarp value in the status field.
  HostRecord record;
  record.set("abe", "127.0.0.1", "9001", "", "7");
  record.setStatus("ready");
  EXPECT_EQ(record.getStatus(), "ready");
  EXPECT_EQ(record.getSpec(),
            "community=abe,hostip=127.0.0.1,port_db=9001,timewarp=7,status=7");

  record.setPShareIRoutes("localhost:udp");
  EXPECT_EQ(record.getPortUDP(), "");
  EXPECT_FALSE(record.valid("port_udp"));
  EXPECT_TRUE(record.valid("pshare_iroutes"));

  // A numeric pShare route port backfills port_udp when it was unset.
  record.setPShareIRoutes("localhost:9201");
  EXPECT_EQ(record.getPortUDP(), "9201");
  EXPECT_TRUE(record.valid("port_udp"));
}

// Covers host record utils behavior: parses phi host info and node broker payloads.
TEST(HostRecordUtilsTest, ParsesPhiHostInfoAndNodeBrokerPayloads)
{
  HostRecord phi = string2HostRecord(
      "community=alpha,hostip=123.1.1.0,port_db=9000,port_udp=9200,"
      "timewarp=2,keyword=lemon,key=1,status=ok,time=12.5");

  EXPECT_TRUE(phi.valid("port_udp"));
  EXPECT_EQ(phi.getCommunity(), "alpha");
  EXPECT_EQ(phi.getHostIP(), "123.1.1.0");
  EXPECT_EQ(phi.getPortDB(), "9000");
  EXPECT_EQ(phi.getPortUDP(), "9200");
  EXPECT_EQ(phi.getTimeWarp(), "2");
  EXPECT_EQ(phi.getKeyword(), "lemon");
  EXPECT_EQ(phi.getKey(), "1");
  EXPECT_EQ(phi.getStatus(), "ok");
  EXPECT_EQ(phi.getTimeStamp(), "12.5");

  HostRecord node_broker_ping = string2HostRecord(
      "vname=henry,hostip=12.16.1.22,port_db=9000,pshare_iroutes=12.16.1.22:9200");
  EXPECT_TRUE(node_broker_ping.valid("pshare_iroutes"));
  EXPECT_EQ(node_broker_ping.getCommunity(), "henry");
  EXPECT_EQ(node_broker_ping.getPortUDP(), "9200");
}

// Covers host record utils behavior: rejects malformed records by returning empty record.
TEST(HostRecordUtilsTest, RejectsMalformedRecordsByReturningEmptyRecord)
{
  EXPECT_FALSE(string2HostRecord("").valid());
  EXPECT_FALSE(string2HostRecord("community=abe,hostip=bad,port_db=9000").valid());
  EXPECT_FALSE(string2HostRecord("community=abe,hostip=127.0.0.1,port_db=db").valid());

  HostRecord ignores_unknown = string2HostRecord(
      "community=abe,hostip=127.0.0.1,port_db=9000,unknown=value");
  EXPECT_TRUE(ignores_unknown.valid());
}

// Covers host record utils behavior: parses aliases case insensitively and last field wins.
TEST(HostRecordUtilsTest, ParsesAliasesCaseInsensitivelyAndLastFieldWins)
{
  // NodeBroker/PHI host records accept several aliases; duplicate canonical
  // fields are resolved by the last parsed value.
  HostRecord record = string2HostRecord(
      "VNAME=abe,HOSTIP=127.0.0.1,PORT_DB=9001,PORT_UDP=9201,"
      "community=ben,port_db=9002,hostip_alts=127.0.0.1,10.0.0.2");

  EXPECT_TRUE(record.valid("port_udp"));
  EXPECT_EQ(record.getCommunity(), "ben");
  EXPECT_EQ(record.getPortDB(), "9002");
  EXPECT_EQ(record.getSpecTerse(),
            "community=ben,hostip=127.0.0.1,port_db=9002,port_udp=9201,"
            "hostip_alts=");
}
