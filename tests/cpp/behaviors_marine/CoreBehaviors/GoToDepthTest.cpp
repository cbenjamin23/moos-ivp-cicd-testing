#include <gtest/gtest.h>

#include <memory>

#include "BHV_GoToDepth.h"
#include "BehaviorMarineTestUtils.h"
#include "InfoBuffer.h"

// Covers BHV go to depth behavior: advances depth levels after arrival and plateau.
TEST(BHVGoToDepthTest, AdvancesDepthLevelsAfterArrivalAndPlateau)
{
  InfoBuffer info;
  info.setCurrTime(10);
  info.setValue("NAV_DEPTH", 9.8);

  BHV_GoToDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "10,0:20"));
  ASSERT_TRUE(behavior.setParam("arrival_delta", "0.5"));
  ASSERT_TRUE(behavior.setParam("arrival_flag", "ARRIVED"));
  EXPECT_FALSE(behavior.setParam("depth", "10,-1"));
  EXPECT_FALSE(behavior.setParam("repeat", "-1"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*first_ipf, 10), evalOneDimPoint(*first_ipf, 20));

  VarDataPair arrival;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "GTD_ARRIVED", arrival));
  EXPECT_DOUBLE_EQ(arrival.get_ddata(), 1);

  behavior.clearMessages();
  info.setCurrTime(11);
  info.setValue("NAV_DEPTH", 10.0);

  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*second_ipf, 20), evalOneDimPoint(*second_ipf, 10));

  VarDataPair next_level;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LEVEL_DEPTH", next_level));
  EXPECT_DOUBLE_EQ(next_level.get_ddata(), 20);
}

// Covers BHV go to depth behavior: repeats depth sequence and completes after repeat count.
TEST(BHVGoToDepthTest, RepeatsDepthSequenceAndCompletesAfterRepeatCount)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 5);

  BHV_GoToDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "5:8"));
  ASSERT_TRUE(behavior.setParam("arrival_delta", "0"));
  ASSERT_TRUE(behavior.setParam("repeat", "1"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*first_ipf, 5), evalOneDimPoint(*first_ipf, 8));

  info.setCurrTime(1);
  info.setValue("NAV_DEPTH", 5);
  std::unique_ptr<IvPFunction> second_ipf(behavior.onRunState());
  ASSERT_NE(second_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*second_ipf, 8), evalOneDimPoint(*second_ipf, 5));

  info.setCurrTime(2);
  info.setValue("NAV_DEPTH", 8);
  std::unique_ptr<IvPFunction> arrival_ipf(behavior.onRunState());
  ASSERT_NE(arrival_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*arrival_ipf, 8), evalOneDimPoint(*arrival_ipf, 5));

  info.setCurrTime(3);
  info.setValue("NAV_DEPTH", 5);
  std::unique_ptr<IvPFunction> repeated_ipf(behavior.onRunState());
  ASSERT_NE(repeated_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*repeated_ipf, 5), evalOneDimPoint(*repeated_ipf, 8));

  info.setCurrTime(4);
  info.setValue("NAV_DEPTH", 5);
  std::unique_ptr<IvPFunction> repeated_arrival_ipf(behavior.onRunState());
  ASSERT_NE(repeated_arrival_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*repeated_arrival_ipf, 8), evalOneDimPoint(*repeated_arrival_ipf, 5));

  info.setCurrTime(5);
  info.setValue("NAV_DEPTH", 8);
  std::unique_ptr<IvPFunction> final_level_ipf(behavior.onRunState());
  ASSERT_NE(final_level_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*final_level_ipf, 8), evalOneDimPoint(*final_level_ipf, 5));

  info.setCurrTime(6);
  info.setValue("NAV_DEPTH", 8);
  std::unique_ptr<IvPFunction> completed_ipf(behavior.onRunState());
  EXPECT_EQ(completed_ipf, nullptr);
}

// Covers BHV go to depth behavior: missing depth mail or domain posts behavior errors.
TEST(BHVGoToDepthTest, MissingDepthMailOrDomainPostsBehaviorErrors)
{
  BHV_GoToDepth missing_mail(makeDepthDomain());
  ASSERT_TRUE(missing_mail.setParam("depth", "10"));
  std::unique_ptr<IvPFunction> missing_mail_ipf(missing_mail.onRunState());
  EXPECT_EQ(missing_mail_ipf, nullptr);
  EXPECT_FALSE(missing_mail.stateOK());

  VarDataPair missing_mail_error;
  ASSERT_TRUE(findBehaviorMessage(missing_mail.getMessages(), "BHV_ERROR",
                                  missing_mail_error));
  EXPECT_NE(missing_mail_error.get_sdata().find("No ownship Depth"),
            std::string::npos);

  BHV_GoToDepth missing_domain(makeCourseSpeedDomain());
  ASSERT_TRUE(missing_domain.setParam("depth", "10"));
  std::unique_ptr<IvPFunction> missing_domain_ipf(missing_domain.onRunState());
  EXPECT_EQ(missing_domain_ipf, nullptr);
  EXPECT_FALSE(missing_domain.stateOK());

  VarDataPair missing_domain_error;
  ASSERT_TRUE(findBehaviorMessage(missing_domain.getMessages(), "BHV_ERROR",
                                  missing_domain_error));
  EXPECT_NE(missing_domain_error.get_sdata().find("No 'depth' variable"),
            std::string::npos);
}

// Covers BHV go to depth behavior: rejects malformed depth sequence without clearing prior plan.
TEST(BHVGoToDepthTest, RejectsMalformedDepthSequenceWithoutClearingPriorPlan)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 0);

  BHV_GoToDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "4:9"));
  EXPECT_FALSE(behavior.setParam("depth", "bad"));
  EXPECT_FALSE(behavior.setParam("depth", "4,1,2"));
  EXPECT_FALSE(behavior.setParam("depth", "4,-1"));
  EXPECT_FALSE(behavior.setParam("arrival_delta", "-0.1"));

  std::unique_ptr<IvPFunction> ipf(behavior.onRunState());
  ASSERT_NE(ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*ipf, 4), evalOneDimPoint(*ipf, 9));
}

// Covers BHV go to depth behavior: crossing target depth triggers arrival in both directions.
TEST(BHVGoToDepthTest, CrossingTargetDepthTriggersArrivalInBothDirections)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 0);

  BHV_GoToDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "10,2:4"));
  ASSERT_TRUE(behavior.setParam("arrival_delta", "0.25"));
  ASSERT_TRUE(behavior.setParam("arrival_flag", "LEVEL_HIT"));

  std::unique_ptr<IvPFunction> first_ipf(behavior.onRunState());
  ASSERT_NE(first_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*first_ipf, 10), evalOneDimPoint(*first_ipf, 4));

  behavior.clearMessages();
  info.setCurrTime(1);
  info.setValue("NAV_DEPTH", 12);
  std::unique_ptr<IvPFunction> down_cross_ipf(behavior.onRunState());
  ASSERT_NE(down_cross_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*down_cross_ipf, 10),
            evalOneDimPoint(*down_cross_ipf, 4));

  VarDataPair first_arrival;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "GTD_LEVEL_HIT",
                                  first_arrival));
  EXPECT_DOUBLE_EQ(first_arrival.get_ddata(), 1);

  behavior.clearMessages();
  info.setCurrTime(2);
  info.setValue("NAV_DEPTH", 12);
  std::unique_ptr<IvPFunction> plateau_ipf(behavior.onRunState());
  ASSERT_NE(plateau_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*plateau_ipf, 10),
            evalOneDimPoint(*plateau_ipf, 4));
  EXPECT_FALSE(findBehaviorMessage(behavior.getMessages(), "LEVEL_DEPTH",
                                   first_arrival));

  info.setCurrTime(4);
  info.setValue("NAV_DEPTH", 12);
  std::unique_ptr<IvPFunction> next_level_ipf(behavior.onRunState());
  ASSERT_NE(next_level_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*next_level_ipf, 4),
            evalOneDimPoint(*next_level_ipf, 10));

  VarDataPair next_level;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "LEVEL_DEPTH",
                                  next_level));
  EXPECT_DOUBLE_EQ(next_level.get_ddata(), 4);

  behavior.clearMessages();
  info.setCurrTime(5);
  info.setValue("NAV_DEPTH", 2);
  std::unique_ptr<IvPFunction> up_cross_ipf(behavior.onRunState());
  ASSERT_NE(up_cross_ipf, nullptr);

  VarDataPair second_arrival;
  ASSERT_TRUE(findBehaviorMessage(behavior.getMessages(), "GTD_LEVEL_HIT",
                                  second_arrival));
  EXPECT_DOUBLE_EQ(second_arrival.get_ddata(), 2);
}

// Covers BHV go to depth behavior: base perpetual param is rejected and single level completes.
TEST(BHVGoToDepthTest, BasePerpetualParamIsRejectedAndSingleLevelCompletes)
{
  InfoBuffer info;
  info.setCurrTime(0);
  info.setValue("NAV_DEPTH", 10);

  BHV_GoToDepth behavior(makeDepthDomain());
  behavior.setInfoBuffer(&info);

  ASSERT_TRUE(behavior.setParam("depth", "10"));
  ASSERT_TRUE(behavior.setParam("arrival_delta", "0"));
  EXPECT_FALSE(behavior.setParam("perpetual", "true"));

  std::unique_ptr<IvPFunction> arrival_ipf(behavior.onRunState());
  ASSERT_NE(arrival_ipf, nullptr);
  EXPECT_GT(evalOneDimPoint(*arrival_ipf, 10), evalOneDimPoint(*arrival_ipf, 20));

  info.setCurrTime(1);
  info.setValue("NAV_DEPTH", 10);
  std::unique_ptr<IvPFunction> complete_ipf(behavior.onRunState());
  EXPECT_EQ(complete_ipf, nullptr);
}
