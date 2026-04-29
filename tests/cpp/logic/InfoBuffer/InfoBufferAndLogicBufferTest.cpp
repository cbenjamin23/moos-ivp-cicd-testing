#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "InfoBuffer.h"
#include "LogicBuffer.h"

namespace {

void expectVectorEq(const std::vector<std::string>& actual,
                    const std::vector<std::string>& expected)
{
  ASSERT_EQ(actual.size(), expected.size());
  for(std::size_t i = 0; i < expected.size(); ++i)
    EXPECT_EQ(actual[i], expected[i]) << "at index " << i;
}

}  // namespace

TEST(InfoBufferTest, StoresStringDoubleTimesAndDeltas)
{
  InfoBuffer buffer;
  buffer.setStartTime(10);
  buffer.setCurrTime(100);
  EXPECT_DOUBLE_EQ(buffer.getLocalTime(), 90);

  EXPECT_TRUE(buffer.setValue("NAV_X", 12.5, 90));
  EXPECT_TRUE(buffer.setValue("MODE", "ACTIVE"));
  EXPECT_TRUE(buffer.setValue("MARK", "75"));

  bool ok = false;
  EXPECT_DOUBLE_EQ(buffer.dQuery("NAV_X", ok), 12.5);
  EXPECT_TRUE(ok);
  EXPECT_EQ(buffer.sQuery("MODE", ok), "ACTIVE");
  EXPECT_TRUE(ok);
  EXPECT_DOUBLE_EQ(buffer.dQuery("MARK_DELTA", ok), 25);
  EXPECT_TRUE(ok);

  EXPECT_DOUBLE_EQ(buffer.tQuery("NAV_X"), 0);
  EXPECT_DOUBLE_EQ(buffer.tQuery("NAV_X", false), 100);
  EXPECT_DOUBLE_EQ(buffer.mtQuery("NAV_X", false), 90);
  EXPECT_TRUE(buffer.isKnown("MODE"));
  EXPECT_FALSE(buffer.isKnown("UNKNOWN"));
}

TEST(InfoBufferTest, TracksDeltaVectorsAndClearsOnlyDeltaHistory)
{
  InfoBuffer buffer;
  buffer.setCurrTime(10);
  buffer.setValue("NAV_X", 1.0);
  buffer.setValue("NAV_X", 2.0);
  buffer.setValue("MODE", "ACTIVE");
  buffer.setValue("MODE", "SURVEYING");

  bool ok = false;
  expectVectorEq(buffer.sQueryDeltas("MODE", ok), {"ACTIVE", "SURVEYING"});
  EXPECT_TRUE(ok);
  std::vector<double> x_deltas = buffer.dQueryDeltas("NAV_X", ok);
  ASSERT_EQ(x_deltas.size(), 2u);
  EXPECT_DOUBLE_EQ(x_deltas[0], 1.0);
  EXPECT_DOUBLE_EQ(x_deltas[1], 2.0);

  buffer.clearDeltaVectors();
  EXPECT_TRUE(buffer.sQuery("MODE", ok) == "SURVEYING");
  EXPECT_TRUE(ok);
  EXPECT_TRUE(buffer.dQueryDeltas("NAV_X", ok).empty());
  EXPECT_FALSE(ok);
}

TEST(InfoBufferTest, ReportsKnownAndUnknownVariables)
{
  InfoBuffer buffer;
  buffer.setCurrTime(50);
  buffer.setValue("MODE", "ACTIVE");
  buffer.setValue("NAV_X", 12.345);

  expectVectorEq(buffer.getReport({"MODE", "NAV_X", "UNKNOWN"}),
                 {"   MODE  ACTIVE", "  NAV_X  12.35", "UNKNOWN  [---]"});
}

TEST(LogicBufferTest, EvaluatesAllAndAnyConditionsAgainstInfoBuffer)
{
  InfoBuffer info;
  LogicBuffer logic;
  logic.setInfoBuffer(&info);

  EXPECT_TRUE(logic.addNewCondition("MODE=ACTIVE"));
  EXPECT_TRUE(logic.addNewCondition("NAV_X > 10"));
  EXPECT_EQ(logic.size(), 2u);
  EXPECT_EQ(logic.getAllVarsSet(), (std::set<std::string>{"MODE", "NAV_X"}));

  logic.updateInfoBuffer("MODE", "ACTIVE");
  logic.updateInfoBuffer("NAV_X", 12.5);
  logic.updateInfoBuffer("IGNORED", 99.0);

  EXPECT_TRUE(logic.checkConditions("all"));
  EXPECT_TRUE(logic.checkConditions("any"));

  logic.updateInfoBuffer("NAV_X", 5.0);
  EXPECT_FALSE(logic.checkConditions("all"));
  EXPECT_EQ(logic.getNotableCondition(), "NAV_X > 10");
  EXPECT_TRUE(logic.checkConditions("any"));
  EXPECT_EQ(logic.getNotableCondition(), "MODE=ACTIVE");
}

TEST(LogicBufferTest, SupportsDeltaConditionsAndSpecOutput)
{
  InfoBuffer info;
  LogicBuffer logic;
  logic.setInfoBuffer(&info);
  logic.setCurrTime(120);

  EXPECT_TRUE(logic.addNewCondition("MARK_DELTA < 30"));
  EXPECT_EQ(logic.getAllVarsSet(),
            (std::set<std::string>{"MARK", "MARK_DELTA"}));
  logic.updateInfoBuffer("MARK", 100.0);
  EXPECT_TRUE(logic.checkConditions());

  logic.setCurrTime(140);
  EXPECT_FALSE(logic.checkConditions());
  EXPECT_EQ(logic.getSpec("  "), std::vector<std::string>{"  MARK_DELTA < 30"});
}
