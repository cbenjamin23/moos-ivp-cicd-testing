#include <gtest/gtest.h>

#include <set>
#include <string>

#include "PipeWay.h"

namespace {

PipeWay ValidVarsPipeWay()
{
  PipeWay pipeway;
  pipeway.setClient("pRealmViewer");
  pipeway.addVar("DEPLOY");
  pipeway.setDuration(5);
  return pipeway;
}

}  // namespace

// Source audit note: this suite covers pRealm PipeWay's pure parser/state
// behavior: validity requirements, descriptor construction, display flags,
// duration/expiry math, modified-state consumption, set semantics, and
// equality. Realm's MOOS registration/publication behavior is left for
// harnesses.

TEST(PipeWayTest, DefaultsAreInvalidAndModifiedOnce)
{
  PipeWay pipeway;

  EXPECT_FALSE(pipeway.valid());
  EXPECT_FALSE(pipeway.isChanneled());
  EXPECT_TRUE(pipeway.showSource());
  EXPECT_TRUE(pipeway.showCommunity());
  EXPECT_TRUE(pipeway.showSubscriptions());
  EXPECT_TRUE(pipeway.showMasked());
  EXPECT_FALSE(pipeway.wrapContent());
  EXPECT_FALSE(pipeway.truncContent());
  EXPECT_FALSE(pipeway.timeFormatUTC());
  EXPECT_FALSE(pipeway.forceRefresh());
  EXPECT_DOUBLE_EQ(pipeway.getDuration(), 0);
  EXPECT_EQ(pipeway.getContentDescriptor(), "null pipeway");
  EXPECT_TRUE(pipeway.modified());
  EXPECT_FALSE(pipeway.modified());
}

TEST(PipeWayTest, ValidityRequiresClientDurationAndContent)
{
  PipeWay pipeway;

  pipeway.setClient("client");
  pipeway.setDuration(3);
  EXPECT_FALSE(pipeway.valid());

  pipeway.addVar("DEPLOY");
  EXPECT_TRUE(pipeway.valid());

  PipeWay channel_pipeway;
  channel_pipeway.setClient("client");
  channel_pipeway.setDuration(3);
  channel_pipeway.setChannel("pHelmIvP");
  EXPECT_TRUE(channel_pipeway.valid());
}

TEST(PipeWayTest, NegativeDurationIsAcceptedAsNonzero)
{
  PipeWay pipeway;
  pipeway.setClient("client");
  pipeway.addVar("DEPLOY");
  pipeway.setDuration(-1);

  EXPECT_TRUE(pipeway.valid());
}

TEST(PipeWayTest, SettersControlFlagsAndContent)
{
  PipeWay pipeway = ValidVarsPipeWay();

  pipeway.setShowSource(false);
  pipeway.setShowCommunity(false);
  pipeway.setShowSubscriptions(false);
  pipeway.setShowMasked(false);
  pipeway.setWrapContent(true);
  pipeway.setTruncContent(true);
  pipeway.setTimeFormatUTC(true);
  pipeway.setForceRefresh(true);

  EXPECT_FALSE(pipeway.showSource());
  EXPECT_FALSE(pipeway.showCommunity());
  EXPECT_FALSE(pipeway.showSubscriptions());
  EXPECT_FALSE(pipeway.showMasked());
  EXPECT_TRUE(pipeway.wrapContent());
  EXPECT_TRUE(pipeway.truncContent());
  EXPECT_TRUE(pipeway.timeFormatUTC());
  EXPECT_TRUE(pipeway.forceRefresh());
}

TEST(PipeWayTest, VarsAreStoredAsSortedUniqueSet)
{
  PipeWay pipeway;
  pipeway.addVar("RETURN");
  pipeway.addVar("DEPLOY");
  pipeway.addVar("RETURN");

  const std::set<std::string> vars = pipeway.getVars();

  ASSERT_EQ(vars.size(), 2u);
  EXPECT_EQ(*vars.begin(), "DEPLOY");
}

TEST(PipeWayTest, ContentDescriptorPrefersChannelOverVars)
{
  PipeWay pipeway = ValidVarsPipeWay();
  pipeway.setChannel("pHelmIvP");
  pipeway.addVar("RETURN");

  EXPECT_EQ(pipeway.getContentDescriptor(), "pHelmIvP");
}

TEST(PipeWayTest, ContentDescriptorBuildsSortedVarsAndTruncates)
{
  PipeWay pipeway = ValidVarsPipeWay();
  pipeway.addVar("RETURN");
  pipeway.addVar("STATION");

  EXPECT_EQ(pipeway.getContentDescriptor(), "vars=DEPLOY,RETURN,STATION");
  EXPECT_EQ(pipeway.getContentDescriptor(12), "vars=DEPLOY,");
}

TEST(PipeWayTest, ExpiryUsesDurationScaledByWarp)
{
  PipeWay pipeway = ValidVarsPipeWay();

  pipeway.setStartExpTime(100, 4);

  EXPECT_DOUBLE_EQ(pipeway.timeUntilExpire(110), 10);
}

TEST(PipeWayTest, ExplicitExpireTimeOverridesDurationCalculation)
{
  PipeWay pipeway = ValidVarsPipeWay();

  pipeway.setStartExpTime(100, 4);
  pipeway.setExpireTime(105);

  EXPECT_DOUBLE_EQ(pipeway.timeUntilExpire(100), 5);
}

TEST(PipeWayTest, SetModifiedRestoresOneShotModifiedState)
{
  PipeWay pipeway = ValidVarsPipeWay();
  EXPECT_TRUE(pipeway.modified());
  EXPECT_FALSE(pipeway.modified());

  pipeway.setModified();

  EXPECT_TRUE(pipeway.modified());
  EXPECT_FALSE(pipeway.modified());
}

TEST(PipeWayTest, EqualityIncludesClientChannelFlagsDurationForceAndVars)
{
  PipeWay left = string2PipeWay(
      "client=viewer,vars=DEPLOY:RETURN,duration=5,nosrc,nocom,nosubs,mask,wrap,trunc,utc,force");
  PipeWay right = string2PipeWay(
      "client=viewer,vars=RETURN:DEPLOY,duration=5,nosrc,nocom,nosubs,mask,wrap,trunc,utc,force");

  EXPECT_TRUE(left == right);
  EXPECT_FALSE(left != right);

  right.setForceRefresh(false);
  EXPECT_FALSE(left == right);
  EXPECT_TRUE(left != right);
}

TEST(PipeWayTest, ParsesChannelRequestWithAllFlags)
{
  PipeWay pipeway = string2PipeWay(
      "client=viewer,channel=pHelmIvP,duration=3,nosrc,nocom,nosubs,mask,wrap,trunc,utc,force");

  EXPECT_TRUE(pipeway.valid());
  EXPECT_TRUE(pipeway.isChanneled());
  EXPECT_EQ(pipeway.getClient(), "viewer");
  EXPECT_EQ(pipeway.getChannel(), "pHelmIvP");
  EXPECT_DOUBLE_EQ(pipeway.getDuration(), 3);
  EXPECT_FALSE(pipeway.showSource());
  EXPECT_FALSE(pipeway.showCommunity());
  EXPECT_FALSE(pipeway.showSubscriptions());
  EXPECT_FALSE(pipeway.showMasked());
  EXPECT_TRUE(pipeway.wrapContent());
  EXPECT_TRUE(pipeway.truncContent());
  EXPECT_TRUE(pipeway.timeFormatUTC());
  EXPECT_TRUE(pipeway.forceRefresh());
}

TEST(PipeWayTest, ParsesVarsRequestWithColonSeparatedVars)
{
  PipeWay pipeway =
      string2PipeWay("client=viewer,vars=DEPLOY:RETURN:STATION,duration=3");

  EXPECT_TRUE(pipeway.valid());
  EXPECT_FALSE(pipeway.isChanneled());
  EXPECT_EQ(pipeway.getVars().size(), 3u);
  EXPECT_EQ(pipeway.getContentDescriptor(), "vars=DEPLOY,RETURN,STATION");
}

TEST(PipeWayTest, ParserTrimsParamNamesButPreservesValues)
{
  PipeWay pipeway =
      string2PipeWay(" client = viewer , vars = DEPLOY , duration = 3 ");

  EXPECT_TRUE(pipeway.valid());
  EXPECT_EQ(pipeway.getClient(), "viewer");
  EXPECT_EQ(pipeway.getContentDescriptor(), "vars=DEPLOY");
}

TEST(PipeWayTest, ParserReturnsNullForUnknownOrMalformedEntries)
{
  EXPECT_FALSE(string2PipeWay("client=viewer,vars=DEPLOY,duration=3,bad").valid());
  EXPECT_FALSE(string2PipeWay("client=viewer,vars=DEPLOY,duration=fast").valid());
  EXPECT_FALSE(string2PipeWay("client=viewer,vars=DEPLOY,duration=3,unknown=true").valid());
}

TEST(PipeWayTest, FlagParserIgnoresAssignedValues)
{
  PipeWay pipeway =
      string2PipeWay("client=viewer,vars=DEPLOY,duration=3,wrap=false,mask=true");

  EXPECT_TRUE(pipeway.valid());
  EXPECT_TRUE(pipeway.wrapContent());
  EXPECT_FALSE(pipeway.showMasked());
}

TEST(PipeWayTest, ParserDoesNotRequireFinalObjectToBeValid)
{
  PipeWay missing_duration = string2PipeWay("client=viewer,vars=DEPLOY");
  PipeWay missing_content = string2PipeWay("client=viewer,duration=3");

  EXPECT_EQ(missing_duration.getClient(), "viewer");
  EXPECT_EQ(missing_duration.getContentDescriptor(), "null pipeway");
  EXPECT_FALSE(missing_duration.valid());

  EXPECT_EQ(missing_content.getClient(), "viewer");
  EXPECT_FALSE(missing_content.valid());
}
