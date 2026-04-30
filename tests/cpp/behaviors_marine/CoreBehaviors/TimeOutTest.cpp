#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "BHV_TimeOut.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

class TestableTimeOut : public BHV_TimeOut {
public:
  explicit TestableTimeOut(IvPDomain domain) : BHV_TimeOut(domain) {}
  using BHV_TimeOut::setTimeStamps;
};

// Covers BHV time out behavior: returns no objective and posts error after max time.
TEST(BHVTimeOutTest, ReturnsNoObjectiveAndPostsErrorAfterMaxTime)
{
  InfoBuffer info;
  info.setCurrTime(5);

  TestableTimeOut behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("max_time", "3"));
  EXPECT_FALSE(behavior.setParam("max_time", "-1"));
  EXPECT_FALSE(behavior.setParam("max_time", "soon"));

  behavior.setTimeStamps();
  std::unique_ptr<IvPFunction> initial_ipf(behavior.onRunState());
  EXPECT_EQ(initial_ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());

  info.setCurrTime(9);
  behavior.setTimeStamps();
  std::unique_ptr<IvPFunction> timed_out_ipf(behavior.onRunState());
  EXPECT_EQ(timed_out_ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("MaxTime:3"), std::string::npos);
}

// Covers BHV time out behavior: boundary at max time is still ok and later time fails.
TEST(BHVTimeOutTest, BoundaryAtMaxTimeIsStillOkAndLaterTimeFails)
{
  // Timeout is strict: elapsed time equal to max_time is still runnable, and
  // only time greater than the threshold fails the behavior.
  InfoBuffer info;
  info.setCurrTime(100);

  TestableTimeOut behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("max_time", "4"));

  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_TRUE(behavior.stateOK());

  info.setCurrTime(104);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_TRUE(behavior.stateOK());

  info.setCurrTime(104.01);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("Elapsed Time"), std::string::npos);
}

// Covers BHV time out behavior: reset state ok allows subsequent short window checks.
TEST(BHVTimeOutTest, ResetStateOkAllowsSubsequentShortWindowChecks)
{
  InfoBuffer info;
  info.setCurrTime(0);

  TestableTimeOut behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("max_time", "1"));

  behavior.setTimeStamps();
  info.setCurrTime(2);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_FALSE(behavior.stateOK());

  behavior.resetStateOK();
  behavior.clearMessages();
  ASSERT_TRUE(behavior.setParam("max_time", "5"));
  info.setCurrTime(3);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_TRUE(behavior.stateOK());

  VarDataPair error;
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
}

// Covers BHV time out behavior: zero max time allows initial invocation but fails on any elapsed time.
TEST(BHVTimeOutTest, ZeroMaxTimeAllowsInitialInvocationButFailsOnAnyElapsedTime)
{
  InfoBuffer info;
  info.setCurrTime(12);

  TestableTimeOut behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("max_time", "0"));

  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_TRUE(behavior.stateOK());

  info.setCurrTime(12.01);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  EXPECT_NE(error.get_sdata().find("MaxTime:0"), std::string::npos);
}

// Covers BHV time out behavior: first timestamp becomes start time even when mission time is nonzero.
TEST(BHVTimeOutTest, FirstTimestampBecomesStartTimeEvenWhenMissionTimeIsNonzero)
{
  // The first timestamp observed by the behavior becomes its local start time;
  // elapsed duration is not measured from mission time zero.
  InfoBuffer info;
  info.setCurrTime(500);

  TestableTimeOut behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);
  ASSERT_TRUE(behavior.setParam("max_time", "10"));

  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_TRUE(behavior.stateOK());

  info.setCurrTime(509.5);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_TRUE(behavior.stateOK());

  info.setCurrTime(510.5);
  behavior.setTimeStamps();
  EXPECT_EQ(behavior.onRunState(), nullptr);
  EXPECT_FALSE(behavior.stateOK());
}
