#include <gtest/gtest.h>

#include <memory>

#include "BHV_FullStop.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

namespace {

class InspectableFullStop : public BHV_FullStop {
 public:
  using BHV_FullStop::BHV_FullStop;
  bool completed() const { return m_completed; }
};

void seedFullStopInfo(InfoBuffer& info, double time, double speed)
{
  info.setCurrTime(time);
  info.setValue("NAV_X", 10);
  info.setValue("NAV_Y", 20);
  info.setValue("NAV_HEADING", 90);
  info.setValue("NAV_SPEED", speed);
}

}  // namespace

// Covers BHV full stop behavior: holds stem heading and commands zero speed until stopped.
TEST(BHVFullStopTest, HoldsStemHeadingAndCommandsZeroSpeedUntilStopped)
{
  InfoBuffer info;
  seedFullStopInfo(info, 10, 2.0);

  InspectableFullStop behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("stop_thresh", "0.25"));
  ASSERT_TRUE(behavior.setParam("mark_duration", "5"));
  ASSERT_TRUE(behavior.setParam("delay_complete", "0"));
  ASSERT_TRUE(behavior.setParam("mark_flag", "FULL_STOP_MARK=true"));
  ASSERT_TRUE(behavior.setParam("unmark_flag", "FULL_STOP_MARK=false"));
  EXPECT_FALSE(behavior.setParam("stop_thresh", "-1"));

  behavior.onSetParamComplete();
  behavior.onIdleToRunState();

  VarDataPair begin_point;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", begin_point));
  EXPECT_NE(begin_point.get_sdata().find("x=10"), std::string::npos);
  EXPECT_NE(begin_point.get_sdata().find("y=20"), std::string::npos);

  behavior.clearMessages();
  std::unique_ptr<IvPFunction> braking_ipf(behavior.onRunState());
  ASSERT_NE(braking_ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*braking_ipf, 90, 0),
            evalCourseSpeedPoint(*braking_ipf, 180, 0));
  EXPECT_GT(evalCourseSpeedPoint(*braking_ipf, 90, 0),
            evalCourseSpeedPoint(*braking_ipf, 90, 2));

  VarDataPair mark;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FULL_STOP_MARK", mark));
  EXPECT_EQ(mark.get_sdata(), "true");

  behavior.clearMessages();
  seedFullStopInfo(info, 11, 0.05);
  std::unique_ptr<IvPFunction> stopped_ipf(behavior.onRunState());
  EXPECT_EQ(stopped_ipf, nullptr);
  EXPECT_TRUE(behavior.completed());

  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "VIEW_POINT", begin_point));
  EXPECT_NE(begin_point.get_sdata().find("active=false"), std::string::npos);
}

// Covers BHV full stop behavior: delay complete keeps objective until delay expires.
TEST(BHVFullStopTest, DelayCompleteKeepsObjectiveUntilDelayExpires)
{
  InfoBuffer info;
  seedFullStopInfo(info, 20, 1.0);

  InspectableFullStop behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("stop_thresh", "0.25"));
  ASSERT_TRUE(behavior.setParam("delay_complete", "3"));
  ASSERT_TRUE(behavior.setParam("mark_duration", "2"));
  ASSERT_TRUE(behavior.setParam("mark_flag", "FULL_STOP_MARK=true"));
  ASSERT_TRUE(behavior.setParam("unmark_flag", "FULL_STOP_MARK=false"));

  behavior.onSetParamComplete();
  behavior.onIdleToRunState();
  behavior.clearMessages();

  seedFullStopInfo(info, 21, 0.05);
  std::unique_ptr<IvPFunction> pending_ipf(behavior.onRunState());
  ASSERT_NE(pending_ipf, nullptr);
  EXPECT_FALSE(behavior.completed());

  VarDataPair mark;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FULL_STOP_MARK", mark));
  EXPECT_EQ(mark.get_sdata(), "true");

  behavior.clearMessages();
  seedFullStopInfo(info, 24.5, 0.0);
  std::unique_ptr<IvPFunction> done_ipf(behavior.onRunState());
  EXPECT_EQ(done_ipf, nullptr);
  EXPECT_TRUE(behavior.completed());

  VarDataPair unmark;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FULL_STOP_MARK", unmark));
  EXPECT_EQ(unmark.get_sdata(), "false");
}

// Covers BHV full stop behavior: missing navigation mail prevents begin and run.
TEST(BHVFullStopTest, MissingNavigationMailPreventsBeginAndRun)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_X", 10);
  info.setValue("NAV_Y", 20);
  info.setValue("NAV_SPEED", 1);

  InspectableFullStop behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  behavior.onIdleToRunState();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  ASSERT_TRUE(error.is_string());
  EXPECT_NE(error.get_sdata().find("NAV_HEADING"), std::string::npos);
}

// Covers BHV full stop behavior: moving back toward begin point completes even before speed threshold.
TEST(BHVFullStopTest, MovingBackTowardBeginPointCompletesEvenBeforeSpeedThreshold)
{
  InfoBuffer info;
  seedFullStopInfo(info, 30, 2.0);

  InspectableFullStop behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("stop_thresh", "0.1"));
  ASSERT_TRUE(behavior.setParam("delay_complete", "0"));

  behavior.onSetParamComplete();
  behavior.onIdleToRunState();

  info.setValue("NAV_X", 20);
  info.setValue("NAV_Y", 20);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> outbound_ipf(behavior.onRunState());
  ASSERT_NE(outbound_ipf, nullptr);
  EXPECT_FALSE(behavior.completed());

  info.setCurrTime(31);
  info.setValue("NAV_X", 15);
  info.setValue("NAV_Y", 20);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> returning_ipf(behavior.onRunState());
  EXPECT_EQ(returning_ipf, nullptr);
  EXPECT_TRUE(behavior.completed());
}

// Covers BHV full stop behavior: mark flags expand stop distance and elapsed time.
TEST(BHVFullStopTest, MarkFlagsExpandStopDistanceAndElapsedTime)
{
  InfoBuffer info;
  seedFullStopInfo(info, 40, 1.0);

  InspectableFullStop behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("mark_duration", "10"));
  ASSERT_TRUE(behavior.setParam("mark_flag", "FULL_STOP_STATUS=dist=$[STOP_DIST],time=$[STOP_TIME]"));

  behavior.onSetParamComplete();
  behavior.onIdleToRunState();

  info.setCurrTime(41);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> baseline_ipf(behavior.onRunState());
  ASSERT_NE(baseline_ipf, nullptr);

  info.setCurrTime(43);
  info.setValue("NAV_X", 13);
  info.setValue("NAV_Y", 24);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  VarDataPair status;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "FULL_STOP_STATUS", status));
  ASSERT_TRUE(status.is_string());
  EXPECT_NE(status.get_sdata().find("dist=5"), std::string::npos);
  EXPECT_NE(status.get_sdata().find("time=3"), std::string::npos);
}

// Covers BHV full stop behavior: stale navigation mail posts dedicated error.
TEST(BHVFullStopTest, StaleNavigationMailPostsDedicatedError)
{
  InfoBuffer info;
  seedFullStopInfo(info, 0, 1.0);

  InspectableFullStop behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  behavior.onSetParamComplete();
  behavior.onIdleToRunState();

  info.setCurrTime(20);
  behavior.clearMessages();
  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  ASSERT_TRUE(error.is_string());
  EXPECT_NE(error.get_sdata().find("stale"), std::string::npos);
}
