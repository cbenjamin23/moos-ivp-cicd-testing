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
