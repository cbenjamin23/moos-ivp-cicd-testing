#include <gtest/gtest.h>

#include <list>
#include <set>
#include <string>
#include <vector>

#include "BundleOut.h"
#include "OpenURL.h"
#include "ReleaseInfo.h"

// Covers bundle out behavior: prints vector lines in original order.
TEST(BundleOutTest, PrintsVectorLinesInOriginalOrder)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::vector<std::string>{"alpha", "bravo"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "alpha\nbravo\n");
}

// Covers bundle out behavior: prints empty vector as no output.
TEST(BundleOutTest, PrintsEmptyVectorAsNoOutput)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::vector<std::string>{});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "");
}

// Covers bundle out behavior: prints blank lines and does not trim payloads.
TEST(BundleOutTest, PrintsBlankLinesAndDoesNotTrimPayloads)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::vector<std::string>{"  leading", "", "trailing  "});
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            "  leading\n\ntrailing  \n");
}

// Covers bundle out behavior: prints list lines in original order.
TEST(BundleOutTest, PrintsListLinesInOriginalOrder)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::list<std::string>{"first", "second"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "first\nsecond\n");
}

// Covers bundle out behavior: prints set lines in sorted order.
TEST(BundleOutTest, PrintsSetLinesInSortedOrder)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::set<std::string>{"delta", "alpha", "charlie"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(), "alpha\ncharlie\ndelta\n");
}

// Covers bundle out behavior: preserves MOOS style payloads and embedded newlines.
TEST(BundleOutTest, PreservesMoosStylePayloadsAndEmbeddedNewlines)
{
  testing::internal::CaptureStdout();
  bundle_cout(std::vector<std::string>{
      "NODE_REPORT_LOCAL = NAME=abe,X=12.4,Y=-8.1,SPD=1.8,HDG=270",
      "VIEW_POLYGON = pts={0,0:20,0:20,20:0,20},label=survey_box",
      "APPCAST = pHelmIvP\nstatus=running"});
  EXPECT_EQ(testing::internal::GetCapturedStdout(),
            "NODE_REPORT_LOCAL = NAME=abe,X=12.4,Y=-8.1,SPD=1.8,HDG=270\n"
            "VIEW_POLYGON = pts={0,0:20,0:20,20:0,20},label=survey_box\n"
            "APPCAST = pHelmIvP\nstatus=running\n");
}

// Covers release info behavior: prints standard MOOS-IvP release banner.
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

// Covers release info behavior: handles long app names without padding assumptions.
TEST(ReleaseInfoTest, HandlesLongAppNamesWithoutPaddingAssumptions)
{
  testing::internal::CaptureStdout();
  showReleaseInfo("uFldMessageHandlerExtended", "gpl");
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("uFldMessageHandlerExtended - Part of the MOOS-IvP Release Bundle"),
            std::string::npos);
  EXPECT_NE(output.find("Version 24.8"), std::string::npos);
  EXPECT_NE(output.find("This is free software"), std::string::npos);
}

// Covers release info behavior: pins license flag case sensitivity.
TEST(ReleaseInfoTest, PinsLicenseFlagCaseSensitivity)
{
  testing::internal::CaptureStdout();
  showReleaseInfo("uXMS", "GPL");
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("uXMS - Part of the MOOS-IvP Release Bundle"),
            std::string::npos);
  EXPECT_EQ(output.find("This is free software"), std::string::npos);
}

// Covers release info behavior: exits after printing standard banner.
TEST(ReleaseInfoTest, ExitsAfterPrintingStandardBanner)
{
  EXPECT_EXIT(showReleaseInfoAndExit("pHelmIvP", "gpl"),
              testing::ExitedWithCode(0), "");
}

// Covers open URL behavior: ignores non http URLs and exits for documented open URL x path.
TEST(OpenURLTest, IgnoresNonHttpUrlsAndExitsForDocumentedOpenUrlXPath)
{
  openURL("");
  openURL("file:///tmp/not-opened");
  openURL("mailto:test@example.com");

  EXPECT_EXIT(openURLX("not-a-url"), testing::ExitedWithCode(0), "");
}
