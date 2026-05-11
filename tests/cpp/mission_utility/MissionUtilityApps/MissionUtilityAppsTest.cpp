#include <gtest/gtest.h>

#include <string>

#include "HashUtils.h"
#include "MayFinish.h"
#include "MissionHash_MOOSApp.h"

namespace {

bool contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

std::string lineValue(const std::string& report, const std::string& key)
{
  const std::string prefix = key + "=";
  const std::string::size_type pos = report.find(prefix);
  if(pos == std::string::npos)
    return "";

  const std::string::size_type start = pos + prefix.size();
  const std::string::size_type end = report.find('\n', start);
  if(end == std::string::npos)
    return report.substr(start);
  return report.substr(start, end - start);
}

class TestableMissionHash : public MissionHash_MOOSApp {
 public:
  using MissionHash_MOOSApp::setMissionHash;

  std::string report()
  {
    m_msgs.clear();
    m_msgs.str("");
    buildReport();
    return m_msgs.str();
  }
};

class TestableMayFinish : public MayFinish {
 public:
  std::string report()
  {
    m_msgs.clear();
    m_msgs.str("");
    buildReport();
    return m_msgs.str();
  }
};

}  // namespace

// Covers pMissionHash app behavior: generated mission hash and short hash agree.
TEST(MissionHashAppTest, GeneratesConsistentMissionHashReportFields)
{
  TestableMissionHash app;

  ASSERT_TRUE(app.setMissionHash());
  const std::string report = app.report();
  const std::string mission_hash = lineValue(report, "MISSION_HASH");
  const std::string short_hash = lineValue(report, "MHASH");

  EXPECT_TRUE(contains(mission_hash, "mhash="));
  EXPECT_TRUE(contains(mission_hash, ",utc="));
  EXPECT_EQ(missionHashShort(mission_hash), short_hash);
  EXPECT_GT(missionHashUTC(mission_hash), 0);
}

// Covers uMayFinish config behavior: finish variable rejects missing or invalid names.
TEST(MayFinishConfigTest, ValidatesFinishVariableNames)
{
  TestableMayFinish app;

  EXPECT_FALSE(app.setFinishVar(""));
  EXPECT_FALSE(app.setFinishVar("HAS SPACE"));
  EXPECT_TRUE(app.setFinishVar("MISSION_DONE"));
  EXPECT_TRUE(app.setFinishVal("complete now"));

  const std::string report = app.report();
  EXPECT_TRUE(contains(report, "Finish Var: MISSION_DONE"));
  EXPECT_TRUE(contains(report, "Finish Val: complete now"));
}

// Covers uMayFinish config behavior: finish value requires a non-empty payload.
TEST(MayFinishConfigTest, ValidatesFinishValues)
{
  TestableMayFinish app;

  EXPECT_FALSE(app.setFinishVal(""));
  EXPECT_TRUE(app.setFinishVal("true"));
  EXPECT_TRUE(app.setFinishVal("mission complete"));

  const std::string report = app.report();
  EXPECT_TRUE(contains(report, "Finish Val: mission complete"));
}

// Covers uMayFinish config behavior: max DB uptime accepts numeric values only.
TEST(MayFinishConfigTest, ValidatesMaxDbUptime)
{
  TestableMayFinish app;

  EXPECT_FALSE(app.setMaxDBUpTime(""));
  EXPECT_FALSE(app.setMaxDBUpTime("soon"));
  EXPECT_TRUE(app.setMaxDBUpTime("12.5"));
  EXPECT_TRUE(app.setFinishVar("MISSION_DONE"));
  EXPECT_TRUE(app.setFinishVal("true"));

  const std::string report = app.report();
  EXPECT_TRUE(contains(report, "Max DBTime: 12.5"));
  EXPECT_TRUE(contains(report, "DB_UPTIME: 0"));
  EXPECT_TRUE(contains(report, "MISSION_DONE:"));
}
