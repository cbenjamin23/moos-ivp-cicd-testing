#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "CommandSummary.h"

namespace {

bool reportContains(const std::vector<std::string>& report,
                    const std::string& needle)
{
  for(const std::string& line : report) {
    if(line.find(needle) != std::string::npos)
      return true;
  }
  return false;
}

bool reportHasToken(const std::vector<std::string>& report,
                    const std::string& token)
{
  for(const std::string& line : report) {
    std::istringstream stream(line);
    std::string word;
    while(stream >> word) {
      if(word == token)
        return true;
    }
  }
  return false;
}

}  // namespace

TEST(CommandSummaryTest, ReportsNewestCommandPostsFirstAndClearsPendingFlag)
{
  CommandSummary summary;
  EXPECT_FALSE(summary.reportPending());

  summary.addPosting("DEPLOY", "true", "pid_1");
  summary.addPosting("RETURN", "true", "pid_2");
  EXPECT_TRUE(summary.reportPending());

  std::vector<std::string> report = summary.getCommandReport();
  EXPECT_FALSE(summary.reportPending());
  ASSERT_GE(report.size(), 4u);
  EXPECT_TRUE(reportContains(report, "MOOSVar"));
  EXPECT_TRUE(reportContains(report, "RETURN"));
  EXPECT_TRUE(reportContains(report, "pid_2"));
  EXPECT_TRUE(reportContains(report, "DEPLOY"));
  EXPECT_TRUE(reportContains(report, "pid_1"));

  std::string joined;
  for(const std::string& line : report)
    joined += line + "\n";
  EXPECT_LT(joined.find("RETURN"), joined.find("DEPLOY"));
}

TEST(CommandSummaryTest, TestPostsAreShownOnceAndThenRemovedFromHistory)
{
  CommandSummary summary;
  summary.addPosting("DEPLOY", "true", "pid_1");
  summary.addPosting("RETURN", "true", "test_pid", true);

  std::vector<std::string> first = summary.getCommandReport();
  EXPECT_TRUE(reportContains(first, "RETURN"));
  EXPECT_TRUE(reportContains(first, "test_pid"));
  EXPECT_TRUE(reportContains(first, "DEPLOY"));

  std::vector<std::string> second = summary.getCommandReport();
  EXPECT_FALSE(reportContains(second, "RETURN"));
  EXPECT_FALSE(reportContains(second, "test_pid"));
  EXPECT_TRUE(reportContains(second, "DEPLOY"));
  EXPECT_TRUE(reportContains(second, "pid_1"));
}

TEST(CommandSummaryTest, TruncatesCommandHistoryToTwentyPosts)
{
  CommandSummary summary;
  for(int i = 0; i < 22; ++i)
    summary.addPosting("VAR_" + std::to_string(i),
                       "VAL_" + std::to_string(i),
                       "pid_" + std::to_string(i));

  std::vector<std::string> report = summary.getCommandReport();
  EXPECT_TRUE(reportHasToken(report, "VAR_21"));
  EXPECT_TRUE(reportHasToken(report, "pid_21"));
  EXPECT_TRUE(reportHasToken(report, "VAR_2"));
  EXPECT_TRUE(reportHasToken(report, "pid_2"));
  EXPECT_FALSE(reportHasToken(report, "VAR_1"));
  EXPECT_FALSE(reportHasToken(report, "pid_1"));
  EXPECT_FALSE(reportHasToken(report, "VAR_0"));
  EXPECT_FALSE(reportHasToken(report, "pid_0"));
}
