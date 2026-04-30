#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "ColoredPoint.h"
#include "ContactLedger.h"
#include "NodeRecord.h"
#include "NodeRecordUtils.h"

namespace {

NodeRecord makeRecord(const std::string& name,
                      double x,
                      double y,
                      double utc,
                      double spd = 0,
                      double hdg = 0,
                      const std::string& group = "red")
{
  NodeRecord record;
  record.setName(name);
  record.setX(x);
  record.setY(y);
  record.setLat(42.0 + (y * 0.00001));
  record.setLon(-70.0 + (x * 0.00001));
  record.setTimeStamp(utc);
  record.setSpeed(spd);
  record.setHeading(hdg);
  record.setGroup(group);
  return record;
}

}  // namespace

// Covers colored point behavior: tracks validity for viewer trail points.
TEST(ColoredPointTest, TracksValidityForViewerTrailPoints)
{
  ColoredPoint empty;
  EXPECT_FALSE(empty.isValid());
  empty.setValid();
  EXPECT_TRUE(empty.isValid());
  empty.setInvalid();
  EXPECT_FALSE(empty.isValid());

  ColoredPoint point(12.5, -4.25);
  EXPECT_TRUE(point.isValid());
  EXPECT_DOUBLE_EQ(point.m_x, 12.5);
  EXPECT_DOUBLE_EQ(point.m_y, -4.25);
}

// Covers contact ledger behavior: processes NODE_REPORTs and applies type defaults.
TEST(ContactLedgerTest, ProcessesNodeReportsAndAppliesTypeDefaults)
{
  ContactLedger ledger(3);
  ledger.setCurrTimeUTC(110);
  std::string whynot;
  std::string vname = ledger.processNodeReport(
      "NAME=abe,X=10,Y=20,LAT=42.1,LON=-70.2,SPD=2.5,HDG=45,UTC_TIME=100,"
      "TYPE=hovercraft,GROUP=alpha,COLOR=yellow,VSOURCE=pNodeReporter",
      whynot);

  EXPECT_EQ(vname, "abe");
  EXPECT_EQ(whynot, "");
  EXPECT_EQ(ledger.size(), 1u);
  EXPECT_EQ(ledger.totalReports(), 1u);
  EXPECT_EQ(ledger.totalReportsValid(), 1u);
  EXPECT_TRUE(ledger.hasVName("abe"));
  EXPECT_TRUE(ledger.hasVNameValid("abe"));
  EXPECT_EQ(ledger.getActiveVName(), "abe");
  EXPECT_DOUBLE_EQ(ledger.getX("abe"), 10);
  EXPECT_DOUBLE_EQ(ledger.getY("abe"), 20);
  EXPECT_DOUBLE_EQ(ledger.getSpeed("abe"), 2.5);
  EXPECT_DOUBLE_EQ(ledger.getHeading("abe"), 45);
  EXPECT_EQ(ledger.getType("abe"), "ship");
  EXPECT_EQ(ledger.getGroup("abe"), "alpha");
  EXPECT_EQ(ledger.getColor("abe"), "yellow");
  EXPECT_EQ(ledger.getVSource("abe"), "pNodeReporter");
  EXPECT_DOUBLE_EQ(ledger.getUTC("abe"), 100);
  EXPECT_DOUBLE_EQ(ledger.getUTCAge("abe"), 10);
  EXPECT_DOUBLE_EQ(ledger.getUTCReceived("abe"), 110);
  EXPECT_DOUBLE_EQ(ledger.getUTCAgeReceived("abe"), 0);
}

// Covers contact ledger behavior: rejects malformed NODE_REPORTs with reasons.
TEST(ContactLedgerTest, RejectsMalformedNodeReportsWithReasons)
{
  ContactLedger ledger;
  std::string whynot;

  EXPECT_EQ(ledger.processNodeReport("X=1,Y=2,UTC_TIME=10", whynot), "");
  EXPECT_NE(whynot.find("Missing vname."), std::string::npos);

  whynot.clear();
  EXPECT_EQ(ledger.processNodeReport("NAME=abe,X=1,Y=2", whynot), "");
  EXPECT_NE(whynot.find("Missing timestamp."), std::string::npos);

  whynot.clear();
  EXPECT_EQ(ledger.processNodeReport("NAME=abe,UTC_TIME=10", whynot), "");
  EXPECT_NE(whynot.find("Missing both x/y and lat/lon."), std::string::npos);
  EXPECT_EQ(ledger.size(), 0u);
  EXPECT_EQ(ledger.totalReports(), 0u);
}

// Covers contact ledger behavior: rejects underwater vehicle records without depth.
TEST(ContactLedgerTest, RejectsUnderwaterVehicleRecordsWithoutDepth)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(20);
  NodeRecord auv = makeRecord("uuv1", 0, 0, 10);
  auv.setType("auv");

  std::string whynot;
  EXPECT_EQ(ledger.processNodeRecord(auv, whynot), "");
  EXPECT_EQ(whynot, "underwater vehicle type with no depth reported");
  EXPECT_EQ(ledger.size(), 0u);
  EXPECT_EQ(ledger.totalReports(), 1u);
  EXPECT_EQ(ledger.totalReportsValid(), 1u);

  auv.setDepth(12);
  whynot.clear();
  EXPECT_EQ(ledger.processNodeRecord(auv, whynot), "uuv1");
  EXPECT_EQ(ledger.getType("uuv1"), "auv");
  EXPECT_DOUBLE_EQ(ledger.getDepth("uuv1"), 12);
}

// Covers contact ledger behavior: maintains history and limits to configured size.
TEST(ContactLedgerTest, MaintainsHistoryAndLimitsToConfiguredSize)
{
  ContactLedger ledger(2);
  ledger.setCurrTimeUTC(10);
  std::string whynot;
  EXPECT_EQ(ledger.processNodeRecord(makeRecord("abe", 0, 0, 1), whynot), "abe");
  EXPECT_EQ(ledger.processNodeRecord(makeRecord("abe", 1, 1, 2), whynot), "abe");
  EXPECT_EQ(ledger.processNodeRecord(makeRecord("abe", 2, 2, 3), whynot), "abe");

  CPList hist = ledger.getVHist("abe");
  ASSERT_EQ(hist.size(), 2u);
  auto it = hist.begin();
  EXPECT_DOUBLE_EQ(it->m_x, 1);
  EXPECT_DOUBLE_EQ(it->m_y, 1);
  ++it;
  EXPECT_DOUBLE_EQ(it->m_x, 2);
  EXPECT_DOUBLE_EQ(it->m_y, 2);
  EXPECT_TRUE(ledger.getVHist("ben").empty());
}

// Covers contact ledger behavior: filters by groups and finds closest vehicle.
TEST(ContactLedgerTest, FiltersByGroupsAndFindsClosestVehicle)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(20);
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 10, 0, 0, "red"), whynot);
  ledger.processNodeRecord(makeRecord("ben", 10, 0, 11, 0, 0, "blue"), whynot);
  ledger.processNodeRecord(makeRecord("cal", 4, 3, 12, 0, 0, "red"), whynot);

  EXPECT_EQ(ledger.getVNames(), (std::vector<std::string>{"abe", "ben", "cal"}));
  EXPECT_EQ(ledger.getVNamesByGroup("red"), (std::set<std::string>{"abe", "cal"}));
  EXPECT_EQ(ledger.getVNamesByGroup("all"), (std::set<std::string>{"abe", "ben", "cal"}));
  EXPECT_TRUE(ledger.getVNamesByGroup("").empty());
  EXPECT_TRUE(ledger.groupMatch("abe", "cal"));
  EXPECT_FALSE(ledger.groupMatch("abe", "ben"));
  EXPECT_EQ(ledger.getClosestVehicle(3, 3), "cal");

  double x = 0;
  double y = 0;
  EXPECT_TRUE(ledger.getWeightedCenter(x, y));
  EXPECT_NEAR(x, 14.0 / 3.0, 1e-9);
  EXPECT_NEAR(y, 1.0, 1e-9);
}

// Covers contact ledger behavior: tracks stale contacts and clears with keep list.
TEST(ContactLedgerTest, TracksStaleContactsAndClearsWithKeepList)
{
  ContactLedger ledger;
  ledger.setStaleThresh(5);
  std::string whynot;
  ledger.setCurrTimeUTC(80);
  ledger.processNodeRecord(makeRecord("old", 0, 0, 80), whynot);
  ledger.setCurrTimeUTC(99);
  ledger.processNodeRecord(makeRecord("fresh", 0, 0, 99), whynot);
  ledger.setCurrTimeUTC(100);

  EXPECT_TRUE(ledger.isStale("old", 5));
  EXPECT_FALSE(ledger.isStale("fresh", 5));
  EXPECT_EQ(ledger.getVNamesStale(5), (std::vector<std::string>{"old"}));
  EXPECT_TRUE(ledger.hasVNameValidNotStale("fresh"));
  EXPECT_FALSE(ledger.hasVNameValidNotStale("old"));

  ledger.clearStaleNodes({"old"});
  EXPECT_TRUE(ledger.hasVName("old"));
  EXPECT_FALSE(ledger.hasVName("fresh"));

  EXPECT_EQ(ledger.processNodeRecord(makeRecord("fresh", 0, 0, 99), whynot),
            "fresh");
  ledger.clearStaleNodes({"old"}, 1);
  EXPECT_FALSE(ledger.hasVName("old"));
  EXPECT_TRUE(ledger.hasVName("fresh"));
}

// Covers contact ledger behavior: extrapolates by heading and course over ground.
TEST(ContactLedgerTest, ExtrapolatesByHeadingAndCourseOverGround)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(100);
  EXPECT_TRUE(ledger.setExtrapPolicy("mode=hdg,thresh=0,decay=off"));
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 90, 2, 90), whynot);
  ledger.extrapolate(100);

  NodeRecord extrap_hdg = ledger.getRecord("abe");
  EXPECT_NEAR(extrap_hdg.getX(), 20, 1e-6);
  EXPECT_NEAR(extrap_hdg.getY(), 0, 1e-6);
  EXPECT_DOUBLE_EQ(ledger.getRecord("abe", false).getX(), 0);

  EXPECT_TRUE(ledger.setExtrapPolicy("mode=cog,thresh=0,decay=off"));
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 100, 2, 0), whynot);
  ledger.processNodeRecord(makeRecord("abe", 0, 10, 101, 2, 0), whynot);
  ledger.extrapolate(106);
  NodeRecord extrap_cog = ledger.getRecord("abe");
  EXPECT_NEAR(extrap_cog.getX(), 0, 1e-6);
  EXPECT_GT(extrap_cog.getY(), 10);
}

// Covers contact ledger behavior: manages active vehicle and clear operations.
TEST(ContactLedgerTest, ManagesActiveVehicleAndClearOperations)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(10);
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 1), whynot);
  ledger.processNodeRecord(makeRecord("ben", 1, 0, 2), whynot);
  EXPECT_EQ(ledger.getActiveVName(), "abe");
  ledger.setActiveVName("ben");
  EXPECT_EQ(ledger.getActiveVName(), "ben");
  ledger.setActiveVName("CYCLE_ACTIVE");
  EXPECT_EQ(ledger.getActiveVName(), "abe");
  ledger.setActiveVName("missing");
  EXPECT_EQ(ledger.getActiveVName(), "abe");

  ledger.clearNode("abe");
  EXPECT_FALSE(ledger.hasVName("abe"));
  EXPECT_TRUE(ledger.hasVName("ben"));
  ledger.clearAllNodes();
  EXPECT_EQ(ledger.size(), 0u);
  double x = 1;
  double y = 1;
  EXPECT_FALSE(ledger.getWeightedCenter(x, y));
  EXPECT_DOUBLE_EQ(x, 0);
  EXPECT_DOUBLE_EQ(y, 0);
}

// Covers contact ledger behavior: builds summary and report skew statistics.
TEST(ContactLedgerTest, BuildsSummaryAndReportSkewStatistics)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(100);
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 90), whynot);
  ledger.processNodeRecord(makeRecord("ben", 0, 0, 95), whynot);
  ledger.processNodeRecord(makeRecord("cal", 0, 0, 99), whynot);

  EXPECT_EQ(ledger.totalReports("abe"), 1u);
  EXPECT_EQ(ledger.totalReports("missing"), 0u);
  EXPECT_EQ(ledger.getSummary(2), "now:3, reps:3, who:abe,ben");
  EXPECT_EQ(ledger.getSummary(4), "now:3, reps:3, who:abe,ben,cal");
}

// Covers contact ledger behavior: applies MOOS vehicle type aliases and default lengths.
TEST(ContactLedgerTest, AppliesMoosVehicleTypeAliasesAndDefaultLengths)
{
  struct Case {
    std::string input_type;
    std::string stored_type;
    double expected_length;
    bool needs_depth;
  };

  const std::vector<Case> cases = {
      {"uuv", "auv", 3.0, true},
      {"auv", "auv", 3.0, true},
      {"glider", "glider", 2.0, true},
      {"kayak", "kayak", 3.0, false},
      {"heron", "heron", 3.0, false},
      {"cray", "cray", 1.5, false},
      {"bcrayx", "bcrayx", 1.5, false},
      {"wamv", "wamv", 6.0, false},
      {"smr", "smr", 6.0, false},
      {"longship", "longship", 15.0, false},
      {"unknown_contact_type", "ship", 12.0, false},
  };

  for(const Case& test_case : cases) {
    ContactLedger ledger;
    ledger.setCurrTimeUTC(100);
    NodeRecord rec = makeRecord("v_" + test_case.input_type, 1, 2, 90);
    rec.setType(test_case.input_type);
    if(test_case.needs_depth)
      rec.setDepth(4);

    std::string whynot;
    ASSERT_EQ(ledger.processNodeRecord(rec, whynot), rec.getName())
        << test_case.input_type;
    EXPECT_EQ(ledger.getType(rec.getName()), test_case.stored_type)
        << test_case.input_type;
    EXPECT_DOUBLE_EQ(ledger.getRecord(rec.getName(), false).getLength(),
                     test_case.expected_length)
        << test_case.input_type;
  }
}

// Covers contact ledger behavior: preserves positive reported length instead of applying type default.
TEST(ContactLedgerTest, PreservesPositiveReportedLengthInsteadOfApplyingTypeDefault)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(100);
  NodeRecord record = makeRecord("abe", 0, 0, 90);
  record.setType("wamv");
  record.setLength(8.5);

  std::string whynot;
  EXPECT_EQ(ledger.processNodeRecord(record, whynot), "abe");
  EXPECT_DOUBLE_EQ(ledger.getRecord("abe", false).getLength(), 8.5);

  record.setLength(0);
  EXPECT_EQ(ledger.processNodeRecord(record, whynot), "abe");
  EXPECT_DOUBLE_EQ(ledger.getRecord("abe", false).getLength(), 6.0);
}

// Covers contact ledger behavior: distinguishes report timestamp age from receive timestamp age.
TEST(ContactLedgerTest, DistinguishesReportTimestampAgeFromReceiveTimestampAge)
{
  ContactLedger ledger;
  ledger.setStaleThresh(20);
  std::string whynot;

  ledger.setCurrTimeUTC(100);
  ASSERT_EQ(ledger.processNodeRecord(makeRecord("abe", 0, 0, 70), whynot),
            "abe");
  ledger.setCurrTimeUTC(105);

  EXPECT_DOUBLE_EQ(ledger.getUTC("abe"), 70);
  EXPECT_DOUBLE_EQ(ledger.getUTCAge("abe"), 35);
  EXPECT_DOUBLE_EQ(ledger.getUTCReceived("abe"), 100);
  EXPECT_DOUBLE_EQ(ledger.getUTCAgeReceived("abe"), 5);

  EXPECT_FALSE(ledger.isStale("abe", 20));
  EXPECT_FALSE(ledger.getVNamesStale(20).size());
  EXPECT_FALSE(ledger.hasVNameValidNotStale("abe"));
  EXPECT_TRUE(ledger.hasVNameValidNotStale("abe", 40));
}

// Covers contact ledger behavior: stale checks are inclusive at threshold and can be disabled.
TEST(ContactLedgerTest, StaleChecksAreInclusiveAtThresholdAndCanBeDisabled)
{
  ContactLedger ledger;
  std::string whynot;
  ledger.setCurrTimeUTC(100);
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 100), whynot);

  ledger.setCurrTimeUTC(105);
  EXPECT_TRUE(ledger.isStale("abe", 5));
  EXPECT_FALSE(ledger.isStale("abe", 5.1));
  EXPECT_FALSE(ledger.isStale("abe", -1));
  EXPECT_TRUE(ledger.getVNamesStale(-1).empty());

  EXPECT_TRUE(ledger.hasVNameValidNotStale("abe", 5));
  EXPECT_FALSE(ledger.hasVNameValidNotStale("abe", 4.9));
}

// Covers contact ledger behavior: clear stale nodes uses keep list factor and resets per node stats.
TEST(ContactLedgerTest, ClearStaleNodesUsesKeepListFactorAndResetsPerNodeStats)
{
  ContactLedger ledger;
  ledger.setStaleThresh(10);
  std::string whynot;
  ledger.setCurrTimeUTC(80);
  ledger.processNodeRecord(makeRecord("old", 0, 0, 80), whynot);
  ledger.setCurrTimeUTC(81);
  ledger.processNodeRecord(makeRecord("kept", 1, 0, 81), whynot);
  ledger.setCurrTimeUTC(101);
  ledger.processNodeRecord(makeRecord("fresh", 2, 0, 100), whynot);

  ledger.clearStaleNodes({"kept"}, 2);

  EXPECT_FALSE(ledger.hasVName("old"));
  EXPECT_TRUE(ledger.hasVName("kept"));
  EXPECT_TRUE(ledger.hasVName("fresh"));
  EXPECT_EQ(ledger.totalReports("old"), 0u);
  EXPECT_EQ(ledger.totalReports("kept"), 1u);
}

// Covers contact ledger behavior: default negative stale threshold still clears very old nodes.
TEST(ContactLedgerTest, DefaultNegativeStaleThresholdStillClearsVeryOldNodes)
{
  ContactLedger ledger;
  std::string whynot;
  ledger.setCurrTimeUTC(100);
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 100), whynot);

  ledger.setCurrTimeUTC(102);
  ledger.clearStaleNodes();

  EXPECT_FALSE(ledger.hasVName("abe"));
}

// Covers contact ledger behavior: extrap policy parser applies valid parts and reports partial failure.
TEST(ContactLedgerTest, ExtrapPolicyParserAppliesValidPartsAndReportsPartialFailure)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(100);
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 90, 2, 90), whynot);

  EXPECT_FALSE(ledger.setExtrapPolicy("mode=hdg,decay=off,bad=value"));
  ledger.extrapolate(100);
  EXPECT_NEAR(ledger.getRecord("abe").getX(), 20, 1e-6);

  EXPECT_FALSE(ledger.setExtrapPolicy("mode=bogus"));
  ledger.extrapolate(110);
  EXPECT_NEAR(ledger.getRecord("abe").getX(), 40, 1e-6);

  EXPECT_TRUE(ledger.setExtrapPolicy("mode=off,thresh=0,decay=off"));
  ledger.extrapolate(120);
  EXPECT_DOUBLE_EQ(ledger.getRecord("abe").getX(), 0);
}

// Covers contact ledger behavior: extrap policy supports decay window and decimal threshold truncation.
TEST(ContactLedgerTest, ExtrapPolicySupportsDecayWindowAndDecimalThresholdTruncation)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(100);
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 100, 2, 90), whynot);

  EXPECT_TRUE(ledger.setExtrapPolicy("mode=hdg,thresh=0.9,decay=5:5"));
  ledger.extrapolate(100.5);
  EXPECT_NEAR(ledger.getRecord("abe").getX(), 1.0, 1e-6);

  ledger.extrapolate(110);
  EXPECT_NEAR(ledger.getRecord("abe").getX(), 10.0, 1e-6);
}

// Covers contact ledger behavior: get x and get y ignore extrap flag while get record honors it.
TEST(ContactLedgerTest, GetXAndGetYIgnoreExtrapFlagWhileGetRecordHonorsIt)
{
  ContactLedger ledger;
  ledger.setCurrTimeUTC(100);
  EXPECT_TRUE(ledger.setExtrapPolicy("mode=hdg,thresh=0,decay=off"));
  std::string whynot;
  ledger.processNodeRecord(makeRecord("abe", 0, 0, 90, 2, 90), whynot);
  ledger.extrapolate(100);

  EXPECT_NEAR(ledger.getRecord("abe").getX(), 20, 1e-6);
  EXPECT_DOUBLE_EQ(ledger.getRecord("abe", false).getX(), 0);
  EXPECT_NEAR(ledger.getX("abe", false), 20, 1e-6);
  EXPECT_NEAR(ledger.getY("abe", false), 0, 1e-6);
}

// Covers contact ledger behavior: process NODE_REPORT fills missing coordinates when geodesy is set.
TEST(ContactLedgerTest, ProcessNodeReportFillsMissingCoordinatesWhenGeodesyIsSet)
{
  ContactLedger ledger;
  ASSERT_TRUE(ledger.setGeodesy(42.0, -70.0));
  ledger.setCurrTimeUTC(100);

  std::string whynot;
  EXPECT_EQ(ledger.processNodeReport(
                "NAME=global_only,LAT=42.0001,LON=-69.9999,SPD=1,HDG=90,"
                "UTC_TIME=99,TYPE=ship",
                whynot),
            "global_only");
  EXPECT_TRUE(ledger.getRecord("global_only", false).isSetXY());
  EXPECT_NE(ledger.getX("global_only"), 0);
  EXPECT_NE(ledger.getY("global_only"), 0);

  whynot.clear();
  EXPECT_EQ(ledger.processNodeReport(
                "NAME=local_only,X=10,Y=20,SPD=1,HDG=90,UTC_TIME=99,"
                "TYPE=ship",
                whynot),
            "local_only");
  EXPECT_TRUE(ledger.getRecord("local_only", false).isSetLatLon());
  EXPECT_NEAR(ledger.getLat("local_only"), 42.0, 0.001);
  EXPECT_NEAR(ledger.getLon("local_only"), -70.0, 0.001);
}

// Covers contact ledger behavior: missing contacts return empty defaults and missing groups can match.
TEST(ContactLedgerTest, MissingContactsReturnEmptyDefaultsAndMissingGroupsCanMatch)
{
  ContactLedger ledger;
  EXPECT_FALSE(ledger.isValid("ghost"));
  EXPECT_FALSE(ledger.hasVName("ghost"));
  EXPECT_FALSE(ledger.hasVNameValid("ghost"));
  EXPECT_FALSE(ledger.hasVNameValidNotStale("ghost"));
  EXPECT_DOUBLE_EQ(ledger.getX("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getY("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getSpeed("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getHeading("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getDepth("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getLat("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getLon("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getUTC("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getUTCAge("ghost"), -1);
  EXPECT_DOUBLE_EQ(ledger.getUTCReceived("ghost"), 0);
  EXPECT_DOUBLE_EQ(ledger.getUTCAgeReceived("ghost"), -1);
  EXPECT_EQ(ledger.getType("ghost"), "");
  EXPECT_EQ(ledger.getGroup("ghost"), "");
  EXPECT_EQ(ledger.getColor("ghost"), "");
  EXPECT_EQ(ledger.getVSource("ghost"), "");
  EXPECT_EQ(ledger.getSpec("ghost"), "");
  EXPECT_TRUE(ledger.getRecord("ghost").getName().empty());
  EXPECT_EQ(ledger.getClosestVehicle(0, 0), "");
  EXPECT_TRUE(ledger.groupMatch("ghost_a", "ghost_b"));
}
