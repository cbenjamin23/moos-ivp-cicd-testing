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

// Covers info buffer behavior: stores string double times and deltas.
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

// Covers info buffer behavior: tracks delta vectors and clears only delta history.
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

// Covers info buffer behavior: reports known and unknown variables.
TEST(InfoBufferTest, ReportsKnownAndUnknownVariables)
{
  InfoBuffer buffer;
  buffer.setCurrTime(50);
  buffer.setValue("MODE", "ACTIVE");
  buffer.setValue("NAV_X", 12.345);

  expectVectorEq(buffer.getReport({"MODE", "NAV_X", "UNKNOWN"}),
                 {"   MODE  ACTIVE", "  NAV_X  12.35", "UNKNOWN  [---]"});
}

// Covers info buffer behavior: pins mixed type and delta timestamp semantics.
TEST(InfoBufferTest, PinsMixedTypeAndDeltaTimestampSemantics)
{
  InfoBuffer buffer;
  buffer.setCurrTime(200);
  buffer.setValue("NAV_X", 12.5, 175);
  buffer.setValue("NAV_X", "twelve");
  buffer.setValue("MARK", "not-a-number");
  buffer.setValue("EVENT_UTC", 180.0);

  bool ok = false;
  EXPECT_DOUBLE_EQ(buffer.dQuery("NAV_X", ok), 12.5);
  EXPECT_TRUE(ok);
  EXPECT_EQ(buffer.sQuery("NAV_X", ok), "twelve");
  EXPECT_TRUE(ok);
  EXPECT_EQ(buffer.getReport(std::vector<std::string>{"NAV_X"}),
            std::vector<std::string>{"NAV_X  12.5"});

  EXPECT_DOUBLE_EQ(buffer.mtQuery("NAV_X", false), 200);
  EXPECT_DOUBLE_EQ(buffer.tQuery("NAV_X", false), 200);
  EXPECT_DOUBLE_EQ(buffer.dQuery("EVENT_UTC_DELTA", ok), 20);
  EXPECT_TRUE(ok);
  EXPECT_DOUBLE_EQ(buffer.dQuery("MARK_DELTA", ok), 0);
  EXPECT_FALSE(ok);

  EXPECT_EQ(buffer.tQuery("UNKNOWN"), -1);
  EXPECT_EQ(buffer.mtQuery("UNKNOWN"), -1);
  EXPECT_GT(buffer.size(), 0u);
  EXPECT_EQ(buffer.size(), buffer.sizeFull());
}

// Covers logic buffer behavior: evaluates all and any conditions against info buffer.
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

// Covers logic buffer behavior: supports delta conditions and spec output.
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

// Covers logic buffer behavior: handles missing info buffer and empty condition sets.
TEST(LogicBufferTest, HandlesMissingInfoBufferAndEmptyConditionSets)
{
  LogicBuffer detached;
  EXPECT_FALSE(detached.checkConditions());
  EXPECT_DOUBLE_EQ(detached.getCurrTime(), 0);
  EXPECT_TRUE(detached.getAllVarsSet().empty());
  EXPECT_TRUE(detached.getSpec().empty());
  EXPECT_TRUE(detached.getInfoBuffReport().empty());

  InfoBuffer info;
  LogicBuffer logic;
  logic.setInfoBuffer(&info);
  EXPECT_TRUE(logic.checkConditions("all"));
  EXPECT_FALSE(logic.checkConditions("any"));
  EXPECT_EQ(logic.getNotableCondition(), "required=any");
}
