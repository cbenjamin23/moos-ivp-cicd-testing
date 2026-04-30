#include <gtest/gtest.h>

#include <list>
#include <set>
#include <string>

#include "VCheckSet.h"

namespace {

bool contains(const std::string& haystack, const std::string& needle)
{
  return haystack.find(needle) != std::string::npos;
}

std::string joinedReport(const std::list<std::string>& report)
{
  std::string out;
  for(const std::string& line : report)
    out += line + "\n";
  return out;
}

}  // namespace

// Covers v check set behavior: validates mission eval vcheck configuration.
TEST(VCheckSetTest, ValidatesMissionEvalVcheckConfiguration)
{
  VCheckSet empty;
  EXPECT_FALSE(empty.enabled());
  EXPECT_EQ(empty.getStatus(), "unused");
  EXPECT_EQ(empty.checkConfig(), "");

  VCheckSet missing_start;
  ASSERT_TRUE(missing_start.addVCheck(
      "var=MISSION_RESULT,sval=success,time=10,max_tdelta=2"));
  EXPECT_EQ(missing_start.checkConfig(), "A valid vcheck_start is missing");

  VCheckSet missing_check;
  ASSERT_TRUE(missing_check.setStartTrigger("MISSION_START=true"));
  EXPECT_EQ(missing_check.checkConfig(), "A valid vcheck is missing");
  EXPECT_FALSE(missing_check.setStartTrigger("MISSION_START=true"));
  EXPECT_FALSE(missing_check.setFailOnFirst("maybe"));
  EXPECT_FALSE(missing_check.setMaxReportSize("-1"));
}

// Covers v check set behavior: passes string and numeric checks after start trigger.
TEST(VCheckSetTest, PassesStringAndNumericChecksAfterStartTrigger)
{
  VCheckSet set;
  ASSERT_TRUE(set.setStartTrigger("MISSION_START=true"));
  ASSERT_TRUE(set.addVCheck(
      "var=MISSION_RESULT,sval=success,time=15,max_tdelta=2"));
  ASSERT_TRUE(set.addVCheck("var=NAV_X,dval=50,time=20,max_vdelta=0.5"));
  EXPECT_EQ(set.getVars(),
            (std::set<std::string>{"MISSION_START", "MISSION_RESULT", "NAV_X"}));
  EXPECT_TRUE(set.enabled());
  EXPECT_EQ(set.getStatus(), "pending");

  set.handleMail("MISSION_RESULT", "success", 0, 24);
  EXPECT_EQ(set.getTotalPassed(), 0u);
  set.handleMail("MISSION_START", "true", 0, 10);
  set.handleMail("MISSION_RESULT", "success", 0, 24);
  set.handleMail("NAV_X", "", 50.4, 29);
  EXPECT_EQ(set.getTotalPassed(), 1u);
  EXPECT_FALSE(set.isEvaluated());

  set.handleMail("NAV_X", "", 50.4, 30);
  EXPECT_TRUE(set.isEvaluated());
  EXPECT_TRUE(set.isSatisfied());
  EXPECT_EQ(set.getStatus(), "pass");
  EXPECT_EQ(set.getTotalPassed(), 2u);
  EXPECT_EQ(set.getTotalFailed(), 0u);
  EXPECT_DOUBLE_EQ(set.getPassRate(), 1.0);

  const std::string report = joinedReport(set.getReport());
  EXPECT_TRUE(contains(report, "VCheck Status: (2)"));
  EXPECT_TRUE(contains(report, "MISSION_RESULT"));
  EXPECT_TRUE(contains(report, "NAV_X"));
  EXPECT_TRUE(contains(report, "pass"));
}

// Covers v check set behavior: fails value delta checks and records compact failure report.
TEST(VCheckSetTest, FailsValueDeltaChecksAndRecordsCompactFailureReport)
{
  VCheckSet set;
  ASSERT_TRUE(set.setStartTrigger("MISSION_START=true"));
  ASSERT_TRUE(set.addVCheck("var=NAV_DEPTH,dval=12,time=5,max_vdelta=0.25"));
  set.handleMail("MISSION_START", "true", 0, 100);
  set.handleMail("NAV_DEPTH", "", 12.5, 105);

  EXPECT_TRUE(set.isEvaluated());
  EXPECT_FALSE(set.isSatisfied());
  EXPECT_EQ(set.getStatus(), "fail");
  EXPECT_EQ(set.getTotalPassed(), 0u);
  EXPECT_EQ(set.getTotalFailed(), 1u);
  EXPECT_DOUBLE_EQ(set.getPassRate(), 0.0);
  EXPECT_EQ(set.getFailReport(),
            "var=NAV_DEPTH, eval=12, etime=5, aval=12.5, atime=5");
}

// Covers v check set behavior: times out missing exact posts and supports fail on first.
TEST(VCheckSetTest, TimesOutMissingExactPostsAndSupportsFailOnFirst)
{
  VCheckSet set;
  ASSERT_TRUE(set.setStartTrigger("MISSION_START=true"));
  ASSERT_TRUE(set.addVCheck(
      "var=RETURNED_HOME,sval=true,time=20,max_tdelta=5"));
  ASSERT_TRUE(set.addVCheck(
      "var=MISSION_RESULT,sval=success,time=30,max_tdelta=5"));
  ASSERT_TRUE(set.setFailOnFirst("true"));

  set.handleMail("MISSION_START", "true", 0, 10);
  set.update(36);

  EXPECT_TRUE(set.isEvaluated());
  EXPECT_FALSE(set.isSatisfied());
  EXPECT_EQ(set.getStatus(), "fail");
  EXPECT_EQ(set.getTotalFailed(), 1u);
  EXPECT_EQ(set.getFailReport(),
            "var=RETURNED_HOME, eval=true, etime=20, aval=-1, atime=26");
}

// Covers v check set behavior: supports numeric start triggers and ignores pre start mail.
TEST(VCheckSetTest, SupportsNumericStartTriggersAndIgnoresPreStartMail)
{
  VCheckSet set;
  ASSERT_TRUE(set.setStartTrigger("MISSION_PHASE=2"));
  ASSERT_TRUE(set.addVCheck("var=NAV_X,dval=100,time=5,max_vdelta=1"));

  set.handleMail("NAV_X", "", 100.0, 101);
  EXPECT_FALSE(set.isEvaluated());
  EXPECT_EQ(set.getTotalPassed(), 0u);

  set.handleMail("MISSION_PHASE", "", 1.0, 100);
  set.handleMail("NAV_X", "", 100.0, 106);
  EXPECT_FALSE(set.isEvaluated());

  set.handleMail("MISSION_PHASE", "", 2.0, 100);
  set.handleMail("NAV_X", "", 100.5, 105);
  EXPECT_TRUE(set.isEvaluated());
  EXPECT_TRUE(set.isSatisfied());
  EXPECT_EQ(set.getTotalPassed(), 1u);
}

// Covers v check set behavior: pins strict time window boundary.
TEST(VCheckSetTest, PinsStrictTimeWindowBoundary)
{
  VCheckSet set;
  ASSERT_TRUE(set.setStartTrigger("START=true"));
  ASSERT_TRUE(set.addVCheck("var=ARRIVED,sval=true,time=10,max_tdelta=2"));

  set.handleMail("START", "true", 0, 50);
  set.handleMail("ARRIVED", "true", 0, 62);
  EXPECT_FALSE(set.isEvaluated());

  set.update(62.01);
  EXPECT_TRUE(set.isEvaluated());
  EXPECT_FALSE(set.isSatisfied());
  EXPECT_EQ(set.getFailReport(),
            "var=ARRIVED, eval=true, etime=10, aval=-1, atime=12.01");
}

// Covers v check set behavior: limits reports by hiding passed checks first.
TEST(VCheckSetTest, LimitsReportsByHidingPassedChecksFirst)
{
  VCheckSet set;
  ASSERT_TRUE(set.setStartTrigger("START=true"));
  ASSERT_TRUE(set.setMaxReportSize("2"));
  ASSERT_TRUE(set.addVCheck("var=CHK_A,sval=ok,time=1,max_tdelta=1"));
  ASSERT_TRUE(set.addVCheck("var=CHK_B,sval=ok,time=2,max_tdelta=1"));
  ASSERT_TRUE(set.addVCheck("var=CHK_C,sval=ok,time=3,max_tdelta=1"));
  ASSERT_TRUE(set.addVCheck("var=CHK_D,sval=ok,time=4,max_tdelta=1"));

  set.handleMail("START", "true", 0, 100);
  set.handleMail("CHK_A", "ok", 0, 101);
  set.handleMail("CHK_B", "ok", 0, 102);

  const std::string report = joinedReport(set.getReport());
  EXPECT_TRUE(contains(report, "2 additional checks PASSED, not shown"));
  EXPECT_FALSE(contains(report, "additional checks pending, not shown"));
  EXPECT_FALSE(contains(report, "CHK_A"));
  EXPECT_FALSE(contains(report, "CHK_B"));
  EXPECT_TRUE(contains(report, "CHK_C"));
  EXPECT_TRUE(contains(report, "CHK_D"));
}
