#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "CPAEvent.h"
#include "CPAMonitor.h"

namespace {

class ExposedCPAMonitor : public CPAMonitor {
 public:
  using CPAMonitor::examineAndReport;
  using CPAMonitor::pairTag;
  using CPAMonitor::relBng;
  using CPAMonitor::updatePairRangeAndRate;

  double ignoreRange() const { return m_ignore_range; }
  double reportRange() const { return m_report_range; }
  double swingRange() const { return m_swing_range; }
  bool updated(const std::string& vname) const
  {
    auto it = m_map_updated.find(vname);
    return (it != m_map_updated.end()) && it->second;
  }
  bool pairExamined(const std::string& tag) const
  {
    auto it = m_map_pair_examined.find(tag);
    return (it != m_map_pair_examined.end()) && it->second;
  }
  bool pairValid(const std::string& tag) const
  {
    auto it = m_map_pair_valid.find(tag);
    return (it != m_map_pair_valid.end()) && it->second;
  }
  bool pairClosing(const std::string& tag) const
  {
    auto it = m_map_pair_closing.find(tag);
    return (it != m_map_pair_closing.end()) && it->second;
  }
  double pairDistance(const std::string& tag) const
  {
    auto it = m_map_pair_dist.find(tag);
    return (it == m_map_pair_dist.end()) ? -1 : it->second;
  }
};

std::string reportFor(const std::string& name,
                      double x,
                      double y,
                      double utc,
                      const std::string& group = "red",
                      double heading = 90)
{
  return "NAME=" + name +
         ",X=" + std::to_string(x) +
         ",Y=" + std::to_string(y) +
         ",UTC_TIME=" + std::to_string(utc) +
         ",SPD=1,HDG=" + std::to_string(heading) +
         ",TYPE=kayak,GROUP=" + group;
}

void addRound(ExposedCPAMonitor& monitor,
              double range,
              double utc,
              const std::string& group1 = "red",
              const std::string& group2 = "blue")
{
  std::string whynot;
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("abe", 0, 0, utc, group1, 90),
                                      whynot))
      << whynot;
  whynot.clear();
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("ben", range, 0, utc, group2, 270),
                                      whynot))
      << whynot;
  ASSERT_TRUE(monitor.examineAndReport());
}

void runOpeningClosingSequence(ExposedCPAMonitor& monitor)
{
  addRound(monitor, 10, 1);
  monitor.clear();
  addRound(monitor, 12, 2);
  monitor.clear();
  addRound(monitor, 8, 3);
  monitor.clear();
  addRound(monitor, 5, 4);
  monitor.clear();
  addRound(monitor, 7, 5);
}

}  // namespace

// Source audit note: this suite covers CPAMonitor's deterministic state machine
// in uFldCollisionDetect/CPAMonitor.cpp: config clamping, group filtering,
// NODE_REPORT admission, pair-tag normalization, range/opening/closing
// transitions, event emission thresholds, clear semantics, and contact-density
// helpers. CollisionDetector's MOOS mail, Notify/AppCast paths, viewer ring
// posts, and condition-buffer integration remain harness or app-level concerns.

TEST(CPAMonitorTest, RangeSettersMaintainIgnoreReportInvariantAndClampSwing)
{
  ExposedCPAMonitor monitor;

  EXPECT_DOUBLE_EQ(monitor.ignoreRange(), 100);
  EXPECT_DOUBLE_EQ(monitor.reportRange(), 50);
  EXPECT_DOUBLE_EQ(monitor.swingRange(), 1);

  monitor.setIgnoreRange(25);
  EXPECT_DOUBLE_EQ(monitor.ignoreRange(), 25);
  EXPECT_DOUBLE_EQ(monitor.reportRange(), 25);

  monitor.setReportRange(40);
  EXPECT_DOUBLE_EQ(monitor.ignoreRange(), 40);
  EXPECT_DOUBLE_EQ(monitor.reportRange(), 40);

  monitor.setSwingRange(-5);
  EXPECT_DOUBLE_EQ(monitor.swingRange(), 0);
}

TEST(CPAMonitorTest, GroupListsLowercaseDeduplicateAndRejectWhitespace)
{
  ExposedCPAMonitor monitor;

  EXPECT_TRUE(monitor.addIgnoreGroup("Alpha"));
  EXPECT_FALSE(monitor.addIgnoreGroup("alpha"));
  EXPECT_FALSE(monitor.addIgnoreGroup("red team"));
  EXPECT_EQ(monitor.getIgnoreGroups(), "alpha");

  EXPECT_TRUE(monitor.addRejectGroup("BLUE"));
  EXPECT_FALSE(monitor.addRejectGroup("blue"));
  EXPECT_FALSE(monitor.addRejectGroup("blue team"));
  EXPECT_EQ(monitor.getRejectGroups(), "blue");
}

TEST(CPAMonitorTest, PairTagsAreOrderIndependentAndLexicographic)
{
  ExposedCPAMonitor monitor;

  EXPECT_EQ(monitor.pairTag("abe", "ben"), "abe#ben");
  EXPECT_EQ(monitor.pairTag("ben", "abe"), "abe#ben");
  EXPECT_EQ(monitor.pairTag("Abe", "abe"), "Abe#abe");
}

TEST(CPAMonitorTest, InvalidNodeReportsAreRejectedWithoutAddingVehicles)
{
  ExposedCPAMonitor monitor;
  std::string whynot;

  EXPECT_FALSE(monitor.handleNodeReport("NAME=abe,X=1,Y=2", whynot));
  EXPECT_NE(whynot.find("Missing timestamp"), std::string::npos);
  EXPECT_TRUE(monitor.getVNames().empty());
  EXPECT_FALSE(monitor.updated("abe"));
}

TEST(CPAMonitorTest, AcceptedNodeReportsExposeRecordsAndDensity)
{
  ExposedCPAMonitor monitor;
  std::string whynot;

  ASSERT_TRUE(monitor.handleNodeReport(reportFor("abe", 0, 0, 1, "red"), whynot));
  whynot.clear();
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("ben", 3, 4, 1, "blue"), whynot));
  whynot.clear();
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("cal", 30, 0, 1, "blue"), whynot));

  EXPECT_EQ(monitor.getVNames(), (std::vector<std::string>{"abe", "ben", "cal"}));
  EXPECT_EQ(monitor.getContactDensity("abe", 5), 1u);
  EXPECT_EQ(monitor.getContactDensity("abe", 30), 2u);
  EXPECT_EQ(monitor.getVRecord("missing").valid(), false);
  EXPECT_EQ(monitor.getVRecord("ben").getGroup(), "blue");
}

TEST(CPAMonitorTest, RejectGroupClearsAcceptedReportBeforeItCanBeExamined)
{
  ExposedCPAMonitor monitor;
  ASSERT_TRUE(monitor.addRejectGroup("blue"));
  std::string whynot;

  EXPECT_TRUE(monitor.handleNodeReport(reportFor("ben", 0, 0, 1, "BLUE"), whynot));

  EXPECT_TRUE(monitor.getVNames().empty());
  EXPECT_TRUE(monitor.updated("ben"));
  EXPECT_FALSE(monitor.examineAndReport("ben", "abe"));
}

TEST(CPAMonitorTest, UpdatePairRequiresBothNodeRecords)
{
  ExposedCPAMonitor monitor;
  std::string whynot;
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("abe", 0, 0, 1), whynot));

  EXPECT_FALSE(monitor.updatePairRangeAndRate("abe", "missing"));
  EXPECT_FALSE(monitor.updatePairRangeAndRate("missing", "abe"));
}

TEST(CPAMonitorTest, FirstPairExaminePreSeedsMapsSoTrackIsImmediatelyValid)
{
  ExposedCPAMonitor monitor;
  addRound(monitor, 10, 1);

  const std::string tag = monitor.pairTag("abe", "ben");
  EXPECT_DOUBLE_EQ(monitor.pairDistance(tag), 10);
  EXPECT_TRUE(monitor.pairValid(tag));
  EXPECT_FALSE(monitor.pairClosing(tag));
  EXPECT_TRUE(monitor.pairExamined(tag));
  EXPECT_DOUBLE_EQ(monitor.getClosestRange(), 10);
}

TEST(CPAMonitorTest, OpeningThenClosingThenOpeningEmitsSingleCPAEvent)
{
  ExposedCPAMonitor monitor;
  monitor.setReportRange(20);

  runOpeningClosingSequence(monitor);

  ASSERT_EQ(monitor.getEventCount(), 1u);
  const CPAEvent event = monitor.getEvent(0);
  EXPECT_EQ(event.getVName1(), "abe");
  EXPECT_EQ(event.getVName2(), "ben");
  EXPECT_DOUBLE_EQ(event.getCPA(), 5);
  EXPECT_DOUBLE_EQ(event.getX(), 3.5);
  EXPECT_DOUBLE_EQ(event.getY(), 0);
  EXPECT_NEAR(event.getAlpha(), 0, 1e-9);
  EXPECT_NEAR(event.getBeta(), 0, 1e-9);
}

TEST(CPAMonitorTest, ReportRangeSuppressesOpeningEventAboveThreshold)
{
  ExposedCPAMonitor monitor;
  monitor.setReportRange(4);

  runOpeningClosingSequence(monitor);

  EXPECT_EQ(monitor.getEventCount(), 0u);
  EXPECT_DOUBLE_EQ(monitor.getClosestRangeEver(), 5);
}

TEST(CPAMonitorTest, ReportRangeBoundaryIncludesEqualCPA)
{
  ExposedCPAMonitor monitor;
  monitor.setReportRange(5);

  runOpeningClosingSequence(monitor);

  ASSERT_EQ(monitor.getEventCount(), 1u);
  EXPECT_DOUBLE_EQ(monitor.getEvent(0).getCPA(), 5);
}

TEST(CPAMonitorTest, SwingRangeUsesStrictGreaterThanForModeTransitions)
{
  ExposedCPAMonitor monitor;
  monitor.setReportRange(20);
  monitor.setSwingRange(1);

  addRound(monitor, 10, 1);
  monitor.clear();
  addRound(monitor, 12, 2);
  monitor.clear();
  addRound(monitor, 8, 3);
  monitor.clear();
  addRound(monitor, 5, 4);
  monitor.clear();
  addRound(monitor, 6, 5);

  EXPECT_EQ(monitor.getEventCount(), 0u);
  EXPECT_TRUE(monitor.pairClosing(monitor.pairTag("abe", "ben")));

  monitor.clear();
  addRound(monitor, 6.01, 6);
  EXPECT_EQ(monitor.getEventCount(), 1u);
}

TEST(CPAMonitorTest, IgnoreRangeClearsPairStateAndInvalidatesExistingTrack)
{
  ExposedCPAMonitor monitor;
  monitor.setIgnoreRange(15);

  addRound(monitor, 10, 1);
  monitor.clear();
  addRound(monitor, 12, 2);
  monitor.clear();
  addRound(monitor, 16, 3);

  const std::string tag = monitor.pairTag("abe", "ben");
  EXPECT_DOUBLE_EQ(monitor.pairDistance(tag), -1);
  EXPECT_FALSE(monitor.pairValid(tag));
  EXPECT_FALSE(monitor.pairClosing(tag));
}

TEST(CPAMonitorTest, IgnoreRangeBoundaryKeepsEqualDistanceButClearsGreaterDistance)
{
  ExposedCPAMonitor monitor;
  monitor.setIgnoreRange(10);

  addRound(monitor, 10, 1);
  const std::string tag = monitor.pairTag("abe", "ben");
  EXPECT_DOUBLE_EQ(monitor.pairDistance(tag), 10);
  EXPECT_TRUE(monitor.pairValid(tag));

  monitor.clear();
  addRound(monitor, 10.01, 2);
  EXPECT_DOUBLE_EQ(monitor.pairDistance(tag), -1);
  EXPECT_FALSE(monitor.pairValid(tag));
}

TEST(CPAMonitorTest, IgnoreGroupsSuppressEventOnlyWhenBothStoredGroupsMatch)
{
  ExposedCPAMonitor monitor;
  ASSERT_TRUE(monitor.addIgnoreGroup("red"));
  ASSERT_TRUE(monitor.addIgnoreGroup("blue"));

  runOpeningClosingSequence(monitor);

  EXPECT_EQ(monitor.getEventCount(), 0u);
  EXPECT_DOUBLE_EQ(monitor.getClosestRange(), -1);
}

TEST(CPAMonitorTest, OneSidedIgnoreGroupDoesNotSuppressEvent)
{
  ExposedCPAMonitor monitor;
  ASSERT_TRUE(monitor.addIgnoreGroup("red"));
  monitor.setReportRange(20);

  runOpeningClosingSequence(monitor);

  EXPECT_EQ(monitor.getEventCount(), 1u);
}

TEST(CPAMonitorTest, IgnoreGroupComparisonUsesStoredGroupCase)
{
  ExposedCPAMonitor monitor;
  ASSERT_TRUE(monitor.addIgnoreGroup("red"));
  ASSERT_TRUE(monitor.addIgnoreGroup("blue"));
  monitor.setReportRange(20);

  addRound(monitor, 10, 1, "RED", "BLUE");
  monitor.clear();
  addRound(monitor, 12, 2, "RED", "BLUE");
  monitor.clear();
  addRound(monitor, 8, 3, "RED", "BLUE");
  monitor.clear();
  addRound(monitor, 5, 4, "RED", "BLUE");
  monitor.clear();
  addRound(monitor, 7, 5, "RED", "BLUE");

  EXPECT_EQ(monitor.getEventCount(), 1u);
}

TEST(CPAMonitorTest, ClearResetsRoundFlagsAndEventsButPreservesPairHistory)
{
  ExposedCPAMonitor monitor;
  monitor.setReportRange(20);
  runOpeningClosingSequence(monitor);
  ASSERT_EQ(monitor.getEventCount(), 1u);

  monitor.clear();

  const std::string tag = monitor.pairTag("abe", "ben");
  EXPECT_EQ(monitor.getEventCount(), 0u);
  EXPECT_FALSE(monitor.updated("abe"));
  EXPECT_FALSE(monitor.updated("ben"));
  EXPECT_FALSE(monitor.pairExamined(tag));
  EXPECT_DOUBLE_EQ(monitor.pairDistance(tag), 7);
}

TEST(CPAMonitorTest, ReexaminingSameRoundDoesNotDuplicateEvent)
{
  ExposedCPAMonitor monitor;
  monitor.setReportRange(20);
  runOpeningClosingSequence(monitor);
  ASSERT_EQ(monitor.getEventCount(), 1u);

  EXPECT_TRUE(monitor.examineAndReport());

  EXPECT_EQ(monitor.getEventCount(), 1u);
}

TEST(CPAMonitorTest, OutOfRangeEventLookupReturnsDefaultEvent)
{
  ExposedCPAMonitor monitor;
  runOpeningClosingSequence(monitor);
  ASSERT_EQ(monitor.getEventCount(), 1u);

  const CPAEvent event = monitor.getEvent(1);

  EXPECT_EQ(event.getVName1(), "");
  EXPECT_EQ(event.getVName2(), "");
  EXPECT_DOUBLE_EQ(event.getCPA(), 0);
  EXPECT_EQ(event.getID(), -1);
}

TEST(CPAMonitorTest, ContactDensityRejectsInvalidInputsAndExcludesOwnship)
{
  ExposedCPAMonitor monitor;
  std::string whynot;
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("abe", 0, 0, 1), whynot));

  EXPECT_EQ(monitor.getContactDensity("", 10), 0u);
  EXPECT_EQ(monitor.getContactDensity("abe", 0), 0u);
  EXPECT_EQ(monitor.getContactDensity("missing", 10), 0u);
  EXPECT_EQ(monitor.getContactDensity("abe", 10), 0u);
}

TEST(CPAMonitorTest, ResetClosestRangeActuallyResetsClosestEver)
{
  ExposedCPAMonitor monitor;
  addRound(monitor, 10, 1);
  EXPECT_DOUBLE_EQ(monitor.getClosestRange(), 10);
  EXPECT_DOUBLE_EQ(monitor.getClosestRangeEver(), 10);

  monitor.resetClosestRange();
  EXPECT_DOUBLE_EQ(monitor.getClosestRange(), 10);
  EXPECT_DOUBLE_EQ(monitor.getClosestRangeEver(), -1);

  monitor.resetClosestRangeEver();
  EXPECT_DOUBLE_EQ(monitor.getClosestRangeEver(), -1);
}

TEST(CPAMonitorTest, RelativeBearingReturnsZeroForMissingRecords)
{
  ExposedCPAMonitor monitor;
  std::string whynot;
  ASSERT_TRUE(monitor.handleNodeReport(reportFor("abe", 0, 0, 1, "red", 90),
                                      whynot));

  EXPECT_DOUBLE_EQ(monitor.relBng("abe", "missing"), 0);
  EXPECT_DOUBLE_EQ(monitor.relBng("missing", "abe"), 0);
}
