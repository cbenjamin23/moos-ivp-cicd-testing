#include <gtest/gtest.h>

#include <list>
#include <set>
#include <string>
#include <vector>

#include "InfoBuffer.h"
#include "LogicAspect.h"
#include "LogicTestSequence.h"

namespace {

bool vectorContains(const std::vector<std::string>& lines,
                    const std::string& needle)
{
  for(const std::string& line : lines) {
    if(line.find(needle) != std::string::npos)
      return true;
  }
  return false;
}

}  // namespace

TEST(LogicAspectTest, EvaluatesMissionEvalLeadPassAndFailConditions)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);

  EXPECT_EQ(aspect.checkConfig(), "No lead_conditions");
  EXPECT_TRUE(aspect.addLeadCondition("STATION_REACHED = true"));
  EXPECT_TRUE(aspect.addPassCondition("FLAG_A = true"));
  EXPECT_TRUE(aspect.addPassCondition("NAV_X > 20"));
  EXPECT_TRUE(aspect.addFailCondition("COLLISION = true"));
  EXPECT_EQ(aspect.checkConfig(), "");
  EXPECT_TRUE(aspect.enabled());
  EXPECT_EQ(aspect.getInfo(), "lead_cnt=1, pass_cnt=2, fail_cnt=1");
  EXPECT_EQ(aspect.getLogicVars(),
            (std::set<std::string>{"COLLISION", "FLAG_A", "NAV_X",
                                   "STATION_REACHED"}));

  aspect.update();
  EXPECT_FALSE(aspect.isEvaluated());
  EXPECT_EQ(aspect.getStatus(), "unmet_lead_cond: [STATION_REACHED = true]");

  info.setValue("STATION_REACHED", "true");
  info.setValue("FLAG_A", "true");
  info.setValue("NAV_X", 25.0);
  info.setValue("COLLISION", "false");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_TRUE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "pass");

  std::vector<std::string> spec = aspect.getSpec();
  EXPECT_TRUE(vectorContains(spec, "Lead Conditions: (1)"));
  EXPECT_TRUE(vectorContains(spec, "STATION_REACHED = true"));
  EXPECT_TRUE(vectorContains(spec, "Pass Conditions: (2)"));
  EXPECT_TRUE(vectorContains(spec, "Fail Conditions: (1)"));
  EXPECT_TRUE(vectorContains(spec, "COLLISION = true"));
  EXPECT_TRUE(vectorContains(spec, "Satisfied(true)"));
}

TEST(LogicAspectTest, FailsWhenFailConditionIsMetAfterPassConditions)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addPassCondition("MISSION_DONE = true"));
  ASSERT_TRUE(aspect.addFailCondition("COLLISION = true"));
  ASSERT_EQ(aspect.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("MISSION_DONE", "true");
  info.setValue("COLLISION", "true");
  aspect.update();

  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_FALSE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "met_fail_cond: [COLLISION = true]");
}

TEST(LogicTestSequenceTest, SplitsMissionEvalConditionsIntoSequentialAspects)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);

  ASSERT_TRUE(sequence.addLeadCondition("STATION_REACHED = true"));
  ASSERT_TRUE(sequence.addPassCondition("FLAG_A = true"));
  ASSERT_TRUE(sequence.addLeadCondition("RETURN_STARTED = true"));
  ASSERT_TRUE(sequence.addPassCondition("RETURNED_HOME = true"));

  EXPECT_EQ(sequence.size(), 2u);
  EXPECT_EQ(sequence.leadConditions(0), 1u);
  EXPECT_EQ(sequence.passConditions(0), 1u);
  EXPECT_EQ(sequence.leadConditions(1), 1u);
  EXPECT_EQ(sequence.passConditions(1), 1u);
  EXPECT_EQ(sequence.getLogicVars(),
            (std::set<std::string>{"FLAG_A", "RETURNED_HOME",
                                   "RETURN_STARTED", "STATION_REACHED"}));
  EXPECT_EQ(sequence.checkConfig(), "");
  EXPECT_TRUE(sequence.enabled());

  sequence.update();
  EXPECT_FALSE(sequence.isEvaluated());
  EXPECT_EQ(sequence.currIndex(), 0u);
  EXPECT_EQ(sequence.getStatus(), "unmet_lead_cond: [STATION_REACHED = true]");

  info.setValue("STATION_REACHED", "true");
  info.setValue("FLAG_A", "true");
  sequence.update();
  EXPECT_FALSE(sequence.isEvaluated());
  EXPECT_EQ(sequence.currIndex(), 1u);
  EXPECT_EQ(sequence.getStatus(), "pass");

  info.setValue("RETURN_STARTED", "true");
  info.setValue("RETURNED_HOME", "true");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_TRUE(sequence.isSatisfied());
  EXPECT_EQ(sequence.currIndex(), 2u);

  std::vector<std::string> spec = sequence.getSpec();
  EXPECT_TRUE(vectorContains(spec, "LogicTestSequence: Total Aspects:2"));
  EXPECT_TRUE(vectorContains(spec, "Satisfied: true"));
}

TEST(LogicTestSequenceTest, StopsSequenceAtFirstUnsatisfiedAspect)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);
  ASSERT_TRUE(sequence.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(sequence.addPassCondition("MISSION_DONE = true"));
  ASSERT_TRUE(sequence.addFailCondition("COLLISION = true"));
  ASSERT_TRUE(sequence.addLeadCondition("NEVER_REACHED = true"));
  ASSERT_TRUE(sequence.addPassCondition("SHOULD_NOT_EVALUATE = true"));
  ASSERT_EQ(sequence.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("MISSION_DONE", "true");
  info.setValue("COLLISION", "true");
  sequence.update();

  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_FALSE(sequence.isSatisfied());
  EXPECT_EQ(sequence.currIndex(), 0u);
  EXPECT_EQ(sequence.getStatus(), "met_fail_cond: [COLLISION = true]");
}
