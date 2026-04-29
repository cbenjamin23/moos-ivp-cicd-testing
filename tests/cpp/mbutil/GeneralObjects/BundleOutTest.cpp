#include <gtest/gtest.h>

#include <list>
#include <set>
#include <string>
#include <vector>

#include "BundleOut.h"
#include "OpenURL.h"
#include "ReleaseInfo.h"

TEST(BundleOutTest, PrintsVectorLinesInOriginalOrder)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::vector<std::string>{"alpha", "bravo"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "alpha\nbravo\n");
}

TEST(BundleOutTest, PrintsListLinesInOriginalOrder)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::list<std::string>{"first", "second"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "first\nsecond\n");
}

TEST(BundleOutTest, PrintsSetLinesInSortedOrder)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::set<std::string>{"delta", "alpha", "charlie"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "alpha\ncharlie\ndelta\n");
}

TEST(ReleaseInfoTest, PrintsStandardMoosIvPReleaseBanner)
{
  testing::internal::CaptureStdout();
  showReleaseInfo("pHelmIvP", "gpl");
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("pHelmIvP - Part of the MOOS-IvP Release Bundle"),
            std::string::npos);
  EXPECT_NE(output.find("Version 24.8"), std::string::npos);
  EXPECT_NE(output.find("This is free software"), std::string::npos);

  testing::internal::CaptureStdout();
  showReleaseInfo("uXMS", "");
  output = testing::internal::GetCapturedStdout();
  EXPECT_NE(output.find("uXMS - Part of the MOOS-IvP Release Bundle"),
            std::string::npos);
  EXPECT_EQ(output.find("This is free software"), std::string::npos);
}

TEST(ReleaseInfoTest, ExitsAfterPrintingStandardBanner)
{
  EXPECT_EXIT(showReleaseInfoAndExit("pHelmIvP", "gpl"),
              testing::ExitedWithCode(0), "");
}

TEST(OpenURLTest, IgnoresNonHttpUrlsAndExitsForDocumentedOpenUrlXPath)
{
  openURL("");
  openURL("file:///tmp/not-opened");
  openURL("mailto:test@example.com");

  EXPECT_EXIT(openURLX("not-a-url"), testing::ExitedWithCode(0), "");
}
