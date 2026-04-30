#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "LifeEvent.h"
#include "LifeEventHistory.h"
#include "BehaviorCoreTestUtils.h"

// Covers life event behavior: formats spec with optional seed post mortem and posting index.
TEST(LifeEventTest, FormatsSpecWithOptionalSeedPostMortemAndPostingIndex)
{
  LifeEvent event;
  event.setIteration(12);
  event.setTimeStamp(100.5);
  event.setBehaviorName("waypt_survey");
  event.setBehaviorType("BHV_Waypoint");
  event.setEventType("spawn");
  event.setSpawnString("helm startup");
  event.setPostingIndex(4);

  EXPECT_EQ(event.getSpec(7.25),
            "iter=12,time=7.25, bname=waypt_survey, btype=BHV_Waypoint, "
            "event=spawn, seed=helm startup, posting_index=4");

  LifeEvent same = event;
  EXPECT_TRUE(event == same);
  same.setPostMortem("duration");
  EXPECT_TRUE(event == same);
  same.setPostingIndex(5);
  EXPECT_FALSE(event == same);
}

// Covers life event behavior: formats minimal abort spec without time seed or posting index.
TEST(LifeEventTest, FormatsMinimalAbortSpecWithoutTimeSeedOrPostingIndex)
{
  LifeEvent event;
  event.setIteration(3);
  event.setBehaviorName("avoid_alpha");
  event.setBehaviorType("BHV_AvoidCollision");
  event.setEventType("abort");
  event.setPostMortem("contact timed out");

  EXPECT_EQ(event.getSpec(0),
            "iter=3, bname=avoid_alpha, btype=BHV_AvoidCollision, "
            "event=abort, post_mortem=contact timed out");
}

// Covers life event history behavior: parses deduplicates and reports behavior life events.
TEST(LifeEventHistoryTest, ParsesDeduplicatesAndReportsBehaviorLifeEvents)
{
  LifeEventHistory history;
  history.setBannerActive(false);
  history.setColorActive(false);

  history.addLifeEvent(
      "time=100.25,iter=4,bname=waypt,btype=BHV_Waypoint,event=spawn,"
      "seed=helm startup");
  history.addLifeEvent(
      "time=100.25,iter=4,bname=waypt,btype=BHV_Waypoint,event=spawn,"
      "seed=helm startup");
  history.addLifeEvent(
      "time=120.5,iter=9,bname=avoid,btype=BHV_AvoidCollision,event=abort,"
      "post_mortem=contact lost");

  std::vector<std::string> report = history.getReport();
  ASSERT_EQ(report.size(), 4u);
  EXPECT_TRUE(containsText(report[0], "Time"));
  EXPECT_TRUE(containsText(report[0], "Behavior Type"));
  EXPECT_TRUE(containsText(report[2], "waypt"));
  EXPECT_TRUE(containsText(report[2], "helm startup"));
  EXPECT_TRUE(containsText(report[3], "avoid"));
  EXPECT_TRUE(containsText(report[3], "contact lost"));
  EXPECT_FALSE(history.isStale());
}

// Covers life event history behavior: parses mixed case fields and caches report until new event.
TEST(LifeEventHistoryTest, ParsesMixedCaseFieldsAndCachesReportUntilNewEvent)
{
  LifeEventHistory history;
  history.setBannerActive(false);
  history.setColorActive(false);

  // Life event parsing is case-insensitive, and getReport() caches formatted
  // rows until a new event marks the history stale.
  history.addLifeEvent(
      " TIME = 42.75 , ITER = 8 , BNAME = loiter_port , "
      "BTYPE = BHV_Loiter , EVENT = spawn , SEED = survey_region ");

  std::vector<std::string> report = history.getReport();
  ASSERT_EQ(report.size(), 3u);
  EXPECT_FALSE(history.isStale());
  EXPECT_TRUE(containsText(report[2], "42.75"));
  EXPECT_TRUE(containsText(report[2], "8"));
  EXPECT_TRUE(containsText(report[2], "loiter_port"));
  EXPECT_TRUE(containsText(report[2], "BHV_Loiter"));
  EXPECT_TRUE(containsText(report[2], "survey_region"));

  EXPECT_EQ(history.getReport(), report);
  EXPECT_FALSE(history.isStale());

  history.addLifeEvent(
      "time=45.00,iter=9,bname=avoid_alpha,btype=BHV_AvoidCollision,"
      "event=abort,post_mortem=contact range exceeded");
  EXPECT_TRUE(history.isStale());

  report = history.getReport();
  ASSERT_EQ(report.size(), 4u);
  EXPECT_TRUE(containsText(report[3], "avoid_alpha"));
  EXPECT_TRUE(containsText(report[3], "contact range exceeded"));
}

// Covers life event history behavior: suppresses seed column for compact reports.
TEST(LifeEventHistoryTest, SuppressesSeedColumnForCompactReports)
{
  LifeEventHistory history;
  history.setBannerActive(false);
  history.setColorActive(false);
  history.setSeedActive(false);

  history.addLifeEvent(
      "time=12.00,iter=3,bname=waypt,btype=BHV_Waypoint,event=spawn,"
      "seed=survey lane");
  history.addLifeEvent(
      "time=19.00,iter=4,bname=avoid,btype=BHV_AvoidCollision,event=abort,"
      "post_mortem=contact lost");

  std::vector<std::string> report = history.getReport();
  ASSERT_EQ(report.size(), 4u);
  EXPECT_FALSE(containsText(report[0], "Spawning Seed"));
  EXPECT_FALSE(containsText(report[2], "survey lane"));
  EXPECT_FALSE(containsText(report[3], "contact lost"));
  EXPECT_TRUE(containsText(report[2], "waypt"));
  EXPECT_TRUE(containsText(report[3], "avoid"));
}
