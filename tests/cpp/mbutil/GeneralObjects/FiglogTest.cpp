#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Figlog.h"

// Covers figlog behavior: stores and augments messages warnings and errors.
TEST(FiglogTest, StoresAndAugmentsMessagesWarningsAndErrors)
{
  Figlog log;
  log.setLabel("thrust_map");

  log.addMessage("loaded");
  log.augMessage(" config");
  log.addWarning("low thrust");
  log.augWarning(" table");
  log.addError("bad reverse");
  log.augError(" row");

  EXPECT_EQ(log.getLabel(), "thrust_map");
  EXPECT_EQ(log.messages(), 1u);
  EXPECT_EQ(log.warnings(), 1u);
  EXPECT_EQ(log.errors(), 1u);
  EXPECT_EQ(log.getMessage(0), "loaded config");
  EXPECT_EQ(log.getWarning(0), "low thrust table");
}

// Covers figlog behavior: pins get error indexing current behavior.
TEST(FiglogTest, PinsGetErrorIndexingCurrentBehavior)
{
  Figlog log;
  log.addWarning("warning-zero");
  log.addError("error-zero");

  EXPECT_EQ(log.getError(0), "warning-zero");
  EXPECT_EQ(log.getError(1), "");
}

// Covers figlog behavior: augments only most recent entry per channel.
TEST(FiglogTest, AugmentsOnlyMostRecentEntryPerChannel)
{
  Figlog log;
  log.addMessage("first");
  log.addMessage("second");
  log.augMessage("-tail");
  log.addWarning("warn-a");
  log.addWarning("warn-b");
  log.augWarning("-tail");
  log.addError("err-a");
  log.addError("err-b");
  log.augError("-tail");

  EXPECT_EQ(log.getMessage(0), "first");
  EXPECT_EQ(log.getMessage(1), "second-tail");
  EXPECT_EQ(log.getWarning(0), "warn-a");
  EXPECT_EQ(log.getWarning(1), "warn-b-tail");
  EXPECT_EQ(log.getErrors()[0], "err-a");
  EXPECT_EQ(log.getErrors()[1], "err-b-tail");
}

// Covers figlog behavior: returns copies and empty strings for out of range indexes.
TEST(FiglogTest, ReturnsCopiesAndEmptyStringsForOutOfRangeIndexes)
{
  Figlog log;
  log.addMessage("message-zero");
  log.addWarning("warning-zero");
  log.addError("error-zero");

  std::vector<std::string> messages = log.getMessages();
  std::vector<std::string> warnings = log.getWarnings();
  std::vector<std::string> errors = log.getErrors();
  messages.push_back("local-only");
  warnings.clear();
  errors.clear();

  EXPECT_EQ(log.messages(), 1u);
  EXPECT_EQ(log.warnings(), 1u);
  EXPECT_EQ(log.errors(), 1u);
  EXPECT_EQ(log.getMessage(99), "");
  EXPECT_EQ(log.getWarning(99), "");
  EXPECT_EQ(log.getError(99), "");
}

// Covers figlog behavior: clears selected channels and all channels.
TEST(FiglogTest, ClearsSelectedChannelsAndAllChannels)
{
  Figlog log;
  log.augMessage("created-by-augment");
  log.augWarning("warning");
  log.augError("error");

  EXPECT_EQ(log.getMessage(0), "created-by-augment");
  log.clearWarnings();
  EXPECT_EQ(log.warnings(), 0u);
  EXPECT_EQ(log.messages(), 1u);
  EXPECT_EQ(log.errors(), 1u);

  log.clear();
  EXPECT_EQ(log.messages(), 0u);
  EXPECT_EQ(log.warnings(), 0u);
  EXPECT_EQ(log.errors(), 0u);
  EXPECT_EQ(log.getLabel(), "");
}

// Covers figlog behavior: individual clears leave other channels and label untouched.
TEST(FiglogTest, IndividualClearsLeaveOtherChannelsAndLabelUntouched)
{
  Figlog log;
  log.setLabel("manifest_scan");
  log.addMessage("loaded pHelmIvP");
  log.addWarning("missing optional doc_url");
  log.addError("bad borndate");

  log.clearMessages();
  EXPECT_EQ(log.messages(), 0u);
  EXPECT_EQ(log.warnings(), 1u);
  EXPECT_EQ(log.errors(), 1u);
  EXPECT_EQ(log.getLabel(), "manifest_scan");

  log.clearErrors();
  EXPECT_EQ(log.messages(), 0u);
  EXPECT_EQ(log.warnings(), 1u);
  EXPECT_EQ(log.errors(), 0u);
  EXPECT_EQ(log.getWarning(0), "missing optional doc_url");
}

// Covers figlog behavior: clear leaves label for reusable component logs.
TEST(FiglogTest, ClearLeavesLabelForReusableComponentLogs)
{
  Figlog log;
  log.setLabel("pHelmIvP");
  log.addMessage("configured");
  log.addWarning("late mail");
  log.addError("bad helm mode");

  log.clear();
  EXPECT_EQ(log.getLabel(), "pHelmIvP");
  EXPECT_EQ(log.messages(), 0u);
  EXPECT_EQ(log.warnings(), 0u);
  EXPECT_EQ(log.errors(), 0u);

  log.augMessage("reused");
  EXPECT_EQ(log.getMessage(0), "reused");
}

// Covers figlog behavior: print shows summary and pins current error count label quirk.
TEST(FiglogTest, PrintShowsSummaryAndPinsCurrentErrorCountLabelQuirk)
{
  Figlog log;
  log.setLabel("pMissionEval");
  log.addMessage("configured");
  log.addWarning("late NAV_X");
  log.addError("MISSION_RESULT mismatch");
  log.addError("MISSION_HASH mismatch");

  testing::internal::CaptureStdout();
  log.print();
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("Figlog Summary: (pMissionEval)"), std::string::npos);
  EXPECT_NE(output.find("Messages: (1)"), std::string::npos);
  EXPECT_NE(output.find("Warnings: (1)"), std::string::npos);
  EXPECT_NE(output.find("configured"), std::string::npos);
  EXPECT_NE(output.find("late NAV_X"), std::string::npos);
  EXPECT_NE(output.find("MISSION_RESULT mismatch"), std::string::npos);
  EXPECT_NE(output.find("MISSION_HASH mismatch"), std::string::npos);
  EXPECT_NE(output.find("Errors: (1)"), std::string::npos);
}
