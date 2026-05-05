#include <gtest/gtest.h>

#include <cctype>
#include <cstdlib>
#include <string>

#include "TS_MOOSApp.h"

namespace {

class TestableTimerScript : public TS_MOOSApp {
 public:
  using TS_MOOSApp::addBlockApps;
  using TS_MOOSApp::addNewEvent;
  using TS_MOOSApp::checkBlockApps;
  using TS_MOOSApp::handleMathExpr;
  using TS_MOOSApp::jumpToNextPostingTime;
  using TS_MOOSApp::randomNumString;
  using TS_MOOSApp::scheduleEvents;

  unsigned int eventCount() const { return m_pairs.size(); }
  std::string eventVar(unsigned int ix) const { return m_pairs[ix].get_var(); }
  std::string eventStringValue(unsigned int ix) const
  {
    return m_pairs[ix].get_sdata();
  }
  double eventTime(unsigned int ix) const { return m_ptime[ix]; }
  double eventTimeMin(unsigned int ix) const { return m_ptime_min[ix]; }
  double eventTimeMax(unsigned int ix) const { return m_ptime_max[ix]; }
  bool eventPosted(unsigned int ix) const { return m_poked[ix]; }
  double skipTime() const { return m_skip_time; }
  unsigned int blockAppCount() const { return m_block_apps.size(); }
  bool hasBlockApp(const std::string& app) const
  {
    return m_block_apps.count(app) > 0;
  }

  void setEventPosted(unsigned int ix, bool posted) { m_poked[ix] = posted; }
  void setEventTime(unsigned int ix, double ptime) { m_ptime[ix] = ptime; }
  void setSkipTime(double skip_time) { m_skip_time = skip_time; }
  void setElapsedTime(double elapsed_time) { m_elapsed_time = elapsed_time; }
  void setAppNameForTest(const std::string& app_name) { m_sAppName = app_name; }
};

bool IsDigits(const std::string& value)
{
  for(char c : value) {
    if(!std::isdigit(static_cast<unsigned char>(c)))
      return false;
  }
  return true;
}

}  // namespace

// Coverage note: this suite exposes only protected, deterministic TS_MOOSApp
// helpers. It deliberately avoids Iterate, Notify/Register side effects, help
// text functions that exit, and live MOOSDB behavior, which belong in harnesses.

TEST(TS_MOOSAppEventTest, AddsMultipleEventsWithIndexExpansionAndTimeRange)
{
  TestableTimerScript app;

  app.addNewEvent("var=FOO,val=\"msg_$$IDX_$(IDX)_$[IDX]\",time=2:4,amt=3");

  ASSERT_EQ(app.eventCount(), 3u);
  EXPECT_EQ(app.eventVar(0), "FOO");
  EXPECT_EQ(app.eventStringValue(0), "msg_000_000_000");
  EXPECT_EQ(app.eventStringValue(1), "msg_001_001_001");
  EXPECT_EQ(app.eventStringValue(2), "msg_002_002_002");
  EXPECT_DOUBLE_EQ(app.eventTimeMin(0), 2);
  EXPECT_DOUBLE_EQ(app.eventTimeMax(0), 4);
}

TEST(TS_MOOSAppEventTest, PreservesQuotedCommasInEventValues)
{
  TestableTimerScript app;

  app.addNewEvent("var=PAYLOAD,val=\"x=1,y=2,label=alpha\",time=1");

  ASSERT_EQ(app.eventCount(), 1u);
  EXPECT_EQ(app.eventVar(0), "PAYLOAD");
  EXPECT_EQ(app.eventStringValue(0), "x=1,y=2,label=alpha");
  EXPECT_DOUBLE_EQ(app.eventTimeMin(0), 1);
}

TEST(TS_MOOSAppEventTest, RejectsEventsMissingVariableOrValue)
{
  TestableTimerScript app;

  app.addNewEvent("val=payload,time=1");
  app.addNewEvent("var=FOO,time=1");

  EXPECT_EQ(app.eventCount(), 0u);
}

TEST(TS_MOOSAppEventTest, QuitEventPostsExitMarkerForCurrentAppName)
{
  TestableTimerScript app;
  app.setAppNameForTest("uTimerScript");

  app.addNewEvent("quit,time=5");

  ASSERT_EQ(app.eventCount(), 1u);
  EXPECT_EQ(app.eventVar(0), "EXITED_NORMALLY");
  EXPECT_EQ(app.eventStringValue(0), "uTimerScript");
  EXPECT_DOUBLE_EQ(app.eventTimeMin(0), 5);
  EXPECT_DOUBLE_EQ(app.eventTimeMax(0), 5);
}

TEST(TS_MOOSAppEventTest, InvalidAmountFallsBackToOneEvent)
{
  TestableTimerScript app;

  app.addNewEvent("var=FOO,val=bar,time=3,amt=0");

  ASSERT_EQ(app.eventCount(), 1u);
  EXPECT_EQ(app.eventStringValue(0), "bar");
  EXPECT_DOUBLE_EQ(app.eventTimeMin(0), 3);
}

TEST(TS_MOOSAppEventTest, InvalidTimeSpecsStillAddEventAtDefaultTimeZero)
{
  TestableTimerScript app;

  app.addNewEvent("var=A,val=negative,time=-1");
  app.addNewEvent("var=B,val=reversed,time=5:3");
  app.addNewEvent("var=C,val=bad,time=soon");

  ASSERT_EQ(app.eventCount(), 3u);
  EXPECT_DOUBLE_EQ(app.eventTimeMin(0), 0);
  EXPECT_DOUBLE_EQ(app.eventTimeMax(0), 0);
  EXPECT_DOUBLE_EQ(app.eventTimeMin(1), 0);
  EXPECT_DOUBLE_EQ(app.eventTimeMax(1), 0);
  EXPECT_DOUBLE_EQ(app.eventTimeMin(2), 0);
  EXPECT_DOUBLE_EQ(app.eventTimeMax(2), 0);
}

TEST(TS_MOOSAppEventTest, ScheduleAssignsTimesWithinRangesAndClearsPostedState)
{
  TestableTimerScript app;
  app.addNewEvent("var=A,val=one,time=2:5");
  app.addNewEvent("var=B,val=two,time=8");
  app.setEventPosted(0, true);
  app.setEventPosted(1, true);

  app.scheduleEvents(false);

  EXPECT_GE(app.eventTime(0), 2);
  EXPECT_LE(app.eventTime(0), 5);
  EXPECT_DOUBLE_EQ(app.eventTime(1), 8);
  EXPECT_FALSE(app.eventPosted(0));
  EXPECT_FALSE(app.eventPosted(1));
}

TEST(TS_MOOSAppEventTest, ScheduleWithoutShuffleKeepsAlreadyScheduledTimes)
{
  TestableTimerScript app;
  app.addNewEvent("var=A,val=one,time=2:5");
  app.scheduleEvents(false);
  app.setEventTime(0, 42);
  app.setEventPosted(0, true);

  app.scheduleEvents(false);

  EXPECT_DOUBLE_EQ(app.eventTime(0), 42);
  EXPECT_FALSE(app.eventPosted(0));
}

TEST(TS_MOOSAppEventTest, JumpToNextPostingUsesEarliestUnpostedEvent)
{
  TestableTimerScript app;
  app.addNewEvent("var=A,val=one,time=4");
  app.addNewEvent("var=B,val=two,time=9");
  app.scheduleEvents(false);
  app.setElapsedTime(3);
  app.setEventPosted(0, true);

  app.jumpToNextPostingTime();

  EXPECT_DOUBLE_EQ(app.skipTime(), 6);
}

TEST(TS_MOOSAppEventTest, JumpToNextPostingDoesNothingWhenAllEventsArePosted)
{
  TestableTimerScript app;
  app.addNewEvent("var=A,val=one,time=4");
  app.scheduleEvents(false);
  app.setEventPosted(0, true);
  app.setElapsedTime(3);
  app.setSkipTime(12);

  app.jumpToNextPostingTime();

  EXPECT_DOUBLE_EQ(app.skipTime(), 12);
}

TEST(TS_MOOSAppEventTest, JumpToNextPostingCanMoveBackwardForOverdueEvents)
{
  TestableTimerScript app;
  app.addNewEvent("var=A,val=one,time=4");
  app.scheduleEvents(false);
  app.setElapsedTime(9);

  app.jumpToNextPostingTime();

  EXPECT_DOUBLE_EQ(app.skipTime(), -5);
}

TEST(TS_MOOSAppMathTest, ExpandsOneDeepestExpressionPerCall)
{
  TestableTimerScript app;
  std::string value = "speed={{3+2}*4}";

  EXPECT_TRUE(app.handleMathExpr(value));
  EXPECT_EQ(value, "speed={5*4}");

  EXPECT_TRUE(app.handleMathExpr(value));
  EXPECT_EQ(value, "speed=20");
}

TEST(TS_MOOSAppMathTest, HandlesOperatorsAndNegativeLeftOperand)
{
  TestableTimerScript app;
  std::string product = "x={2.5*4}";
  std::string quotient = "x={9/2}";
  std::string difference = "x={5-8}";
  std::string negative_sum = "x={-3+1}";

  EXPECT_TRUE(app.handleMathExpr(product));
  EXPECT_TRUE(app.handleMathExpr(quotient));
  EXPECT_TRUE(app.handleMathExpr(difference));
  EXPECT_TRUE(app.handleMathExpr(negative_sum));

  EXPECT_EQ(product, "x=10");
  EXPECT_EQ(quotient, "x=4.5");
  EXPECT_EQ(difference, "x=-3");
  EXPECT_EQ(negative_sum, "x=-2");
}

TEST(TS_MOOSAppMathTest, OperatorSearchOrderPrefersMultiplyBeforeAddition)
{
  TestableTimerScript app;
  std::string value = "x={2+3*4}";

  EXPECT_FALSE(app.handleMathExpr(value));
  EXPECT_EQ(value, "x={2+3*4}");
}

TEST(TS_MOOSAppMathTest, RejectsMalformedOrNonNumericExpressionsWithoutMutation)
{
  TestableTimerScript app;
  std::string no_braces = "x=3+1";
  std::string unmatched = "x={3+1";
  std::string reversed = "x=3+1}";
  std::string no_operator = "x={3}";
  std::string non_numeric = "x={A+1}";

  EXPECT_FALSE(app.handleMathExpr(no_braces));
  EXPECT_FALSE(app.handleMathExpr(unmatched));
  EXPECT_FALSE(app.handleMathExpr(reversed));
  EXPECT_FALSE(app.handleMathExpr(no_operator));
  EXPECT_FALSE(app.handleMathExpr(non_numeric));

  EXPECT_EQ(no_braces, "x=3+1");
  EXPECT_EQ(unmatched, "x={3+1");
  EXPECT_EQ(reversed, "x=3+1}");
  EXPECT_EQ(no_operator, "x={3}");
  EXPECT_EQ(non_numeric, "x={A+1}");
}

TEST(TS_MOOSAppUtilityTest, RandomNumStringReturnsOnlyDigitsAndSkipsLeadingZero)
{
  TestableTimerScript app;
  srand(1);

  EXPECT_EQ(app.randomNumString(0), "");
  const std::string digits = app.randomNumString(12);

  EXPECT_LE(digits.size(), 12u);
  EXPECT_FALSE(digits.empty());
  EXPECT_TRUE(IsDigits(digits));
  EXPECT_NE(digits[0], '0');
}

TEST(TS_MOOSAppUtilityTest, BlockAppsRejectWhitespaceAndDuplicates)
{
  TestableTimerScript app;

  EXPECT_TRUE(app.addBlockApps("pHostInfo,pMarineViewer"));
  EXPECT_FALSE(app.addBlockApps("pHostInfo"));
  EXPECT_FALSE(app.addBlockApps("bad app"));

  EXPECT_EQ(app.blockAppCount(), 2u);
  EXPECT_TRUE(app.hasBlockApp("pHostInfo"));
  EXPECT_TRUE(app.hasBlockApp("pMarineViewer"));
}

TEST(TS_MOOSAppUtilityTest, EmptyBlockAppTokenIsAcceptedAsCurrentBehavior)
{
  TestableTimerScript app;

  EXPECT_TRUE(app.addBlockApps("pHostInfo,"));

  EXPECT_EQ(app.blockAppCount(), 1u);
  EXPECT_TRUE(app.hasBlockApp("pHostInfo"));
  EXPECT_FALSE(app.hasBlockApp(""));
}

TEST(TS_MOOSAppUtilityTest, CheckBlockAppsRemovesExactClientNamesOnly)
{
  TestableTimerScript app;
  ASSERT_TRUE(app.addBlockApps("pHostInfo,pMarineViewer"));

  app.checkBlockApps("uTimerScript, pHostInfo");
  EXPECT_TRUE(app.hasBlockApp("pHostInfo"));

  app.checkBlockApps("uTimerScript,pHostInfo,pMarineViewer");
  EXPECT_EQ(app.blockAppCount(), 0u);
}
