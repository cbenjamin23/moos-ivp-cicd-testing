#include <gtest/gtest.h>

#include "Figlog.h"

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

TEST(FiglogTest, PinsGetErrorIndexingCurrentBehavior)
{
  Figlog log;
  log.addWarning("warning-zero");
  log.addError("error-zero");

  EXPECT_EQ(log.getError(0), "warning-zero");
  EXPECT_EQ(log.getError(1), "");
}

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
}
