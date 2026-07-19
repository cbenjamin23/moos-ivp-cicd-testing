#include <gtest/gtest.h>

#include <list>
#include <string>

#include "MOOS/libMOOS/Comms/MOOSMsg.h"
#include "MOOS/libMOOS/MOOSLib.h"
#include "NodeReporter.h"

namespace {

class TestableNodeReporter : public NodeReporter {
 public:
  using NodeReporter::addPlatformVar;
  using NodeReporter::assembleNodeReport;
  using NodeReporter::assemblePlatformReport;
  using NodeReporter::crossFillCoords;
  using NodeReporter::handleHelmSwitch;
  using NodeReporter::handleLocalHelmSummary;
  using NodeReporter::handleMailRiderVars;
  using NodeReporter::setCrossFillPolicy;
  using NodeReporter::updatePlatformVar;

  TestableNodeReporter()
  {
    m_curr_time = 10;
    m_vessel_name = "alpha";
    m_record.setName("alpha");
    m_record.setType("kayak");
    m_record.setColor("red");
    m_record.setLength(4.2);
    m_record.setBeam(1.4);
  }

  void setTime(double time) { m_curr_time = time; }
  void setHelmLastMsg(double time) { m_helm_lastmsg = time; }
  void setHelmPrimary(const std::string& value) { m_helm_status_primary = value; }
  void setHelmStandby(const std::string& value) { m_helm_status_standby = value; }
  void setHelmModeForTest(const std::string& value) { m_helm_mode = value; }
  void setHelmAllstop(const std::string& value) { m_helm_allstop_mode = value; }
  void setTerseReports(bool value) { m_terse_reports = value; }
  void setCoordPolicyGlobal(bool value) { m_coord_policy_global = value; }
  void setAllowColorChange(bool value) { m_allow_color_change = value; }
  void setJsonReport(const std::string& value) { m_json_report = value; }
  void setAltNavPrefix(const std::string& value) { m_alt_nav_prefix = value; }
  void setCurrMHash(const std::string& value) { m_curr_mhash = value; }

  NodeRecord& record() { return m_record; }
  NodeRecord& altRecord() { return m_record_gt; }
  std::string crossfillPolicy() const { return m_crossfill_policy; }
  std::string nodeReportVar() const { return m_node_report_var; }
  std::string platformReportVar() const { return m_plat_report_var; }
  std::string helmMode() const { return m_helm_mode; }
  std::string helmAllstop() const { return m_helm_allstop_mode; }
  bool helmSwitchNoted() const { return m_helm_switch_noted; }
  bool paused() const { return m_paused; }
  bool thrustModeReverse() const { return m_record.getThrustModeReverse(); }
  bool navWarningPosted() const { return m_nav_warning_posted; }
  bool addRider(const std::string& spec) { return m_riderset.addNodeRider(spec); }

  void initGeodesy()
  {
    ASSERT_TRUE(m_geodesy.Initialise(43.8253, -70.3304));
  }
};

bool Contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

MOOSMSG_LIST mail(std::initializer_list<CMOOSMsg> messages)
{
  MOOSMSG_LIST result;
  for(const CMOOSMsg& msg : messages)
    result.push_back(msg);
  return result;
}

CMOOSMsg dmail(const std::string& key, double value, double time = MOOSTime())
{
  return CMOOSMsg(MOOS_NOTIFY, key, value, time);
}

CMOOSMsg smail(const std::string& key, const std::string& value, double time = MOOSTime())
{
  return CMOOSMsg(MOOS_NOTIFY, key, value, time);
}

NodeRecord baseRecord()
{
  NodeRecord record("alpha", "kayak");
  record.setX(10);
  record.setY(-5);
  record.setSpeed(2.5);
  record.setHeading(123);
  record.setDepth(7);
  record.setLat(43.82535);
  record.setLon(-70.33045);
  record.setColor("red");
  record.setLength(4.2);
  record.setBeam(1.4);
  record.setAltitude(33);
  record.setYaw(1.25);
  return record;
}

}  // namespace

// Covers pNodeReporter report assembly: DRIVE mode uses helm summary modes when available.
TEST(NodeReporterTest, AssembleNodeReportUsesHelmSummaryModeAndCoreFields)
{
  TestableNodeReporter reporter;
  reporter.setTime(20);
  reporter.setHelmLastMsg(19);
  reporter.setHelmPrimary("DRIVE");
  reporter.setHelmAllstop("clear");
  reporter.handleLocalHelmSummary("iter=9,modes=MODE@ACTIVE:TRANSIT,halted=false");

  const std::string report = reporter.assembleNodeReport(baseRecord());

  EXPECT_TRUE(Contains(report, "NAME=alpha"));
  EXPECT_TRUE(Contains(report, "X=10"));
  EXPECT_TRUE(Contains(report, "Y=-5"));
  EXPECT_TRUE(Contains(report, "SPD=2.5"));
  EXPECT_TRUE(Contains(report, "HDG=123"));
  EXPECT_TRUE(Contains(report, "DEP=7"));
  EXPECT_TRUE(Contains(report, "LAT=43.82535"));
  EXPECT_TRUE(Contains(report, "LON=-70.33045"));
  EXPECT_TRUE(Contains(report, "MODE=MODE@ACTIVE:TRANSIT"));
  EXPECT_TRUE(Contains(report, "ALLSTOP=clear"));
}

// Covers pNodeReporter report assembly: no helm and stale helm states produce NOHELM markers.
TEST(NodeReporterTest, AssembleNodeReportMarksNoHelmEverAndStaleHelm)
{
  TestableNodeReporter reporter;
  reporter.setTime(20);
  reporter.setHelmLastMsg(-1);

  EXPECT_TRUE(Contains(reporter.assembleNodeReport(baseRecord()), "MODE=NOHELM-EVER"));

  reporter.setHelmLastMsg(11);
  EXPECT_TRUE(Contains(reporter.assembleNodeReport(baseRecord()), "MODE=NOHELM-9"));
}

// Covers pNodeReporter report assembly: standby DRIVE/PARK status overrides primary helm status.
TEST(NodeReporterTest, AssembleNodeReportUsesStandbyDriveOrParkOverride)
{
  TestableNodeReporter reporter;
  reporter.setTime(20);
  reporter.setHelmLastMsg(19);
  reporter.setHelmPrimary("DRIVE");
  reporter.setHelmStandby("PARK+");
  reporter.setHelmModeForTest("MODE@ACTIVE:TRANSIT");

  EXPECT_TRUE(Contains(reporter.assembleNodeReport(baseRecord()), "MODE=PARK+"));
}

// Covers pNodeReporter report assembly: terse and global-coordinate policies suppress the expected fields.
TEST(NodeReporterTest, TerseAndGlobalCoordinatePoliciesControlSerializedFields)
{
  TestableNodeReporter reporter;
  reporter.setTime(20);
  reporter.setHelmLastMsg(19);
  reporter.setHelmPrimary("DRIVE");
  reporter.setHelmAllstop("clear");

  reporter.setTerseReports(true);
  std::string terse = reporter.assembleNodeReport(baseRecord());
  EXPECT_TRUE(Contains(terse, "X=10"));
  EXPECT_FALSE(Contains(terse, "DEP=7"));
  EXPECT_FALSE(Contains(terse, "LAT=43.82535"));
  EXPECT_FALSE(Contains(terse, "YAW="));

  reporter.setTerseReports(false);
  reporter.setCoordPolicyGlobal(true);
  std::string global = reporter.assembleNodeReport(baseRecord());
  EXPECT_TRUE(Contains(global, "LAT=43.82535"));
  EXPECT_FALSE(Contains(global, "X=10"));
  EXPECT_FALSE(Contains(global, "Y=-5"));
}

// Covers pNodeReporter rider behavior: configured riders are appended to node reports.
TEST(NodeReporterTest, RiderPayloadsAreAppendedToNodeReport)
{
  TestableNodeReporter reporter;
  reporter.setTime(20);
  reporter.setHelmLastMsg(19);
  reporter.setHelmPrimary("DRIVE");
  ASSERT_TRUE(reporter.addRider("var=BATTERY_VOLTAGE,rfld=batt,policy=always,prec=2"));
  ASSERT_TRUE(reporter.handleMailRiderVars("BATTERY_VOLTAGE", "", 12.345));

  const std::string report = reporter.assembleNodeReport(baseRecord());

  EXPECT_TRUE(Contains(report, "batt=12.35"));
}

// Covers pNodeReporter platform reports: parser aliases variables and obeys per-variable post gaps.
TEST(NodeReporterTest, PlatformReportAliasesVariablesAndHonorsPostGap)
{
  TestableNodeReporter reporter;
  reporter.setTime(10);
  ASSERT_TRUE(reporter.addPlatformVar("BATTERY_VOLTAGE,alias=batt,gap=2"));

  reporter.updatePlatformVar("BATTERY_VOLTAGE", "12.7");
  const std::string first = reporter.assemblePlatformReport();
  EXPECT_TRUE(Contains(first, "platform=alpha"));
  EXPECT_TRUE(Contains(first, "batt=12.7"));

  reporter.setTime(11);
  reporter.updatePlatformVar("BATTERY_VOLTAGE", "12.8");
  EXPECT_TRUE(reporter.assemblePlatformReport().empty());

  reporter.setTime(13);
  const std::string report = reporter.assemblePlatformReport();
  EXPECT_TRUE(Contains(report, "platform=alpha"));
  EXPECT_TRUE(Contains(report, "batt=12.8"));

  reporter.setTime(14);
  reporter.updatePlatformVar("BATTERY_VOLTAGE", "12.9");
  EXPECT_TRUE(reporter.assemblePlatformReport().empty());

  reporter.setTime(16);
  const std::string later = reporter.assemblePlatformReport();
  EXPECT_TRUE(Contains(later, "batt=12.9"));
}

// Covers pNodeReporter platform reports: invalid and duplicate platform-variable configs are rejected.
TEST(NodeReporterTest, PlatformReportRejectsInvalidAndDuplicateVariableConfigs)
{
  TestableNodeReporter reporter;

  EXPECT_FALSE(reporter.addPlatformVar("BAD VAR,alias=bad"));
  EXPECT_TRUE(reporter.addPlatformVar("GPS_SATS,alias=sats,gap=-5"));
  EXPECT_FALSE(reporter.addPlatformVar("GPS_SATS,alias=sats2"));
  EXPECT_TRUE(reporter.addPlatformVar("RAW_TEMP,alias=bad alias"));
}

// Covers pNodeReporter cross-fill policy: accepts known spellings and rejects unknown policies.
TEST(NodeReporterTest, CrossFillPolicyAcceptsKnownPoliciesAndNormalizesUnderscore)
{
  TestableNodeReporter reporter;

  EXPECT_TRUE(reporter.setCrossFillPolicy("fill_empty"));
  EXPECT_EQ(reporter.crossfillPolicy(), "fill-empty");
  EXPECT_TRUE(reporter.setCrossFillPolicy("global-terse"));
  EXPECT_EQ(reporter.crossfillPolicy(), "global-terse");
  EXPECT_FALSE(reporter.setCrossFillPolicy("nearest"));
  EXPECT_EQ(reporter.crossfillPolicy(), "global-terse");
}

// Covers pNodeReporter coordinate behavior: local and global cross-fill synthesize missing coordinate pairs.
TEST(NodeReporterTest, CrossFillSynthesizesLocalAndGlobalCoordinates)
{
  TestableNodeReporter reporter;
  reporter.initGeodesy();

  NodeRecord local("alpha", "kayak");
  local.setX(-4.717421778);
  local.setY(6.741529793);
  ASSERT_TRUE(reporter.setCrossFillPolicy("local"));
  reporter.crossFillCoords(local, 10, 0);
  EXPECT_TRUE(local.valid("lat"));
  EXPECT_TRUE(local.valid("lon"));
  EXPECT_NEAR(local.getLat(), 43.82536, 2e-5);
  EXPECT_NEAR(local.getLon(), -70.33046, 2e-5);

  NodeRecord global("alpha", "kayak");
  global.setLat(43.82536);
  global.setLon(-70.33046);
  ASSERT_TRUE(reporter.setCrossFillPolicy("global"));
  reporter.crossFillCoords(global, 0, 10);
  EXPECT_TRUE(global.valid("x"));
  EXPECT_TRUE(global.valid("y"));
  EXPECT_NEAR(global.getX(), -4.717421778, 0.25);
  EXPECT_NEAR(global.getY(), 6.741529793, 0.25);
}

// Covers pNodeReporter coordinate behavior: fill-empty and use-latest policies update only under their stated conditions.
TEST(NodeReporterTest, CrossFillFillEmptyAndUseLatestRespectUpdateTimestamps)
{
  TestableNodeReporter reporter;
  reporter.initGeodesy();

  NodeRecord filled("alpha", "kayak");
  filled.setLat(43.82536);
  filled.setLon(-70.33046);
  ASSERT_TRUE(reporter.setCrossFillPolicy("fill-empty"));
  reporter.crossFillCoords(filled, 0, 10);
  EXPECT_TRUE(filled.valid("x"));
  EXPECT_TRUE(filled.valid("y"));

  NodeRecord latest("alpha", "kayak");
  latest.setX(99);
  latest.setY(99);
  latest.setLat(43.82536);
  latest.setLon(-70.33046);
  ASSERT_TRUE(reporter.setCrossFillPolicy("use-latest"));
  reporter.crossFillCoords(latest, 1, 10);
  EXPECT_NE(latest.getX(), 99);
  EXPECT_NE(latest.getY(), 99);

  latest.setX(25);
  latest.setY(5);
  reporter.crossFillCoords(latest, 20, 10);
  EXPECT_NEAR(latest.getLat(), 43.82535, 1e-4);
}

// Covers pNodeReporter mail handling: navigation, helm, color, aux, all-stop, load, pause, and reverse inputs update state.
TEST(NodeReporterTest, OnNewMailUpdatesReportStateFromAppFacingInputs)
{
  TestableNodeReporter reporter;
  reporter.setTime(10);
  reporter.setAllowColorChange(false);

  MOOSMSG_LIST first = mail({
      dmail("NAV_X", 10),
      dmail("NAV_Y", -5),
      dmail("NAV_SPEED", 2.5),
      dmail("NAV_HEADING", 123),
      smail("PLATFORM_COLOR", "orange"),
      smail("NODE_COLOR_CHANGE", "yellow"),
      smail("THRUST_MODE_REVERSE", "true"),
      smail("AUX_MODE", "survey"),
      smail("IVPHELM_STATE", "DRIVE"),
      smail("IVPHELM_ALLSTOP", "manual"),
      smail("LOAD_WARNING", "app=pHelmIvP,maxgap=5"),
      smail("PNR_PAUSE", "true")});
  ASSERT_TRUE(reporter.OnNewMail(first));

  std::string report = reporter.assembleNodeReport(reporter.record());
  EXPECT_TRUE(Contains(report, "COLOR=orange"));
  EXPECT_FALSE(Contains(report, "COLOR=yellow"));
  EXPECT_TRUE(Contains(report, "THRUST_MODE_REVERSE=true"));
  EXPECT_TRUE(Contains(report, "MODE_AUX=survey"));
  EXPECT_TRUE(Contains(report, "ALLSTOP=manual"));
  EXPECT_TRUE(Contains(report, "LOAD_WARNING=pHelmIvP:5"));
  EXPECT_TRUE(reporter.paused());

  reporter.setAllowColorChange(true);
  MOOSMSG_LIST color_mail = mail({smail("NODE_COLOR_CHANGE", "yellow")});
  ASSERT_TRUE(reporter.OnNewMail(color_mail));
  EXPECT_TRUE(Contains(reporter.assembleNodeReport(reporter.record()), "COLOR=yellow"));
}

// Covers pNodeReporter mail handling: heading normalization and detailed NAV fields are serialized.
TEST(NodeReporterTest, OnNewMailNormalizesHeadingAndCapturesDetailedNavigationFields)
{
  TestableNodeReporter reporter;
  reporter.setTime(10);

  MOOSMSG_LIST nav_mail = mail({
      dmail("NAV_X", 10),
      dmail("NAV_Y", -5),
      dmail("NAV_LAT", 43.82537),
      dmail("NAV_LONG", -70.33047),
      dmail("NAV_HEADING", 405),
      dmail("NAV_DEPTH", 9),
      dmail("NAV_ALTITUDE", 31),
      dmail("NAV_YAW", 1.7),
      dmail("NAV_TRANSP", 0.4),
      smail("NAV_TRAJECTORY", "pts={0,0:5,5}"),
      smail("PLATFORM_COLOR", "not_a_color")});
  ASSERT_TRUE(reporter.OnNewMail(nav_mail));

  const std::string report = reporter.assembleNodeReport(reporter.record());

  EXPECT_TRUE(Contains(report, "HDG=45"));
  EXPECT_TRUE(Contains(report, "DEP=9"));
  EXPECT_TRUE(Contains(report, "LAT=43.82537"));
  EXPECT_TRUE(Contains(report, "LON=-70.33047"));
  EXPECT_TRUE(Contains(report, "ALTITUDE=31"));
  EXPECT_TRUE(Contains(report, "TRANSPARENCY=0.4"));
  EXPECT_TRUE(Contains(report, "TRAJECTORY={pts={0,0:5,5}}"));
  EXPECT_TRUE(Contains(report, "COLOR=red"));
  EXPECT_FALSE(Contains(report, "not_a_color"));
}

// Covers pNodeReporter mail handling: group updates apply to both primary and alternate records.
TEST(NodeReporterTest, OnNewMailGroupUpdateAppliesToPrimaryAndAlternateRecords)
{
  TestableNodeReporter reporter;
  reporter.altRecord().setName("alpha_GT");

  MOOSMSG_LIST group_mail = mail({smail("NODE_GROUP_UPDATE", "dynamic")});
  ASSERT_TRUE(reporter.OnNewMail(group_mail));

  EXPECT_EQ(reporter.record().getGroup(), "dynamic");
  EXPECT_EQ(reporter.altRecord().getGroup(), "dynamic");
}

// Covers pNodeReporter mail handling: alternate-nav prefix updates the alternate record fields.
TEST(NodeReporterTest, OnNewMailAltNavPrefixUpdatesAlternateRecord)
{
  TestableNodeReporter reporter;
  reporter.setAltNavPrefix("NAV_GT_");

  MOOSMSG_LIST alt_mail = mail({
      dmail("NAV_GT_X", 20),
      dmail("NAV_GT_Y", -15),
      dmail("NAV_GT_SPEED", 3.1),
      dmail("NAV_GT_HEADING", 210),
      dmail("NAV_GT_DEPTH", 4),
      dmail("NAV_GT_ALTITUDE", 40),
      dmail("NAV_GT_TRANSP", 0.25)});
  ASSERT_TRUE(reporter.OnNewMail(alt_mail));

  EXPECT_DOUBLE_EQ(reporter.altRecord().getX(), 20);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getY(), -15);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getSpeed(), 3.1);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getHeading(), 210);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getDepth(), 4);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getAltitude(), 40);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getTransparency(), 0.25);
}

// Covers pNodeReporter mail handling: alternate-nav global fields and yaw are routed to the alternate record.
TEST(NodeReporterTest, OnNewMailAltNavPrefixUpdatesGlobalAndYawFields)
{
  TestableNodeReporter reporter;
  reporter.setAltNavPrefix("NAV_GT_");

  MOOSMSG_LIST alt_mail = mail({
      dmail("NAV_GT_LAT", 43.82539),
      dmail("NAV_GT_LONG", -70.33049),
      dmail("NAV_GT_HEADING", 725),
      dmail("NAV_GT_YAW", 2.25)});
  ASSERT_TRUE(reporter.OnNewMail(alt_mail));

  EXPECT_DOUBLE_EQ(reporter.altRecord().getLat(), 43.82539);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getLon(), -70.33049);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getHeading(), 5);
  EXPECT_DOUBLE_EQ(reporter.altRecord().getYaw(), 2.25);
}

// Covers pNodeReporter helm-switch handling: standby takeover clears stale mode and all-stop once.
TEST(NodeReporterTest, HelmSwitchClearsModeAndAllstopOnce)
{
  TestableNodeReporter reporter;
  reporter.setHelmModeForTest("MODE@ACTIVE:TRANSIT");
  reporter.setHelmAllstop("clear");

  reporter.handleHelmSwitch();
  EXPECT_TRUE(reporter.helmSwitchNoted());
  EXPECT_EQ(reporter.helmMode(), "");
  EXPECT_EQ(reporter.helmAllstop(), "");

  reporter.setHelmModeForTest("new");
  reporter.setHelmAllstop("newstop");
  reporter.handleHelmSwitch();
  EXPECT_EQ(reporter.helmMode(), "new");
  EXPECT_EQ(reporter.helmAllstop(), "newstop");
}

// Covers pNodeReporter startup defaults: report variables and JSON setting defaults match app contract.
TEST(NodeReporterTest, DefaultsExposeStandardReportVariables)
{
  TestableNodeReporter reporter;

  EXPECT_EQ(reporter.nodeReportVar(), "NODE_REPORT_LOCAL");
  EXPECT_EQ(reporter.platformReportVar(), "PLATFORM_REPORT_LOCAL");
  EXPECT_EQ(reporter.crossfillPolicy(), "literal");
}
