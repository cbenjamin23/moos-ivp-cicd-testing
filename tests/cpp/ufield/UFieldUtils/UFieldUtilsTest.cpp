#include <gtest/gtest.h>

#include <string>

#include "CommsCatalog.h"
#include "CommsEntry.h"
#include "UFieldUtils.h"

// Covers u field utils behavior: validates literal p share routes.
TEST(UFieldUtilsTest, ValidatesLiteralPShareRoutes)
{
  EXPECT_TRUE(isValidPShareRoute("192.168.1.10:9200"));
  EXPECT_TRUE(isValidPShareRoute("localhost:9200"));
  EXPECT_TRUE(isValidPShareRoute("multicast_8"));
  EXPECT_TRUE(isValidPShareRoute("multicast_0"));

  EXPECT_FALSE(isValidPShareRoute("192.168.1.10"));
  EXPECT_FALSE(isValidPShareRoute("192.168.1.10:udp"));
  EXPECT_FALSE(isValidPShareRoute("broadcast_8"));
  EXPECT_FALSE(isValidPShareRoute("multicast_alpha"));
  EXPECT_FALSE(isValidPShareRoute("192.168.1.10:9200:extra"));
}

// Covers u field utils behavior: pins p share route case whitespace and numeric permissiveness.
TEST(UFieldUtilsTest, PinsPShareRouteCaseWhitespaceAndNumericPermissiveness)
{
  // Literal pShare route validation is intentionally permissive in some places
  // because it uses lightweight token checks rather than full socket parsing.
  EXPECT_FALSE(isValidPShareRoute(" multicast_8"));
  EXPECT_FALSE(isValidPShareRoute("MULTICAST_8"));
  EXPECT_TRUE(isValidPShareRoute("multicast_8 "));
  EXPECT_TRUE(isValidPShareRoute("multicast_-1"));

  EXPECT_TRUE(isValidPShareRoute(" 127.0.0.1:9200"));
  EXPECT_TRUE(isValidPShareRoute("127.0.0.1:9200 "));
  EXPECT_TRUE(isValidPShareRoute("127.0.0.1:-9200"));
  EXPECT_TRUE(isValidPShareRoute("127.0.0.1:9200.5"));
}

// Covers u field utils behavior: resolves hostname routes in mutable overload.
TEST(UFieldUtilsTest, ResolvesHostnameRoutesInMutableOverload)
{
  std::string err;
  std::string route = "127.0.0.1:9200";
  EXPECT_TRUE(isValidPShareRoute(route, err));
  EXPECT_EQ(route, "127.0.0.1:9200");
  EXPECT_EQ(err, "");

  route = "localhost:9200";
  err.clear();
  EXPECT_TRUE(isValidPShareRoute(route, err));
  EXPECT_NE(route.find("9200"), std::string::npos);

  route = "definitely-not-a-real-moos-host.invalid:9200";
  err.clear();
  EXPECT_FALSE(isValidPShareRoute(route, err));
  EXPECT_NE(err.find("Failed to resolve hostname"), std::string::npos);
}

// Covers u field utils behavior: resolves domain names and reports failures.
TEST(UFieldUtilsTest, ResolvesDomainNamesAndReportsFailures)
{
  std::string err;
  std::string localhost = resolveDomainName("localhost", err);
  EXPECT_TRUE(localhost == "127.0.0.1" || localhost == "::1");
  EXPECT_EQ(err, "");

  err.clear();
  EXPECT_EQ(resolveDomainName("definitely-not-a-real-moos-host.invalid", err), "");
  EXPECT_NE(err.find("Failed to resolve hostname"), std::string::npos);
}

// Covers u field utils behavior: mutable route validation preserves multicast and failure inputs.
TEST(UFieldUtilsTest, MutableRouteValidationPreservesMulticastAndFailureInputs)
{
  std::string err;
  std::string route = "multicast_9";
  EXPECT_TRUE(isValidPShareRoute(route, err));
  EXPECT_EQ(route, "multicast_9");
  EXPECT_EQ(err, "");

  route = "127.0.0.1:udp";
  err = "previous";
  EXPECT_FALSE(isValidPShareRoute(route, err));
  EXPECT_EQ(route, "127.0.0.1:udp");
  EXPECT_EQ(err, "previous");

  route = ":9200";
  err.clear();
  EXPECT_FALSE(isValidPShareRoute(route, err));
  EXPECT_NE(err.find("Failed to resolve hostname"), std::string::npos);
}

// Covers comms entry behavior: serializes and parses comms catalog entries.
TEST(CommsEntryTest, SerializesAndParsesCommsCatalogEntries)
{
  CommsEntry entry(12.34567, -8.7654, 100.12549, 42);
  EXPECT_EQ(entry.getSpec(), "x=12.346,y=-8.765,utc=100.125,id=42");

  CommsEntry parsed = stringToCommsEntry(entry.getSpec());
  EXPECT_DOUBLE_EQ(parsed.getX(), 12.346);
  EXPECT_DOUBLE_EQ(parsed.getY(), -8.765);
  EXPECT_DOUBLE_EQ(parsed.getUTC(), 100.125);
  EXPECT_EQ(parsed.getID(), 42u);
}

// Covers comms entry behavior: setters feed stable comms entry spec.
TEST(CommsEntryTest, SettersFeedStableCommsEntrySpec)
{
  CommsEntry entry;
  entry.setXY(-1.23456, 2.34567);
  entry.setUTC(123456.7894);
  entry.setID(7);
  EXPECT_EQ(entry.getSpec(), "x=-1.235,y=2.346,utc=123456.789,id=7");

  entry.setX(0);
  entry.setY(-0.0049);
  entry.setUTC(-1.2345);
  entry.setID(0);
  EXPECT_EQ(entry.getSpec(), "x=0,y=-0.005,utc=-1.234,id=0");
}

// Covers comms entry behavior: parses out of order duplicates and decimal ids.
TEST(CommsEntryTest, ParsesOutOfOrderDuplicatesAndDecimalIds)
{
  // CommsEntry parsing accepts out-of-order fields, resolves duplicates by last
  // value, and uses atoi-style conversion for ids.
  CommsEntry out_of_order = stringToCommsEntry("id=4,utc=3,y=2,x=1");
  EXPECT_DOUBLE_EQ(out_of_order.getX(), 1);
  EXPECT_DOUBLE_EQ(out_of_order.getY(), 2);
  EXPECT_DOUBLE_EQ(out_of_order.getUTC(), 3);
  EXPECT_EQ(out_of_order.getID(), 4u);

  CommsEntry duplicate = stringToCommsEntry("x=1,y=2,utc=3,id=4,x=5,id=6");
  EXPECT_DOUBLE_EQ(duplicate.getX(), 5);
  EXPECT_EQ(duplicate.getID(), 6u);

  CommsEntry decimal_id = stringToCommsEntry("x=1,y=2,utc=3,id=4.9");
  EXPECT_EQ(decimal_id.getID(), 4u);
}

// Covers comms entry behavior: rejects malformed specs by returning default entry.
TEST(CommsEntryTest, RejectsMalformedSpecsByReturningDefaultEntry)
{
  CommsEntry missing = stringToCommsEntry("x=1,y=2,utc=3");
  EXPECT_DOUBLE_EQ(missing.getX(), 0);
  EXPECT_DOUBLE_EQ(missing.getY(), 0);
  EXPECT_DOUBLE_EQ(missing.getUTC(), 0);
  EXPECT_EQ(missing.getID(), 0u);

  CommsEntry bad_value = stringToCommsEntry("x=1,y=two,utc=3,id=4");
  EXPECT_DOUBLE_EQ(bad_value.getX(), 0);
  EXPECT_EQ(bad_value.getID(), 0u);

  CommsEntry unknown = stringToCommsEntry("x=1,y=2,utc=3,id=4,node=abe");
  EXPECT_DOUBLE_EQ(unknown.getX(), 0);
  EXPECT_EQ(unknown.getID(), 0u);
}

// Covers comms entry behavior: pins case and whitespace sensitive parsing.
TEST(CommsEntryTest, PinsCaseAndWhitespaceSensitiveParsing)
{
  // Field names are case-sensitive, but whitespace around comma-delimited
  // tokens is trimmed before lookup.
  EXPECT_EQ(stringToCommsEntry("X=1,y=2,utc=3,id=4").getID(), 0u);
  EXPECT_EQ(stringToCommsEntry(" x=1,y=2,utc=3,id=4").getID(), 4u);
  EXPECT_EQ(stringToCommsEntry("x=1, y=2,utc=3,id=4").getID(), 4u);
  EXPECT_EQ(stringToCommsEntry("x=1,y=2,utc=3,id=4,").getID(), 4u);
}

// Covers comms entry behavior: parses negative id through unsigned atoi quirk.
TEST(CommsEntryTest, ParsesNegativeIdThroughUnsignedAtoiQuirk)
{
  // The parser converts through an unsigned id, so negative text wraps.
  CommsEntry entry = stringToCommsEntry("x=1,y=2,utc=3,id=-4");
  EXPECT_DOUBLE_EQ(entry.getX(), 1);
  EXPECT_DOUBLE_EQ(entry.getY(), 2);
  EXPECT_DOUBLE_EQ(entry.getUTC(), 3);
  EXPECT_EQ(entry.getID(), 4294967292u);
}

// Covers comms catalog behavior: can accept entries without observable state.
TEST(CommsCatalogTest, CanAcceptEntriesWithoutObservableState)
{
  CommsCatalog catalog;
  catalog.addEntry(0, 0, 100, 1);
  catalog.addEntry(10, 20, 105, 2);
  SUCCEED();
}
