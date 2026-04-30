#include <gtest/gtest.h>

#include <memory>

#include "BHV_AbortToPoint.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV abort to point behavior: builds course speed objective toward abort point.
TEST(BHVAbortToPointTest, BuildsCourseSpeedObjectiveTowardAbortPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1.0);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("radius", "10"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "80"));
  EXPECT_TRUE(behavior.setParam("ipf-type", "zaic"));
  EXPECT_FALSE(behavior.setParam("point", "bad"));
  EXPECT_FALSE(behavior.setParam("speed", "0"));
  EXPECT_FALSE(behavior.setParam("radius", "0"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 90, 2));
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 0, 0));
}

// Covers BHV abort to point behavior: rate of closure objective rewards closing toward point.
TEST(BHVAbortToPointTest, RateOfClosureObjectiveRewardsClosingTowardPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1.0);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=100,y=0"));
  ASSERT_TRUE(behavior.setParam("speed", "3"));
  ASSERT_TRUE(behavior.setParam("ipf-type", "rate_of_closure"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 90, 3),
            evalCourseSpeedPoint(*ipf, 270, 3));
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 90, 3),
            evalCourseSpeedPoint(*ipf, 90, 0));
}

// Covers BHV abort to point behavior: rejects bad IPF type without replacing previous method.
TEST(BHVAbortToPointTest, RejectsBadIpfTypeWithoutReplacingPreviousMethod)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1.0);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("ipf-type", "roc"));
  EXPECT_TRUE(behavior.setParam("ipf-type", "not_a_known_method"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 180, 2));
}

// Covers BHV abort to point behavior: missing ownship navigation suppresses objective and posts error.
TEST(BHVAbortToPointTest, MissingOwnshipNavigationSuppressesObjectiveAndPostsError)
{
  InfoBuffer info;
  info.setValue("NAV_SPEED", 1.0);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  EXPECT_EQ(ipf, nullptr);
  EXPECT_FALSE(behavior.stateOK());

  VarDataPair error;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "BHV_ERROR", error));
  ASSERT_TRUE(error.is_string());
  EXPECT_FALSE(error.get_sdata().empty());
}

// Covers BHV abort to point behavior: point parser accepts bare coordinate pair format.
TEST(BHVAbortToPointTest, PointParserAcceptsBareCoordinatePairFormat)
{
  InfoBuffer info;
  info.setValue("NAV_X", 10);
  info.setValue("NAV_Y", -5);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "10,95"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 180, 2));
}

// Covers BHV abort to point behavior: ZAIC course objective wraps cleanly across north.
TEST(BHVAbortToPointTest, ZaicCourseObjectiveWrapsCleanlyAcrossNorth)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=-10,y=100"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("ipf-type", "zaic"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);

  EXPECT_GT(evalCourseSpeedPoint(*ipf, 354, 2),
            evalCourseSpeedPoint(*ipf, 174, 2));
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 354, 2),
            evalCourseSpeedPoint(*ipf, 6, 2));
}

// Covers BHV abort to point behavior: missing speed mail still builds from configured cruise speed.
TEST(BHVAbortToPointTest, MissingSpeedMailStillBuildsFromConfiguredCruiseSpeed)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 0);
  info.setValue("NAV_HEADING", 0);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_TRUE(behavior.stateOK());
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 180, 2));
}

// Covers BHV abort to point behavior: capture radii are accepted but do not suppress objective at point.
TEST(BHVAbortToPointTest, CaptureRadiiAreAcceptedButDoNotSuppressObjectiveAtPoint)
{
  InfoBuffer info;
  info.setValue("NAV_X", 0);
  info.setValue("NAV_Y", 100);
  info.setValue("NAV_HEADING", 0);
  info.setValue("NAV_SPEED", 1);

  BHV_AbortToPoint behavior(makeCourseSpeedDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("point", "x=0,y=100"));
  ASSERT_TRUE(behavior.setParam("speed", "2"));
  ASSERT_TRUE(behavior.setParam("radius", "25"));
  ASSERT_TRUE(behavior.setParam("nm_radius", "50"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(evalCourseSpeedPoint(*ipf, 0, 2),
            evalCourseSpeedPoint(*ipf, 180, 2));
}
