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

// Covers logic aspect behavior: evaluates mission eval lead pass and fail conditions.
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

// Covers logic aspect behavior: fails when fail condition is met after pass conditions.
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

// Covers logic aspect behavior: lead-only mission eval aspects pass once led.
TEST(LogicAspectTest, LeadOnlyAspectPassesWhenLeadConditionIsMet)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_EQ(aspect.checkConfig(), "");
  EXPECT_EQ(aspect.getInfo(), "lead_cnt=1, pass_cnt=0, fail_cnt=0");

  aspect.update();
  EXPECT_FALSE(aspect.isEvaluated());
  EXPECT_EQ(aspect.getStatus(), "unmet_lead_cond: [DEPLOY = true]");

  info.setValue("DEPLOY", "true");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_TRUE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "pass");
}

// Covers logic aspect behavior: fail-only mission eval aspects fail on fail condition.
TEST(LogicAspectTest, FailOnlyAspectFailsWhenFailConditionIsMet)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addFailCondition("FORCE_FAIL = true"));
  ASSERT_EQ(aspect.checkConfig(), "");
  EXPECT_EQ(aspect.getInfo(), "lead_cnt=1, pass_cnt=0, fail_cnt=1");

  info.setValue("DEPLOY", "true");
  info.setValue("FORCE_FAIL", "true");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_FALSE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "met_fail_cond: [FORCE_FAIL = true]");
}

// Covers logic aspect behavior: fail-only mission eval aspects pass when fail condition is unmet.
TEST(LogicAspectTest, FailOnlyAspectPassesWhenFailConditionIsUnmet)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addFailCondition("FORCE_FAIL = true"));
  ASSERT_EQ(aspect.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("FORCE_FAIL", "false");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_TRUE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "pass");
}

// Covers logic aspect behavior: pass-only mission eval aspects fail when pass condition is unmet.
TEST(LogicAspectTest, PassOnlyAspectFailsWhenPassConditionIsUnmet)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addPassCondition("MISSION_DONE = true"));
  ASSERT_EQ(aspect.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_FALSE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "unmet_pass_cond: [MISSION_DONE = true]");
}

// Covers logic aspect behavior: all lead conditions must be met before evaluation.
TEST(LogicAspectTest, RequiresAllLeadConditionsBeforeEvaluating)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addLeadCondition("READY = true"));
  ASSERT_TRUE(aspect.addPassCondition("MISSION_DONE = true"));
  ASSERT_EQ(aspect.checkConfig(), "");
  EXPECT_EQ(aspect.getInfo(), "lead_cnt=2, pass_cnt=1, fail_cnt=0");

  info.setValue("DEPLOY", "true");
  info.setValue("MISSION_DONE", "true");
  aspect.update();
  EXPECT_FALSE(aspect.isEvaluated());
  EXPECT_EQ(aspect.getStatus(), "unmet_lead_cond: [READY = true]");

  info.setValue("READY", "true");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_TRUE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "pass");
}

// Covers logic aspect behavior: all pass conditions must be met before passing.
TEST(LogicAspectTest, RequiresAllPassConditionsBeforePassing)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addPassCondition("LEG_ONE_DONE = true"));
  ASSERT_TRUE(aspect.addPassCondition("LEG_TWO_DONE = true"));
  ASSERT_TRUE(aspect.addFailCondition("COLLISION = true"));
  ASSERT_EQ(aspect.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("LEG_ONE_DONE", "true");
  info.setValue("LEG_TWO_DONE", "false");
  info.setValue("COLLISION", "false");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_FALSE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "unmet_pass_cond: [LEG_TWO_DONE = true]");
}

// Covers logic aspect behavior: an unmet pass condition is reported before a met fail condition.
TEST(LogicAspectTest, ReportsUnmetPassBeforeMetFailCondition)
{
  InfoBuffer info;
  LogicAspect aspect;
  aspect.setInfoBuffer(&info);
  ASSERT_TRUE(aspect.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(aspect.addPassCondition("MISSION_DONE = true"));
  ASSERT_TRUE(aspect.addFailCondition("COLLISION = true"));
  ASSERT_EQ(aspect.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("MISSION_DONE", "false");
  info.setValue("COLLISION", "true");
  aspect.update();
  EXPECT_TRUE(aspect.isEvaluated());
  EXPECT_FALSE(aspect.isSatisfied());
  EXPECT_EQ(aspect.getStatus(), "unmet_pass_cond: [MISSION_DONE = true]");
}

// Covers logic test sequence behavior: splits mission eval conditions into sequential aspects.
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

// Covers logic test sequence behavior: stops sequence at first unsatisfied aspect.
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

// Covers logic test sequence behavior: reports configuration warnings before enablement.
TEST(LogicTestSequenceTest, ReportsConfigurationWarningsBeforeEnablement)
{
  LogicTestSequence empty;
  EXPECT_FALSE(empty.enabled());
  EXPECT_EQ(empty.checkConfig(), "LogicAspect 0: No lead_conditions");
  EXPECT_FALSE(empty.enabled());

  LogicTestSequence no_pass;
  ASSERT_TRUE(no_pass.addLeadCondition("DEPLOY=true"));
  EXPECT_EQ(no_pass.checkConfig(), "");
  EXPECT_TRUE(no_pass.enabled());
}

// Covers logic test sequence behavior: appends new lead conditions after any pass or fail condition.
TEST(LogicTestSequenceTest, AppendsNewLeadConditionsAfterAnyPassOrFailCondition)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);

  ASSERT_TRUE(sequence.addLeadCondition("LEG_ONE=true"));
  ASSERT_TRUE(sequence.addFailCondition("COLLISION=true"));
  ASSERT_TRUE(sequence.addLeadCondition("LEG_TWO=true"));
  ASSERT_TRUE(sequence.addPassCondition("DONE=true"));

  EXPECT_EQ(sequence.size(), 2u);
  EXPECT_EQ(sequence.failConditions(0), 1u);
  EXPECT_EQ(sequence.leadConditions(1), 1u);
  EXPECT_EQ(sequence.passConditions(1), 1u);
  EXPECT_EQ(sequence.checkConfig(), "");

  info.setValue("LEG_ONE", "true");
  info.setValue("COLLISION", "false");
  sequence.update();
  EXPECT_FALSE(sequence.isEvaluated());
  EXPECT_EQ(sequence.currIndex(), 1u);

  info.setValue("LEG_TWO", "true");
  info.setValue("DONE", "true");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_TRUE(sequence.isSatisfied());
}

// Covers logic test sequence behavior: multiple leads remain in one aspect before pass/fail conditions.
TEST(LogicTestSequenceTest, KeepsConsecutiveLeadConditionsInSameAspect)
{
  LogicTestSequence sequence;

  ASSERT_TRUE(sequence.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(sequence.addLeadCondition("READY = true"));
  ASSERT_TRUE(sequence.addPassCondition("MISSION_DONE = true"));

  EXPECT_EQ(sequence.size(), 1u);
  EXPECT_EQ(sequence.leadConditions(0), 2u);
  EXPECT_EQ(sequence.passConditions(0), 1u);
  EXPECT_EQ(sequence.failConditions(0), 0u);
  EXPECT_EQ(sequence.checkConfig(), "");
}

// Covers logic test sequence behavior: stops sequence at an unmet pass condition in a later aspect.
TEST(LogicTestSequenceTest, StopsSequenceAtLaterUnmetPassCondition)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);
  ASSERT_TRUE(sequence.addLeadCondition("LEG_ONE = true"));
  ASSERT_TRUE(sequence.addPassCondition("LEG_ONE_DONE = true"));
  ASSERT_TRUE(sequence.addLeadCondition("LEG_TWO = true"));
  ASSERT_TRUE(sequence.addPassCondition("LEG_TWO_DONE = true"));
  ASSERT_EQ(sequence.checkConfig(), "");

  info.setValue("LEG_ONE", "true");
  info.setValue("LEG_ONE_DONE", "true");
  sequence.update();
  EXPECT_FALSE(sequence.isEvaluated());
  EXPECT_EQ(sequence.currIndex(), 1u);

  info.setValue("LEG_TWO", "true");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_FALSE(sequence.isSatisfied());
  EXPECT_EQ(sequence.currIndex(), 1u);
  EXPECT_EQ(sequence.getStatus(), "unmet_pass_cond: [LEG_TWO_DONE = true]");
}

// Covers logic test sequence behavior: completed failing evaluations are sticky.
TEST(LogicTestSequenceTest, FailingEvaluationDoesNotReopenAfterMailChanges)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);
  ASSERT_TRUE(sequence.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(sequence.addPassCondition("MISSION_DONE = true"));
  ASSERT_TRUE(sequence.addFailCondition("COLLISION = true"));
  ASSERT_EQ(sequence.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("MISSION_DONE", "true");
  info.setValue("COLLISION", "true");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_FALSE(sequence.isSatisfied());
  EXPECT_EQ(sequence.getStatus(), "met_fail_cond: [COLLISION = true]");

  info.setValue("COLLISION", "false");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_FALSE(sequence.isSatisfied());
  EXPECT_EQ(sequence.getStatus(), "met_fail_cond: [COLLISION = true]");
}

// Covers logic test sequence behavior: completed passing evaluations are sticky.
TEST(LogicTestSequenceTest, PassingEvaluationDoesNotReopenAfterMailChanges)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);
  ASSERT_TRUE(sequence.addLeadCondition("DEPLOY = true"));
  ASSERT_TRUE(sequence.addPassCondition("MISSION_DONE = true"));
  ASSERT_TRUE(sequence.addFailCondition("COLLISION = true"));
  ASSERT_EQ(sequence.checkConfig(), "");

  info.setValue("DEPLOY", "true");
  info.setValue("MISSION_DONE", "true");
  info.setValue("COLLISION", "false");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_TRUE(sequence.isSatisfied());
  EXPECT_EQ(sequence.getStatus(), "pass");

  info.setValue("COLLISION", "true");
  sequence.update();
  EXPECT_TRUE(sequence.isEvaluated());
  EXPECT_TRUE(sequence.isSatisfied());
  EXPECT_EQ(sequence.getStatus(), "pass");
}

// Covers logic test sequence behavior: report includes current aspect and info buffer state.
TEST(LogicTestSequenceTest, ReportIncludesCurrentAspectAndInfoBufferState)
{
  InfoBuffer info;
  LogicTestSequence sequence;
  sequence.setInfoBuffer(&info);
  ASSERT_TRUE(sequence.addLeadCondition("DEPLOY=true"));
  ASSERT_TRUE(sequence.addPassCondition("MISSION_DONE=true"));
  ASSERT_EQ(sequence.checkConfig(), "");

  info.setValue("DEPLOY", "false");
  std::list<std::string> report = sequence.getReport();
  std::string joined;
  for(const std::string& line : report)
    joined += line + "\n";

  EXPECT_NE(joined.find("Logic Test Sequence: Total Aspects (1 total)"),
            std::string::npos);
  EXPECT_NE(joined.find("InfoBuffer:"), std::string::npos);
  EXPECT_NE(joined.find("DEPLOY"), std::string::npos);
  EXPECT_NE(joined.find("MISSION_DONE"), std::string::npos);
}
